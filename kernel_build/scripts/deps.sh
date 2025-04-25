if [ -f "/etc/doas.conf" ] && [ -f "/usr/bin/doas" ]; then
    ROOT="doas"
elif [ -f "/usr/bin/sudo" ]; then
    ROOT="sudo"
else
    echo -e "ERROR: Doas and sudo not found. Install doas or sudo!"
    exit
fi

# Define dependency lists for each distro
UBUNTU_DEPS="lz4 brotli flex bc cpio kmod ccache zip binutils-aarch64-linux-gnu ccache"
ARCH_DEPS="lz4 brotli flex bc cpio kmod ccache zip aarch64-linux-gnu-binutils ccache"
GENTOO_DEPS="app-arch/lz4 app-arch/brotli sys-devel/flex sys-devel/bc app-arch/cpio sys-apps/kmod dev-util/ccache app-arch/zip dev-util/ccache"
COMMON_DEPS=$UBUNTU_DEPS

if [ ! -f "$KDIR/kernel_build/.deps" ]; then
    if grep -q "Ubuntu" /etc/os-release; then
        MISSING_DEPS=$(dpkg-query -W -f='${Status} ${Package}\n' $UBUNTU_DEPS 2>/dev/null | grep -v "install ok installed" | awk '{print $4}')
        if [ -n "$MISSING_DEPS" ]; then
            "$ROOT" apt update -qq
            "$ROOT" apt install -y $MISSING_DEPS
        fi
    elif grep -q "arch" "/etc/os-release"; then
        MISSING_DEPS=$(pacman -T $ARCH_DEPS 2>/dev/null)
        if [ -n "$MISSING_DEPS" ]; then
            "$ROOT" pacman -Syyuu --needed --noconfirm $MISSING_DEPS
        fi
    elif grep -q "gentoo" "/etc/os-release"; then
        for dep in $GENTOO_DEPS; do
            if ! equery list "$dep" >/dev/null 2>&1; then
                "$ROOT" emerge -navq "$dep"
            fi
        done
        if ! equery list crossdev >/dev/null 2>&1; then
            "$ROOT" emerge -navq sys-devel/crossdev
            "$ROOT" crossdev --target aarch64-linux-gnu
        fi
    else
        echo -e "\nINFO: Your distro is not Supported, skipping dependencies installation..."
        echo -e "INFO: Make sure you have these dependencies installed before proceeding: $COMMON_DEPS\n"
    fi
    touch "$KDIR/kernel_build/.deps"
fi
