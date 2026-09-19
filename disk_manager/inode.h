#ifndef INODE_H
#define INODE_H

#include "fs_types.h"

// Allocates a free inode, zeros its content, and marks it as used. Returns its index, or -1 if there are no more free inodes.
int32_t inode_alloc(void);

// Frees all data blocks belonging to the inode and then marks the inode itself as free again.
void inode_free(uint32_t idx);

// Allocates a free data block and zeros its content. Returns the block number, or INVALID_BLOCK if there are no more free blocks.
uint32_t block_alloc(void);

// Frees a data block, marking it as free in the bitmap and incrementing the count of free blocks in the superblock.
void block_free(uint32_t block_num);

// Returns the block number that holds the `logical_index`-th block of an inode's content (starting from 0). 
// Blocks beyond DIRECT_BLOCKS are accessed by following the chain of continuation inodes pointed to by `next_inode`.
// If `allocate` is non-zero, missing blocks/continuation inodes are created as needed; otherwise, missing blocks return INVALID_BLOCK.
uint32_t inode_get_block(inode_t *inode, uint32_t logical_index, int allocate);

// Frees all data blocks (and any continuation inodes) of `inode`, leaving its content empty (size becomes 0).
void inode_truncate(inode_t *inode);

// Returns a pointer to the position of the inode `idx` in the inode table on disk.
inode_t *inode_get(uint32_t idx);

#endif
