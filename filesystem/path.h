#ifndef PATH_H
#define PATH_H

#include "../disk_manager/fs_types.h"
#include <stddef.h>

#define MAX_PATH_LEN 512
#define MAX_DEPTH 64 // numero maximo de componentes de caminho / niveis de diretorio

// Separa `path` em ate MAX_DEPTH componentes usando '/' (barras consecutivas
// e segmentos vazios sao ignorados). Retorna a quantidade de componentes encontrados.
int path_split(const char *path, char comps[MAX_DEPTH][MAX_NAME_LEN]);

// Resolve `path` (absoluto se comecar com '/', relativo a `base_inode`
// caso contrario). Symlinks encontrados em componentes intermediarios sao
// sempre seguidos; o ultimo componente so e seguido se `follow_final_link`
// for diferente de zero. "." e ".." sao tratados de forma especial.
//
// Em caso de sucesso (retorno 0):
//   - *out_inode e o i-node resolvido, ou INVALID_INODE se o ultimo
//     componente ainda nao existir (util para touch/mkdir/echo).
//   - *out_parent e o diretorio que contem (ou deveria conter) o alvo.
//   - out_name recebe o nome do ultimo componente do caminho.
// Retorna -1 se algum componente intermediario nao existir ou nao for um diretorio.
int path_resolve(const char *path, uint32_t base_inode, int follow_final_link,
                  uint32_t *out_inode, uint32_t *out_parent, char *out_name);

// Monta o caminho absoluto de `inode_idx` (ex.: "/a/b/c") em `out`.
void path_absolute(uint32_t inode_idx, char *out, size_t out_sz);

#endif
