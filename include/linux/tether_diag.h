/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TETHER_DIAG_H
#define _LINUX_TETHER_DIAG_H

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/string.h>

/*
 * A vfork child still has netd's executable before exec. Unlike task comm,
 * this also identifies netd after its main thread is renamed by Binder.
 */
static inline bool tether_diag_current(void)
{
	struct file *exe;
	struct dentry *dentry;
	bool match;

	exe = get_task_exe_file(current);
	if (!exe)
		return false;

	dentry = exe->f_path.dentry;
	spin_lock(&dentry->d_lock);
	match = (dentry->d_name.len == 4 &&
		 !memcmp(dentry->d_name.name, "netd", 4)) ||
		(dentry->d_name.len == 7 &&
		 !memcmp(dentry->d_name.name, "dnsmasq", 7));
	spin_unlock(&dentry->d_lock);
	fput(exe);

	return match;
}

#endif /* _LINUX_TETHER_DIAG_H */
