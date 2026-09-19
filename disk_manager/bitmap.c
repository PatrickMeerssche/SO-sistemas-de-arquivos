#include "bitmap.h"

#include <stdint.h>

// Tests if the bit at the given index is set (1) or clear (0).
int bitmap_test(const uint8_t *bitmap, uint32_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

// Sets the bit at the given index to 1 (marking it as used).
void bitmap_set(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] |= (uint8_t)(1u << (bit % 8));
}

// Clears the bit at the given index to 0 (marking it as free).
void bitmap_clear(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] &= (uint8_t)~(1u << (bit % 8));
}

// Finds the index of the first free bit (0) in the bitmap within the range [0, total_bits).
// Returns the index of the first free bit, or -1 if the bitmap is completely full.
int32_t bitmap_find_first_free(const uint8_t *bitmap, uint32_t total_bits) {
    for (uint32_t i = 0; i < total_bits; i++) {
        if (!bitmap_test(bitmap, i)) {
            return (int32_t)i;
        }
    }
    return -1;
}
