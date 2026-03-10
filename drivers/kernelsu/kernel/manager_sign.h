#ifndef __KSU_H_MANAGER_SIGN
#define __KSU_H_MANAGER_SIGN

#include <linux/types.h>

// KOWX712/KernelSU
#define EXPECTED_SIZE_KOWX712 0x375
#define EXPECTED_HASH_KOWX712 \
    "484fcba6e6c43b1fb09700633bf2fb4758f13cb0b2f4457b80d075084b26c588"

// KernelSU-Next/KernelSU-Next
#define EXPECTED_SIZE_NEXT 0x3e6
#define EXPECTED_HASH_NEXT \
    "79e590113c4c4c0c222978e413a5faa801666957b1212a328e46c00c69821bf7"

typedef struct {
    u32 size;
    const char *sha256;
} apk_sign_key_t;

#endif /* __KSU_H_MANAGER_SIGN */
