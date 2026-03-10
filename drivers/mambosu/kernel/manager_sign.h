#ifndef __KSU_H_MANAGER_SIGN
#define __KSU_H_MANAGER_SIGN

#include <linux/types.h>

// rsuntk/KernelSU
#define EXPECTED_SIZE_RSUNTK 0x396
#define EXPECTED_HASH_RSUNTK \
    "f415f4ed9435427e1fdf7f1fccd4dbc07b3d6b8751e4dbcec6f19671f427870b"

// 5ec1cff/KernelSU
#define EXPECTED_SIZE_5EC1CFF 384
#define EXPECTED_HASH_5EC1CFF \
    "7e0c6d7278a3bb8e364e0fcba95afaf3666cf5ff3c245a3b63c8833bd0445cc4"

// tiann/KernelSU
#define EXPECTED_SIZE_OFFICIAL 0x033b
#define EXPECTED_HASH_OFFICIAL \
    "c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6"

// KOWX712/KernelSU
#define EXPECTED_SIZE_KOWX712 0x375
#define EXPECTED_HASH_KOWX712 \
    "484fcba6e6c43b1fb09700633bf2fb4758f13cb0b2f4457b80d075084b26c588"

// KernelSU-Next/KernelSU-Next
#define EXPECTED_SIZE_NEXT 0x3e6
#define EXPECTED_HASH_NEXT \
    "79e590113c4c4c0c222978e413a5faa801666957b1212a328e46c00c69821bf7"

// ShirkNeko/SukiSU
#define EXPECTED_SIZE_SHIRKNEKO 0x35c
#define EXPECTED_HASH_SHIRKNEKO                                                \
    "947ae944f3de4ed4c21a7e4f7953ecf351bfa2b36239da37a34111ad29993eef"

// Neko/KernelSU
#define EXPECTED_SIZE_NEKO 0x29c
#define EXPECTED_HASH_NEKO                                                     \
    "946b0557e450a6430a0ba6b6bccee5bc12953ec8735d55e26139b0ec12303b21"

// ReSukiSU/ReSukiSU
#define EXPECTED_SIZE_RESUKISU 0x377
#define EXPECTED_HASH_RESUKISU                                                 \
    "d3469712b6214462764a1d8d3e5cbe1d6819a0b629791b9f4101867821f1df64"

// KernelSU-WILD/KernelSU-WILD
#define EXPECTED_SIZE_WILD 0x381
#define EXPECTED_HASH_WILD                                                     \
    "52d52d8c8bfbe53dc2b6ff1c613184e2c03013e090fe8905d8e3d5dc2658c2e4"

// RapliVx/KernelSU (MamboSU)
#define EXPECTED_SIZE_MAMBO 0x384
#define EXPECTED_HASH_MAMBO \
    "a9462b8b98ea1ca7901b0cbdcebfaa35f0aa95e51b01d66e6b6d2c81b97746d8"

// Kaminarich/KamiSU
#define EXPECTED_SIZE_KAMISU 0x2e8
#define EXPECTED_HASH_KAMISU \
    "653fcbd25f27860a44dff957578fd081bb705a5f319ab7bd5a5d287873db65d0"
// -------------------------------

typedef struct {
    u32 size;
    const char *sha256;
} apk_sign_key_t;

#endif /* __KSU_H_MANAGER_SIGN */
