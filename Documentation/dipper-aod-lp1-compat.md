# Dipper AOD LP1 compatibility mode

## Observed failure and scope

On the supplied HyperOS logs, AOD initially displays, then a 6000 ms
DozeScreenState timer requests OFF followed by DOZE_SUSPEND. The display driver
enters LP2. Later dumps still show AOD enabled, DOZE_SUSPEND committed and a
nonzero software brightness. This narrows the failure to the low-power display
path but does not prove a particular HAL or panel command is responsible.

The existing dsi_display_set_backlight() handles AOD brightness in LP1 and skips
it in LP2. This patch is a reversible compatibility workaround for testing that
path, not a complete implementation of newer HyperOS display interfaces.

## Implementation

For DSI connectors on machines whose root DT model contains `Dipper`, convert
the LP property value LP2 to LP1 before msm_property_atomic_set(). This keeps
the cached connector property, drm_dev->doze_state, panel power handling and
encoder/performance consumers consistent. Kernel-generated PM commits use the
same setter, so their LP1-to-LP2 request is normalized as well.

Other machines, non-DSI connectors, ON, LP1 and OFF values are unchanged. A real
DPMS OFF still follows the existing shutdown path. No panel DCS bytes, panel
brightness limits or vendor userspace files are changed.

The mode is enabled by default but may consume more power during AOD. It has
not been tested on hardware. The later goal should be a working LP2 path if
the compatibility test confirms this is the failure boundary.

## Test on the phone after rebuilding and installing this test kernel

Keep a known-good boot image available. Disable any other display experiments
for this test. Confirm that normal unlock and full screen-off still work.

Read the switch from a root shell:

```sh
cat /sys/module/msm_drm/parameters/dipper_aod_lp1
```

Enable AOD, lock the phone and wait at least 60 seconds. Check multiple
lock/unlock cycles, low/high AOD brightness and full AOD disable. The first
normalized request should log:

```text
dipper AOD compatibility: keeping LP2 requests in LP1
```

Android dumpsys may still report DOZE_SUSPEND because the HAL requested that
state; use kernel logs to check the effective driver state. The patch does not
rewrite Android's requested state.

To restore the original behavior without reflashing, from a root shell:

```sh
echo 0 > /sys/module/msm_drm/parameters/dipper_aod_lp1
```

Wake the phone fully, then lock it again so a fresh state transition occurs.
Use `echo 1` to re-enable, followed by the same wake/lock cycle. The switch is
not persisted across reboots. To disable from boot, use the kernel command-line
parameter `msm_drm.dipper_aod_lp1=0`.

If AOD still becomes black with this mode enabled, retain a fresh logcat,
dmesg and dumpsys display/power from that test. Do not infer that another
brightness value or a larger delay will repair the LP2/DisplayFeature ABI.

## Validation status

Checkpatch and host-side checks of the actual normalization helper cover the
board/connector/power-state scope and the runtime switch. No full kernel build,
phone boot or AOD power measurement has been completed for this change.
