#include "inode.h"
#include "bitmap.h"
#include "disk.h"
#include "fs_types.h"

#include <string.h>

// Allocates a free inode, zeros its content, and marks it as used. Returns its index, or -1 if there are no more free inodes.
int32_t inode_alloc(void) {
    // Get the superblock and inode bitmap
    superblock_t *sb = disk_superblock();
    uint8_t *bmap = disk_inode_bitmap();

    // Find the first free inode in the bitmap and mark it as used. If no free inode is found, return -1.
    int32_t idx = bitmap_find_first_free(bmap, sb->num_inodes);
    if (idx < 0) {
        return -1;
    }
    bitmap_set(bmap, (uint32_t)idx);

    // Decrement the count of free inodes in the superblock.
    sb->free_inodes--;

    // Zero the content of the allocated inode and initialize its fields.
    inode_t *inode = inode_get(idx);
    memset(inode, 0, sizeof(inode_t));
    inode->used = 1;
    inode->next_inode = -1;
    inode->parent_inode = -1;
    return idx;
}

// Frees all data blocks belonging to the inode and then marks the inode itself as free again.
void inode_free(uint32_t idx) {
    inode_t *inode = inode_get(idx);
    inode_truncate(inode);
    memset(inode, 0, sizeof(inode_t));
    bitmap_clear(disk_inode_bitmap(), idx);
    disk_superblock()->free_inodes++;
}

// Allocates a free data block and zeros its content. Returns the block number, or INVALID_BLOCK if there are no more free blocks.
uint32_t block_alloc(void) {
    // Get the superblock and block bitmap
    superblock_t *sb = disk_superblock();
    uint8_t *bmap = disk_block_bitmap();

    // Find the first free block in the bitmap and mark it as used. If no free block is found, return INVALID_BLOCK.
    int32_t idx = bitmap_find_first_free(bmap, sb->total_blocks);
    if (idx < 0) {
        return INVALID_BLOCK;
    }
    bitmap_set(bmap, (uint32_t)idx);

    // Decrement the count of free blocks in the superblock.
    sb->free_blocks--;

    // Zero the content of the allocated block.
    memset(disk_block_ptr((uint32_t)idx), 0, BLOCK_SIZE); // nunca deixa dados antigos vazarem
    return (uint32_t)idx;
}

// Frees a data block, marking it as free in the bitmap and incrementing the count of free blocks in the superblock.
void block_free(uint32_t block_num) {
    bitmap_clear(disk_block_bitmap(), block_num);
    disk_superblock()->free_blocks++;
}

// Returns the block number that holds the `logical_index`-th block of an inode's content (starting from 0). 
// Blocks beyond DIRECT_BLOCKS are accessed by following the chain of continuation inodes pointed to by `next_inode`.
// If `allocate` is non-zero, missing blocks/continuation inodes are created as needed; otherwise, missing blocks return INVALID_BLOCK.
uint32_t inode_get_block(inode_t *inode, uint32_t logical_index, int allocate) {
    if (logical_index < DIRECT_BLOCKS) {
        if (inode->direct_blocks[logical_index] == INVALID_BLOCK && allocate) {
            inode->direct_blocks[logical_index] = block_alloc();
        }
        return inode->direct_blocks[logical_index];
    }

    logical_index -= DIRECT_BLOCKS;
    
    if (inode->next_inode == -1) {
        if (!allocate) {
            return INVALID_BLOCK;
        }
        int32_t cont_idx = inode_alloc();
        if (cont_idx < 0) {
            return INVALID_BLOCK;
        }
        inode_get((uint32_t)cont_idx)->type = INODE_CONTINUATION;
        inode->next_inode = cont_idx;
    }

    return inode_get_block(inode_get((uint32_t)inode->next_inode), logical_index, allocate);
}

// Frees all data blocks (and any continuation inodes) of `inode`, leaving its content empty (size becomes 0).
void inode_truncate(inode_t *inode) {
    for (int i = 0; i < DIRECT_BLOCKS; i++) {
        if (inode->direct_blocks[i] != INVALID_BLOCK) {
            block_free(inode->direct_blocks[i]);
            inode->direct_blocks[i] = INVALID_BLOCK;
        }
    }

    if (inode->next_inode != -1) {
        uint32_t cont_idx = (uint32_t)inode->next_inode;
        inode_t *cont = inode_get(cont_idx);
        inode_truncate(cont);
        memset(cont, 0, sizeof(inode_t));
        bitmap_clear(disk_inode_bitmap(), cont_idx);
        disk_superblock()->free_inodes++;
        inode->next_inode = -1;
    }

    inode->size = 0;
}

// Returns a pointer to the position of the inode `idx` in the inode table on disk.
inode_t *inode_get(uint32_t idx) {
    return &disk_inode_table()[idx];
}
