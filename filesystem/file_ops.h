#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "../disk_manager/fs_types.h"

// Toda operacao recebe o i-node do diretorio atual de quem chamou (`cwd`)
// e o nome do "user" (usado nas checagens de permissao), e retorna 0 em
// caso de sucesso ou um valor negativo em caso de falha (erros tambem sao
// impressos na saida padrao).

int fs_touch(uint32_t cwd, const char *path, const char *user);
int fs_rm(uint32_t cwd, const char *path, const char *user);

// Escreve `content` no arquivo em `path`, criando-o se necessario.
// `append` escolhe entre "echo > " (sobrescreve) e "echo >> " (acrescenta).
int fs_write_content(uint32_t cwd, const char *path, const char *content, int append, const char *user);

int fs_cat(uint32_t cwd, const char *path, const char *user);
int fs_cp(uint32_t cwd, const char *src, const char *dst, const char *user);
int fs_mv(uint32_t cwd, const char *src, const char *dst, const char *user);

// Cria um link simbolico chamado `linkname` apontando para `target`.
int fs_ln(uint32_t cwd, const char *target, const char *linkname, const char *user);

#endif
