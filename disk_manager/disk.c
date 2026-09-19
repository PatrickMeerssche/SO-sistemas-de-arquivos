#include "disk.h"
#include "superblock.h"
#include "fs_types.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Define _POSIX_C_SOURCE 200809L to ensure that ftruncate() is available when compiling with -std=c11.
//  This is necessary for truncating the disk image file to the correct size.
#define _POSIX_C_SOURCE 200809L

/* Set up global variables for the disk image file descriptor and memory-mapped image.
 *      g_fd: file descriptor for the disk image file
 *      g_image: pointer to the memory-mapped disk image
 */
static int g_fd = -1;
static uint8_t *g_image = NULL;

// Mounts the disk image at the specified path. If the file does not exist or has an incorrect size, 
// it is created and formatted as a new filesystem. If it exists and is valid, it is mounted for use,
// maintaining the existing data. Returns 0 on success, -1 on failure. 
int disk_mount(const char *path) {
    // Open the disk image file.
    g_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (g_fd < 0) {
        perror("disk_mount: open");
        return -1;
    }

    // Get the file's status to check its size.
    struct stat st;
    if (fstat(g_fd, &st) != 0) {
        perror("disk_mount: fstat");
        close(g_fd);
        return -1;
    }

    int need_format = 0;

    // If the image does not have exactly the expected size, it is recreated from scratch, 
    // writing zeroed bytes to ensure that the file actually occupies 128 MB on disk, rather 
    //than being a sparse file. If the file already existed with the correct size but is not 
    // one of our images, it is also formatted from scratch.
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

    // Map the disk image into memory for direct access to its blocks.
    g_image = mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (g_image == MAP_FAILED) {
        perror("disk_mount: mmap");
        close(g_fd);
        g_image = NULL;
        return -1;
    }

    // Check the superblock to determine if the disk needs to be formatted.
    superblock_t *sb = disk_superblock();

    // If the magic number does not match, the disk is not a valid filesystem image and needs to be formatted.
    if (!need_format && sb->magic != SUPERBLOCK_MAGIC) {
        need_format = 1;
    }

    // If formatting is needed, call superblock_format() to initialize the filesystem structures and create 
    // the root directory. Otherwise, print a message indicating that an existing disk has been mounted,
    // along with the number of free blocks and inodes. 
    if (need_format) {
        superblock_format();
        printf("Disk formatted: %u blocks of %u bytes, %u inodes.\n",
               TOTAL_BLOCKS, BLOCK_SIZE, NUM_INODES);
        printf("Total disk size is %u MB.\n", DISK_SIZE / (1024 * 1024));
    } else {
        printf("Existing disk mounted (%u/%u free blocks, %u/%u free inodes).\n",
               sb->free_blocks, sb->total_blocks, sb->free_inodes, sb->num_inodes);
    }

    return 0;
}

// Synchronizes the memory-mapped disk image with the underlying file on disk, ensuring that all changes persist.
void disk_sync(void) {
    if (g_image != NULL) {
        msync(g_image, DISK_SIZE, MS_SYNC);
    }
}

// Unmounts the disk image by synchronizing any pending changes, unmapping the memory, and closing the file descriptor.
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

// Returns a pointer to the start of the specified block number within the memory-mapped disk image.
uint8_t *disk_block_ptr(uint32_t block_num) {
    return g_image + ((size_t)block_num * BLOCK_SIZE);
}

// Returns a pointer to the superblock structure within the memory-mapped disk image.
superblock_t *disk_superblock(void) {
    return (superblock_t *)disk_block_ptr(0);
}

// Returns a pointer to the block bitmap within the memory-mapped disk image.
uint8_t *disk_block_bitmap(void) {
    return disk_block_ptr(disk_superblock()->block_bitmap_start);
}

// Returns a pointer to the inode bitmap within the memory-mapped disk image.
uint8_t *disk_inode_bitmap(void) {
    return disk_block_ptr(disk_superblock()->inode_bitmap_start);
}

// Returns a pointer to the inode table within the memory-mapped disk image.
inode_t *disk_inode_table(void) {
    return (inode_t *)disk_block_ptr(disk_superblock()->inode_table_start);
}
