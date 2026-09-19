#ifndef FS_TYPES_H
#define FS_TYPES_H

#include <stdint.h>
#include <time.h>

// Disk layout constants and structures for the filesystem.
#define BLOCK_SIZE   2048u                      // bytes per block
#define DISK_SIZE    (128u * 1024u * 1024u)     // 128 MB total disk size
#define TOTAL_BLOCKS (DISK_SIZE / BLOCK_SIZE)   // total number of blocks in the disk
#define NUM_INODES   4096u                      // total number of inodes in the filesystem

// The number of direct block pointers an inode can address directly.
#define DIRECT_BLOCKS 10    // number of direct block pointers in an inode

// Maximum lengths for names and paths in the filesystem.
#define MAX_NAME_LEN  32    // maximum length of a file or directory name
#define MAX_USER_LEN  16    // maximum length of a username

// The number that identifies a formatted disk.
#define SUPERBLOCK_MAGIC 0x46534653u    // "FSFS" in ASCII, used to identify a valid superblock

// Definition of invalid inode and block numbers for error handling and uninitialized states.
#define INVALID_INODE ((uint32_t)-1)
#define INVALID_BLOCK 0u

// Number of directory entries that can fit in a single block.
#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(dirent_t))

// Permission bits for owner and other users, allowing read, write, and execute permissions to be represented as bit flags.
#define PERM_READ  4
#define PERM_WRITE 2
#define PERM_EXEC  1

// Inode types used to differentiate between free, file, directory, symbolic link, and continuation inodes.
typedef enum {
    INODE_FREE = 0,     // unused inode
    INODE_FILE,         // regular file: content = data blocks
    INODE_DIR,          // directory: content = array of dirent_t entries
    INODE_LINK,         // symbolic link: content = target path
    INODE_CONTINUATION  // continuation inode: only stores extra block pointers
} inode_type_t;

// Superblock structure.
typedef struct {
    // Basic filesystem metadata
    uint32_t magic;             // magic number to identify a valid superblock
    uint32_t block_size;        // size of each block in bytes
    uint32_t total_blocks;      // total number of blocks in the filesystem
    uint32_t num_inodes;        // total number of inodes in the filesystem

    // Block layout information
    uint32_t block_bitmap_start;    // starting block number of the block bitmap
    uint32_t block_bitmap_blocks;   // number of blocks used by the block bitmap
    uint32_t inode_bitmap_start;    // starting block number of the inode bitmap
    uint32_t inode_bitmap_blocks;   // number of blocks used by the inode bitmap
    uint32_t inode_table_start;     // starting block number of the inode table
    uint32_t inode_table_blocks;    // number of blocks used by the inode table
    uint32_t data_start;            // starting block number of the data area

    // Additional fields for tracking free space and root inode
    uint32_t free_blocks;       // number of free blocks in the filesystem
    uint32_t free_inodes;       // number of free inodes in the filesystem
    uint32_t root_inode;        // inode number of the root directory
} superblock_t;

// Inode structure.
typedef struct {
    // Inode metadata
    uint8_t used;       // 1 if the inode is in use, 0 if free
    uint8_t type;       // inode type (INODE_FILE, INODE_DIR, etc.)

    // Inode identification and ownership  
    char name[MAX_NAME_LEN];        // name of the file or directory
    char creator[MAX_USER_LEN];     // username of the creator of the inode
    char owner[MAX_USER_LEN];       // username of the current owner of the inode

    // Inode size and timestamps
    uint32_t size;          // size of the file or directory in bytes
    time_t created_at;      // timestamp of when the inode was created
    time_t modified_at;     // timestamp of when the inode was last modified

    // Permission bit flags
    uint8_t perm_owner;     // permission bits for the owner (read, write, execute)
    uint8_t perm_other;     // permission bits for other users (read, write, execute)

    // Block pointers and continuation  
    uint32_t direct_blocks[DIRECT_BLOCKS];      // direct block pointers for the inode's data
    int32_t next_inode;                         // pointer to the next inode for continuation (if needed), -1 if none
    int32_t parent_inode;                       // pointer to the parent inode (used for ".." in directories), -1 if none
} inode_t;

// Directory entry structure.
typedef struct {
    uint8_t used;           // 1 if the entry is in use, 0 if free
    int32_t inode_num;      // inode number that this directory entry points to
} dirent_t;                 // Each directory entry corresponds to a file or subdirectory within a directory.

#endif
