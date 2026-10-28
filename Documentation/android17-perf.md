# Android 17 backport for perf

## Dipper source-built test package (v3)

The v3 package uses Image.gz-dtb produced directly by this source tree,
including DTBs built from the external qcom-sdm845 and master-dipper sources.
No kernel, DTB, or installer files are taken from the supplied 1.71 ZIP.
The BPF, EROFS, initramfs, devtmpfs and diagnostic configuration is retained.
Ramoops uses the reserved-memory node in the source-built DTB; the legacy
command-line reservation and platform-device fallback are removed.
The installer is configured from a clean upstream osm0sis/AnyKernel3 checkout.
Do not supply DTB_BLOB; the build script no longer accepts a DTB override.
Boot and radio operation still require physical-device validation.

## Scope

- BPF verifier, atomic instructions, BTF/module support, task-local storage,
  memory-cgroup accounting and socket/cgroup helpers from the reference branch.
- IPv4/IPv6 nexthop, netlink and file/locking dependencies used by those changes.
- The reference branch's packet-pointer `regsafe()` fix.
- A `5.15.197` uname release only for tasks named exactly `bpfloader`,
  `netbpfload` or `netd`, matching the reference branch's compatibility approach.
  Other tasks still see the actual 4.19 release.
- The 4.19 `mmap_sem` member remains an alias of `mmap_lock` for vendor code.
- BPF preload is disabled, as in A17. Perf's device configuration and drivers
  are retained, apart from API adaptations required by the backport.

FUSE-BPF, DMA-BUF heap conversion and A17's display-panel changes are outside
this backport. LTO/CFI/shadow-call-stack are not enabled by the tested config.

## Build

Use Linux with Clang/LLVM 14, lld, GNU make, flex, bison, bc, cpio, xz, perl,
libssl-dev, libelf-dev and ARM64/ARM32 binutils. The existing perf tree expects
external mi845 and device-tree repositories. The validation build uses these
pinned upstream revisions, without editing their contents:

```sh
git clone https://github.com/duckyduckG/kernel_techpack_mi845_419.git techpack/mi845
git -C techpack/mi845 checkout 87be21823dfeeca50735ae575f2c1b51f348cb70
git clone https://github.com/duckyduckG/devicetree_qcom-sdm845_419.git arch/arm64/boot/dts/qcom-sdm845
git -C arch/arm64/boot/dts/qcom-sdm845 checkout be1279422f14437df8c185f952b4139ff7e10157
git clone -b master-dipper https://github.com/duckyduckG/devicetree_xiaomi_419.git arch/arm64/boot/dts/xiaomi
git -C arch/arm64/boot/dts/xiaomi checkout c573577e1fb2e92f36f9546d2e45738e618316af
CLANG_BIN=/path/to/llvm-14/bin JOBS=8 bash scripts/build-perf-a17.sh dipper
```

The script also accepts `perseus`, `polaris`, `beryllium`, `equuleus` and `ursa`.
Only the configurations explicitly reported below have been build-validated.
`OUT_DIR`, `CROSS_COMPILE` and `CROSS_COMPILE_ARM32` can be overridden.
`Image.gz-dtb` is a kernel payload, not a complete boot image or a flashable ZIP.
Package it using the device's existing boot-image/AnyKernel workflow.

## Build validation (2026-10-27)

The perf defconfig plus the common Xiaomi and dipper fragments was fully built
with Ubuntu Clang/LLVM 14.0.6 and LLVM's integrated assembler. `vmlinux`,
`Image.gz` and `Image.gz-dtb` linked successfully. The build script generates
the IKHEADERS archive serially before parallel compilation to avoid an archive
timestamp race observed in this build environment.

This validates the dipper build only. Hardware and Android 17 runtime tests
have not been performed. In particular, SIM/mobile-data stability remains
unverified.

## Device validation

A successful build does not establish successful Android 17 boot or radio
operation. Validate on the target device with its actual ROM and vendor:

1. Keep a known-working boot image available for recovery.
2. Verify boot completion and capture a full boot log, including bpfloader,
   netbpfload and netd messages. Look for rejected programs or missing helpers.
3. Test Wi-Fi, SIM registration, mobile data, airplane-mode transitions,
   tethering and reboot with a SIM inserted.
4. Check camera, audio and suspend/resume for regressions.

From the host computer:

```sh
adb shell uname -a
adb shell getprop sys.boot_completed
adb shell ls -l /sys/fs/bpf
adb logcat -b all -d > a17-boot-logcat.txt
adb shell su -c dmesg > a17-dmesg.txt
```

The host-shell `uname` returning 4.19 is intentional. Inspect logs rather than
using the displayed release as proof that the required BPF programs loaded.
