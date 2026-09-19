#ifndef BITMAP_H
#define BITMAP_H

// Utilitario simples de bitmap, usado para controlar quais blocos/i-nodes
// estao livres ou ocupados. Opera direto sobre um buffer de bytes (em geral
// um ponteiro dentro da imagem de disco mapeada em memoria), um bit por
// bloco/i-node.

#include <stdint.h>

int bitmap_test(const uint8_t *bitmap, uint32_t bit);
void bitmap_set(uint8_t *bitmap, uint32_t bit);
void bitmap_clear(uint8_t *bitmap, uint32_t bit);

// Retorna o indice do primeiro bit livre em [0, total_bits), ou -1 se o
// bitmap estiver completamente cheio.
int32_t bitmap_find_first_free(const uint8_t *bitmap, uint32_t total_bits);

#endif
