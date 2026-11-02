# Per-thread WALT load boost

With `CONFIG_SCHED_WALT` and `CONFIG_SYSCTL`, the kernel exposes
`/proc/sys/walt/task_load_boost` for Android performance services.

Write a thread ID followed by an integer percentage from 0 to 100. For example,
`1234 70` multiplies that thread's frequency-scaled WALT runtime by 1.70.
`1234 0` restores normal accounting for future intervals. Existing demand
history ages out through the normal WALT windows; it is not discarded.

This changes WALT demand and busy-time accounting, not scheduler priority,
CPU affinity or a global frequency floor. Task history samples remain bounded
by one WALT window. With boost zero, the runtime scaling is unchanged.

The handler serializes writes and updates the task under its runqueue lock,
accounting the elapsed interval before changing the boost. It holds a task
reference during the update. The field is zeroed by existing WALT task
initialization/reset paths, so newly forked tasks do not inherit a boost.

An invalid pair, extra tokens, embedded NUL, integer overflow, nonpositive TID,
or out-of-range boost returns `EINVAL`. Boosting a missing or exiting thread
returns `ESRCH`. Resetting a thread that has already exited succeeds and does
not create persistent per-PID state. Reading the node returns the last
successfully accepted pair, not a query of all active boosts.

The node is root-owned with mode 0644. Device init ownership and SELinux policy
must also permit the performance service to write it. This kernel patch does
not bypass those checks.

## Build configuration

The functional settings formerly duplicated in `dipper-a17-boot.config` and
`dipper.config` now live in `vendor/sdm845-perf_defconfig` and
`vendor/xiaomi/sdm845-common.config`. The standard merge order is perf, common,
then the device fragment. The legacy dipper boot fragment retains only its
local-version settings. Device fragments keep their hardware-specific choices,
including DT overlay settings.

## Validation status

The six device merges (dipper, polaris, perseus, beryllium, equuleus and ursa)
were checked with `merge_config.sh -m` for all 18 shared boot settings. This is
a fragment merge check, not Kconfig dependency resolution or a kernel build.
Whitespace and patch style checks were run. No compilation or hardware testing
has been performed for the WALT change.

On a test device, verify valid/reset writes, malformed inputs, exiting-thread
resets, and performance-service access. Confirm task demand/frequency responses
with scheduler tracing, and compare idle power, sustained load and frame times
against the preceding kernel before considering this change validated.
