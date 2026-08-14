/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Rebuild an Android vendor_boot header v4 image while replacing one DLKM
 * vendor ramdisk fragment and optionally the DTB section.  This deliberately
 * preserves every other fragment (including the platform ramdisk/fstab) and
 * the bootconfig verbatim.
 *
 * Usage: vendor_boot_repack <original> <new-dlkm-fragment> [new-dtb] <output>
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define VENDOR_BOOT_MAGIC "VNDRBOOT"
#define VENDOR_BOOT_MAGIC_SIZE 8
#define VENDOR_BOOT_HEADER_VERSION_OFFSET 8
#define VENDOR_BOOT_PAGE_SIZE_OFFSET 12
#define VENDOR_BOOT_RAMDISK_SIZE_OFFSET 24
#define VENDOR_BOOT_HEADER_SIZE_OFFSET 2096
#define VENDOR_BOOT_DTB_SIZE_OFFSET 2100
#define VENDOR_BOOT_TABLE_SIZE_OFFSET 2112
#define VENDOR_BOOT_TABLE_NUM_OFFSET 2116
#define VENDOR_BOOT_TABLE_ENTRY_SIZE_OFFSET 2120
#define VENDOR_BOOT_BOOTCONFIG_SIZE_OFFSET 2124
#define VENDOR_RAMDISK_TYPE_DLKM 3
#define VENDOR_RAMDISK_NAME_OFFSET 12
#define VENDOR_RAMDISK_NAME_SIZE 32
#define COPY_BUFFER_SIZE (64 * 1024)

struct fragment {
    uint32_t index;
    uint32_t offset;
    uint32_t size;
    uint32_t new_offset;
};

static uint32_t read_u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_u32(unsigned char *p, uint32_t value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static uint64_t align_up(uint64_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

static int seek_file(FILE *file, uint64_t offset, const char *name) {
    if (offset > INT64_MAX || fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fprintf(stderr, "Unable to seek %s: %s\n", name, strerror(errno));
        return -1;
    }
    return 0;
}

static int copy_range(FILE *in, FILE *out, uint64_t offset, uint64_t length,
                      const char *input_name) {
    unsigned char buffer[COPY_BUFFER_SIZE];

    if (seek_file(in, offset, input_name) != 0)
        return -1;
    while (length != 0) {
        size_t chunk = length > sizeof(buffer) ? sizeof(buffer) : (size_t)length;
        if (fread(buffer, 1, chunk, in) != chunk) {
            fprintf(stderr, "Unable to read %s: %s\n", input_name,
                    ferror(in) ? strerror(errno) : "unexpected end of file");
            return -1;
        }
        if (fwrite(buffer, 1, chunk, out) != chunk) {
            fprintf(stderr, "Unable to write output: %s\n", strerror(errno));
            return -1;
        }
        length -= chunk;
    }
    return 0;
}

static int copy_file(FILE *in, FILE *out, uint64_t length, const char *name) {
    return copy_range(in, out, 0, length, name);
}

static int write_padding(FILE *out, uint64_t written, uint32_t alignment) {
    unsigned char zeros[4096] = {0};
    uint64_t padding = align_up(written, alignment) - written;

    while (padding != 0) {
        size_t chunk = padding > sizeof(zeros) ? sizeof(zeros) : (size_t)padding;
        if (fwrite(zeros, 1, chunk, out) != chunk) {
            fprintf(stderr, "Unable to write output padding: %s\n", strerror(errno));
            return -1;
        }
        padding -= chunk;
    }
    return 0;
}

static int compare_fragments(const void *left, const void *right) {
    const struct fragment *a = left;
    const struct fragment *b = right;
    return a->offset < b->offset ? -1 : a->offset > b->offset;
}

static int file_size(FILE *file, uint64_t *size, const char *name) {
    struct stat st;
    if (fstat(fileno(file), &st) != 0 || st.st_size < 0) {
        fprintf(stderr, "Unable to stat %s: %s\n", name, strerror(errno));
        return -1;
    }
    *size = (uint64_t)st.st_size;
    return 0;
}

int main(int argc, char *argv[]) {
    const char *input_name;
    const char *replacement_name;
    const char *dtb_name = NULL;
    const char *output_name;
    FILE *input = NULL, *replacement = NULL, *replacement_dtb = NULL;
    FILE *output = NULL;
    unsigned char *header = NULL, *table = NULL;
    struct fragment *fragments = NULL;
    uint64_t input_size, replacement_size, replacement_dtb_size = 0;
    uint64_t header_span, ramdisk_offset, dtb_offset, table_offset;
    uint64_t bootconfig_offset, old_image_end;
    uint64_t new_ramdisk_size = 0;
    uint64_t new_dtb_offset, new_table_offset, new_bootconfig_offset;
    uint32_t page_size, header_version, header_size, old_ramdisk_size;
    uint32_t dtb_size, new_dtb_size, table_size, table_entries;
    uint32_t entry_size, bootconfig_size;
    uint32_t dlkm_index = UINT32_MAX;
    int result = 1;

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: %s <original> <new-dlkm-fragment> [new-dtb] <output>\n",
                argv[0]);
        return 2;
    }
    input_name = argv[1];
    replacement_name = argv[2];
    output_name = argv[argc - 1];
    if (argc == 5)
        dtb_name = argv[3];

    input = fopen(input_name, "rb");
    replacement = fopen(replacement_name, "rb");
    output = fopen(output_name, "wb");
    if (dtb_name)
        replacement_dtb = fopen(dtb_name, "rb");
    if (!input || !replacement || !output || (dtb_name && !replacement_dtb)) {
        fprintf(stderr, "Unable to open image files: %s\n", strerror(errno));
        goto out;
    }
    if (file_size(input, &input_size, input_name) != 0 ||
        file_size(replacement, &replacement_size, replacement_name) != 0 ||
        replacement_size > UINT32_MAX ||
        (replacement_dtb &&
         (file_size(replacement_dtb, &replacement_dtb_size, dtb_name) != 0 ||
          replacement_dtb_size == 0 || replacement_dtb_size > UINT32_MAX))) {
        if (replacement_dtb && replacement_dtb_size == 0)
            fprintf(stderr, "DTB replacement is empty\n");
        goto out;
    }

    header = calloc(1, 4096);
    if (!header || fread(header, 1, 2128, input) != 2128) {
        fprintf(stderr, "Unable to read vendor_boot header\n");
        goto out;
    }
    if (memcmp(header, VENDOR_BOOT_MAGIC, VENDOR_BOOT_MAGIC_SIZE) != 0) {
        fprintf(stderr, "Not a vendor_boot image\n");
        goto out;
    }
    header_version = read_u32(header + VENDOR_BOOT_HEADER_VERSION_OFFSET);
    page_size = read_u32(header + VENDOR_BOOT_PAGE_SIZE_OFFSET);
    old_ramdisk_size = read_u32(header + VENDOR_BOOT_RAMDISK_SIZE_OFFSET);
    header_size = read_u32(header + VENDOR_BOOT_HEADER_SIZE_OFFSET);
    dtb_size = read_u32(header + VENDOR_BOOT_DTB_SIZE_OFFSET);
    table_size = read_u32(header + VENDOR_BOOT_TABLE_SIZE_OFFSET);
    table_entries = read_u32(header + VENDOR_BOOT_TABLE_NUM_OFFSET);
    entry_size = read_u32(header + VENDOR_BOOT_TABLE_ENTRY_SIZE_OFFSET);
    bootconfig_size = read_u32(header + VENDOR_BOOT_BOOTCONFIG_SIZE_OFFSET);
    if (header_version != 4 || page_size == 0 || header_size < 2128 ||
        entry_size < VENDOR_RAMDISK_NAME_OFFSET + VENDOR_RAMDISK_NAME_SIZE ||
        table_entries == 0 || table_size != (uint64_t)table_entries * entry_size) {
        fprintf(stderr, "Unsupported or malformed vendor_boot v4 header\n");
        goto out;
    }

    header_span = align_up(header_size, page_size);
    if (header_span > SIZE_MAX) {
        fprintf(stderr, "vendor_boot header is too large\n");
        goto out;
    }
    free(header);
    header = malloc((size_t)header_span);
    if (!header || seek_file(input, 0, input_name) != 0 ||
        fread(header, 1, (size_t)header_span, input) != header_span) {
        fprintf(stderr, "Unable to read complete vendor_boot header\n");
        goto out;
    }

    ramdisk_offset = header_span;
    dtb_offset = align_up(ramdisk_offset + old_ramdisk_size, page_size);
    table_offset = align_up(dtb_offset + dtb_size, page_size);
    bootconfig_offset = align_up(table_offset + table_size, page_size);
    old_image_end = align_up(bootconfig_offset + bootconfig_size, page_size);
    if (old_image_end > input_size) {
        fprintf(stderr, "vendor_boot sections exceed the input image\n");
        goto out;
    }
    new_dtb_size = replacement_dtb ? (uint32_t)replacement_dtb_size : dtb_size;

    table = malloc(table_size);
    fragments = calloc(table_entries, sizeof(*fragments));
    if (!table || !fragments || seek_file(input, table_offset, input_name) != 0 ||
        fread(table, 1, table_size, input) != table_size) {
        fprintf(stderr, "Unable to read vendor ramdisk table\n");
        goto out;
    }
    for (uint32_t i = 0; i < table_entries; ++i) {
        unsigned char *entry = table + (uint64_t)i * entry_size;
        uint32_t type = read_u32(entry + 8);
        fragments[i].index = i;
        fragments[i].size = read_u32(entry);
        fragments[i].offset = read_u32(entry + 4);
        if ((uint64_t)fragments[i].offset + fragments[i].size > old_ramdisk_size) {
            fprintf(stderr, "vendor ramdisk table entry %u is out of bounds\n", i);
            goto out;
        }
        if (type == VENDOR_RAMDISK_TYPE_DLKM) {
            if (dlkm_index != UINT32_MAX) {
                fprintf(stderr, "More than one DLKM vendor ramdisk fragment found\n");
                goto out;
            }
            dlkm_index = i;
        }
    }
    if (dlkm_index == UINT32_MAX) {
        fprintf(stderr, "No DLKM vendor ramdisk fragment found\n");
        goto out;
    }

    qsort(fragments, table_entries, sizeof(*fragments), compare_fragments);
    for (uint32_t i = 0; i < table_entries; ++i) {
        if (i != 0 && (uint64_t)fragments[i - 1].offset + fragments[i - 1].size >
                          fragments[i].offset) {
            fprintf(stderr, "vendor ramdisk fragments overlap\n");
            goto out;
        }
        fragments[i].new_offset = (uint32_t)new_ramdisk_size;
        if (fragments[i].index == dlkm_index)
            fragments[i].size = (uint32_t)replacement_size;
        new_ramdisk_size += fragments[i].size;
        if (new_ramdisk_size > UINT32_MAX) {
            fprintf(stderr, "Replacement vendor ramdisk is too large\n");
            goto out;
        }
    }
    for (uint32_t i = 0; i < table_entries; ++i) {
        unsigned char *entry = table + (uint64_t)fragments[i].index * entry_size;
        write_u32(entry, fragments[i].size);
        write_u32(entry + 4, fragments[i].new_offset);
    }
    write_u32(header + VENDOR_BOOT_RAMDISK_SIZE_OFFSET, (uint32_t)new_ramdisk_size);
    write_u32(header + VENDOR_BOOT_DTB_SIZE_OFFSET, new_dtb_size);

    new_dtb_offset = align_up(ramdisk_offset + new_ramdisk_size, page_size);
    new_table_offset = align_up(new_dtb_offset + new_dtb_size, page_size);
    new_bootconfig_offset = align_up(new_table_offset + table_size, page_size);

    if (fwrite(header, 1, (size_t)header_span, output) != header_span) {
        fprintf(stderr, "Unable to write vendor_boot header\n");
        goto out;
    }
    for (uint32_t i = 0; i < table_entries; ++i) {
        if (fragments[i].index == dlkm_index) {
            if (seek_file(replacement, 0, replacement_name) != 0 ||
                copy_file(replacement, output, replacement_size, replacement_name) != 0)
                goto out;
        } else if (copy_range(input, output,
                              ramdisk_offset + fragments[i].offset,
                              fragments[i].size, input_name) != 0) {
            goto out;
        }
    }
    if (write_padding(output, ramdisk_offset + new_ramdisk_size, page_size) != 0)
        goto out;
    if (replacement_dtb) {
        if (seek_file(replacement_dtb, 0, dtb_name) != 0 ||
            copy_file(replacement_dtb, output, new_dtb_size, dtb_name) != 0)
            goto out;
    } else if (copy_range(input, output, dtb_offset, dtb_size, input_name) != 0) {
        goto out;
    }
    if (write_padding(output, new_dtb_offset + new_dtb_size, page_size) != 0 ||
        fwrite(table, 1, table_size, output) != table_size ||
        write_padding(output, new_table_offset + table_size, page_size) != 0 ||
        copy_range(input, output, bootconfig_offset, bootconfig_size, input_name) != 0 ||
        write_padding(output, new_bootconfig_offset + bootconfig_size, page_size) != 0) {
        fprintf(stderr, "Unable to rebuild vendor_boot image\n");
        goto out;
    }
    result = 0;

out:
    if (input)
        fclose(input);
    if (replacement)
        fclose(replacement);
    if (replacement_dtb)
        fclose(replacement_dtb);
    if (output && fclose(output) != 0 && result == 0) {
        fprintf(stderr, "Unable to finalize output: %s\n", strerror(errno));
        result = 1;
    }
    free(header);
    free(table);
    free(fragments);
    return result;
}
