#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
DEVICE=${1:-dipper}
case "$DEVICE" in
  dipper|perseus|polaris|beryllium|equuleus|ursa) ;;
  *) echo "Unsupported device: $DEVICE" >&2; exit 2 ;;
esac

for dependency in techpack/mi845/Kconfig \
  arch/arm64/boot/dts/qcom-sdm845/Makefile \
  arch/arm64/boot/dts/xiaomi/Makefile; do
  if [[ ! -f "$ROOT/$dependency" ]]; then
    echo "Missing $dependency; see Documentation/android17-perf.md." >&2
    exit 1
  fi
done
if [[ -n ${CLANG_BIN:-} ]]; then
  export PATH="$CLANG_BIN:$PATH"
fi
OUT_DIR=${OUT_DIR:-"$ROOT/out-a17-$DEVICE"}
mkdir -p "$OUT_DIR"
OUT_DIR=$(cd "$OUT_DIR" && pwd)
JOBS=${JOBS:-8}

BUILD_ARGS=(O="$OUT_DIR" ARCH=arm64 LLVM=1 LLVM_IAS=1
  CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"
  CROSS_COMPILE_ARM32="${CROSS_COMPILE_ARM32:-arm-linux-gnueabi-}")

clang --version
make -C "$ROOT" "${BUILD_ARGS[@]}" vendor/sdm845-perf_defconfig
"$ROOT/scripts/kconfig/merge_config.sh" -m -O "$OUT_DIR" \
  "$OUT_DIR/.config" \
  "$ROOT/arch/arm64/configs/vendor/xiaomi/sdm845-common.config" \
  "$ROOT/arch/arm64/configs/vendor/xiaomi/$DEVICE.config"
if [[ "$DEVICE" == dipper ]]; then
  "$ROOT/scripts/kconfig/merge_config.sh" -m -O "$OUT_DIR" \
    "$OUT_DIR/.config" \
    "$ROOT/arch/arm64/configs/vendor/xiaomi/dipper-a17-boot.config"
fi
make -C "$ROOT" "${BUILD_ARGS[@]}" olddefconfig

for option in BPF_SYSCALL BPF_JIT BPF_JIT_ALWAYS_ON CGROUP_BPF \
              NET_CLS_BPF NET_ACT_BPF MEMCG; do
  if ! grep -qx "CONFIG_${option}=y" "$OUT_DIR/.config"; then
    echo "Required option is disabled: CONFIG_$option" >&2
    exit 1
  fi
done

# Avoid archiving generated headers while parallel compilation updates them.
make -C "$ROOT" "${BUILD_ARGS[@]}" prepare
if grep -qx 'CONFIG_IKHEADERS=y' "$OUT_DIR/.config"; then
  mkdir -p "$OUT_DIR/kernel"
  (cd "$OUT_DIR" && SRCARCH=arm64 srctree="$ROOT" \
    bash "$ROOT/kernel/gen_kheaders.sh" kernel/kheaders_data.tar.xz)
fi

make -C "$ROOT" "${BUILD_ARGS[@]}" -j"$JOBS" Image.gz-dtb
echo "Built: $OUT_DIR/arch/arm64/boot/Image.gz-dtb"
