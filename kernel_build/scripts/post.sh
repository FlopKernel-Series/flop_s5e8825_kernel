post_build() {
    if [[ ! -f "$OUT_KERNEL" ]]; then
        echo -e "\nERROR: Kernel files not found! Compilation failed?"
        if [[ "$DO_OSHI" == "1" ]]; then
            echo -e "\nINFO: Uploading log to oshi.at\n"
            curl -T log.txt oshi.at
        fi
        exit 1
    fi

    rm -rf "$TMPDIR"
    rm -f "$OUT_BOOTIMG" "$OUT_VENDORBOOTIMG"
    mkdir -p "$TMPDIR" "$MODULES_DIR/0.0" "$PLATFORM_RAMDISK_DIR"

    cp -rf "$IN_PLATFORM"/* "$PLATFORM_RAMDISK_DIR/"
    mkdir "$PLATFORM_RAMDISK_DIR/first_stage_ramdisk"
    cp -f "$PLATFORM_RAMDISK_DIR/fstab.s5e8825" "$PLATFORM_RAMDISK_DIR/first_stage_ramdisk/fstab.s5e8825"

    if ! find "$MOD_OUTDIR/lib/modules" -mindepth 1 -type d | read; then
        echo -e "\nERROR: Unknown error!\n"
        exit 1
    fi

    missing_modules=""
    for module in $(cat "$IN_DLKM/modules.load"); do
        i=$(find "$MOD_OUTDIR/lib/modules" -name "$module")
        if [[ -f "$i" ]]; then
            cp -f "$i" "$MODULES_DIR/0.0/$module"
        else
            missing_modules="$missing_modules $module"
        fi
    done

    if [[ -n "$missing_modules" ]]; then
        echo "ERROR: the following modules were not found: $missing_modules"
        exit 1
    fi

    depmod 0.0 -b "$DLKM_RAMDISK_DIR"
    sed -i 's/\([^ ]\+\)/\/lib\/modules\/\1/g' "$MODULES_DIR/0.0/modules.dep"
    cd "$MODULES_DIR/0.0"
    for i in $(find . -name "modules.*" -type f); do
        if [[ "$(basename "$i")" != "modules.dep" && "$(basename "$i")" != "modules.softdep" && "$(basename "$i")" != "modules.alias" ]]; then
            rm -f "$i"
        fi
    done
    cd "$KDIR"

    cp -f "$IN_DLKM/modules.load" "$MODULES_DIR/0.0/modules.load"
    mv "$MODULES_DIR/0.0"/* "$MODULES_DIR/"
    rm -rf "$MODULES_DIR/0.0"
}
