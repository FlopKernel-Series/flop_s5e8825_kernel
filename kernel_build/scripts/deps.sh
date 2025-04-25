#!/usr/bin/env bash

if [ -f /etc/doas.conf ] && [ -x /usr/bin/doas ]; then
	ROOT="doas"
elif [ -x /usr/bin/sudo ]; then
	ROOT="sudo"
else
	echo "ERROR: neither doas nor sudo found." >&2
	return 1
fi

UBUNTU_DEPS=( lz4 brotli flex bc cpio kmod zip binutils-aarch64-linux-gnu ccache )
ARCH_DEPS=( lz4 brotli flex bc cpio kmod zip aarch64-linux-gnu-binutils ccache )
GENTOO_DEPS=( app-arch/lz4 app-arch/brotli sys-devel/flex sys-devel/bc app-arch/cpio sys-apps/kmod dev-util/ccache app-arch/zip )

. /etc/os-release

case "$ID" in
	ubuntu|debian)
		MISSING=()
		for pkg in "${UBUNTU_DEPS[@]}"; do
			if ! dpkg -s "$pkg" >/dev/null 2>&1; then
				MISSING+=("$pkg")
			fi
		done
		if [ ${#MISSING[@]} -gt 0 ]; then
			$ROOT apt-get update -qq
			$ROOT apt-get install -y "${MISSING[@]}"
		fi
		;;
	arch)
		MISSING=$(pacman -T "${ARCH_DEPS[@]}" 2>/dev/null)
		if [ -n "$MISSING" ]; then
			$ROOT pacman -Syyuu --needed --noconfirm $MISSING
		fi
		;;
	gentoo)
		for dep in "${GENTOO_DEPS[@]}"; do
			if ! equery list "$dep" >/dev/null 2>&1; then
				$ROOT emerge -av "$dep"
			fi
		done
		if ! equery list crossdev >/dev/null 2>&1; then
			$ROOT emerge -av sys-devel/crossdev
			$ROOT crossdev --target aarch64-linux-gnu
		fi
		;;
	*)
		echo
		echo "INFO: distro not supported, install manually: ${UBUNTU_DEPS[*]}"
		echo
		;;
esac
