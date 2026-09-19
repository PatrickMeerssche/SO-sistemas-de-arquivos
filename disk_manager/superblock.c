#include "superblock.h"
#include "bitmap.h"
#include "disk.h"
#include "inode.h"

#include <string.h>
#include <time.h>

// Divisao com arredondamento para cima, para inteiros positivos, ex: ceil_div(10, 3) == 4.
static uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

void superblock_format(void) {
    superblock_t *sb = disk_superblock();
    memset(sb, 0, sizeof(superblock_t));

    sb->magic = SUPERBLOCK_MAGIC;
    sb->block_size = BLOCK_SIZE;
    sb->total_blocks = TOTAL_BLOCKS;
    sb->num_inodes = NUM_INODES;

    // Layout: [superbloco][bitmap de blocos][bitmap de i-nodes][tabela de i-nodes][dados]
    sb->block_bitmap_start = 1; // o bloco 0 e o proprio superbloco
    sb->block_bitmap_blocks = ceil_div(ceil_div(TOTAL_BLOCKS, 8), BLOCK_SIZE);

    sb->inode_bitmap_start = sb->block_bitmap_start + sb->block_bitmap_blocks;
    sb->inode_bitmap_blocks = ceil_div(ceil_div(NUM_INODES, 8), BLOCK_SIZE);

    sb->inode_table_start = sb->inode_bitmap_start + sb->inode_bitmap_blocks;
    sb->inode_table_blocks = ceil_div(NUM_INODES * (uint32_t)sizeof(inode_t), BLOCK_SIZE);

    sb->data_start = sb->inode_table_start + sb->inode_table_blocks;

    // Zera a area de gerenciamento (bitmaps + tabela de i-nodes).
    memset(disk_block_bitmap(), 0, (size_t)sb->block_bitmap_blocks * BLOCK_SIZE);
    memset(disk_inode_bitmap(), 0, (size_t)sb->inode_bitmap_blocks * BLOCK_SIZE);
    memset(disk_inode_table(), 0, (size_t)sb->inode_table_blocks * BLOCK_SIZE);

    // Todo bloco antes de data_start e reservado permanentemente para dados
    // de gerenciamento, entao ja marcamos como ocupado e nunca o entregamos
    // via block_alloc.
    uint8_t *bmap = disk_block_bitmap();
    for (uint32_t i = 0; i < sb->data_start; i++) {
        bitmap_set(bmap, i);
    }

    sb->free_blocks = sb->total_blocks - sb->data_start;
    sb->free_inodes = sb->num_inodes;
    sb->root_inode = 0;

    // Cria o diretorio raiz (deve cair no i-node 0, ja que o bitmap de
    // i-nodes esta totalmente livre neste ponto).
    int32_t root = inode_alloc();
    inode_t *root_inode = inode_get((uint32_t)root);
    root_inode->type = INODE_DIR;
    strncpy(root_inode->name, "/", MAX_NAME_LEN - 1);
    strncpy(root_inode->creator, "root", MAX_USER_LEN - 1);
    strncpy(root_inode->owner, "root", MAX_USER_LEN - 1);
    // Como nao existe um sistema de login de verdade, o diretorio raiz fica
    // com permissao de escrita para todos (senao apenas um usuario chamado
    // literalmente "root" conseguiria criar qualquer coisa).
    root_inode->perm_owner = PERM_READ | PERM_WRITE | PERM_EXEC;
    root_inode->perm_other = PERM_READ | PERM_WRITE | PERM_EXEC;
    root_inode->created_at = time(NULL);
    root_inode->modified_at = root_inode->created_at;
    root_inode->parent_inode = 0; // a raiz e seu proprio pai, usado pelo ".."

    sb->root_inode = (uint32_t)root;
}
