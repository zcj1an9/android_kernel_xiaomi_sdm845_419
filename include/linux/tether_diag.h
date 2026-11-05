/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TETHER_DIAG_H
#define _LINUX_TETHER_DIAG_H

#include <linux/rcupdate.h>
#include <linux/sched/signal.h>
#include <linux/string.h>

/* Temporary diagnostics: include children that fail before exec of dnsmasq. */
static inline bool tether_diag_current(void)
{
	struct task_struct *parent;
	char comm[TASK_COMM_LEN];
	bool match;

	get_task_comm(comm, current);
	if (!strcmp(comm, "dnsmasq"))
		return true;

	rcu_read_lock();
	parent = rcu_dereference(current->real_parent);
	get_task_comm(comm, parent->group_leader);
	match = !same_thread_group(current, parent) && !strcmp(comm, "netd");
	rcu_read_unlock();

	return match;
}

#endif /* _LINUX_TETHER_DIAG_H */
