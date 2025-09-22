build() {
    # Delete log.txt at the start
    rm -f log.txt

    if [ "$USE_CCACHE" == "1" ]; then
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

    rm -rf "$MOD_OUTDIR" 2>/dev/null

    if [ "$DO_QUIET" = "1" ]; then
        make -j$(nproc --all) O=$OUTDIR CC="$CC" "$DEFCONFIG" \
            $([ "$DO_KSU" = "1" ] && echo "ksu.config") > /dev/null | tee log.txt
    else
        make -j$(nproc --all) O=$OUTDIR CC="$CC" "$DEFCONFIG" \
            $([ "$DO_KSU" = "1" ] && echo "ksu.config") 2>&1 | tee log.txt
    fi

    if [ "$IS_RELEASE" = "1" ]; then
        VERSION_STR="\"-Floppy-$FK_VER-$FK_TYPE_SHORT/release\""
        VERSION_NOAUTO="1"
    else
        VERSION_STR="\"-Floppy-$FK_VER-$FK_TYPE_SHORT/\""
    fi

    rm -f "$OUT_KERNEL"

    if [ "$DO_REGEN" = "1" ]; then
        if [ "$DO_KSU" = "1" ]; then
            echo "ERROR: Can't regenerate with KSU argument"
            exit 1
        fi
        if [ "$DO_PERM" = "1" ]; then
            echo "ERROR: Can't regenerate with Permissive argument"
            exit 1
        fi
        cp -f out/.config arch/arm64/configs/$DEFCONFIG
        echo "INFO: Configuration regenerated. Check the changes!"
        exit 0
    fi

    scripts/config --file "$KDIR/out/.config" --set-val LOCALVERSION "$VERSION_STR"

    if [ "$VERSION_NOAUTO" = "1" ]; then
        scripts/config --file "$KDIR/out/.config" --disable LOCALVERSION_AUTO
    fi

    if [ "$DO_OC" == "1" ]; then
        scripts/config --file "$KDIR/out/.config" --enable CONFIG_SOC_S5E8825_OVERCLOCK
        scripts/config --file "$KDIR/out/.config" --enable CONFIG_SOC_S5E8825_GPU_OC
        scripts/config --file "$KDIR/out/.config" --set-val CONFIG_SOC_S5E8825_CL1_UV 0
        scripts/config --file "$KDIR/out/.config" --set-val CONFIG_SOC_S5E8825_CL0_UV 0
    fi

    if [ "$DO_MENUCONFIG" = "1" ]; then
        if [ "$DO_QUIET" = "1" ]; then
            make O=$OUTDIR CC="$CC" menuconfig > /dev/null 2>&1
        else
            make O=$OUTDIR CC="$CC" menuconfig 2>&1 >> log.txt
        fi
    fi

    if [ "$DO_FLTO" = "1" ]; then
        scripts/config --file "$KDIR/out/.config" --enable CONFIG_LTO_CLANG_FULL
        scripts/config --file "$KDIR/out/.config" --disable CONFIG_LTO_CLANG_THIN
    fi

    if [ "$DO_PERM" = "1" ]; then
        scripts/config --file "$KDIR/out/.config" --enable CONFIG_SECURITY_SELINUX_ALWAYS_PERMISSIVE
    fi

    echo -e "\nINFO: Starting compilation...\n"

    if [ "$DO_QUIET" = "1" ]; then
        make -j$(nproc --all) O=$OUTDIR CC="$CC" dtbs > /dev/null | tee log.txt

        make -j$(nproc --all) O=$OUTDIR CC="$CC" >/dev/null | tee log.txt

        make -j$(nproc --all) O=$OUTDIR CC="$CC" \
            INSTALL_MOD_STRIP="--strip-debug --keep-section=.ARM.attributes" \
            INSTALL_MOD_PATH="$MOD_OUTDIR" modules_install >/dev/null | tee log.txt
    else
        make -j$(nproc --all) O=$OUTDIR CC="$CC" dtbs 2>&1 | tee -a log.txt

        make -j$(nproc --all) O=$OUTDIR CC="$CC" 2>&1 | tee -a log.txt
        
        make -j$(nproc --all) O=$OUTDIR CC="$CC" \
            INSTALL_MOD_STRIP="--strip-debug --keep-section=.ARM.attributes" \
            INSTALL_MOD_PATH="$MOD_OUTDIR" modules_install 2>&1 | tee -a log.txt
    fi
}
