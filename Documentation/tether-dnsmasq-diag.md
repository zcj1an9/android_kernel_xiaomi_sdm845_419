# Temporary hotspot startup diagnostics

This test-only change traces execution of /system/bin/dnsmasq and execution or
exit of processes whose executable basename is netd or dnsmasq. A vfork child
inherits netd's executable before exec, even when Binder renamed the parent
and child threads. The executable file is referenced while its name is checked.
It leaves the A17 branch and all driver/configuration behavior unchanged.

Messages are warning-level, rate-limited and prefixed with `tether_diag:`.
They include only PID/TGID, execution result and encoded exit status; arguments,
environment, SSIDs and passwords are never recorded. The executable selector
also includes netd itself and pre-exec helpers; correlate messages by PID rather
than assuming that every traced process is dnsmasq. Helpers that successfully
exec a different image are no longer selected for exit logging.

After building and flashing this test commit, boot Android, reproduce one
hotspot failure and collect from the PC (root is not required):

```powershell
adb logcat -b all -d > test-hotspot-debug-logcat.txt
adb bugreport test-hotspot-debug.zip
```

Check the bugreport's kernel log for `tether_diag:`. Some production systems
omit kernel logs; absence of messages alone is not proof that exec was skipped.

Interpretation:

- exec_enter then exec_result ret=0: exec completed successfully. This does not
  establish that the dynamic linker or dnsmasq initialization completed.
- Negative exec_result: kernel exec failure, with a negative errno.
- exit without exec_enter for a netd child: failure before the traced exec call,
  provided the relevant log interval was retained and messages were not limited.
- signal=0: status is the exit value. status=127 may come from spawn setup, exec
  or the dynamic linker and is not sufficient by itself to select a fix.
- signal!=0: terminating signal; core indicates the core-dump status bit, not
  that a tombstone or core file was actually saved.

This is diagnostic instrumentation, not a hotspot fix. Remove it after identifying
and verifying the underlying repair. The previously reverted nl80211 experiment
is not included, and close_range support is not changed.

Validation: patch style and review of filename lifetime and parent access.
No complete kernel build or device validation was performed for this change.

The first diagnostic revision used comm == netd on the parent's group leader.
The supplied bugreport shows PID 1088's leader named binder:1088_3, which made
that revision miss pre-exec child failures. This revision replaces comm matching
with a referenced executable lookup and checks the basename under d_lock.
The previous trace records alone did not establish dnsmasq's exec outcome.
