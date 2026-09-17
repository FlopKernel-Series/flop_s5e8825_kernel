#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "${SCRIPT_DIR}/../lib/log.sh" ]; then
    # shellcheck source=../lib/log.sh
    source "${SCRIPT_DIR}/../lib/log.sh"
fi

command -v log_info >/dev/null 2>&1 || log_info() { echo "INFO: $*"; }
command -v log_err >/dev/null 2>&1 || log_err() { echo "ERROR: $*" >&2; }

KDIR="$(readlink -f "$SCRIPT_DIR/../..")"
cd "$KDIR"

if [ -d /workspace ]; then
    WP="/workspace"
    export CCACHE_DIR="$WP/.ccache"
fi

export WP=${WP:-$(realpath "$KDIR/../")}

if [ ! -d drivers ]; then
    log_err "Please execute from top-level kernel tree"
    exit 1
fi

export PATH="$KDIR/kernel_build/bin:$PATH"

SCRIPTS_DIR="kernel_build/scripts"
source "$SCRIPTS_DIR/tc.sh"

: "${SKIP_FIPS_CRYPTO_INTEGRITY:=1}"
: "${SKIP_EXYNOS_FMP_INTEGRITY:=1}"
export SKIP_FIPS_CRYPTO_INTEGRITY
export SKIP_EXYNOS_FMP_INTEGRITY

USE_CCACHE="${USE_CCACHE:-1}"
if [ "$USE_CCACHE" = "1" ]; then
    export CC="ccache clang"
else
    export CC="clang"
fi

export PLATFORM_VERSION="12"
export ANDROID_MAJOR_VERSION="s"
export TARGET_SOC="s5e8825"
export LLVM=1
export LLVM_IAS=1
export ARCH=arm64

DEFCONFIG="${DEFCONFIG:-s5e8825-unified_defconfig}"
FK_VER="$(grep -oP '^FK_VER="\K[^"]+' kernel_build/ckbuild.sh | head -n 1)"
[ -n "$FK_VER" ] || FK_VER="v6.3.3"
OUTDIR="$KDIR/out"
NH_MOD_OUTDIR="$KDIR/kernel_build/tmp/nh_modules_out"

if [ ! -f "$OUTDIR/.config" ]; then
    log_err "No existing build tree at $OUTDIR, run a full build first"
    exit 1
fi

if [ ! -f "arch/arm64/configs/nethunter.config" ]; then
    log_err "nethunter.config not found"
    exit 1
fi

LOCALVERSION_SAVED="$(sed -n 's/^CONFIG_LOCALVERSION=\(.*\)/\1/p' "$OUTDIR/.config" | head -n 1)"

log_info "Merging nethunter.config onto existing build tree..."
make -j"$(nproc --all)" O="$OUTDIR" CC="$CC" "$DEFCONFIG" nethunter.config

if [ -n "$LOCALVERSION_SAVED" ]; then
    scripts/config --file "$OUTDIR/.config" --set-val LOCALVERSION "$LOCALVERSION_SAVED"
fi

log_info "Building NetHunter modules..."
make -j"$(nproc --all)" O="$OUTDIR" CC="$CC" modules

rm -rf "$NH_MOD_OUTDIR"
make -j"$(nproc --all)" O="$OUTDIR" CC="$CC" \
    INSTALL_MOD_STRIP="--strip-debug --keep-section=.ARM.attributes" \
    INSTALL_MOD_PATH="$NH_MOD_OUTDIR" modules_install

KMOD_DIR="$(find "$NH_MOD_OUTDIR/lib/modules" -mindepth 1 -maxdepth 1 -type d | head -n 1)"
if [ -z "$KMOD_DIR" ]; then
    log_err "No installed modules found in $NH_MOD_OUTDIR/lib/modules"
    exit 1
fi

bash "$SCRIPTS_DIR/gen_nh_module.sh" \
    "$KMOD_DIR" \
    "$KDIR" \
    "$FK_VER" \
    "$KDIR/kernel_build/nh/firmware"

rm -rf "$NH_MOD_OUTDIR"

log_info "NetHunter extras build complete"
