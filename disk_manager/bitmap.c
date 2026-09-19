#include "bitmap.h"

int bitmap_test(const uint8_t *bitmap, uint32_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

void bitmap_set(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] |= (uint8_t)(1u << (bit % 8));
}

void bitmap_clear(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] &= (uint8_t)~(1u << (bit % 8));
}

int32_t bitmap_find_first_free(const uint8_t *bitmap, uint32_t total_bits) {
    for (uint32_t i = 0; i < total_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            return (int32_t)i;
        }
    }
    return -1; // nao ha nada livre
}
