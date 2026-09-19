#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

// Sets the bit at the given index to 1 (marking it as used).
void bitmap_set(uint8_t *bitmap, uint32_t bit);

// Clears the bit at the given index to 0 (marking it as free).
void bitmap_clear(uint8_t *bitmap, uint32_t bit);

// Finds the index of the first free bit (0) in the bitmap within the range [0, total_bits).
// Returns the index of the first free bit, or -1 if the bitmap is completely full.
int32_t bitmap_find_first_free(const uint8_t *bitmap, uint32_t total_bits);

#endif
