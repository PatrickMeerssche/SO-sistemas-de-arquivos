# SO-sistemas-de-arquivos

# Sistema de Arquivos em C

Este projeto implementa um sistema de arquivos didático em C. O conteúdo é guardado em uma imagem de disco local de 128 MiB, organizada em blocos, bitmaps e uma tabela de i-nodes. Um shell próprio permite criar, consultar, copiar, mover e remover arquivos e diretórios, além de criar links simbólicos.

O programa não usa os arquivos comuns do sistema operacional para representar cada arquivo do sistema simulado. Em vez disso, todos os objetos e conteúdos ficam dentro de um único arquivo de imagem, como em um dispositivo de armazenamento simplificado.

## Compilar e executar

Na raiz do projeto:

```bash
make
./fs
```

Para remover os arquivos de compilação:

```bash
make clean
```

O caminho padrão da imagem é `disk/disk_storage`. Também é possível indicar outro caminho como argumento:

```bash
./filesystem /caminho/para/imagem
```

O `Makefile` compila os módulos com GCC, C11 e as opções `-Wall -Wextra -g`. O executável é chamado `fs`.

## Organização do código

- `main.c`: escolhe o caminho da imagem, monta o disco, inicia o shell e desmonta o disco ao terminar.
- `disk_manager/fs_types.h`: define constantes do formato, tipos de i-node, permissões e estruturas que são gravadas na imagem.
- `disk_manager/disk.c`: abre ou cria a imagem, mapeia seus bytes com `mmap`, oferece acesso aos blocos e sincroniza alterações com `msync`.
- `disk_manager/superblock.c`: calcula o layout das regiões, inicializa os bitmaps e a tabela de i-nodes e cria o diretório raiz.
- `disk_manager/bitmap.c`: consulta, marca e libera bits usados para controlar alocação.
- `disk_manager/inode.c`: aloca e libera i-nodes e blocos, e traduz números de blocos lógicos em endereços da imagem.
- `filesystem/directory.c`: procura, insere, remove e enumera entradas de diretório; também cria i-nodes filhos.
- `filesystem/path.c`: separa caminhos e resolve componentes como `.`, `..`, caminhos absolutos, relativos e links simbólicos.
- `filesystem/perm.c`: verifica permissões do dono ou dos demais usuários.
- `filesystem/file_ops.c`: implementa criação, remoção, leitura, escrita, cópia, movimentação e links simbólicos.
- `filesystem/dir_ops.c`: implementa criação, remoção, listagem e mudança de diretório.
- `shell/shell.c`: lê comandos, separa argumentos e encaminha cada operação para a camada do sistema de arquivos.
- `Makefile`: lista os fontes, opções de compilação e tarefas `all` e `clean`.

## Percurso de uma operação

Uma operação começa no shell e desce pelas camadas até modificar bytes da imagem:

1. `main.c` chama `disk_mount()` e, se a montagem funcionar, chama `shell_run()`.
2. O shell lê uma linha, identifica o comando e passa o diretório atual, o caminho e o usuário à função correspondente. O usuário vem da variável de ambiente `USER`; se ela não estiver definida, usa-se `user`.
3. A função pública em `file_ops.c` ou `dir_ops.c` valida o tipo do objeto e as permissões necessárias. Para localizar caminhos, ela chama `path_resolve()`.
4. `path_resolve()` percorre diretórios usando `dir_find_entry()`. Para chegar a um arquivo ou diretório, cada entrada fornece um número de i-node.
5. As funções de diretório e arquivo usam `inode_get()`, `inode_get_block()`, os bitmaps e `disk_block_ptr()` para acessar ou alocar os dados na imagem mapeada.
6. Depois de cada comando processado, o shell chama `disk_sync()`. Ao sair, `main.c` chama `disk_unmount()`, que sincroniza novamente, desfaz o mapeamento e fecha o arquivo.

Esse desenho separa a interface de comandos da organização física do disco: o shell não precisa saber em qual bloco está um arquivo; ele pede a operação à camada do sistema de arquivos, que consulta os i-nodes e os diretórios.

## Imagem e layout do disco

As constantes principais estão em `disk_manager/fs_types.h`:

- `DISK_SIZE`: 128 MiB, ou 134.217.728 bytes.
- `BLOCK_SIZE`: 2048 bytes por bloco.
- `TOTAL_BLOCKS`: `DISK_SIZE / BLOCK_SIZE`, portanto 65.536 blocos.
- `NUM_INODES`: capacidade fixa de 4096 i-nodes.

O bloco é a unidade de alocação do conteúdo. O endereço de um bloco `n` na imagem é calculado como `base + n * BLOCK_SIZE`. Como o tamanho é potência de dois, a divisão do disco é regular e os cálculos de posição são simples. A escolha de 2048 bytes é uma decisão deste projeto, não uma exigência de hardware nem uma regra universal: blocos menores reduzem o espaço desperdiçado no último bloco de um arquivo, mas aumentam a quantidade de metadados e de operações de alocação; blocos maiores reduzem essa quantidade, mas podem deixar mais espaço interno sem uso. Com 2 KiB, cada arquivo ocupa blocos em uma granularidade moderada e a imagem de 128 MiB tem exatamente 65.536 blocos.

O disco é organizado em regiões consecutivas:

| Região           | Blocos nesta configuração | Para que serve                                          |
| ----------------- | --------------------------: | ------------------------------------------------------- |
| Superbloco        |                           1 | Descreve o formato e as posições das outras regiões. |
| Bitmap de blocos  |                           4 | Um bit para cada um dos 65.536 blocos.                  |
| Bitmap de i-nodes |                           1 | Um bit para cada um dos 4096 i-nodes.                   |
| Tabela de i-nodes |      288 nesta compilação | Guarda os metadados dos objetos.                        |
| Área de dados    |            65.242 restantes | Guarda conteúdo de arquivos e entradas de diretórios. |

Assim, os blocos 0 a 293 são reservados para metadados, e a área de dados começa no bloco 294. O bitmap de blocos precisa de 65.536 bits, ou 8192 bytes, que ocupam quatro blocos de 2048 bytes. O bitmap de i-nodes precisa de 4096 bits, ou 512 bytes; a região é reservada em um bloco inteiro.

Na compilação Linux x86-64 usada para conferir estes números, `sizeof(inode_t)` é 144 bytes. Portanto, `4096 * 144 = 589.824` bytes, ou 288 blocos de 2048 bytes, para a tabela de i-nodes. Esse tamanho depende do ABI e do compilador, pois a imagem armazena estruturas C diretamente; imagens não são garantidamente compatíveis entre plataformas ou versões que alterem o layout dessas estruturas.

## Por que 4096 i-nodes?

O valor é uma capacidade fixa escolhida pelo projeto. Não é calculado automaticamente a partir do tamanho do disco e não é imposto pelo conceito de i-node. Um i-node representa um objeto do sistema de arquivos: arquivo, diretório ou link simbólico. O diretório raiz também consome um i-node. I-nodes de continuação, descritos abaixo, também usam posições dessa tabela.

Essa capacidade permite representar milhares de objetos, inclusive arquivos vazios, sem reservar um bloco de conteúdo para cada um. Em troca, a tabela ocupa aproximadamente 576 KiB nesta plataforma (cerca de 0,44% do disco). Aumentar `NUM_INODES` aumenta a tabela e seu bitmap, mas permite mais objetos; diminuí-lo economiza metadados, mas pode deixar o sistema sem i-nodes mesmo que ainda haja blocos de dados livres. O limite de arquivos e diretórios, portanto, é diferente do limite de bytes armazenados.

## Estruturas persistidas

### Superbloco

O `superblock_t`, armazenado no bloco 0, registra o identificador `SUPERBLOCK_MAGIC`, o tamanho e a quantidade de blocos, a quantidade de i-nodes e o início/tamanho de cada região. Também mantém os contadores de espaço livre e o número do i-node raiz. Na montagem, o valor mágico ajuda a reconhecer se a imagem já contém este formato.

### Bitmaps

Um bitmap guarda um bit por recurso. Bit 0 significa livre e bit 1 significa ocupado. `bitmap.c` calcula o byte pelo índice dividido por 8 e o bit dentro desse byte pelo resto da divisão por 8. Na formatação, todos os blocos anteriores à área de dados são marcados como ocupados. `bitmap_find_first_free()` percorre os bits do começo ao fim e retorna o primeiro livre; por isso a política de alocação atual é simples, mas não busca uma posição a partir de uma dica ou índice recente.

### I-nodes

Um `inode_t` guarda metadados, não o conteúdo completo do arquivo. Entre seus campos estão:

- tipo e estado de alocação;
- nome, criador e dono;
- tamanho e datas de criação e modificação;
- bits de permissão do dono e dos demais usuários;
- dez ponteiros diretos para blocos;
- `next_inode`, usado para continuar a lista de ponteiros;
- `parent_inode`, usado para voltar ao pai e resolver caminhos relativos de links.

Ao criar um objeto, `inode_alloc()` encontra o primeiro bit livre, marca-o e zera o i-node. Ao liberar, `inode_free()` libera os blocos associados, limpa a estrutura e libera seu bit no bitmap.

### Blocos e i-nodes de continuação

Os dez ponteiros diretos de um i-node permitem endereçar diretamente dez blocos, isto é, até 20.480 bytes de conteúdo antes de precisar de continuação. Quando um índice lógico ultrapassa esses dez ponteiros, `inode_get_block()` aloca um i-node do tipo `INODE_CONTINUATION`; ele fornece mais dez ponteiros diretos. Se necessário, outro i-node de continuação é ligado ao próximo, formando uma cadeia.

Um arquivo escrito por partes é dividido em trechos que cabem nos blocos. Para cada trecho, `file_write()` localiza ou aloca o bloco, copia os bytes e atualiza o tamanho e a data de modificação. Ao sobrescrever, os blocos anteriores são liberados primeiro; ao acrescentar (`>>`), a escrita começa no tamanho já registrado. `file_read()` percorre os blocos até copiar exatamente o número de bytes indicado pelo tamanho do i-node.

### Diretórios

O conteúdo de um diretório é uma sequência de `dirent_t`, cada uma contendo um indicador `used` e o número do i-node filho. O nome não é duplicado na entrada: fica no i-node filho. Nesta plataforma, `sizeof(dirent_t)` é 8 bytes, então cabem 256 entradas em cada bloco de 2048 bytes.

`dir_find_entry()` percorre os blocos do diretório e compara o nome encontrado no i-node filho. `dir_add_entry()` primeiro reutiliza uma posição marcada como livre; se não houver, aloca outro bloco. O campo `size` de um diretório representa a capacidade alocada em blocos, não a quantidade de filhos. Remover uma entrada marca a posição como livre, e remover um diretório vazio libera seus blocos por meio de `inode_free()`.

## Formatação, montagem e persistência

`disk_mount()` abre a imagem para leitura e escrita e verifica seu tamanho. Se ela não tiver exatamente 128 MiB, o código a recria e escreve blocos zerados antes de mapear o arquivo. Em seguida, `mmap()` torna os bytes da imagem acessíveis como memória. Se o tamanho estiver correto, mas o superbloco não tiver o identificador esperado, o formato é inicializado. `superblock_format()` calcula as regiões, limpa bitmaps e tabela, marca os metadados como ocupados e cria o diretório raiz.

O mapeamento é compartilhado (`MAP_SHARED`), mas o código também chama `msync()` explicitamente em `disk_sync()` e ao desmontar. Assim, os comandos alteram a imagem diretamente e sincronizam as mudanças sem precisar serializar estruturas para outro formato.

## Caminhos, links e permissões

`path_split()` divide o caminho nas barras, ignora componentes vazios e limita a profundidade a `MAX_DEPTH` (64). `path_resolve()` começa na raiz para caminhos absolutos e no diretório atual para caminhos relativos. `.` mantém o diretório; `..` consulta `parent_inode`. Componentes intermediários precisam existir e ser diretórios. Quando o último componente não existe, a resolução pode retornar essa informação ao chamador para permitir criação.

Links simbólicos guardam como conteúdo o texto do caminho-alvo. Ao resolver um link, `follow_link()` interpreta alvos relativos a partir do diretório pai do próprio link, segue links encadeados e limita a dez saltos para evitar ciclos infinitos.

As permissões são três bits: leitura (`4`), escrita (`2`) e execução (`1`). O sistema compara o usuário atual ao dono do i-node e consulta `perm_owner` ou `perm_other`. Não há grupos, usuários autenticados, nem permissões separadas para grupo. Diretórios novos dão leitura, escrita e execução ao dono, e leitura e execução aos demais; arquivos e links dão leitura e escrita ao dono e leitura aos demais.

## Comandos do shell

Digite `help` para listar os comandos. Os comandos disponíveis são:

```text
touch <arquivo>                  cria um arquivo vazio
rm <arquivo>                     remove arquivo ou link
echo "texto" > <arquivo>         cria ou sobrescreve um arquivo
echo "texto" >> <arquivo>        acrescenta texto ao arquivo
cat <arquivo>                    mostra o conteúdo de um arquivo
cp <origem> <destino>            copia um arquivo comum
mv <origem> <destino>            move ou renomeia um objeto
ln -s <alvo> <link>              cria um link simbólico
mkdir <diretorio>                cria um diretório
rmdir <diretorio>                remove um diretório vazio
ls [diretorio]                   lista um diretório
cd <diretorio>                   muda o diretório atual
pwd                              mostra o caminho atual
exit | quit                      encerra o programa
```

O parser é propositalmente simples: os comandos são interpretados pelo próprio programa, não por um shell Bash. Portanto, não há suporte geral a encadeamento com `&&`, expansão de variáveis ou todas as regras de aspas do Bash.

## Limitações e observações da implementação

- Os nomes têm espaço para 32 bytes, incluindo o terminador nulo; assim, o limite usual é 31 bytes. Usuários têm espaço para 16 bytes, incluindo o terminador.
- O caminho tem limite de 512 bytes e no máximo 64 componentes.
- O tamanho dos arquivos é guardado em `uint32_t`; além desse limite de metadados, o disco de 128 MiB impõe um limite físico muito menor para o conteúdo total.
- O sistema é didático e não implementa journal, recuperação contra queda durante uma escrita, usuários/grupos completos ou concorrência entre processos.
- `superblock_format()` zera o superbloco e define o contador `free_inodes`, mas atualmente não inicializa `free_blocks` com a quantidade de blocos da área de dados. Como `block_alloc()` decrementa esse contador, o valor pode sofrer underflow na primeira alocação de bloco. O bitmap continua sendo o mecanismo usado para procurar blocos livres, mas a métrica de blocos livres no superbloco fica incorreta até esse contador ser inicializado adequadamente.
- A imagem guarda estruturas C diretamente. Para manter compatibilidade, o layout e o tamanho de `inode_t` precisam ser iguais entre quem cria e quem monta a imagem.
