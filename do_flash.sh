#!/usr/bin/env bash
# ============================================================================
# FloppyKernel Interactive Flash Tool v1.0
# Professional graphical flash interface for FlopKernel
# ============================================================================

set -euo pipefail

# ── Colors ──────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
DIM='\033[2m'
BOLD='\033[1m'
RESET='\033[0m'

# ── Unicode Box Drawing ─────────────────────────────────────────────────────
TL="┌"
TR="┐"
BL="└"
BR="┘"
H="─"
V="│"
DH="═"
LJ="├"
RJ="┤"
TJ="┬"
BJ="┴"
CROSS="┼"

# ── Config ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGES_DIR="$SCRIPT_DIR/kernel_build/images"
BIN_DIR="$SCRIPT_DIR/kernel_build/bin"

# ── Helper Functions ────────────────────────────────────────────────────────
print_line() {
    local color="$1" char="$2" width="$3"
    printf "${color}"
    for ((i = 0; i < width; i++)); do printf "%s" "$char"; done
    printf "${RESET}"
}

print_border_top() {
    local width="$1" color="${2:-$CYAN}"
    printf "${color}${TL}"
    print_line "$color" "$DH" $((width - 2))
    printf "${TR}${RESET}\n"
}

print_border_bottom() {
    local width="$1" color="${2:-$CYAN}"
    printf "${color}${BL}"
    print_line "$color" "$DH" $((width - 2))
    printf "${BR}${RESET}\n"
}

print_border_mid() {
    local width="$1" color="${2:-$CYAN}"
    printf "${color}${LJ}"
    print_line "$color" "$DH" $((width - 2))
    printf "${RJ}${RESET}\n"
}

print_centered() {
    local text="$1" width="$2" color="${3:-$WHITE}"
    local stripped
    stripped=$(echo -e "$text" | sed 's/\x1b\[[0-9;]*m//g')
    local len=${#stripped}
    local padding=$(( (width - len - 2) / 2 ))
    local pad_right=$(( width - len - 2 - padding ))
    printf "${color}${V} "
    printf "${text}"
    printf "%${pad_right}s${V}${RESET}\n"
}

print_kv() {
    local key="$1" val="$2" width="$3" key_color="${4:-$CYAN}" val_color="${5:-$WHITE}"
    local key_stripped val_stripped
    key_stripped=$(echo -e "$key" | sed 's/\x1b\[[0-9;]*m//g')
    val_stripped=$(echo -e "$val" | sed 's/\x1b\[[0-9;]*m//g')
    local total=$(( ${#key_stripped} + ${#val_stripped} + 1 ))
    local pad=$(( width - total - 2 ))
    printf "${val_color}${V} ${key_color}${key}${RESET} ${val_color}${val}"
    printf "%${pad}s ${val_color}${V}${RESET}\n"
}

clear_screen() {
    clear 2>/dev/null || printf '\033[2J\033[H'
}

spinner() {
    local pid=$1 msg="$2"
    local spin='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    local i=0
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r  ${CYAN}${spin:i++%${#spin}:1}${RESET} ${msg}  "
        sleep 0.1
    done
    printf "\r"
}

# ── Detection Functions ─────────────────────────────────────────────────────
detect_adb() {
    if command -v adb &>/dev/null; then
        ADB="adb"
    elif [ -x "$BIN_DIR/adb" ]; then
        ADB="$BIN_DIR/adb"
    else
        ADB=""
    fi
}

detect_fastboot() {
    if command -v fastboot &>/dev/null; then
        FASTBOOT="fastboot"
    elif [ -x "$BIN_DIR/fastboot" ]; then
        FASTBOOT="$BIN_DIR/fastboot"
    else
        FASTBOOT=""
    fi
}

detect_device() {
    DEVICE_CONNECTED=0
    DEVICE_SERIAL=""
    if [ -n "$ADB" ]; then
        local out
        out=$($ADB devices 2>/dev/null | grep -w "device" | head -1) || true
        if [ -n "$out" ]; then
            DEVICE_CONNECTED=1
            DEVICE_SERIAL=$(echo "$out" | awk '{print $1}')
        fi
    fi
}

detect_rom_type() {
    ROM_TYPE="Unknown"
    if [ "$DEVICE_CONNECTED" -eq 1 ] && [ -n "$ADB" ]; then
        local build_id
        build_id=$($ADB -s "$DEVICE_SERIAL" shell getprop ro.build.display.id 2>/dev/null | tr -d '\r') || true
        local fingerprint
        fingerprint=$($ADB -s "$DEVICE_SERIAL" shell getprop ro.build.fingerprint 2>/dev/null | tr -d '\r') || true

        if echo "$build_id" | grep -qi "one ui\|oneui\|samsung"; then
            ROM_TYPE="${GREEN}OneUI${RESET}"
            ROM_KEY="oneui"
        elif echo "$fingerprint" | grep -qi "aosp\|lineage\|pixel\|crdroid\|evolution\|derp\|project.extended"; then
            ROM_TYPE="${MAGENTA}AOSP Custom ROM${RESET}"
            ROM_KEY="aosp"
        elif echo "$fingerprint" | grep -qi "samsung"; then
            ROM_TYPE="${YELLOW}Samsung Stock${RESET}"
            ROM_KEY="oneui"
        else
            ROM_TYPE="${YELLOW}Custom ROM (Assuming AOSP)${RESET}"
            ROM_KEY="aosp"
        fi
    fi
}

detect_root() {
    ROOT_TYPE="${DIM}None${RESET}"
    if [ "$DEVICE_CONNECTED" -eq 1 ] && [ -n "$ADB" ]; then
        local su_path
        su_path=$($ADB -s "$DEVICE_SERIAL" shell which ksud 2>/dev/null | tr -d '\r') || true
        if [ -n "$su_path" ] && [ "$su_path" != "which: not found" ]; then
            local ksud_ver
            ksud_ver=$($ADB -s "$DEVICE_SERIAL" shell ksud --version 2>/dev/null | head -1 | tr -d '\r') || true
            if echo "$ksud_ver" | grep -qi "sukisu\|resuki"; then
                ROOT_TYPE="${MAGENTA}SukiSU Ultra${RESET}"
            elif echo "$ksud_ver" | grep -qi "mambo"; then
                ROOT_TYPE="${YELLOW}MamboSU${RESET}"
            else
                ROOT_TYPE="${GREEN}KernelSU Next${RESET}"
            fi
        else
            local magisk_path
            magisk_path=$($ADB -s "$DEVICE_SERIAL" shell which magisk 2>/dev/null | tr -d '\r') || true
            if [ -n "$magisk_path" ] && [ "$magisk_path" != "which: not found" ]; then
                ROOT_TYPE="${GREEN}Magisk${RESET}"
            fi
        fi
    fi
}

detect_device_info() {
    DEVICE_MODEL="N/A"
    DEVICE_ANDROID="N/A"
    DEVICE_KERNEL="N/A"
    DEVICE_SERIAL_NUM="N/A"
    if [ "$DEVICE_CONNECTED" -eq 1 ] && [ -n "$ADB" ]; then
        DEVICE_MODEL=$($ADB -s "$DEVICE_SERIAL" shell getprop ro.product.model 2>/dev/null | tr -d '\r') || true
        DEVICE_ANDROID=$($ADB -s "$DEVICE_SERIAL" shell getprop ro.build.version.release 2>/dev/null | tr -d '\r') || true
        DEVICE_KERNEL=$($ADB -s "$DEVICE_SERIAL" shell uname -r 2>/dev/null | tr -d '\r') || true
        DEVICE_SERIAL_NUM=$($ADB -s "$DEVICE_SERIAL" shell getprop ro.serialno 2>/dev/null | tr -d '\r') || true
    fi
}

# ── Flash Functions ─────────────────────────────────────────────────────────
select_boot_image() {
    if [ "$ROM_KEY" = "aosp" ]; then
        SELECTED_BOOT="$IMAGES_DIR/boot_aosp.img"
    else
        SELECTED_BOOT="$IMAGES_DIR/boot_oneui.img"
    fi
}

check_images() {
    local missing=0
    for f in boot_oneui.img boot_aosp.img vendor_boot.img dtb.img dtbo.img; do
        if [ ! -f "$IMAGES_DIR/$f" ]; then
            missing=1
            break
        fi
    done
    return $missing
}

flash_boot() {
    local boot_img="$1"
    if [ -z "$FASTBOOT" ]; then
        print_border_top 60 "$RED"
        print_centered "${RED}${BOLD}ERROR: fastboot not found${RESET}" 60 "$RED"
        print_border_bottom 60 "$RED"
        return 1
    fi

    printf "\n"
    print_border_top 60 "$YELLOW"
    print_centered "${YELLOW}${BOLD}Flashing boot.img...${RESET}" 60 "$YELLOW"
    print_border_mid 60 "$YELLOW"

    local fname
    fname=$(basename "$boot_img")
    print_kv "  Image:" "$fname" 60
    print_kv "  Size:" "$(du -h "$boot_img" | cut -f1)" 60
    print_border_bottom 60 "$YELLOW"

    printf "\n"
    read -rp "  ${YELLOW}Proceed? [y/N]: ${RESET}" confirm
    if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
        printf "  ${DIM}Cancelled.${RESET}\n"
        return 1
    fi

    printf "\n  ${CYAN}Flashing...${RESET}\n"
    if $FASTBOOT flash boot "$boot_img" 2>&1 | while IFS= read -r line; do printf "    %s\n" "$line"; done; then
        printf "\n  ${GREEN}boot.img flashed successfully!${RESET}\n"
        return 0
    else
        printf "\n  ${RED}Failed to flash boot.img!${RESET}\n"
        return 1
    fi
}

flash_vendor_boot() {
    local vb_img="$IMAGES_DIR/vendor_boot.img"
    if [ ! -f "$vb_img" ]; then
        printf "  ${RED}vendor_boot.img not found!${RESET}\n"
        return 1
    fi

    printf "\n"
    print_border_top 60 "$YELLOW"
    print_centered "${YELLOW}${BOLD}Flashing vendor_boot.img...${RESET}" 60 "$YELLOW"
    print_border_mid 60 "$YELLOW"
    print_kv "  Size:" "$(du -h "$vb_img" | cut -f1)" 60
    print_border_bottom 60 "$YELLOW"

    printf "\n"
    read -rp "  ${YELLOW}Proceed? [y/N]: ${RESET}" confirm
    if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
        printf "  ${DIM}Cancelled.${RESET}\n"
        return 1
    fi

    printf "\n  ${CYAN}Flashing...${RESET}\n"
    if $FASTBOOT flash vendor_boot "$vb_img" 2>&1 | while IFS= read -r line; do printf "    %s\n" "$line"; done; then
        printf "\n  ${GREEN}vendor_boot.img flashed successfully!${RESET}\n"
        return 0
    else
        printf "\n  ${RED}Failed to flash vendor_boot.img!${RESET}\n"
        return 1
    fi
}

flash_dtb() {
    local dtb_img="$IMAGES_DIR/dtb.img"
    if [ ! -f "$dtb_img" ]; then
        printf "  ${RED}dtb.img not found!${RESET}\n"
        return 1
    fi

    printf "\n  ${CYAN}Flashing dtb.img...${RESET}\n"
    if $FASTBOOT flash dtb "$dtb_img" 2>&1 | while IFS= read -r line; do printf "    %s\n" "$line"; done; then
        printf "  ${GREEN}dtb.img flashed!${RESET}\n"
        return 0
    else
        printf "  ${RED}Failed to flash dtb.img!${RESET}\n"
        return 1
    fi
}

flash_dtbo() {
    local dtbo_img="$IMAGES_DIR/dtbo.img"
    if [ ! -f "$dtbo_img" ]; then
        printf "  ${RED}dtbo.img not found!${RESET}\n"
        return 1
    fi

    printf "\n  ${CYAN}Flashing dtbo.img...${RESET}\n"
    if $FASTBOOT flash dtbo "$dtbo_img" 2>&1 | while IFS= read -r line; do printf "    %s\n" "$line"; done; then
        printf "  ${GREEN}dtbo.img flashed!${RESET}\n"
        return 0
    else
        printf "  ${RED}Failed to flash dtbo.img!${RESET}\n"
        return 1
    fi
}

reboot_device() {
    printf "\n  ${CYAN}Rebooting device...${RESET}\n"
    $FASTBOOT reboot 2>/dev/null || $ADB reboot 2>/dev/null || true
    printf "  ${GREEN}Device rebooting. Check your device.${RESET}\n"
}

# ── TAR Creation ────────────────────────────────────────────────────────────
create_tar() {
    printf "\n"
    print_border_top 60 "$MAGENTA"
    print_centered "${MAGENTA}${BOLD}Create Odin Flashable TAR${RESET}" 60 "$MAGENTA"
    print_border_mid 60 "$MAGENTA"
    print_centered "${DIM}Select TAR variant:${RESET}" 60 "$MAGENTA"
    print_border_bottom 60 "$MAGENTA"

    printf "\n"
    printf "  ${CYAN}1)${RESET} OneUI\n"
    printf "  ${CYAN}2)${RESET} AOSP\n"
    printf "  ${CYAN}3)${RESET} OneUI + Permissive\n"
    printf "  ${CYAN}4)${RESET} AOSP + Permissive\n"
    printf "  ${CYAN}5)${RESET} OneUI + Unlocked\n"
    printf "  ${CYAN}6)${RESET} AOSP + Unlocked\n"
    printf "  ${DIM}0)${RESET} Back\n"
    printf "\n"
    read -rp "  ${WHITE}Select [1-6]: ${RESET}" tar_choice

    local tar_name="FloppyKernel.tar"
    local boot_img=""
    local vb_img="$IMAGES_DIR/vendor_boot.img"
    local dtbo_img="$IMAGES_DIR/dtbo.img"

    case "$tar_choice" in
        1) tar_name="FloppyKernel-OneUI.tar"; boot_img="$IMAGES_DIR/boot_oneui.img" ;;
        2) tar_name="FloppyKernel-AOSP.tar"; boot_img="$IMAGES_DIR/boot_aosp.img" ;;
        3|4|5|6)
            printf "\n  ${YELLOW}Note: Permissive/Unlocked TARs require hex-patching during build.${RESET}\n"
            printf "  ${YELLOW}Use ./do_build.sh with 'p' (permissive) or 'd' (unlocked) flags.${RESET}\n"
            printf "  ${DIM}Falling back to standard variant...${RESET}\n"
            sleep 2
            if [ "$tar_choice" -eq 3 ] || [ "$tar_choice" -eq 5 ]; then
                tar_name="FloppyKernel-OneUI.tar"; boot_img="$IMAGES_DIR/boot_oneui.img"
            else
                tar_name="FloppyKernel-AOSP.tar"; boot_img="$IMAGES_DIR/boot_aosp.img"
            fi
            ;;
        0) return 0 ;;
        *) printf "  ${RED}Invalid choice.${RESET}\n"; return 1 ;;
    esac

    if [ ! -f "$boot_img" ]; then
        printf "  ${RED}boot.img not found: $boot_img${RESET}\n"
        return 1
    fi

    local out_tar="$SCRIPT_DIR/$tar_name"
    printf "\n  ${CYAN}Creating $tar_name ...${RESET}\n"

    local tmpdir
    tmpdir=$(mktemp -d)
    cd "$tmpdir"

    lz4 -c -12 -B6 --content-size "$boot_img" > boot.img.lz4 2>/dev/null
    lz4 -c -12 -B6 --content-size "$vb_img" > vendor_boot.img.lz4 2>/dev/null
    if [ -f "$dtbo_img" ]; then
        lz4 -c -12 -B6 --content-size "$dtbo_img" > dtbo.img.lz4 2>/dev/null
        tar -cf "$out_tar" boot.img.lz4 vendor_boot.img.lz4 dtbo.img.lz4
    else
        tar -cf "$out_tar" boot.img.lz4 vendor_boot.img.lz4
    fi

    cd "$SCRIPT_DIR"
    rm -rf "$tmpdir"

    if [ -f "$out_tar" ]; then
        printf "  ${GREEN}Created: $(basename "$out_tar") ($(du -h "$out_tar" | cut -f1))${RESET}\n"
        printf "  ${DIM}Flash via Odin: AP tab -> Select file -> Start${RESET}\n"
    else
        printf "  ${RED}Failed to create TAR!${RESET}\n"
    fi
}

# ── Main Menu ───────────────────────────────────────────────────────────────
show_header() {
    local W=60
    clear_screen
    printf "\n"
    printf "${CYAN}"
    print_border_top $W "$CYAN"
    printf "${CYAN}${V}${RESET}                                                            ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}██████╗ ███████╗████████╗██████╗  ██████╗ ████████╗${RESET}  ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗╚══██╔══╝${RESET}  ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}██████╔╝█████╗     ██║   ██████╔╝██║   ██║   ██║${RESET}     ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}██╔══██╗██╔══╝     ██║   ██╔══██╗██║   ██║   ██║${RESET}     ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}██║  ██║███████╗   ██║   ██████╔╝╚██████╔╝   ██║${RESET}     ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═════╝  ╚═════╝    ╚═╝${RESET}     ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}                                                            ${CYAN}${V}${RESET}\n"
    print_centered "${DIM}Interactive Flash Tool v1.0 — FlopKernel Series${RESET}" $W "$CYAN"
    print_border_bottom $W "$CYAN"
    printf "\n"
}

show_device_info() {
    local W=60
    print_border_top $W "$GREEN"
    print_centered "${GREEN}${BOLD}Device Information${RESET}" $W "$GREEN"
    print_border_mid $W "$GREEN"

    if [ "$DEVICE_CONNECTED" -eq 1 ]; then
        print_kv "  Model:" "$DEVICE_MODEL" $W
        print_kv "  Serial:" "${DIM}$DEVICE_SERIAL_NUM${RESET}" $W
        print_kv "  Android:" "$DEVICE_ANDROID" $W
        print_kv "  Kernel:" "${DIM}${DEVICE_KERNEL}${RESET}" $W
    else
        print_centered "${RED}No device connected via ADB${RESET}" $W "$GREEN"
        print_centered "${DIM}Connect device with USB debugging enabled${RESET}" $W "$GREEN"
    fi

    print_border_mid $W "$GREEN"
    print_kv "  ROM:" "$ROM_TYPE" $W
    print_kv "  Root:" "$ROOT_TYPE" $W
    print_kv "  ADB:" "$([ -n "$ADB" ] && echo "${GREEN}Found${RESET}" || echo "${RED}Not found${RESET}")" $W
    print_kv "  fastboot:" "$([ -n "$FASTBOOT" ] && echo "${GREEN}Found${RESET}" || echo "${RED}Not found${RESET}")" $W
    print_border_bottom $W "$GREEN"
}

show_build_status() {
    local W=60
    print_border_top $W "$BLUE"
    print_centered "${BLUE}${BOLD}Build Artifacts${RESET}" $W "$BLUE"
    print_border_mid $W "$BLUE"

    local files=("boot_oneui.img" "boot_aosp.img" "vendor_boot.img" "dtb.img" "dtbo.img")
    for f in "${files[@]}"; do
        local path="$IMAGES_DIR/$f"
        local status
        if [ -f "$path" ]; then
            local size
            size=$(du -h "$path" | cut -f1)
            status="${GREEN}OK${RESET} (${size})"
        else
            status="${RED}MISSING${RESET}"
        fi
        print_kv "  $f:" "$status" $W
    done

    print_border_bottom $W "$BLUE"
}

show_menu() {
    local W=60
    local boot_label
    if [ "$ROM_KEY" = "aosp" ]; then
        boot_label="boot_aosp.img"
    else
        boot_label="boot_oneui.img"
    fi

    print_border_top $W "$CYAN"
    print_centered "${CYAN}${BOLD}Flash Options${RESET}" $W "$CYAN"
    print_border_mid $W "$CYAN"
    printf "${CYAN}${V}${RESET}                                                            ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}1)${RESET} Flash kernel only          ${DIM}($boot_label)${RESET}    ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}2)${RESET} Flash kernel + modules     ${DIM}(boot + vendor_boot)${RESET} ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}3)${RESET} Full flash                 ${DIM}(boot + vb + dtb + dtbo)${RESET} ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}4)${RESET} Create Odin TAR            ${DIM}(for Odin flashing)${RESET}   ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}5)${RESET} Reboot device                                      ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}  ${WHITE}${BOLD}0)${RESET} Exit                                                 ${CYAN}${V}${RESET}\n"
    printf "${CYAN}${V}${RESET}                                                            ${CYAN}${V}${RESET}\n"
    print_border_bottom $W "$CYAN"
}

# ── Main ────────────────────────────────────────────────────────────────────
main() {
    detect_adb
    detect_fastboot
    detect_device
    detect_rom_type
    detect_root
    detect_device_info
    select_boot_image

    while true; do
        show_header
        show_device_info
        show_build_status
        show_menu

        printf "\n"
        read -rp "  ${WHITE}Select option [0-5]: ${RESET}" choice
        printf "\n"

        case "$choice" in
            1)
                if ! check_images; then
                    printf "  ${RED}Build artifacts missing! Run ./do_build.sh first.${RESET}\n"
                    sleep 2
                    continue
                fi
                if [ "$DEVICE_CONNECTED" -ne 1 ]; then
                    printf "  ${RED}No device connected! Connect via USB with debugging enabled.${RESET}\n"
                    sleep 2
                    continue
                fi
                printf "\n  ${BOLD}Auto-selected: ${CYAN}$(basename "$SELECTED_BOOT")${RESET} (${ROM_TYPE} mode)\n"
                if flash_boot "$SELECTED_BOOT"; then
                    read -rp "  ${WHITE}Reboot now? [y/N]: ${RESET}" do_reboot
                    if [[ "$do_reboot" =~ ^[Yy]$ ]]; then
                        reboot_device
                        break
                    fi
                fi
                read -rp "  ${DIM}Press Enter to continue...${RESET}" _
                ;;
            2)
                if ! check_images; then
                    printf "  ${RED}Build artifacts missing! Run ./do_build.sh first.${RESET}\n"
                    sleep 2
                    continue
                fi
                if [ "$DEVICE_CONNECTED" -ne 1 ]; then
                    printf "  ${RED}No device connected!${RESET}\n"
                    sleep 2
                    continue
                fi
                printf "\n  ${BOLD}Auto-selected: ${CYAN}$(basename "$SELECTED_BOOT")${RESET} (${ROM_TYPE} mode)\n"
                if flash_boot "$SELECTED_BOOT"; then
                    flash_vendor_boot
                    read -rp "  ${WHITE}Reboot now? [y/N]: ${RESET}" do_reboot
                    if [[ "$do_reboot" =~ ^[Yy]$ ]]; then
                        reboot_device
                        break
                    fi
                fi
                read -rp "  ${DIM}Press Enter to continue...${RESET}" _
                ;;
            3)
                if ! check_images; then
                    printf "  ${RED}Build artifacts missing! Run ./do_build.sh first.${RESET}\n"
                    sleep 2
                    continue
                fi
                if [ "$DEVICE_CONNECTED" -ne 1 ]; then
                    printf "  ${RED}No device connected!${RESET}\n"
                    sleep 2
                    continue
                fi
                printf "\n  ${BOLD}Full flash: ${CYAN}$(basename "$SELECTED_BOOT")${RESET} + vendor_boot + dtb + dtbo\n"
                printf "  ${DIM}ROM mode: ${ROM_TYPE}${RESET}\n\n"
                read -rp "  ${YELLOW}Full flash will overwrite boot, vendor_boot, dtb, and dtbo. Continue? [y/N]: ${RESET}" confirm
                if [[ "$confirm" =~ ^[Yy]$ ]]; then
                    flash_boot "$SELECTED_BOOT"
                    flash_vendor_boot
                    flash_dtb
                    flash_dtbo
                    read -rp "  ${WHITE}Reboot now? [y/N]: ${RESET}" do_reboot
                    if [[ "$do_reboot" =~ ^[Yy]$ ]]; then
                        reboot_device
                        break
                    fi
                else
                    printf "  ${DIM}Cancelled.${RESET}\n"
                fi
                read -rp "  ${DIM}Press Enter to continue...${RESET}" _
                ;;
            4)
                create_tar
                read -rp "  ${DIM}Press Enter to continue...${RESET}" _
                ;;
            5)
                if [ "$DEVICE_CONNECTED" -eq 1 ] && [ -n "$FASTBOOT" ]; then
                    printf "  ${YELLOW}Rebooting device...${RESET}\n"
                    $FASTBOOT reboot 2>/dev/null || $ADB reboot 2>/dev/null || true
                elif [ "$DEVICE_CONNECTED" -eq 1 ] && [ -n "$ADB" ]; then
                    printf "  ${YELLOW}Rebooting via ADB...${RESET}\n"
                    $ADB reboot
                else
                    printf "  ${RED}No device connected!${RESET}\n"
                fi
                read -rp "  ${DIM}Press Enter to continue...${RESET}" _
                ;;
            0)
                printf "\n  ${GREEN}Goodbye!${RESET}\n\n"
                exit 0
                ;;
            *)
                printf "  ${RED}Invalid option.${RESET}\n"
                sleep 1
                ;;
        esac
    done
}

main "$@"
