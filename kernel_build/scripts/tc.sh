# Toolchains
AOSP_LIST="https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/mirror-goog-main-llvm-toolchain-source"
AOSP_ARCHIVE="https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/mirror-goog-main-llvm-toolchain-source"
PC_REPO="https://github.com/kdrag0n/proton-clang"
LZ_REPO="https://gitlab.com/Jprimero15/lolz_clang.git"
SL_REPO="http://ftp.twaren.net/Unix/Kernel/tools/llvm/files/"
GC_REPO="https://api.github.com/repos/greenforce-project/greenforce_clang/releases/latest"
ZC_REPO="https://raw.githubusercontent.com/ZyCromerZ/Clang/refs/heads/main/Clang-main-link.txt"
RV_REPO="https://api.github.com/repos/Rv-Project/RvClang/releases/latest"
GCC64_REPO="https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-gnu-9.3"

# Logging helpers are expected to be provided by the caller (ckbuild.sh)
# via sourcing kernel_build/lib/log.sh. Standalone usage is not supported.

# Toolchain Dirs
export TC_DIR="$WP/toolchains"
export GCC64_DIR="$TC_DIR/gcc64"
export AC_DIR="$TC_DIR/aospclang"
export PC_DIR="$TC_DIR/protonclang"
export LZ_DIR="$TC_DIR/lolzclang"
export SL_DIR="$TC_DIR/slimllvm"
export GC_DIR="$TC_DIR/greenforceclang"
export ZC_DIR="$TC_DIR/zycclang"
export RV_DIR="$TC_DIR/rvclang"

if [ ! -d "$TC_DIR" ]; then
    mkdir -p "$TC_DIR"
fi

# Custom toolchain directory
if [ -z "$CUST_DIR" ]; then
    export CUST_DIR="$TC_DIR/custom-toolchain"
else
    log_info "Overriding custom toolchain path..."
fi

# aosp, proton, lolz, slim, greenforce, zyc, rv, custom
if [ -z "$CLANG_TYPE" ]; then
    export CLANG_TYPE="aosp"
else
    log_info "Overriding default toolchain"
fi

download_file() {
    local url="$1"
    local dest_dir="$2"
    local dest_file="$3"
    local target="$dest_dir/$dest_file"

    if ! command -v aria2c &>/dev/null; then
        log_err "aria2c is required for downloading toolchains but was not found in PATH! Aborting..."
        return 1
    fi

    mkdir -p "$dest_dir"
    rm -f "$target" "${target}.aria2"

    if ! aria2c -x 16 -s 16 -k 1M --allow-overwrite=true --auto-file-renaming=false \
                --max-tries=5 --retry-wait=2 --connect-timeout=30 --timeout=60 \
                -d "$dest_dir" -o "$dest_file" "$url"; then
        log_err "Download failed from $url! Aborting..."
        rm -f "$target" "${target}.aria2"
        return 1
    fi

    if [ ! -f "$target" ]; then
        log_err "Downloaded file $target was not found after download! Aborting..."
        rm -f "${target}.aria2"
        return 1
    fi

    return 0
}

extract_tar() {
    local archive="$1"
    local target_dir="$2"

    mkdir -p "$target_dir"
    if ! tar -xf "$archive" -C "$target_dir"; then
        log_err "Extraction of $(basename "$archive") failed! Aborting..."
        rm -f "$archive" "${archive}.aria2"
        rm -rf "$target_dir"
        return 1
    fi
    rm -f "$archive" "${archive}.aria2"
    return 0
}

get_toolchain() {
    local toolchain_type="$1"
    local toolchain_dir=""

    case "$toolchain_type" in
        aosp)
            toolchain_dir="$AC_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "AOSP Clang not found! Downloading to $toolchain_dir..."
                # scrape the HTML directory listing on the mirror‑GOOG branch
                HTML=$(curl -s "$AOSP_LIST")
                CURRENT_CLANG=$(
                    printf '%s\n' "$HTML" \
                    | grep -oP 'href="[^"]*clang-r[0-9]+/' \
                    | grep -oP 'clang-r[0-9]+' \
                    | sort -V \
                    | tail -n1
                )

                if [ -z "$CURRENT_CLANG" ]; then
                    log_err "couldn’t find any clang-r### dirs in $AOSP_LIST"
                    exit 1
                fi

                log_info "Latest AOSP Clang is $CURRENT_CLANG, downloading…"
                local archive_name="${CURRENT_CLANG}.tar.gz"
                if ! download_file "${AOSP_ARCHIVE}/${archive_name}" "$TC_DIR" "$archive_name"; then
                    log_err "Download failed! Aborting..."
                    exit 1
                fi
                if ! extract_tar "$TC_DIR/$archive_name" "$toolchain_dir"; then
                    log_err "Extraction failed! Aborting..."
                    exit 1
                fi
                mkdir -p "$toolchain_dir/bin"
                touch "$toolchain_dir/bin/aarch64-linux-gnu-elfedit" && chmod +x "$toolchain_dir/bin/aarch64-linux-gnu-elfedit"
                touch "$toolchain_dir/bin/arm-linux-gnueabi-elfedit" && chmod +x "$toolchain_dir/bin/arm-linux-gnueabi-elfedit"
            fi
            ;;
        proton)
            toolchain_dir="$PC_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "Proton Clang not found! Cloning to $toolchain_dir..."
                if ! git clone -q --depth=1 "$PC_REPO" "$toolchain_dir"; then
                    log_err "Cloning failed! Aborting..."
                    exit 1
                fi
            fi
            ;;
        lolz)
            toolchain_dir="$LZ_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "Lolz Clang not found! Cloning to $toolchain_dir..."
                if ! git clone -q --depth=1 "$LZ_REPO" "$toolchain_dir"; then
                    log_err "Cloning failed! Aborting..."
                    exit 1
                fi
            fi
            ;;
        slim)
            toolchain_dir="$SL_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "Slim LLVM not found! Downloading to $toolchain_dir..."
                FILENAMES=$(curl -s "$SL_REPO" | grep -oP 'llvm-[\d.]+-x86_64\.tar\.xz')
                LATEST_FILE=$(echo "$FILENAMES" | sort -V | tail -n 1)
                if [ -z "$LATEST_FILE" ]; then
                    log_err "Failed to find latest Slim LLVM archive! Aborting..."
                    exit 1
                fi
                if ! download_file "${SL_REPO}${LATEST_FILE}" "$TC_DIR" "${LATEST_FILE}"; then
                    log_err "Download failed! Aborting..."
                    exit 1
                fi
                EXTRACTED_FOLDER=$(basename "$LATEST_FILE" .tar.xz)
                if ! extract_tar "$TC_DIR/${LATEST_FILE}" "$toolchain_dir"; then
                    log_err "Extraction failed! Aborting..."
                    exit 1
                fi
                if [ -d "$toolchain_dir/$EXTRACTED_FOLDER" ]; then
                    mv "$toolchain_dir/$EXTRACTED_FOLDER"/* "$toolchain_dir"
                    rmdir "$toolchain_dir/$EXTRACTED_FOLDER"
                fi
            fi
            ;;
        greenforce)
            toolchain_dir="$GC_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "Greenforce Clang not found! Downloading to $toolchain_dir..."
                LATEST_RELEASE=$(curl -s $GC_REPO | grep "browser_download_url" | grep ".tar.gz" | cut -d '"' -f 4)
                if [ -z "$LATEST_RELEASE" ]; then
                    log_err "Failed to fetch the latest Greenforce Clang release! Aborting..."
                    exit 1
                fi
                if ! download_file "$LATEST_RELEASE" "$TC_DIR" "greenforce-clang.tar.gz"; then
                    log_err "Download failed! Aborting..."
                    exit 1
                fi
                if ! extract_tar "$TC_DIR/greenforce-clang.tar.gz" "$toolchain_dir"; then
                    log_err "Extraction failed! Aborting..."
                    exit 1
                fi
            fi
            ;;
        custom)
            toolchain_dir="$CUST_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_err "Custom toolchain not found! Aborting..."
                log_info "Please provide a toolchain at $CUST_DIR or select a different toolchain"
                exit 1
            fi
            ;;
        zyc)
            toolchain_dir="$ZC_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "ZyC Clang not found! Downloading to $toolchain_dir..."
            fi

            ZYC_VERSION_FILE="$WP/zyc-clang-version.txt"
            LATEST_VERSION=$(curl -s "$ZC_REPO" | head -n 1)
            if [ -z "$LATEST_VERSION" ]; then
                log_warn "Failed to check ZyC Clang version"
            else
                if [ -f "$ZYC_VERSION_FILE" ]; then
                    CURRENT_VERSION=$(cat "$ZYC_VERSION_FILE")
                    if [ "$CURRENT_VERSION" != "$LATEST_VERSION" ]; then
                        log_info "A new version of ZyC Clang is available: $LATEST_VERSION"
                        echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                    fi
                else
                    echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                fi
            fi

            if [ ! -d "$toolchain_dir" ]; then
                if [ -f "$ZYC_VERSION_FILE" ]; then
                    echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                fi
                if [ -z "$LATEST_VERSION" ]; then
                    log_err "Failed to fetch the latest ZyC Clang release! Aborting..."
                    exit 1
                fi
                if ! download_file "$LATEST_VERSION" "$TC_DIR" "zyc-clang.tar.gz"; then
                    log_err "Download failed! Aborting..."
                    rm -f "$ZYC_VERSION_FILE"
                    exit 1
                fi
                if ! extract_tar "$TC_DIR/zyc-clang.tar.gz" "$toolchain_dir"; then
                    log_err "Extraction failed! Aborting..."
                    rm -f "$WP/zyc-clang.tar.gz" "$ZYC_VERSION_FILE"
                    exit 1
                fi
            fi
            ;;
        rv)
            toolchain_dir="$RV_DIR"
            if [ ! -d "$toolchain_dir" ]; then
                log_info "RvClang not found! Fetching the latest version..."
                LATEST_RELEASE=$(curl -s "$RV_REPO" | grep "browser_download_url" | grep ".tar.gz" | cut -d '"' -f 4)
                if [ -z "$LATEST_RELEASE" ]; then
                    log_err "Failed to fetch the latest RvClang release! Aborting..."
                    exit 1
                fi
                if ! download_file "$LATEST_RELEASE" "$TC_DIR" "rvclang.tar.gz"; then
                    log_err "Download failed! Aborting..."
                    exit 1
                fi
                if ! extract_tar "$TC_DIR/rvclang.tar.gz" "$toolchain_dir"; then
                    log_err "Extraction failed! Aborting..."
                    exit 1
                fi
                if [ -d "$toolchain_dir/RvClang" ]; then
                    mv "$toolchain_dir/RvClang"/* "$toolchain_dir/"
                    rmdir "$toolchain_dir/RvClang"
                fi
            fi
            ;;
        *)
              log_err "Unknown toolchain type: $toolchain_type"
              exit 1
              ;;
    esac
}

prep_toolchain() {
      local toolchain_type="$1"
      local toolchain_dir=""

      case "$toolchain_type" in
          aosp)
              toolchain_dir="$AC_DIR"
              log_info "Toolchain: AOSP Clang"
              ;;
          proton)
              toolchain_dir="$PC_DIR"
              log_info "Toolchain: Proton Clang"
              ;;
          lolz)
              toolchain_dir="$LZ_DIR"
              log_info "Toolchain: Lolz Clang"
              ;;
          slim)
              toolchain_dir="$SL_DIR"
              log_info "Toolchain: Slim LLVM Clang"
              ;;
          greenforce)
              toolchain_dir="$GC_DIR"
              log_info "Toolchain: Greenforce Clang"
              ;;
          custom)
              toolchain_dir="$CUST_DIR"
              log_info "Toolchain: Custom"
              ;;
          zyc)
              toolchain_dir="$ZC_DIR"
              log_info "Toolchain: ZyC Clang"
              ;;
          rv)
              toolchain_dir="$RV_DIR"
              log_info "Toolchain: RvClang"
              ;;
          *)
              log_err "Unknown toolchain type: $toolchain_type"
              exit 1
              ;;
      esac

    if [ ! -f "$toolchain_dir/bin/clang" ]; then
        log_err "Clang executable not found at $toolchain_dir/bin/clang! Aborting..."
        exit 1
    fi

    export PATH="${toolchain_dir}/bin:${PATH}"
    KBUILD_COMPILER_STRING=$("$toolchain_dir/bin/clang" -v 2>&1 | head -n 1 | sed 's/(https..*//' | sed 's/ version//')
    export KBUILD_COMPILER_STRING
}

## Pre-build dependencies
get_toolchain "$CLANG_TYPE"
prep_toolchain "$CLANG_TYPE"
