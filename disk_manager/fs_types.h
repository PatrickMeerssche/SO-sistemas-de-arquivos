#ifndef FS_TYPES_H
#define FS_TYPES_H

// Constantes e estruturas em disco compartilhadas por todas as camadas do
// sistema de arquivos (gerenciador de disco, logica do fs e shell).

#include <stdint.h>
#include <time.h>

// Geometria do disco
#define BLOCK_SIZE   2048u                     // bytes por bloco
#define DISK_SIZE    (128u * 1024u * 1024u)    // tamanho total do disco: 128 MB
#define TOTAL_BLOCKS (DISK_SIZE / BLOCK_SIZE)  // 65536 blocos

#define NUM_INODES   4096u  // quantidade fixa de i-nodes suportados pelo fs

#define MAX_NAME_LEN  32   // tamanho maximo do nome de um arquivo/diretorio/link
#define MAX_USER_LEN  16   // tamanho maximo do nome de um usuario (criador/dono)
#define DIRECT_BLOCKS 10   // ponteiros diretos para blocos, guardados em cada i-node

#define SUPERBLOCK_MAGIC 0x46534653u // "FSFS", identifica um disco ja formatado

#define INVALID_INODE ((uint32_t)-1)
#define INVALID_BLOCK 0u // bloco 0 e o superbloco, nunca e um bloco de dados valido

// Tipos de i-node
typedef enum {
    INODE_FREE = 0,
    INODE_FILE,
    INODE_DIR,
    INODE_LINK,        // link simbolico (ln -s): conteudo = texto do caminho alvo
    INODE_CONTINUATION // i-node de overflow: so guarda ponteiros extras para blocos
} inode_type_t;

// Bits de permissao (mesma ideia do rwx do unix, um grupo de 3 bits cada)
#define PERM_READ  4
#define PERM_WRITE 2
#define PERM_EXEC  1

// Superbloco em disco (ocupa o bloco 0)
typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t num_inodes;

    uint32_t block_bitmap_start;
    uint32_t block_bitmap_blocks;
    uint32_t inode_bitmap_start;
    uint32_t inode_bitmap_blocks;
    uint32_t inode_table_start;
    uint32_t inode_table_blocks;
    uint32_t data_start;

    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t root_inode;
} superblock_t;

// I-node em disco: guarda todos os atributos exigidos pelo trabalho (nome,
// criador, dono, tamanho, datas de criacao/modificacao, permissoes de dono/
// outros, apontadores para blocos de dados e um apontador para um eventual
// i-node de continuacao, usado quando o arquivo precisa de mais apontadores
// de bloco do que os diretos conseguem guardar).
typedef struct {
    uint8_t used; // 1 se este i-node esta alocado
    uint8_t type; // um dos valores de inode_type_t

    char name[MAX_NAME_LEN];
    char creator[MAX_USER_LEN];
    char owner[MAX_USER_LEN];

    uint32_t size; // bytes de conteudo real (ou capacidade, no caso de diretorios)
    time_t created_at;
    time_t modified_at;

    uint8_t perm_owner; // bits PERM_READ|PERM_WRITE|PERM_EXEC do dono
    uint8_t perm_other; // mesmos bits para os demais usuarios

    uint32_t direct_blocks[DIRECT_BLOCKS]; // 0 == posicao nao utilizada
    int32_t next_inode;   // indice do i-node de continuacao, ou -1 se nao houver
    int32_t parent_inode; // i-node do diretorio pai, -1 se nao houver
} inode_t;

// Entrada de diretorio (guardada dentro dos blocos de dados de um i-node de diretorio)
typedef struct {
    uint8_t used;
    int32_t inode_num;
} dirent_t;

#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(dirent_t))

#endif
