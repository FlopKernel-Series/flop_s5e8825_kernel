#!/usr/bin/env bash
#
# Shared logging helpers for ckbuild scripts (optional color output)
#
# Usage:
#   source "$KDIR/kernel_build/lib/log.sh"
#   log_info "Hello"
#   log_warn "Something"
#   log_err  "Bad"
#
# Color control:
#   - Colors are enabled only if stdout is a TTY
#   - Set NO_COLOR=1 or CKBUILD_NO_COLOR=1 to disable colors
#

# Decide whether to emit color codes.
# shellcheck disable=SC2154
if [ -n "${NO_COLOR:-}" ] || [ -n "${CKBUILD_NO_COLOR:-}" ] || [ ! -t 1 ]; then
    _CKBUILD_COLOR=0
else
    _CKBUILD_COLOR=1
fi

_ck_c_reset=$'\033[0m'
_ck_c_bold=$'\033[1m'
_ck_c_red=$'\033[31m'
_ck_c_yellow=$'\033[33m'
_ck_c_cyan=$'\033[36m'

_ck_tag() {
    # $1=TAG (e.g. INFO:), $2=COLOR (may be empty)
    if [ "${_CKBUILD_COLOR}" = "1" ]; then
        printf "%b%s%b" "${_ck_c_bold}${2}" "$1" "${_ck_c_reset}"
    else
        printf "%s" "$1"
    fi
}

# INFO -> stdout, WARNING/ERROR -> stderr
log_info() { echo "$(_ck_tag "INFO:" "${_ck_c_cyan}") $*"; }
log_warn() { echo "$(_ck_tag "WARNING:" "${_ck_c_yellow}") $*" >&2; }
log_err()  { echo "$(_ck_tag "ERROR:" "${_ck_c_red}") $*" >&2; }
