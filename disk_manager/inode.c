#include "inode.h"
#include "bitmap.h"
#include "disk.h"

#include <string.h>

int32_t inode_alloc(void) {
    superblock_t *sb = disk_superblock();
    uint8_t *bmap = disk_inode_bitmap();

    int32_t idx = bitmap_find_first_free(bmap, sb->num_inodes);
    if (idx < 0) {
        return -1; // nao ha i-nodes livres
    }
    bitmap_set(bmap, (uint32_t)idx);
    sb->free_inodes--;

    inode_t *inode = &disk_inode_table()[idx];
    memset(inode, 0, sizeof(inode_t));
    inode->used = 1;
    inode->next_inode = -1;
    inode->parent_inode = -1;
    return idx;
}

void inode_free(uint32_t idx) {
    inode_t *inode = inode_get(idx);
    inode_truncate(inode); // libera todos os blocos/continuacoes primeiro
    memset(inode, 0, sizeof(inode_t));
    bitmap_clear(disk_inode_bitmap(), idx);
    disk_superblock()->free_inodes++;
}

inode_t *inode_get(uint32_t idx) {
    return &disk_inode_table()[idx];
}

uint32_t block_alloc(void) {
    superblock_t *sb = disk_superblock();
    uint8_t *bmap = disk_block_bitmap();

    int32_t idx = bitmap_find_first_free(bmap, sb->total_blocks);
    if (idx < 0) {
        return INVALID_BLOCK; // disco cheio
    }
    bitmap_set(bmap, (uint32_t)idx);
    sb->free_blocks--;

    memset(disk_block_ptr((uint32_t)idx), 0, BLOCK_SIZE); // nunca deixa dados antigos vazarem
    return (uint32_t)idx;
}

void block_free(uint32_t block_num) {
    bitmap_clear(disk_block_bitmap(), block_num);
    disk_superblock()->free_blocks++;
}

uint32_t inode_get_block(inode_t *inode, uint32_t logical_index, int allocate) {
    if (logical_index < DIRECT_BLOCKS) {
        if (inode->direct_blocks[logical_index] == INVALID_BLOCK && allocate) {
            inode->direct_blocks[logical_index] = block_alloc();
        }
        return inode->direct_blocks[logical_index];
    }

    // Alem dos ponteiros diretos: entra (ou cria) um i-node de continuacao
    // que existe apenas para guardar ponteiros extras de bloco.
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
        inode_truncate(cont); // libera o resto da cadeia primeiro
        memset(cont, 0, sizeof(inode_t));
        bitmap_clear(disk_inode_bitmap(), cont_idx);
        disk_superblock()->free_inodes++;
        inode->next_inode = -1;
    }

    inode->size = 0;
}
