# close_range compatibility candidate for hotspot startup

The second diagnostic bugreport contains netd-image children exiting with
status 127 (signal 0), including PID 9684 at uptime 69.716147. Netd later logs
cleanup of that same PID after DNS setup fails with Broken pipe. No exec trace
for this PID appears in the retained log. This supports a pre-exec spawn failure;
it does not identify the failing operation by itself (traces are rate-limited).

A17 has close_range; test previously returned ENOSYS for syscall 436. Android's
POSIX_SPAWN_CLOEXEC_DEFAULT setup may use CLOSE_RANGE_CLOEXEC before exec. AOSP
bionic versions with a fallback can work without this syscall; the device's
exact libc behavior has not been verified. This patch is a compatibility
candidate, not a confirmed root-cause fix.

The implementation is adapted from this repository's A17 branch. Keep the
existing 4.19 dup_fd interface and copy the entire table on UNSHARE, instead of
importing A17's newer sparse-copy interface and its fork changes. Preserve
close, CLOEXEC, UNSHARE and combined-flag semantics. Wire syscall 436 into the
native generic and ARM64 compat tables; their existing limits already cover it.

Temporary tether_diag logs record close_range arguments/result for netd and
dnsmasq images. A ret=0 log followed by successful dnsmasq startup and AP-ENABLED
would support this candidate. If it still fails, correlate the syscall and
exit logs; do not assume status 127 proves close_range was responsible.

Validation: checkpatch; compiled an isolated harness using the actual new
file-table functions with mocked kernel glue, testing invalid flags/ranges,
CLOEXEC boundaries, UINT_MAX ranges, unshare allocation failure, table isolation
and combined flags. AddressSanitizer and UndefinedBehaviorSanitizer passed with
leak detection disabled because the runner uses ptrace. This does not test
kernel concurrency, syscall ABI execution, or replace a full kernel build.
Full compilation and device validation remain pending.

Build this test revision, attempt one hotspot start, then collect:

```powershell
adb logcat -b all -d > test-hotspot-close-range-logcat.txt
adb bugreport test-hotspot-close-range.zip
```

If the hotspot starts, also verify client DHCP, internet access, 2.4/5 GHz and
Wi-Fi sharing before removing temporary diagnostics. A17 is unchanged.
