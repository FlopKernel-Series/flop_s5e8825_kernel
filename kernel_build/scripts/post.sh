#
# Module packaging logic
#

kernel_modules() {
    local i

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

    # Find the installed modules directory
    local kmod_dir
    kmod_dir=$(find "$MOD_OUTDIR/lib/modules" -mindepth 1 -maxdepth 1 -type d | head -n 1)
    
    echo "INFO: Generating modules.load..."
    "$SCRIPTS_DIR/gen_modules_load.sh" "$kmod_dir" "$TMPDIR/modules.load" "$KDIR"

    if [ ! -f "$TMPDIR/modules.load" ]; then
         echo "ERROR: Failed to generate modules.load"
         exit 1
    fi

    # Build module lookup table
    declare -A MODULE_MAP
    while IFS= read -r path; do
        name=$(basename "$path")
        MODULE_MAP["$name"]="$path"
    done < <(find "$MOD_OUTDIR/lib/modules" -name "*.ko" -type f)

    for module in $(cat "$TMPDIR/modules.load"); do
        local src="${MODULE_MAP[$module]}"
        if [ -n "$src" ] && [ -f "$src" ]; then
            cp -f "$src" "$MODULES_DIR/0.0/$module"
        fi
    done

    depmod 0.0 -b "$DLKM_RAMDISK_DIR"
    sed -i 's/\([^ ]\+\)/\/lib\/modules\/\1/g' "$MODULES_DIR/0.0/modules.dep"
    cd "$MODULES_DIR/0.0"
    for i in $(find . -name "modules.*" -type f); do
        if [[ "$(basename "$i")" != "modules.dep" && "$(basename "$i")" != "modules.softdep" && "$(basename "$i")" != "modules.alias" ]]; then
            rm -f "$i"
        fi
    done
  
    cd "$KDIR"

    cp -f "$TMPDIR/modules.load" "$MODULES_DIR/0.0/modules.load"
    mv "$MODULES_DIR/0.0"/* "$MODULES_DIR/"
    rm -rf "$MODULES_DIR/0.0"
}

clean_tmp() {
    echo -e "INFO: Cleaning after build..."
    rm -rf "$TMPDIR" "$MOD_OUTDIR"
}
