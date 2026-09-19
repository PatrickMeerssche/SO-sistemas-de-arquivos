#define _POSIX_C_SOURCE 200809L // necessario para ftruncate() com -std=c11

#include "disk.h"
#include "superblock.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Descritor de arquivo e memoria mapeada da imagem de disco atualmente montada.
static int g_fd = -1;
static uint8_t *g_image = NULL;

int disk_mount(const char *path) {
    g_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (g_fd < 0) {
        perror("disk_mount: open");
        return -1;
    }

    struct stat st;
    if (fstat(g_fd, &st) != 0) {
        perror("disk_mount: fstat");
        close(g_fd);
        return -1;
    }

    int need_format = 0;

    // Se a imagem nao tem exatamente o tamanho esperado, ela e recriada do
    // zero, escrevendo bytes zerados de verdade para que o arquivo realmente
    // ocupe 128 MB em disco, em vez de ser um arquivo esparso.
    if (st.st_size != (off_t)DISK_SIZE) {
        if (ftruncate(g_fd, 0) != 0) {
            perror("disk_mount: ftruncate");
            close(g_fd);
            return -1;
        }
        uint8_t zero_block[BLOCK_SIZE];
        memset(zero_block, 0, sizeof(zero_block));
        for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
            if (write(g_fd, zero_block, BLOCK_SIZE) != (ssize_t)BLOCK_SIZE) {
                perror("disk_mount: write");
                close(g_fd);
                return -1;
            }
        }
        need_format = 1;
    }

    g_image = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (g_image == MAP_FAILED) {
        perror("disk_mount: mmap");
        close(g_fd);
        g_image = NULL;
        return -1;
    }

    superblock_t *sb = disk_superblock();
    if (!need_format && sb->magic != SUPERBLOCK_MAGIC) {
        // O arquivo ja existia com o tamanho certo, mas nao e uma das nossas imagens.
        need_format = 1;
    }

    if (need_format) {
        superblock_format();
        printf("Disco formatado: %u blocos de %u bytes, %u i-nodes.\n",
               TOTAL_BLOCKS, BLOCK_SIZE, NUM_INODES);
    } else {
        printf("Disco existente montado (%u/%u blocos livres, %u/%u i-nodes livres).\n",
               sb->free_blocks, sb->total_blocks, sb->free_inodes, sb->num_inodes);
    }

    return 0;
}

void disk_sync(void) {
    if (g_image != NULL) {
        msync(g_image, DISK_SIZE, MS_SYNC);
    }
}

void disk_unmount(void) {
    if (g_image != NULL) {
        disk_sync();
        munmap(g_image, DISK_SIZE);
        g_image = NULL;
    }
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
}

uint8_t *disk_block_ptr(uint32_t block_num) {
    return g_image + ((size_t)block_num * BLOCK_SIZE);
}

superblock_t *disk_superblock(void) {
    return (superblock_t *)disk_block_ptr(0);
}

uint8_t *disk_block_bitmap(void) {
    return disk_block_ptr(disk_superblock()->block_bitmap_start);
}

uint8_t *disk_inode_bitmap(void) {
    return disk_block_ptr(disk_superblock()->inode_bitmap_start);
}

inode_t *disk_inode_table(void) {
    return (inode_t *)disk_block_ptr(disk_superblock()->inode_table_start);
}
