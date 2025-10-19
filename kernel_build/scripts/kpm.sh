#!/bin/bash
#
# KPM patching script
# Based on: https://github.com/TheSillyOk/kernel_workflows/blob/219f69677c6fe959232bdba368746f4dcf404906/.github/workflows/build_kernels.yml#L267
#

apply_kpm_patch() {
    if [[ "$DO_SUKI" != "1" ]]; then
        return 0
    fi

    echo -e "\nINFO: Applying KPM patch..."

    # Vars
    local KPM_URL="https://raw.githubusercontent.com/ShirkNeko/SukiSU_patch/refs/heads/main/kpm/patch_linux"
    local MAGISKBOOT="$KDIR/kernel_build/bin/magiskboot"
    local WORK_DIR="$WP/kpm_work"
    local FULL_OUT_IMAGE="$OUT_KERNEL"

    # Check if magiskboot exists and is executable
    if [[ ! -f "$MAGISKBOOT" ]] || [[ ! -x "$MAGISKBOOT" ]]; then
        echo "ERROR: magiskboot not found or not executable at $MAGISKBOOT"
        return 1
    fi

    # Setup
    mkdir -p "$WORK_DIR"
    cd "$WORK_DIR"

    if ! curl -LSs "$KPM_URL" -o patch; then
        echo "ERROR: Failed to download KPM patch script"
        cd "$KDIR"
        rm -rf "$WORK_DIR"
        return 1
    fi
    chmod +x patch

    # Copy kernel image to working directory
    local img_file=""
    if [[ -f "$FULL_OUT_IMAGE" ]]; then
        cp "$FULL_OUT_IMAGE" .
        img_file=$(basename "$FULL_OUT_IMAGE")
    else
        echo "ERROR: Kernel image not found at $FULL_OUT_IMAGE"
        cd "$KDIR"
        rm -rf "$WORK_DIR"
        return 1
    fi

    # Extract kernel image for patching
    if [[ "$img_file" == *"Image.gz-dtb" ]]; then
        echo "INFO: Extracting kernel from Image.gz-dtb..."
        if ! "$MAGISKBOOT" split "$img_file" || [[ ! -f kernel ]]; then
            echo "ERROR: Failed to split $img_file"
            cd "$KDIR"
            rm -rf "$WORK_DIR"
            return 1
        fi
        cp kernel Image
    elif [[ "$img_file" == *"Image.gz" ]]; then
        echo "INFO: Decompressing Image.gz..."
        if ! "$MAGISKBOOT" decompress "$img_file" Image 2>/dev/null && ! gunzip -c "$img_file" > Image 2>/dev/null; then
            echo "ERROR: Failed to decompress $img_file"
            cd "$KDIR" || exit
            rm -rf "$WORK_DIR"
            return 1
        fi
    elif [[ "$img_file" == *"Image" ]]; then
        # Image is already in the correct format, no need to copy
        echo "INFO: Using uncompressed Image directly"
    else
        echo "ERROR: Unsupported kernel image format: $img_file"
        cd "$KDIR"
        rm -rf "$WORK_DIR"
        return 1
    fi

    # Check Image file
    if [[ ! -f Image ]]; then
        echo "ERROR: Image file not found after extraction"
        cd "$KDIR"
        rm -rf "$WORK_DIR"
        return 1
    fi

    # Apply KPM patch
    echo "INFO: Patching kernel..."
    if ./patch 2>&1; then
        if [[ -f oImage ]]; then
            mv oImage Image
        fi
    else
        echo "ERROR: KPM patch script failed"
        cd "$KDIR"
        rm -rf "$WORK_DIR"
        return 1
    fi

    # Recompress and copy back to original location
    echo "INFO: Repacking kernel image..."
    if [[ "$img_file" == *"Image.gz-dtb" ]]; then
        if ! "$MAGISKBOOT" compress=gzip Image kernel_new && ! gzip -c Image > kernel_new; then
            echo "ERROR: Failed to compress patched Image"
            cd "$KDIR"
            rm -rf "$WORK_DIR"
            return 1
        fi

        if [[ ! -f kernel_dtb ]]; then
            echo "ERROR: kernel_dtb not found, cannot recreate Image.gz-dtb"
            cd "$KDIR"
            rm -rf "$WORK_DIR"
            return 1
        fi

        cat kernel_new kernel_dtb > Image.gz-dtb
        cp Image.gz-dtb "$FULL_OUT_IMAGE"
    elif [[ "$img_file" == *"Image.gz" ]]; then
        if ! gzip -c Image > Image.gz; then
            echo "ERROR: Failed to compress patched Image"
            cd "$KDIR"
            rm -rf "$WORK_DIR"
            return 1
        fi
        cp Image.gz "$FULL_OUT_IMAGE"
    else
        cp Image "$FULL_OUT_IMAGE"
    fi

    echo "INFO: KPM patching completed successfully"
    cd "$KDIR"
    rm -rf "$WORK_DIR"
    return 0
}