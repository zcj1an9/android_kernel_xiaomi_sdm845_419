# close_range backport: verified hotspot repair

The test branch retains the close_range implementation introduced by
bdce8f96da0c9bf5cd0ef8a624b65b94ff002ada. Temporary tether_diag instrumentation
and its helper header have been removed from the production source.

## Device validation on 2026-11-05

On Xiaomi Mi 8 (dipper), the user confirmed:
- 5 GHz hotspot starts.
- 2.4 GHz hotspot starts.
- A connected client can access the internet.

The supplied successful bugreport records close_range(flags=0x4) returning 0,
then exec succeeding for the same PID, dnsmasq starting and configuring DNS,
and hostapd reporting AP-ENABLED on channel 36. Earlier diagnostic builds
recorded netd-image spawn children exiting 127 before a matching exec trace,
followed by DNS control-pipe failures. This before/after evidence supports the
missing close_range interface as the startup compatibility regression.

The user's device tests apply to the instrumented bdce8f9 build. The subsequent
cleanup only removes temporary diagnostics; it still needs compilation and a
brief device smoke test. Repeated toggles, long-duration tethering and other
devices have not been established by these reports.

## Implementation and scope

The implementation was adapted from this repository's A17 branch. It keeps
the existing 4.19 dup_fd interface and copies the full descriptor table when
UNSHARE is needed, preserving close, CLOEXEC, UNSHARE and combined semantics.
Syscall 436 is wired into the native generic and ARM64 compat tables.

The repair changes no device-specific configuration. Other ARM64 devices
built from this test source receive the syscall implementation too, but
their hotspot behavior remains unverified. The A17 branch is unchanged.

## Validation

The implementation passed checkpatch and an isolated harness compiling the
actual file-table functions with mocked kernel glue: invalid arguments,
CLOEXEC boundaries, UINT_MAX ranges, unshare allocation failure, table
isolation and combined flags. ASan/UBSan passed with leak detection disabled
because the runner uses ptrace. This does not test kernel concurrency or
replace a full kernel build.

The production cleanup is checked for remaining diagnostic references and
for preserving the syscall implementation, wiring and UAPI. No new image
was compiled or published as part of the cleanup.
