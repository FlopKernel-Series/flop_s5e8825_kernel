if [ -f "/etc/doas.conf" ] && [ -f "/usr/bin/doas" ]; then
    ROOT="doas"
elif [ -f "/usr/bin/sudo" ]; then
    ROOT="sudo"
else
    echo -e "ERROR: Doas and sudo not found. Install doas or sudo!"
    exit
fi

DEP_LIST="lz4 brotli flex bc cpio kmod ccache zip binutils-aarch64-linux-gnu"

if [ ! -f "$KDIR/kernel_build/.deps" ]; then
    if grep -q "Ubuntu" /etc/os-release; then
        "$ROOT" apt update -qq
        "$ROOT" apt install lz4 brotli flex bc cpio kmod ccache zip binutils-aarch64-linux-gnu -y
    elif grep -q "arch" "/etc/os-release"; then
        "$ROOT" pacman -Syyuu --needed  --noconfirm lz4 brotli flex bc cpio kmod ccache zip aarch64-linux-gnu-binutils
    elif grep -q "gentoo" "/etc/os-release"; then
        "$ROOT" emerge -navq app-arch/lz4 app-arch/brotli sys-devel/flex sys-devel/bc app-arch/cpio sys-apps/kmod dev-util/ccache app-arch/zip sys-devel/crossdev
        "$ROOT" crossdev --target aarch64-linux-gnu
    else
        echo -e "\nINFO: Your distro is not Supported, skipping dependencies installation..."
        echo -e "INFO: Make sure you have these dependencies installed before proceeding: $UB_DEPLIST\n"
    fi
    touch "$KDIR/kernel_build/.deps"
fi
