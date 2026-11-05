# Temporary hotspot startup diagnostics

This test-only change traces execution of /system/bin/dnsmasq and execution or
exit of direct netd child processes. Checking the parent's process leader also
covers children spawned by netd binder threads before their executable changes.
It leaves the A17 branch and all driver/configuration behavior unchanged.

Messages are warning-level, rate-limited and prefixed with `tether_diag:`.
They include only PID/TGID, execution result and encoded exit status; arguments,
environment, SSIDs and passwords are never recorded. The netd-child selector
also includes other direct netd helpers; correlate messages by PID rather than
assuming that every traced child is dnsmasq.

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
