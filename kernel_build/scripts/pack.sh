packing() {
    echo -e "\n$(log_info "Building zip...")"
    cd "$AK3_DIR"
    cp -f "$OUT_VENDORBOOTIMG" vendor_boot.img
    cp -f "$OUT_DTBIMAGE" dtb
    cp -f "$OUT_KERNEL" .
    zip -r9 -q "$ZIP_PATH" * -x .git .github README.md
    cd "$KDIR"
    echo -e "$(log_info "Done!") \n$(log_info "Output: $ZIP_PATH")\n"
    if [ "$AK3_TEST" != "1" ]; then
        rm -rf "$AK3_DIR"
    fi

    if [ "$DO_TAR" = "1" ]; then
        echo -e "\n$(log_info "Building TAR files...")"
        cd "$KDIR/kernel_build"

        # Create OneUI TAR
        echo -e "\n$(log_info "Creating OneUI TAR...")"
        rm -f "$TAR_PATH_ONEUI"
        lz4 -c -12 -B6 --content-size "$OUT_BOOTIMG_ONEUI" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_ONEUI" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        echo -e "$(log_info "Output: $TAR_PATH_ONEUI")"

        # Create AOSP TAR
        echo -e "\n$(log_info "Creating AOSP TAR...")"
        rm -f "$TAR_PATH_AOSP"
        lz4 -c -12 -B6 --content-size "$OUT_BOOTIMG_AOSP" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_AOSP" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        echo -e "$(log_info "Output: $TAR_PATH_AOSP")"

        # Create OneUI Permissive TAR (if requested)
        if [ "$DO_PERM" = "1" ]; then
            echo -e "\n$(log_info "Creating OneUI Permissive TAR...")"
            local TAR_PATH_ONEUI_PERM="${TAR_PATH_ONEUI}"
            local ONEUI_PERM_KERNEL="$TMPDIR/Image_oneui_perm"
            cp "$OUT_KERNEL" "$ONEUI_PERM_KERNEL"

            # Patch kernel: force_perm=0 -> force_perm=1
            "$KDIR/kernel_build/bin/magiskboot" hexpatch "$ONEUI_PERM_KERNEL" \
                666f7263655f7065726d3d30 \
                666f7263655f7065726d3d31 >/dev/null 2>&1 || exit 1

            # Create boot image with permissive kernel (aosp_mode=0, force_perm=1)
            local ONEUI_PERM_BOOTIMG="$TMPDIR/boot_oneui_perm.img"
            "$MKBOOTIMG" --header_version 4 \
                --kernel "$ONEUI_PERM_KERNEL" \
                --output "$ONEUI_PERM_BOOTIMG" \
                --ramdisk "$PREBUILT_RAMDISK" \
                --os_version 15.0.0 \
                --os_patch_level "$(date +%Y-%m)" || exit 1

            rm -f "$TAR_PATH_ONEUI_PERM"
            lz4 -c -12 -B6 --content-size "$ONEUI_PERM_BOOTIMG" > boot.img.lz4 2>/dev/null
            lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
            tar -cf "$TAR_PATH_ONEUI_PERM" boot.img.lz4 vendor_boot.img.lz4
            rm -f boot.img.lz4 vendor_boot.img.lz4
            rm -f "$ONEUI_PERM_KERNEL" "$ONEUI_PERM_BOOTIMG"
            echo -e "$(log_info "Output: $TAR_PATH_ONEUI_PERM")"

            # Create AOSP Permissive TAR
            echo -e "\n$(log_info "Creating AOSP Permissive TAR...")"
            local TAR_PATH_AOSP_PERM="${TAR_PATH_AOSP}"
            local AOSP_PERM_KERNEL="$TMPDIR/Image_aosp_perm"
            cp "$OUT_KERNEL" "$AOSP_PERM_KERNEL"

            # Patch kernel: aosp_mode=0 -> aosp_mode=1
            "$KDIR/kernel_build/bin/magiskboot" hexpatch "$AOSP_PERM_KERNEL" \
                616f73705f6d6f64653d30 \
                616f73705f6d6f64653d31 >/dev/null 2>&1 || exit 1

            # Patch kernel: force_perm=0 -> force_perm=1
            "$KDIR/kernel_build/bin/magiskboot" hexpatch "$AOSP_PERM_KERNEL" \
                666f7263655f7065726d3d30 \
                666f7263655f7065726d3d31 >/dev/null 2>&1 || exit 1

            # Create boot image with permissive kernel (aosp_mode=1, force_perm=1)
            local AOSP_PERM_BOOTIMG="$TMPDIR/boot_aosp_perm.img"
            "$MKBOOTIMG" --header_version 4 \
                --kernel "$AOSP_PERM_KERNEL" \
                --output "$AOSP_PERM_BOOTIMG" \
                --ramdisk "$PREBUILT_RAMDISK" \
                --os_version 15.0.0 \
                --os_patch_level "$(date +%Y-%m)" || exit 1

            rm -f "$TAR_PATH_AOSP_PERM"
            lz4 -c -12 -B6 --content-size "$AOSP_PERM_BOOTIMG" > boot.img.lz4 2>/dev/null
            lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
            tar -cf "$TAR_PATH_AOSP_PERM" boot.img.lz4 vendor_boot.img.lz4
            rm -f boot.img.lz4 vendor_boot.img.lz4
            rm -f "$AOSP_PERM_KERNEL" "$AOSP_PERM_BOOTIMG"
            echo -e "$(log_info "Output: $TAR_PATH_AOSP_PERM")"
        fi

        # Create OneUI Unlocked TAR
        echo -e "\n$(log_info "Creating OneUI Unlocked TAR...")"
        local TAR_PATH_ONEUI_UNLOCKED="${TAR_PATH_ONEUI/-$FK_TYPE_TAR-/-$FK_TYPE_TAR+Unlocked-}"
        local ONEUI_UNLOCKED_KERNEL="$TMPDIR/Image_oneui_unlocked"
        cp "$OUT_KERNEL" "$ONEUI_UNLOCKED_KERNEL"

        # Patch kernel: superfloppy=0 -> superfloppy=1
        "$KDIR/kernel_build/bin/magiskboot" hexpatch "$ONEUI_UNLOCKED_KERNEL" \
            7375706572666c6f7070793d30 \
            7375706572666c6f7070793d31 >/dev/null 2>&1 || exit 1

        # Create boot image with unlocked kernel (aosp_mode=0, superfloppy=1)
        local ONEUI_UNLOCKED_BOOTIMG="$TMPDIR/boot_oneui_unlocked.img"
        "$MKBOOTIMG" --header_version 4 \
            --kernel "$ONEUI_UNLOCKED_KERNEL" \
            --output "$ONEUI_UNLOCKED_BOOTIMG" \
            --ramdisk "$PREBUILT_RAMDISK" \
            --os_version 15.0.0 \
            --os_patch_level "$(date +%Y-%m)" || exit 1

        rm -f "$TAR_PATH_ONEUI_UNLOCKED"
        lz4 -c -12 -B6 --content-size "$ONEUI_UNLOCKED_BOOTIMG" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_ONEUI_UNLOCKED" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        rm -f "$ONEUI_UNLOCKED_KERNEL" "$ONEUI_UNLOCKED_BOOTIMG"
        echo -e "$(log_info "Output: $TAR_PATH_ONEUI_UNLOCKED")"

        # Create AOSP Unlocked TAR
        echo -e "\n$(log_info "Creating AOSP Unlocked TAR...")"
        local TAR_PATH_AOSP_UNLOCKED="${TAR_PATH_AOSP/-$FK_TYPE_TAR-/-$FK_TYPE_TAR+Unlocked-}"
        local AOSP_UNLOCKED_KERNEL="$TMPDIR/Image_aosp_unlocked"
        cp "$OUT_KERNEL" "$AOSP_UNLOCKED_KERNEL"

        # Patch kernel: aosp_mode=0 -> aosp_mode=1
        "$KDIR/kernel_build/bin/magiskboot" hexpatch "$AOSP_UNLOCKED_KERNEL" \
            616f73705f6d6f64653d30 \
            616f73705f6d6f64653d31 >/dev/null 2>&1 || exit 1

        # Patch kernel: superfloppy=0 -> superfloppy=1
        "$KDIR/kernel_build/bin/magiskboot" hexpatch "$AOSP_UNLOCKED_KERNEL" \
            7375706572666c6f7070793d30 \
            7375706572666c6f7070793d31 >/dev/null 2>&1 || exit 1

        # Create boot image with unlocked kernel (aosp_mode=1, superfloppy=1)
        local AOSP_UNLOCKED_BOOTIMG="$TMPDIR/boot_aosp_unlocked.img"
        "$MKBOOTIMG" --header_version 4 \
            --kernel "$AOSP_UNLOCKED_KERNEL" \
            --output "$AOSP_UNLOCKED_BOOTIMG" \
            --ramdisk "$PREBUILT_RAMDISK" \
            --os_version 15.0.0 \
            --os_patch_level "$(date +%Y-%m)" || exit 1

        rm -f "$TAR_PATH_AOSP_UNLOCKED"
        lz4 -c -12 -B6 --content-size "$AOSP_UNLOCKED_BOOTIMG" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_AOSP_UNLOCKED" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        rm -f "$AOSP_UNLOCKED_KERNEL" "$AOSP_UNLOCKED_BOOTIMG"
        echo -e "$(log_info "Output: $TAR_PATH_AOSP_UNLOCKED")"

        cd "$KDIR"
    fi
}
