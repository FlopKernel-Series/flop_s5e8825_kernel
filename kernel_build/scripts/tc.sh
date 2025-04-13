# Toolchains
AOSP_REPO="https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/refs/heads/master"
AOSP_ARCHIVE="https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/heads/master"
PC_REPO="https://github.com/kdrag0n/proton-clang"
LZ_REPO="https://gitlab.com/Jprimero15/lolz_clang.git"
SL_REPO="http://ftp.twaren.net/Unix/Kernel/tools/llvm/files/"
GC_REPO="https://api.github.com/repos/greenforce-project/greenforce_clang/releases/latest"
ZC_REPO="https://raw.githubusercontent.com/ZyCromerZ/Clang/refs/heads/main/Clang-main-link.txt"
RV_REPO="https://api.github.com/repos/Rv-Project/RvClang/releases/latest"
GCC64_REPO="https://github.com/LineageOS/android_prebuilts_gcc_linux-x86_aarch64_aarch64-linux-gnu-9.3"

# Toolchain Dirs
export GCC64_DIR="$WP/gcc64"
export AC_DIR="$WP/aospclang"
export PC_DIR="$WP/protonclang"
export LZ_DIR="$WP/lolzclang"
export SL_DIR="$WP/slimllvm"
export GC_DIR="$WP/greenforceclang"
export ZC_DIR="$WP/zycclang"
export RV_DIR="$WP/rvclang"

# Custom toolchain directory
if [[ -z "$CUST_DIR" ]]; then
    export CUST_DIR="$WP/custom-toolchain"
else
    echo -e "\nINFO: Overriding custom toolchain path..."
fi

# aosp, proton, lolz, slim, greenforce, zyc, rv, custom
if [[ -z "$CLANG_TYPE" ]]; then
    export CLANG_TYPE="rv"
else
    echo -e "\nINFO: Overriding default toolchain"
fi

get_toolchain() {
    local toolchain_type="$1"
    local toolchain_dir=""

    case "$toolchain_type" in
        aosp)
            toolchain_dir="$AC_DIR"
            USE_GCC_BINUTILS=1
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nINFO: AOSP Clang not found! Cloning to $toolchain_dir..."
                CURRENT_CLANG=$(curl -s "$AOSP_REPO" | grep -oE "clang-r[0-9a-f]+" | sort -u | tail -n1)
                if ! curl -LSsO "$AOSP_ARCHIVE/$CURRENT_CLANG.tar.gz"; then
                    echo "ERROR: Cloning failed! Aborting..."
                    exit 1
                fi
                mkdir -p "$toolchain_dir" && tar -xf ./*.tar.gz -C "$toolchain_dir" && rm ./*.tar.gz
                touch "$toolchain_dir/bin/aarch64-linux-gnu-elfedit" && chmod +x "$toolchain_dir/bin/aarch64-linux-gnu-elfedit"
                touch "$toolchain_dir/bin/arm-linux-gnueabi-elfedit" && chmod +x "$toolchain_dir/bin/arm-linux-gnueabi-elfedit"
            fi
            ;;
        proton)
            toolchain_dir="$PC_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nINFO: Proton Clang not found! Cloning to $toolchain_dir..."
                if ! git clone -q --depth=1 "$PC_REPO" "$toolchain_dir"; then
                    echo "ERROR: Cloning failed! Aborting..."
                    exit 1
                fi
            fi
            ;;
        lolz)
            toolchain_dir="$LZ_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nINFO: Lolz Clang not found! Cloning to $toolchain_dir..."
                if ! git clone -q --depth=1 "$LZ_REPO" "$toolchain_dir"; then
                    echo "ERROR: Cloning failed! Aborting..."
                    exit 1
                fi
            fi
            ;;
        slim)
            toolchain_dir="$SL_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nINFO: Slim LLVM not found! Cloning to $toolchain_dir..."
                FILENAMES=$(curl -s "$SL_REPO" | grep -oP 'llvm-[\d.]+-x86_64\.tar\.xz')
                LATEST_FILE=$(echo "$FILENAMES" | sort -V | tail -n 1)
                if ! wget -q --show-progress -O "$WP/${LATEST_FILE}" "${SL_REPO}${LATEST_FILE}"; then
                    echo "ERROR: Cloning failed! Aborting..."
                    exit 1
                fi
                mkdir -p "$toolchain_dir"
                EXTRACTED_FOLDER=$(basename "$LATEST_FILE" .tar.xz)
                tar -xf "$WP/${LATEST_FILE}" -C "$toolchain_dir"
                mv "$toolchain_dir/$EXTRACTED_FOLDER"/* "$toolchain_dir"
                rmdir "$toolchain_dir/$EXTRACTED_FOLDER"
                rm "$WP/${LATEST_FILE}"
            fi
            ;;
        greenforce)
            USE_GCC_BINUTILS=1
            toolchain_dir="$GC_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nINFO: Greenforce Clang not found! Cloning to $toolchain_dir..."
                LATEST_RELEASE=$(curl -s $GC_REPO | grep "browser_download_url" | grep ".tar.gz" | cut -d '"' -f 4)
                if [[ -z "$LATEST_RELEASE" ]]; then
                    echo "ERROR: Failed to fetch the latest Greenforce Clang release! Aborting..."
                    exit 1
                fi
                if ! wget -q --show-progress -O "$WP/greenforce-clang.tar.gz" "$LATEST_RELEASE"; then
                    echo "ERROR: Download failed! Aborting..."
                    exit 1
                fi
                mkdir -p "$toolchain_dir"
                tar -xf "$WP/greenforce-clang.tar.gz" -C "$toolchain_dir"
                rm "$WP/greenforce-clang.tar.gz"
            fi
            ;;
        custom)
            toolchain_dir="$CUST_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
                echo -e "\nERROR: Custom toolchain not found! Aborting..."
                echo "INFO: Please provide a toolchain at $CUST_DIR or select a different toolchain"
                exit 1
            fi
            ;;
        zyc)
            toolchain_dir="$ZC_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
            echo -e "\nINFO: ZyC Clang not found! Cloning to $toolchain_dir..."
            fi

            # Check and cache the latest version
            ZYC_VERSION_FILE="$WP/zyc-clang-version.txt"
            LATEST_VERSION=$(curl -s "$ZC_REPO" | head -n 1)
            if [[ -z "$LATEST_VERSION" ]]; then
                echo "INFO: Failed to check ZyC Clang version"
            else
                if [[ -f "$ZYC_VERSION_FILE" ]]; then
                    CURRENT_VERSION=$(cat "$ZYC_VERSION_FILE")
                    if [[ "$CURRENT_VERSION" != "$LATEST_VERSION" ]]; then
                        echo "INFO: A new version of ZyC Clang is available: $LATEST_VERSION"
                        echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                    fi
                else
                    echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                fi
            fi

            if [[ ! -d "$toolchain_dir" ]]; then
                if [[ -f "$ZYC_VERSION_FILE" ]]; then
                    echo "$LATEST_VERSION" > "$ZYC_VERSION_FILE"
                fi
                if [[ -z "$LATEST_VERSION" ]]; then
                    echo "ERROR: Failed to fetch the latest ZyC Clang release! Aborting..."
                    exit 1
                fi
                if ! wget -q --show-progress -O "$WP/zyc-clang.tar.gz" "$LATEST_VERSION"; then
                    echo "ERROR: Download failed! Aborting..."
                    rm -f "$ZYC_VERSION_FILE"
                    exit 1
                fi
                mkdir -p "$toolchain_dir"
                if ! tar -xf "$WP/zyc-clang.tar.gz" -C "$toolchain_dir"; then
                    echo "ERROR: Extraction failed! Aborting..."
                    rm -f "$WP/zyc-clang.tar.gz" "$ZYC_VERSION_FILE"
                    exit 1
                fi
                rm "$WP/zyc-clang.tar.gz"
            fi
            ;;
        rv)
            toolchain_dir="$RV_DIR"
            if [[ ! -d "$toolchain_dir" ]]; then
            echo -e "\nINFO: RvClang not found! Fetching the latest version..."
            LATEST_RELEASE=$(curl -s "$RV_REPO" | grep "browser_download_url" | grep ".tar.gz" | cut -d '"' -f 4)
            if [[ -z "$LATEST_RELEASE" ]]; then
                echo "ERROR: Failed to fetch the latest RvClang release! Aborting..."
                exit 1
            fi
            if ! wget -q --show-progress -O "$WP/rvclang.tar.gz" "$LATEST_RELEASE"; then
                echo "ERROR: Download failed! Aborting..."
                exit 1
            fi
            mkdir -p "$toolchain_dir"
            if ! tar -xf "$WP/rvclang.tar.gz" -C "$toolchain_dir"; then
                echo "ERROR: Extraction failed! Aborting..."
                rm -f "$WP/rvclang.tar.gz"
                exit 1
            fi
            rm "$WP/rvclang.tar.gz"
            if [[ -d "$toolchain_dir/RvClang" ]]; then
                mv "$toolchain_dir/RvClang"/* "$toolchain_dir/"
                rmdir "$toolchain_dir/RvClang"
            fi
            fi
            ;;
        *)
            echo -e "\nERROR: Unknown toolchain type: $toolchain_type"
            exit 1
            ;;
    esac

    if [[ "$USE_GCC_BINUTILS" == "1" ]]; then
        if [[ ! -d "$GCC64_DIR" ]]; then
            echo "INFO: GCC64 not found! Cloning to $GCC64_DIR..."
            if ! git clone -q --depth=1 "$GCC64_REPO" "$GCC64_DIR"; then
                echo "ERROR: Cloning failed! Aborting..."
                exit 1
            fi
        fi
    fi

}

prep_toolchain() {
    local toolchain_type="$1"
    local toolchain_dir=""

    case "$toolchain_type" in
        aosp)
            toolchain_dir="$AC_DIR"
            echo "INFO: Toolchain: AOSP Clang"
            ;;
        proton)
            toolchain_dir="$PC_DIR"
            echo "INFO: Toolchain: Proton Clang"
            ;;
        lolz)
            toolchain_dir="$LZ_DIR"
            echo "INFO: Toolchain: Lolz Clang"
            ;;
        slim)
            toolchain_dir="$SL_DIR"
            echo "INFO: Toolchain: Slim LLVM Clang"
            ;;
        greenforce)
            toolchain_dir="$GC_DIR"
            echo "INFO: Toolchain: Greenforce Clang"
            ;;
        custom)
            toolchain_dir="$CUST_DIR"
            echo "INFO: Toolchain: Custom"
            ;;
        zyc)
            toolchain_dir="$ZC_DIR"
            echo "INFO: Toolchain: ZyC Clang"
            ;;
        rv)
            toolchain_dir="$RV_DIR"
            echo "INFO: Toolchain: RvClang"
            ;;
        *)
            echo "ERROR: Unknown toolchain type: $toolchain_type"
            exit 1
            ;;
    esac

    export PATH="${toolchain_dir}/bin:${PATH}"
    if [[ "$USE_GCC_BINUTILS" == "1" ]]; then
        export PATH="${GCC64_DIR}/bin:${PATH}"
    fi
    KBUILD_COMPILER_STRING=$("$toolchain_dir/bin/clang" -v 2>&1 | head -n 1 | sed 's/(https..*//' | sed 's/ version//')
    export KBUILD_COMPILER_STRING

    if [[ "$USE_GCC_BINUTILS" == "1" ]]; then
        export CCARM64_PREFIX="aarch64-linux-"
    else
        export CCARM64_PREFIX="aarch64-linux-gnu-"
    fi
}

## Pre-build dependencies
get_toolchain "$CLANG_TYPE"
prep_toolchain "$CLANG_TYPE"
