#include "dir_ops.h"
#include "../disk_manager/disk.h"
#include "../disk_manager/inode.h"
#include "directory.h"
#include "path.h"
#include "perm.h"

#include <stdio.h>
#include <string.h>

int fs_mkdir(uint32_t cwd, const char *path, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 0, &inode_idx, &parent_idx, name) != 0) {
        printf("mkdir: %s: caminho invalido\n", path);
        return -1;
    }
    if (inode_idx != INVALID_INODE) {
        printf("mkdir: %s: ja existe\n", path);
        return -1;
    }

    uint32_t new_idx;
    if (dir_create_child(parent_idx, name, INODE_DIR, user, &new_idx) != 0) {
        printf("mkdir: %s: nao foi possivel criar o diretorio\n", path);
        return -1;
    }
    return 0;
}

int fs_rmdir(uint32_t cwd, const char *path, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 0, &inode_idx, &parent_idx, name) != 0 || inode_idx == INVALID_INODE) {
        printf("rmdir: %s: diretorio nao encontrado\n", path);
        return -1;
    }

    inode_t *inode = inode_get(inode_idx);
    if (inode->type != INODE_DIR) {
        printf("rmdir: %s: nao e um diretorio\n", path);
        return -1;
    }
    if (inode_idx == disk_superblock()->root_inode) {
        printf("rmdir: nao e possivel remover o diretorio raiz\n");
        return -1;
    }
    if (!dir_is_empty(inode_idx)) {
        printf("rmdir: %s: diretorio nao esta vazio\n", path);
        return -1;
    }
    if (!has_permission(inode_get(parent_idx), user, PERM_WRITE)) {
        printf("rmdir: %s: permissao negada\n", path);
        return -1;
    }

    dir_remove_entry(parent_idx, inode_idx);
    inode_free(inode_idx);
    return 0;
}

// Callback usado por fs_ls() para imprimir uma linha por entrada de
// diretorio, no estilo do `ls -l`.
static void print_entry(uint32_t child_idx, void *ctx) {
    (void)ctx;
    inode_t *inode = inode_get(child_idx);
    char type_char = (inode->type == INODE_DIR) ? 'd' : (inode->type == INODE_LINK ? 'l' : '-');

    printf("%c%c%c%c%c%c%c %-10s %8u %s\n",
           type_char,
           (inode->perm_owner & PERM_READ) ? 'r' : '-',
           (inode->perm_owner & PERM_WRITE) ? 'w' : '-',
           (inode->perm_owner & PERM_EXEC) ? 'x' : '-',
           (inode->perm_other & PERM_READ) ? 'r' : '-',
           (inode->perm_other & PERM_WRITE) ? 'w' : '-',
           (inode->perm_other & PERM_EXEC) ? 'x' : '-',
           inode->owner, inode->size, inode->name);
}

int fs_ls(uint32_t cwd, const char *path, const char *user) {
    uint32_t target = cwd;

    if (strlen(path) > 0) {
        uint32_t inode_idx, parent_idx;
        char name[MAX_NAME_LEN];
        if (path_resolve(path, cwd, 1, &inode_idx, &parent_idx, name) != 0 || inode_idx == INVALID_INODE) {
            printf("ls: %s: nao encontrado\n", path);
            return -1;
        }
        target = inode_idx;
    }

    inode_t *dir = inode_get(target);
    if (dir->type != INODE_DIR) {
        printf("ls: nao e um diretorio\n");
        return -1;
    }
    if (!has_permission(dir, user, PERM_READ)) {
        printf("ls: permissao negada\n");
        return -1;
    }

    dir_for_each(target, print_entry, NULL);
    return 0;
}

int fs_cd(uint32_t cwd, const char *path, const char *user, uint32_t *out_inode) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 1, &inode_idx, &parent_idx, name) != 0 || inode_idx == INVALID_INODE) {
        printf("cd: %s: nao encontrado\n", path);
        return -1;
    }

    inode_t *inode = inode_get(inode_idx);
    if (inode->type != INODE_DIR) {
        printf("cd: %s: nao e um diretorio\n", path);
        return -1;
    }
    if (!has_permission(inode, user, PERM_EXEC)) {
        printf("cd: %s: permissao negada\n", path);
        return -1;
    }

    *out_inode = inode_idx;
    return 0;
}
