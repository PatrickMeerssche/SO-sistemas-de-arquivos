#ifndef DIR_OPS_H
#define DIR_OPS_H

#include "../disk_manager/fs_types.h"

int fs_mkdir(uint32_t cwd, const char *path, const char *user);
int fs_rmdir(uint32_t cwd, const char *path, const char *user);

// Lista o conteudo de `path` (ou de `cwd` se `path` for uma string vazia).
int fs_ls(uint32_t cwd, const char *path, const char *user);

// Resolve `path` como um diretorio e retorna seu i-node em *out_inode.
int fs_cd(uint32_t cwd, const char *path, const char *user, uint32_t *out_inode);

#endif
