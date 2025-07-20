packing() {
    echo -e "\nINFO: Building zip..."
    cd "$AK3_DIR"
    cp -f "$OUT_VENDORBOOTIMG" vendor_boot.img
    cp -f "$OUT_DTBIMAGE" dtb
    cp -f "$OUT_KERNEL" .
    zip -r9 -q "$ZIP_PATH" * -x .git .github README.md
    cd "$KDIR"
    echo -e "INFO: Done! \nINFO: Output: $ZIP_PATH\n"
    if [ "$AK3_TEST" != "1" ]; then
        rm -rf "$AK3_DIR"
    fi

    if [ "$DO_TAR" = "1" ]; then
        echo -e "\nINFO: Building TAR files..."
        cd "$KDIR/kernel_build"
        
        # Create OneUI TAR
        echo -e "INFO: Creating OneUI TAR..."
        rm -f "$TAR_PATH_ONEUI"
        lz4 -c -12 -B6 --content-size "$OUT_BOOTIMG_ONEUI" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_ONEUI" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        echo -e "INFO: OneUI TAR created! \nINFO: Output: $TAR_PATH_ONEUI"
        
        # Create AOSP TAR
        echo -e "INFO: Creating AOSP TAR..."
        rm -f "$TAR_PATH_AOSP"
        lz4 -c -12 -B6 --content-size "$OUT_BOOTIMG_AOSP" > boot.img.lz4 2>/dev/null
        lz4 -c -12 -B6 --content-size "$OUT_VENDORBOOTIMG" > vendor_boot.img.lz4 2>/dev/null
        tar -cf "$TAR_PATH_AOSP" boot.img.lz4 vendor_boot.img.lz4
        rm -f boot.img.lz4 vendor_boot.img.lz4
        echo -e "INFO: AOSP TAR created! \nINFO: Output: $TAR_PATH_AOSP\n"
        
        cd "$KDIR"
    fi
}
