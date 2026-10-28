// SPDX-License-Identifier: GPL-2.0
/* Standalone regressions: no libbpf or BTF dependency. Run as root on target. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/filter.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static const struct atomic_test {
	const char *descr;
	struct bpf_insn insns[32];
	unsigned int retval;
} tests[] = {
#include "test_atomic_ops_cases.h"
};

static char log_buf[65536];

static int load_prog(const struct bpf_insn *insns, unsigned int count,
		     enum bpf_prog_type type, enum bpf_attach_type attach)
{
	union bpf_attr attr = {};

	memset(log_buf, 0, sizeof(log_buf));
	attr.prog_type = type;
	attr.expected_attach_type = attach;
	attr.insn_cnt = count;
	attr.insns = (uintptr_t)insns;
	attr.license = (uintptr_t)"GPL";
	attr.log_buf = (uintptr_t)log_buf;
	attr.log_size = sizeof(log_buf);
	attr.log_level = 1;
	return syscall(__NR_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));
}

int main(void)
{
	unsigned int i, failures = 0, passed = 0, skipped = 0;

	for (i = 0; i < ARRAY_SIZE(tests); i++) {
		const struct atomic_test *t = &tests[i];
		unsigned int count = ARRAY_SIZE(t->insns);
		unsigned char input[64] = {}, output[64];
		union bpf_attr attr = {};
		int fd, err;

		while (count && !t->insns[count - 1].code)
			count--;
		fd = load_prog(t->insns, count, BPF_PROG_TYPE_SOCKET_FILTER, 0);
		if (fd < 0) {
			if (!i && errno == EPERM && !log_buf[0]) {
				puts("SKIP: BPF_PROG_LOAD denied; "
				     "run as root with BPF permissions.");
				return 4;
			}
			fprintf(stderr, "FAIL %s: load: %s\n%s", t->descr,
				strerror(errno), log_buf);
			failures++;
			continue;
		}
		attr.test.prog_fd = fd;
		attr.test.data_in = (uintptr_t)input;
		attr.test.data_out = (uintptr_t)output;
		attr.test.data_size_in = sizeof(input);
		attr.test.data_size_out = sizeof(output);
		attr.test.repeat = 1;
		err = syscall(__NR_bpf, BPF_PROG_TEST_RUN, &attr, sizeof(attr));
		close(fd);
		if (err || attr.test.retval != t->retval) {
			fprintf(stderr, "FAIL %s: run=%d retval=%u\n", t->descr,
				err, attr.test.retval);
			failures++;
		} else {
			passed++;
		}
	}

	/* Check the helper in both SOCKOPT attach types, including NULL context. */
	for (i = 0; i < 2; i++) {
		const struct bpf_insn insns[] = {
			BPF_MOV64_IMM(BPF_REG_1, 0),
			BPF_EMIT_CALL(BPF_FUNC_get_netns_cookie),
			BPF_MOV64_IMM(BPF_REG_0, 1),
			BPF_EXIT_INSN(),
		};
		int fd = load_prog(insns, ARRAY_SIZE(insns),
			   BPF_PROG_TYPE_CGROUP_SOCKOPT,
			   i ? BPF_CGROUP_SETSOCKOPT : BPF_CGROUP_GETSOCKOPT);

		if (fd < 0 && errno == EPERM && !log_buf[0]) {
			puts("SKIP SOCKOPT: BPF_PROG_LOAD needs "
			     "additional network/BPF permissions.");
			skipped++;
		} else if (fd < 0) {
			fprintf(stderr, "FAIL SOCKOPT netns_cookie %u: %s\n%s", i,
				strerror(errno), log_buf);
			failures++;
		} else {
			close(fd);
			passed++;
		}
	}
	printf("BPF atomic/SOCKOPT tests: %u passed, %u failed, %u skipped\n",
	       passed, failures, skipped);
	return failures ? 1 : 0;
}
