#!/bin/bash
#
# Build script for FlopKernel (Exynos 1280).
# Based on build script for Quicksilver, by Ghostrider.
# Copyright (C) 2020-2021 Adithya R. (original version)
# Copyright (C) 2022-2025 Flopster101 (rewrite)
#
# Credits to Gabriel2392 for logic in post.sh, see file for more info.
#

## Variables
set -e

# Other
DEFAULT_DEFCONFIG="s5e8825-unified_defconfig"
KERNEL_URL="https://github.com/FlopKernel-Series/flop_s5e8825_kernel"
AK3_URL="https://github.com/FlopKernel-Series/AnyKernel3-s5e8825"
AK3_TEST=0
SECONDS=0 # Built-in bash timer
DATE="$(date '+%Y%m%d-%H%M')"
BUILD_HOST="$USER@$(hostname)"
SCRIPTS_DIR="kernel_build/scripts"

# Workspace
if [ -d /workspace ]; then  
    WP="/workspace"
    IS_GP=1
else
    IS_GP=0
fi

if [ -z "$WP" ]; then
    echo -e "\nERROR: Environment not Gitpod! Please set the WP env var...\n"
    exit 1
fi

if [ ! -d drivers ]; then
    echo -e "\nERROR: Please execute from top-level kernel tree\n"
    exit 1
fi

if [ "$IS_GP" == "1" ]; then
    export KBUILD_BUILD_USER="Flopster101"
    export KBUILD_BUILD_HOST="buildbot"
fi

export PATH="$(pwd)/kernel_build/bin:$PATH"

# Directories
AK3_DIR="$WP/AK3-1280"
AK3_BRANCH="floppy-unity"
KDIR="$(readlink -f .)"
USE_GCC_BINUTILS="0"

## Inherited paths
OUTDIR="$KDIR/out"
MOD_OUTDIR="$KDIR/modules_out"
TMPDIR="$KDIR/kernel_build/tmp"
IN_PLATFORM="$KDIR/kernel_build/vboot_platform"
IN_DLKM="$KDIR/kernel_build/vboot_dlkm"
IN_DTB="$OUTDIR/arch/arm64/boot/dts/exynos/s5e8825.dtb"
IN_DTB_OC="$OUTDIR/arch/arm64/boot/dts/exynos/s5e8825_oc.dtb"
PLATFORM_RAMDISK_DIR="$TMPDIR/ramdisk_platform"
DLKM_RAMDISK_DIR="$TMPDIR/ramdisk_dlkm"
PREBUILT_RAMDISK="$KDIR/kernel_build/boot/ramdisk"
MODULES_DIR="$DLKM_RAMDISK_DIR/lib/modules"
OUT_KERNEL="$OUTDIR/arch/arm64/boot/Image"
IMAGES_DIR="$KDIR/kernel_build/images"
OUT_BOOTIMG="$IMAGES_DIR/boot.img"
OUT_BOOTIMG_ONEUI="$IMAGES_DIR/boot_oneui.img"
OUT_BOOTIMG_AOSP="$IMAGES_DIR/boot_aosp.img"
OUT_VENDORBOOTIMG="$IMAGES_DIR/vendor_boot.img"
OUT_DTBIMAGE="$IMAGES_DIR/dtb.img"

# Tools
MKBOOTIMG="$(pwd)/kernel_build/mkbootimg/mkbootimg.py"
MKDTBOIMG="$(pwd)/kernel_build/dtb/mkdtboimg.py"

## Customizable vars
# Kernel version
FK_VER="v5.5"

# Toggles
USE_CCACHE=1
DO_TAR=1
DO_ZIP=1

## Info message
DEVICE="Exynos 1280 Family"
CODENAME="exynos1280"

## Secrets
if [ -f "../chat_ci" ] && [ -f "../bot_token" ]; then
    chat_ci_hash=$(md5sum ../chat_ci | awk '{print $1}')
    bot_token_hash=$(md5sum ../bot_token | awk '{print $1}')

    if [ "$chat_ci_hash" != "68b329da9893e34099c7d8ad5cb9c940" ] || \
       [ "$bot_token_hash" != "68b329da9893e34099c7d8ad5cb9c940" ]; then
        TELEGRAM_CHAT_ID="$(<../chat_ci)"
        TELEGRAM_BOT_TOKEN="$(<../bot_token)"
        SECRETS="1"
    else
        SECRETS="0"
    fi
else
    SECRETS="0"
fi

## Parse arguments
DO_KSU=0
DO_SUKI=0
DO_CLEAN=0
DO_MENUCONFIG=0
IS_RELEASE=0
DO_TG=0
DO_REGEN=0
DO_OC=0
DO_FLTO=0
DO_QUIET=0
DO_PERM=0
DEFCONFIG=$DEFAULT_DEFCONFIG

for arg in "$@"; do
    if [[ "$arg" == *m* ]]; then
        echo "INFO: menuconfig argument passed, kernel configuration menu will be shown"
        DO_MENUCONFIG=1
    fi
    if [[ "$arg" == *k* ]]; then
        echo "INFO: KernelSU argument passed, a KernelSU build will be made"
        DO_KSU=1
    fi
    if [[ "$arg" == *s* ]]; then
        echo "INFO: SukiSU argument passed, a SukiSU build will be made"
        DO_SUKI=1
    fi
    if [[ "$arg" == *c* ]]; then
        echo "INFO: clean argument passed, output directory will be wiped"
        DO_CLEAN=1
    fi
    if [[ "$arg" == *R* ]]; then
        echo "INFO: Release argument passed, build marked as release"
        IS_RELEASE=1
    fi
    if [[ "$arg" == *t* ]]; then
        if [ "$SECRETS" = "0" ]; then
            echo "WARNING: Telegram argument was passed, but secrets were not found. Skipping Telegram Upload"  
        else
            echo "INFO: Telegram argument passed, build will be uploaded to CI"
            DO_TG=1
        fi
    fi
    if [[ "$arg" == *b* ]]; then
        echo "INFO: bashupload.com argument passed, build will be uploaded to bashupload.com"
        DO_BASHUP=1
    fi
    if [[ "$arg" == *r* ]]; then
        echo "INFO: config regeneration mode"
        DO_REGEN=1
    fi
    if [[ "$arg" == *u* ]]; then
        echo "INFO: Unlocked variant argument passed, unlocked build will be made"
        DO_OC=1
    fi
    if [[ "$arg" == *l* ]]; then
        echo "INFO: Full-LTO argument passed"
        echo "WARNING: Full-LTO is VERY resource heavy and may take a long time to compile"
        DO_FLTO=1
    fi
    if [[ "$arg" == *q* ]]; then
        echo "INFO: Quiet argument passed"
        echo "WARNING: Only errors and warnings will be shown"
        DO_QUIET=1
    fi
    if [[ "$arg" == *p* ]]; then
        echo "INFO: Permissive argument passed"
        DO_PERM=1
    fi
done

if [ "$DO_KSU" == "1" ] && [ "$DO_SUKI" == "1" ]; then
    echo "ERROR: KernelSU and SukiSU are mutually exclusive."
    exit 1
fi

if [ "$IS_RELEASE" == "1" ]; then
    BUILD_TYPE="Release"
else
    BUILD_TYPE="Testing"
fi

## Build type
LINUX_VER=$(make kernelversion 2>/dev/null)

if [ "$DO_KSU" == "1" ]; then
    FK_TYPE="KSUNext"
    FK_TYPE_SHORT="KN"
elif [ "$DO_SUKI" == "1" ]; then
    FK_TYPE="SukiSU-Ultra"
    FK_TYPE_SHORT="SSU"
else
    FK_TYPE="Vanilla"
    FK_TYPE_SHORT="V"
fi

if [ "$DO_OC" == "1" ]; then
    FK_TYPE="$FK_TYPE+Unlocked"
    FK_TYPE_SHORT="$FK_TYPE_SHORT+U"
fi

if [ "$DO_PERM" == "1" ]; then
    FK_TYPE="$FK_TYPE+Permissive"
    FK_TYPE_SHORT="$FK_TYPE_SHORT+P"
fi

ZIP_PATH="$KDIR/kernel_build/Floppy_$FK_VER-$FK_TYPE-$CODENAME-$DATE.zip"
TAR_PATH_ONEUI="$KDIR/kernel_build/FloppyOneUI_$FK_VER-$FK_TYPE-$CODENAME-$DATE.tar"
TAR_PATH_AOSP="$KDIR/kernel_build/FloppyAOSP_$FK_VER-$FK_TYPE-$CODENAME-$DATE.tar"

echo -e "\nINFO: Build info:
- Device: $DEVICE ($CODENAME)
- Addons: $FK_TYPE
- FloppyKernel version: $FK_VER
- Linux version: $LINUX_VER
- Defconfig: $DEFCONFIG
- Build date: $DATE
- Build type: $BUILD_TYPE
- Clean build: $([ "$DO_CLEAN" -eq 1 ] && echo "Yes" || echo "No")
- Permissive: $([ "$DO_PERM" -eq 1 ] && echo "Yes" || echo "No")
"

# Clean images dir
[ -d "$IMAGES_DIR" ] && rm -rf "$IMAGES_DIR" || true
mkdir -p "$IMAGES_DIR"

# Dependencies
source "$SCRIPTS_DIR/deps.sh"

# Configure Toolchain(s)
source "$SCRIPTS_DIR/tc.sh"

# Setup other things
source "$SCRIPTS_DIR/build.sh"
source "$SCRIPTS_DIR/post.sh"
source "$SCRIPTS_DIR/kpm.sh"
source "$SCRIPTS_DIR/images.sh"
source "$SCRIPTS_DIR/pack.sh"
source "$SCRIPTS_DIR/upload.sh"

prep_build() {
    if [ "$USE_CCACHE" == "1" ]; then
        echo "INFO: Using ccache"
        if [ "$IS_GP" == "1" ]; then
            export CCACHE_DIR="$WP/.ccache"
            ccache -M 10G
        else
            echo "WARNING: Environment is not Gitpod, please make sure you setup your own ccache configuration!"
        fi
    fi

    echo -e "INFO: Compiler: $KBUILD_COMPILER_STRING\n"
}


clean() {
    make clean $([[ "$arg" == *q* ]] && echo '> /dev/null 2>&1' || echo '')
    make mrproper $([[ "$arg" == *q* ]] && echo '> /dev/null 2>&1' || echo '')
}


# Do a clean build?
if [ "$DO_CLEAN" = "1" ]; then
    clean
fi

## Run build
prep_build
build

# Technically this should not be needed since we have "set -e" but still let's keep it
if [ ! -f "$OUT_KERNEL" ]; then
    echo -e "\nERROR: Kernel files not found! Compilation failed?"
    exit 1
fi

# Apply KPM patch for SukiSU builds
# DISABLE for now because it's not compatible with Exynos 5.10 kernels!
# if ! apply_kpm_patch; then
#     echo "ERROR: KPM patching failed"
#     exit 1
# fi

kernel_modules
build_images
packing
echo -e "\nINFO: Completed in $((SECONDS / 60)) minute(s) and $((SECONDS % 60)) second(s) !\n"
clean_tmp

upload

# Unset variables after build
unset AC_DIR PC_DIR LZ_DIR SL_DIR GC_DIR ZC_DIR RV_DIR CUST_DIR KBUILD_COMPILER_STRING
