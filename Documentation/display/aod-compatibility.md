# SDM845 AOD compatibility testing

This is a compatibility and diagnostic candidate, not a verified fix for every
SDM845 panel. Physical brightness, ambient transitions and single-tap wake still
require device testing.

## Interfaces

Both connector attributes use the same mode ABI:

- `doze_brightness`: applied mode readback and mode requests.
- `doze_backlight`: legacy alias for the same mode requests/readback.
- Values: 0 releases AOD mode, 1 selects AOD HBM, 2 selects AOD LBM.
- These are mode values, not raw DBV, percentages or nits.
- Invalid values and unsupported bridges return errors.
- The callbacks are optional DRM bridge operations; DRM core does not depend
  directly on DSI symbols.

Requests made before the panel is ready, outside LP mode, or during FOD HBM
remain pending. LP entry and subsequent supported brightness/legacy operations
retry them. Failed commands do not replace the applied mode cache. While an
explicit mode owns the panel, ordinary numeric brightness writes do not choose a
different HBM/LBM mode through the old threshold.

Legacy `disp_param` AOD commands share this state machine. Native panel LP
command sets and device-specific HBM/LBM/NOLP sequences remain in use. No sm8250
or cepheus panel command payloads, brightness constants or panel IDs are copied.

## Removed experiments and retained behavior

The EA8074 direct-follow-numeric-DBV experiment from commit 14ea908 is removed:
hardware testing showed the APK's 0.12 request reached the kernel as 122/1023
without increasing visible AOD brightness.

The earlier successful legacy LP entry and minimum nonzero EA8074 seed remain.
Clock/write error propagation and the last-backlight diagnostic remain.

The separate MIUIAod brightness-v2 APK experiment is not reverted on the phone
by a kernel change. For the next kernel comparison, restore the previously
working DOZE(3)-only APK using the existing ROM deployment/signing procedure.
Do not install the unmodified original APK in place of that DOZE(3) build.

## Read-only diagnostics after unlocking

`/sys/module/msm_drm/parameters/aod_backlight_status` now reports revision 2,
including explicit-mode ownership, pending state, target mode and threshold.

`/sys/module/msm_drm/parameters/aod_panel_history` keeps the last 24 events:
mode requests/results, legacy disp_param input, AOD/LP/NOLP/dimming/HBM command
sets and DBV writes while in low-power/AOD state. Timestamps are monotonic
boottime milliseconds, not wall-clock time. Values are hexadecimal; count is
the command count (zero for request/input records). A zero count command set
did not transmit commands even when its return code is zero.

History survives normal unlock; it is bounded and can be overwritten by later
events. Inspect it after one sleep/wake cycle, rather than many cycles.
It records software operations, not measured luminance or register readback.

## Deployment constraints

The writable attributes use standard 0644 permissions. Existing ueventd and
SELinux policy still govern access. Adding an ABI does not prove the installed
HAL uses it or is allowed to write it; check logs/history on the actual ROM.
Do not use global permissive mode or world-writable sysfs permissions as a fix.
LCD devices or panels without the relevant command sets are not advertised as
OLED AOD-capable by this change.

## References

- sm8250 Linux 4.19.325:
  https://github.com/jsdizkcksv/kernel_xiaomi_sm8250/tree/e7087c93914296299665907812453052bb7161a0
  Separate AOD mode control and deferred request handling.
- cepheus Linux 4.14.357:
  https://github.com/Xirudly/kernel_xiaomi_cepheus/tree/00e41effd8c7a8a8f868f83a76ecc4b71df78e33
  Legacy doze_backlight interface and mode ABI.

These references guide common control flow only. They do not establish that
the currently installed VNDK202504 HAL uses either interface. VNDK30 boot
compatibility is a separate task.

## Validation

Checkpatch passed. An extracted C harness with UBSan exercises the real sysfs,
bridge, panel-mode and history functions: pre-initialization queuing, LP-entry
replay, LBM ownership despite high numeric brightness, FOD deferral, failed
command retries, clock failures, unsupported commands/bridges, legacy/new ABI
cache coherence, normal exit and history wraparound.

A complete kernel build and on-device validation have not been performed for
this change. The single-tap AOD path is not changed here.
