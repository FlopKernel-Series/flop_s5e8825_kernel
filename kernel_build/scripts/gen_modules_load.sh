#!/usr/bin/env bash
# Generate modules.load with proper dependency ordering using depmod

set -e

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <modules_dir> <output_file> [kernel_dir]"
    exit 1
fi

MODULES_DIR="$1"
OUTPUT_FILE="$2"
KERNEL_DIR="${3:-.}"
MODULES_ORDER="$MODULES_DIR/modules.order"

if [ ! -f "$MODULES_ORDER" ]; then
    echo "ERROR: modules.order not found at $MODULES_ORDER"
    exit 1
fi

KERNEL_VERSION=$(basename "$MODULES_DIR")

# Priority modules that must load early
PRIORITY_MODULES=(
    "exynos-chipid_v2.ko"
    "exynos-reboot.ko"
    "clk_exynos.ko"
    "exynos_mct.ko"
    "s3c2410_wdt.ko"
    "i2c-exynos5.ko"
)

# Modules to exclude
EXCLUDE_MODULES=(
)

# Run depmod to generate dependency information
DEPMOD_BASE=$(dirname "$(dirname "$(dirname "$MODULES_DIR")")")
depmod -b "$DEPMOD_BASE" "$KERNEL_VERSION" 2>/dev/null || {
    echo "WARNING: depmod failed, will use modules.order without dependency resolution"
}

# Extract module names from modules.order
ALL_MODULES=$(mktemp)
while IFS= read -r line; do
    basename "$line"
done < "$MODULES_ORDER" > "$ALL_MODULES"

# Remove excluded modules (match against basenames)
TEMP_MODULES=$(mktemp)
while IFS= read -r module; do
    excluded=false
    for pattern in "${EXCLUDE_MODULES[@]}"; do
        # Use bash pattern matching
        if [[ "$module" == $pattern ]]; then
            excluded=true
            break
        fi
    done
    if [ "$excluded" = false ]; then
        echo "$module" >> "$TEMP_MODULES"
    fi
done < "$ALL_MODULES"

mv "$TEMP_MODULES" "$ALL_MODULES"

# Use depmod's modules.dep for dependency-ordered list
MODULES_DEP="$MODULES_DIR/modules.dep"
if [ -f "$MODULES_DEP" ]; then
    SORTED_MODULES=$(mktemp)
    declare -A PROCESSED

    # Recursive dependency resolver
    resolve_deps() {
        local modname="$1"

        [ "${PROCESSED[$modname]}" = "1" ] && return

        # Get dependencies from modules.dep
        local deps
        deps=$(grep "/${modname}:" "$MODULES_DEP" 2>/dev/null | head -1 | cut -d: -f2 || true)

        # Process dependencies first (depth-first)
        for dep in $deps; do
            local depname
            depname=$(basename "$dep")
            if grep -q "^${depname}$" "$ALL_MODULES"; then
                resolve_deps "$depname"
            fi
        done

        PROCESSED[$modname]=1
        echo "$modname" >> "$SORTED_MODULES"
    }

    # Process all modules
    while IFS= read -r module; do
        resolve_deps "$module"
    done < "$ALL_MODULES"

    # Build final modules.load with priority modules first
    : > "$OUTPUT_FILE"

    for module in "${PRIORITY_MODULES[@]}"; do
        if grep -q "^$module$" "$SORTED_MODULES"; then
            echo "$module" >> "$OUTPUT_FILE"
            sed -i "/^$module$/d" "$SORTED_MODULES"
        fi
    done

    cat "$SORTED_MODULES" >> "$OUTPUT_FILE"
    rm -f "$SORTED_MODULES"
else
    # Fallback: use modules.order with priority modules first
    : > "$OUTPUT_FILE"

    for module in "${PRIORITY_MODULES[@]}"; do
        if grep -q "^$module$" "$ALL_MODULES"; then
            echo "$module" >> "$OUTPUT_FILE"
            sed -i "/^$module$/d" "$ALL_MODULES"
        fi
    done

    cat "$ALL_MODULES" >> "$OUTPUT_FILE"
fi

rm -f "$ALL_MODULES"

echo "Generated modules.load with $(wc -l < "$OUTPUT_FILE") modules"
cp -f "$OUTPUT_FILE" "$KERNEL_DIR/modules.load.gen"
