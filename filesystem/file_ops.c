#include "file_ops.h"
#include "../disk_manager/disk.h"
#include "../disk_manager/inode.h"
#include "directory.h"
#include "path.h"
#include "perm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---- pequenos auxiliares locais para mover bytes de/para os blocos de um i-node ----

static int file_write(uint32_t inode_idx, const char *data, size_t len, int append) {
    inode_t *inode = inode_get(inode_idx);
    uint32_t start_offset;

    if (append) {
        start_offset = inode->size;
    } else {
        inode_truncate(inode); // "echo >" substitui o conteudo anterior
        start_offset = 0;
    }

    uint32_t logical = start_offset / BLOCK_SIZE;
    uint32_t offset_in_block = start_offset % BLOCK_SIZE;
    size_t written = 0;

    while (written < len) {
        uint32_t blk = inode_get_block(inode, logical, 1);
        if (blk == INVALID_BLOCK) {
            return -1; // disco cheio
        }
        uint8_t *ptr = disk_block_ptr(blk);
        size_t space = BLOCK_SIZE - offset_in_block;
        size_t chunk = (len - written) < space ? (len - written) : space;
        memcpy(ptr + offset_in_block, data + written, chunk);
        written += chunk;
        offset_in_block = 0;
        logical++;
    }

    inode->size = start_offset + (uint32_t)len;
    inode->modified_at = time(NULL);
    return 0;
}

static void file_read(uint32_t inode_idx, char **out_buf, size_t *out_len) {
    inode_t *inode = inode_get(inode_idx);
    *out_len = inode->size;
    *out_buf = malloc(inode->size + 1);

    uint32_t remaining = inode->size;
    uint32_t logical = 0;
    size_t copied = 0;

    while (remaining > 0) {
        uint32_t blk = inode_get_block(inode, logical, 0);
        size_t chunk = (remaining < BLOCK_SIZE) ? remaining : BLOCK_SIZE;
        if (blk != INVALID_BLOCK) {
            memcpy(*out_buf + copied, disk_block_ptr(blk), chunk);
        } else {
            memset(*out_buf + copied, 0, chunk); // normalmente nao deveria acontecer
        }
        copied += chunk;
        remaining -= chunk;
        logical++;
    }
    (*out_buf)[*out_len] = '\0';
}

// ---- operacoes publicas --------------------------------------------------

int fs_touch(uint32_t cwd, const char *path, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 0, &inode_idx, &parent_idx, name) != 0) {
        printf("touch: %s: caminho invalido\n", path);
        return -1;
    }

    if (inode_idx != INVALID_INODE) {
        inode_get(inode_idx)->modified_at = time(NULL); // como o touch de verdade
        return 0;
    }

    uint32_t new_idx;
    if (dir_create_child(parent_idx, name, INODE_FILE, user, &new_idx) != 0) {
        printf("touch: %s: nao foi possivel criar o arquivo\n", path);
        return -1;
    }
    return 0;
}

int fs_rm(uint32_t cwd, const char *path, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 0, &inode_idx, &parent_idx, name) != 0 || inode_idx == INVALID_INODE) {
        printf("rm: %s: arquivo nao encontrado\n", path);
        return -1;
    }

    inode_t *inode = inode_get(inode_idx);
    if (inode->type == INODE_DIR) {
        printf("rm: %s: e um diretorio (use rmdir)\n", path);
        return -1;
    }
    if (!has_permission(inode_get(parent_idx), user, PERM_WRITE)) {
        printf("rm: %s: permissao negada\n", path);
        return -1;
    }

    dir_remove_entry(parent_idx, inode_idx);
    inode_free(inode_idx);
    return 0;
}

int fs_write_content(uint32_t cwd, const char *path, const char *content, int append, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 1, &inode_idx, &parent_idx, name) != 0) {
        printf("echo: %s: caminho invalido\n", path);
        return -1;
    }

    if (inode_idx == INVALID_INODE) {
        if (dir_create_child(parent_idx, name, INODE_FILE, user, &inode_idx) != 0) {
            printf("echo: %s: nao foi possivel criar o arquivo\n", path);
            return -1;
        }
    }

    inode_t *inode = inode_get(inode_idx);
    if (inode->type != INODE_FILE) {
        printf("echo: %s: nao e um arquivo\n", path);
        return -1;
    }
    if (!has_permission(inode, user, PERM_WRITE)) {
        printf("echo: %s: permissao negada\n", path);
        return -1;
    }

    if (file_write(inode_idx, content, strlen(content), append) != 0) {
        printf("echo: %s: disco cheio\n", path);
        return -1;
    }
    return 0;
}

int fs_cat(uint32_t cwd, const char *path, const char *user) {
    uint32_t inode_idx, parent_idx;
    char name[MAX_NAME_LEN];

    if (path_resolve(path, cwd, 1, &inode_idx, &parent_idx, name) != 0 || inode_idx == INVALID_INODE) {
        printf("cat: %s: arquivo nao encontrado\n", path);
        return -1;
    }

    inode_t *inode = inode_get(inode_idx);
    if (inode->type != INODE_FILE) {
        printf("cat: %s: nao e um arquivo\n", path);
        return -1;
    }
    if (!has_permission(inode, user, PERM_READ)) {
        printf("cat: %s: permissao negada\n", path);
        return -1;
    }

    char *buf;
    size_t len;
    file_read(inode_idx, &buf, &len);
    fwrite(buf, 1, len, stdout);
    printf("\n");
    free(buf);
    return 0;
}

int fs_cp(uint32_t cwd, const char *src, const char *dst, const char *user) {
    uint32_t src_idx, src_parent;
    char src_name[MAX_NAME_LEN];
    if (path_resolve(src, cwd, 1, &src_idx, &src_parent, src_name) != 0 || src_idx == INVALID_INODE) {
        printf("cp: %s: arquivo nao encontrado\n", src);
        return -1;
    }
    inode_t *src_inode = inode_get(src_idx);
    if (src_inode->type != INODE_FILE) {
        printf("cp: %s: nao e um arquivo\n", src);
        return -1;
    }
    if (!has_permission(src_inode, user, PERM_READ)) {
        printf("cp: %s: permissao negada\n", src);
        return -1;
    }

    uint32_t dst_idx, dst_parent;
    char dst_name[MAX_NAME_LEN];
    if (path_resolve(dst, cwd, 0, &dst_idx, &dst_parent, dst_name) != 0) {
        printf("cp: %s: caminho invalido\n", dst);
        return -1;
    }
    if (dst_idx == INVALID_INODE) {
        if (dir_create_child(dst_parent, dst_name, INODE_FILE, user, &dst_idx) != 0) {
            printf("cp: %s: nao foi possivel criar\n", dst);
            return -1;
        }
    } else if (inode_get(dst_idx)->type != INODE_FILE) {
        printf("cp: %s: nao e um arquivo\n", dst);
        return -1;
    }

    char *buf;
    size_t len;
    file_read(src_idx, &buf, &len);
    int r = file_write(dst_idx, buf, len, 0);
    free(buf);
    if (r != 0) {
        printf("cp: %s: disco cheio\n", dst);
        return -1;
    }
    return 0;
}

int fs_mv(uint32_t cwd, const char *src, const char *dst, const char *user) {
    uint32_t src_idx, src_parent;
    char src_name[MAX_NAME_LEN];
    if (path_resolve(src, cwd, 0, &src_idx, &src_parent, src_name) != 0 || src_idx == INVALID_INODE) {
        printf("mv: %s: nao encontrado\n", src);
        return -1;
    }
    if (!has_permission(inode_get(src_parent), user, PERM_WRITE)) {
        printf("mv: %s: permissao negada\n", src);
        return -1;
    }

    uint32_t dst_idx, dst_parent;
    char dst_name[MAX_NAME_LEN];
    if (path_resolve(dst, cwd, 0, &dst_idx, &dst_parent, dst_name) != 0) {
        printf("mv: %s: caminho invalido\n", dst);
        return -1;
    }

    // mv para um diretorio existente mantem o nome original.
    if (dst_idx != INVALID_INODE && inode_get(dst_idx)->type == INODE_DIR) {
        dst_parent = dst_idx;
        strncpy(dst_name, src_name, MAX_NAME_LEN - 1);
        dst_name[MAX_NAME_LEN - 1] = '\0';
    } else if (dst_idx != INVALID_INODE) {
        printf("mv: %s: ja existe\n", dst);
        return -1;
    }

    uint32_t existing;
    int found = dir_find_entry(dst_parent, dst_name, &existing);
    if (found && existing == src_idx) {
        return 0; // mesmo local e mesmo nome: nada a fazer
    }
    if (found) {
        printf("mv: %s: ja existe no destino\n", dst_name);
        return -1;
    }

    dir_remove_entry(src_parent, src_idx);
    dir_add_entry(dst_parent, src_idx);

    inode_t *inode = inode_get(src_idx);
    strncpy(inode->name, dst_name, MAX_NAME_LEN - 1);
    inode->name[MAX_NAME_LEN - 1] = '\0';
    inode->parent_inode = (int32_t)dst_parent; // mantem correta a resolucao de ".."/alvo de symlink
    inode->modified_at = time(NULL);
    return 0;
}

int fs_ln(uint32_t cwd, const char *target, const char *linkname, const char *user) {
    uint32_t link_idx, link_parent;
    char link_name[MAX_NAME_LEN];

    if (path_resolve(linkname, cwd, 0, &link_idx, &link_parent, link_name) != 0) {
        printf("ln: %s: caminho invalido\n", linkname);
        return -1;
    }
    if (link_idx != INVALID_INODE) {
        printf("ln: %s: ja existe\n", linkname);
        return -1;
    }

    uint32_t new_idx;
    if (dir_create_child(link_parent, link_name, INODE_LINK, user, &new_idx) != 0) {
        printf("ln: %s: nao foi possivel criar o link\n", linkname);
        return -1;
    }

    // O "conteudo" de um symlink e simplesmente o texto do caminho alvo.
    if (file_write(new_idx, target, strlen(target), 0) != 0) {
        printf("ln: %s: disco cheio\n", linkname);
        return -1;
    }
    return 0;
}
