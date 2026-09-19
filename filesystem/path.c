#include "path.h"
#include "../disk_manager/disk.h"
#include "../disk_manager/inode.h"
#include "directory.h"

#include <stdio.h>
#include <string.h>

int path_split(const char *path, char comps[MAX_DEPTH][MAX_NAME_LEN]) {
    int count = 0;
    const char *p = path;

    while (*p != '\0' && count < MAX_DEPTH) {
        while (*p == '/') {
            p++; // pula barras (tambem colapsa "//")
        }
        if (*p == '\0') {
            break;
        }
        const char *start = p;
        while (*p != '/' && *p != '\0') {
            p++;
        }
        int len = (int)(p - start);
        if (len >= MAX_NAME_LEN) {
            len = MAX_NAME_LEN - 1;
        }
        memcpy(comps[count], start, (size_t)len);
        comps[count][len] = '\0';
        count++;
    }
    return count;
}

// Segue uma cadeia de symlinks a partir de `link_idx`, resolvendo alvos
// relativos em relacao ao proprio diretorio pai do link (comportamento
// padrao de symlink). Retorna o i-node final que nao e um link, ou
// INVALID_INODE em caso de link quebrado ou saltos demais (protecao contra loop).
static uint32_t follow_link(uint32_t link_idx, int depth) {
    if (depth <= 0) {
        return INVALID_INODE;
    }

    inode_t *link = inode_get(link_idx);
    char target[MAX_PATH_LEN];
    uint32_t len = (link->size < sizeof(target) - 1) ? link->size : (uint32_t)sizeof(target) - 1;

    uint32_t blk = inode_get_block(link, 0, 0);
    if (blk == INVALID_BLOCK) {
        return INVALID_INODE; // link sem conteudo: quebrado
    }
    memcpy(target, disk_block_ptr(blk), len);
    target[len] = '\0';

    uint32_t base = (link->parent_inode >= 0) ? (uint32_t)link->parent_inode
                                               : disk_superblock()->root_inode;

    uint32_t resolved, parent;
    char name[MAX_NAME_LEN];
    if (path_resolve(target, base, 1, &resolved, &parent, name) != 0 || resolved == INVALID_INODE) {
        return INVALID_INODE;
    }
    if (inode_get(resolved)->type == INODE_LINK) {
        return follow_link(resolved, depth - 1);
    }
    return resolved;
}

int path_resolve(const char *path, uint32_t base_inode, int follow_final_link,
                  uint32_t *out_inode, uint32_t *out_parent, char *out_name) {
    char comps[MAX_DEPTH][MAX_NAME_LEN];
    int count = path_split(path, comps);

    uint32_t current = (path[0] == '/') ? disk_superblock()->root_inode : base_inode;
    uint32_t parent = current;

    if (count == 0) {
        // Caminho era "", "/" ou so barras: refere-se ao proprio `current`.
        *out_inode = current;
        *out_parent = current;
        out_name[0] = '\0';
        return 0;
    }

    for (int i = 0; i < count; i++) {
        char *comp = comps[i];
        int is_last = (i == count - 1);

        if (strcmp(comp, ".") == 0) {
            continue;
        }

        if (strcmp(comp, "..") == 0) {
            inode_t *cur = inode_get(current);
            if (cur->type != INODE_DIR) {
                return -1;
            }
            parent = current;
            current = (cur->parent_inode >= 0) ? (uint32_t)cur->parent_inode : current;
            continue;
        }

        inode_t *cur = inode_get(current);
        if (cur->type != INODE_DIR) {
            return -1; // tentando descer em algo que nao e um diretorio
        }

        uint32_t child;
        if (!dir_find_entry(current, comp, &child)) {
            if (is_last) {
                *out_inode = INVALID_INODE;
                *out_parent = current;
                strncpy(out_name, comp, MAX_NAME_LEN - 1);
                out_name[MAX_NAME_LEN - 1] = '\0';
                return 0; // ultimo componente nao existe: quem chamou pode criar
            }
            return -1; // componente intermediario ausente
        }

        if (inode_get(child)->type == INODE_LINK && (!is_last || follow_final_link)) {
            child = follow_link(child, 10);
            if (child == INVALID_INODE) {
                return -1;
            }
        }

        parent = current;
        current = child;
    }

    *out_inode = current;
    *out_parent = parent;
    strncpy(out_name, comps[count - 1], MAX_NAME_LEN - 1);
    out_name[MAX_NAME_LEN - 1] = '\0';
    return 0;
}

void path_absolute(uint32_t inode_idx, char *out, size_t out_sz) {
    uint32_t root = disk_superblock()->root_inode;
    if (inode_idx == root) {
        snprintf(out, out_sz, "/");
        return;
    }

    char names[MAX_DEPTH][MAX_NAME_LEN];
    int depth = 0;
    uint32_t current = inode_idx;

    while (current != root && depth < MAX_DEPTH) {
        inode_t *n = inode_get(current);
        strncpy(names[depth], n->name, MAX_NAME_LEN - 1);
        names[depth][MAX_NAME_LEN - 1] = '\0';
        depth++;
        current = (n->parent_inode >= 0) ? (uint32_t)n->parent_inode : root;
    }

    out[0] = '\0';
    for (int i = depth - 1; i >= 0; i--) {
        strncat(out, "/", out_sz - strlen(out) - 1);
        strncat(out, names[i], out_sz - strlen(out) - 1);
    }
}
