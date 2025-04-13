build_images() {
    echo -e "\nINFO: Building dtb image..."
    if [[ "$DO_OC" == "1" ]]; then
        python "$MKDTBOIMG" create "$OUT_DTBIMAGE" --custom0=0x00000000 --custom1=0xff000000 --version=0 --page_size=2048 "$IN_DTB_OC" || exit 1
    else
        python "$MKDTBOIMG" create "$OUT_DTBIMAGE" --custom0=0x00000000 --custom1=0xff000000 --version=0 --page_size=2048 "$IN_DTB" || exit 1
    fi

    MONTH="$(date +%Y-%m)"

    echo -e "\nINFO: Building boot image..."
    "$MKBOOTIMG" --header_version 4 \
        --kernel "$OUT_KERNEL" \
        --output "$OUT_BOOTIMG" \
        --ramdisk "$PREBUILT_RAMDISK" \
        --os_version 15.0.0 \
        --os_patch_level "$MONTH" || exit 1
    echo -e "INFO: Done!"

    echo -e "\nINFO: Building vendor_boot image..."
    cd "$DLKM_RAMDISK_DIR"
    find . | cpio --quiet -o -H newc -R root:root | lz4 -9cl > "../ramdisk_dlkm.lz4"
    cd "../ramdisk_platform"
    find . | cpio --quiet -o -H newc -R root:root | lz4 -9cl > "../ramdisk_platform.lz4"
    cd ".."
    echo "buildtime_bootconfig=enable" > bootconfig

    "$MKBOOTIMG" --header_version 4 \
        --vendor_boot "$OUT_VENDORBOOTIMG" \
        --vendor_bootconfig "$(pwd)/bootconfig" \
        --dtb "$OUT_DTBIMAGE" \
        --vendor_ramdisk "$(pwd)/ramdisk_platform.lz4" \
        --ramdisk_type dlkm \
        --ramdisk_name dlkm \
        --vendor_ramdisk_fragment "$(pwd)/ramdisk_dlkm.lz4" \
        --os_version 15.0.0 \
        --os_patch_level "$MONTH" || exit 1

    cd "$KDIR"
    echo -e "INFO: Done!"
}
