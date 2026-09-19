#include "shell.h"
#include "../disk_manager/disk.h"
#include "../filesystem/dir_ops.h"
#include "../filesystem/file_ops.h"
#include "../filesystem/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024

// Imprime exatamente a lista de comandos exigida pelo trabalho.
static void print_help(void) {
    printf("Comandos disponiveis:\n");
    printf("  touch <arquivo>                  cria um arquivo vazio\n");
    printf("  rm <arquivo>                     remove um arquivo\n");
    printf("  echo \"texto\" > <arquivo>         cria/sobrescreve arquivo com texto\n");
    printf("  echo \"texto\" >> <arquivo>        cria/acrescenta texto ao arquivo\n");
    printf("  cat <arquivo>                    mostra o conteudo de um arquivo\n");
    printf("  cp <origem> <destino>            copia um arquivo\n");
    printf("  mv <origem> <destino>            renomeia/move arquivo ou diretorio\n");
    printf("  ln -s <alvo> <link>              cria um link simbolico\n");
    printf("  mkdir <diretorio>                cria um diretorio\n");
    printf("  rmdir <diretorio>                remove um diretorio vazio\n");
    printf("  ls [diretorio]                   lista o conteudo de um diretorio\n");
    printf("  cd <diretorio>                   troca de diretorio atual\n");
    printf("  pwd                              mostra o diretorio atual\n");
    printf("  exit | quit                      encerra o programa\n");
}

// Interpreta "echo <conteudo> > <caminho>" / "echo <conteudo> >> <caminho>",
// onde `args` e tudo o que vem depois da palavra "echo". O conteudo pode
// opcionalmente vir entre aspas duplas. Retorna 0 em caso de sucesso, -1 se
// nenhum operador de redirecionamento for encontrado.
static int parse_echo(const char *args, char *content, size_t content_sz,
                       char *path, size_t path_sz, int *append) {
    const char *op = strstr(args, ">>");
    int op_len = 2;
    if (op == NULL) {
        op = strstr(args, ">");
        op_len = 1;
    }
    if (op == NULL) {
        return -1;
    }
    *append = (op_len == 2);

    // conteudo = tudo antes do operador, sem espacos nas pontas e sem aspas
    int clen = (int)(op - args);
    while (clen > 0 && args[clen - 1] == ' ') {
        clen--;
    }
    int cstart = 0;
    while (cstart < clen && args[cstart] == ' ') {
        cstart++;
    }
    if (clen - cstart >= 2 && args[cstart] == '"' && args[clen - 1] == '"') {
        cstart++;
        clen--;
    }
    int outlen = clen - cstart;
    if (outlen < 0) {
        outlen = 0;
    }
    if ((size_t)outlen >= content_sz) {
        outlen = (int)content_sz - 1;
    }
    memcpy(content, args + cstart, (size_t)outlen);
    content[outlen] = '\0';

    // caminho = tudo depois do operador, sem espacos nas pontas
    const char *p = op + op_len;
    while (*p == ' ') {
        p++;
    }
    strncpy(path, p, path_sz - 1);
    path[path_sz - 1] = '\0';
    size_t plen = strlen(path);
    while (plen > 0 && path[plen - 1] == ' ') {
        path[--plen] = '\0';
    }
    if (plen == 0) {
        return -1;
    }
    return 0;
}

void shell_run(void) {
    char user[MAX_USER_LEN];
    const char *env_user = getenv("USER");
    strncpy(user, env_user ? env_user : "user", MAX_USER_LEN - 1);
    user[MAX_USER_LEN - 1] = '\0';

    uint32_t cwd = disk_superblock()->root_inode;
    char line[MAX_LINE_LEN];
    char prompt_path[MAX_PATH_LEN];

    printf("Mini sistema de arquivos baseado em i-nodes. Digite 'help' para ajuda.\n");

    for (;;) {
        path_absolute(cwd, prompt_path, sizeof(prompt_path));
        printf("%s:%s$ ", user, prompt_path);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // separa a palavra do comando, mantendo o resto como texto bruto de argumentos
        char *p = line;
        while (*p == ' ') {
            p++;
        }
        char *cmd_start = p;
        while (*p != '\0' && *p != ' ') {
            p++;
        }
        size_t cmd_len = (size_t)(p - cmd_start);
        if (cmd_len == 0) {
            continue; // linha vazia
        }
        char cmd[64];
        if (cmd_len >= sizeof(cmd)) {
            cmd_len = sizeof(cmd) - 1;
        }
        memcpy(cmd, cmd_start, cmd_len);
        cmd[cmd_len] = '\0';
        while (*p == ' ') {
            p++;
        }
        const char *args = p;

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "pwd") == 0) {
            printf("%s\n", prompt_path);
        } else if (strcmp(cmd, "touch") == 0) {
            fs_touch(cwd, args, user);
        } else if (strcmp(cmd, "rm") == 0) {
            fs_rm(cwd, args, user);
        } else if (strcmp(cmd, "mkdir") == 0) {
            fs_mkdir(cwd, args, user);
        } else if (strcmp(cmd, "rmdir") == 0) {
            fs_rmdir(cwd, args, user);
        } else if (strcmp(cmd, "ls") == 0) {
            fs_ls(cwd, args, user);
        } else if (strcmp(cmd, "cat") == 0) {
            fs_cat(cwd, args, user);
        } else if (strcmp(cmd, "cd") == 0) {
            const char *target = (strlen(args) == 0) ? "/" : args;
            uint32_t new_cwd;
            if (fs_cd(cwd, target, user, &new_cwd) == 0) {
                cwd = new_cwd;
            }
        } else if (strcmp(cmd, "cp") == 0) {
            char a[MAX_PATH_LEN], b[MAX_PATH_LEN];
            if (sscanf(args, "%511s %511s", a, b) == 2) {
                fs_cp(cwd, a, b, user);
            } else {
                printf("uso: cp <origem> <destino>\n");
            }
        } else if (strcmp(cmd, "mv") == 0) {
            char a[MAX_PATH_LEN], b[MAX_PATH_LEN];
            if (sscanf(args, "%511s %511s", a, b) == 2) {
                fs_mv(cwd, a, b, user);
            } else {
                printf("uso: mv <origem> <destino>\n");
            }
        } else if (strcmp(cmd, "ln") == 0) {
            char flag[16], a[MAX_PATH_LEN], b[MAX_PATH_LEN];
            if (sscanf(args, "%15s %511s %511s", flag, a, b) == 3 && strcmp(flag, "-s") == 0) {
                fs_ln(cwd, a, b, user);
            } else {
                printf("uso: ln -s <alvo> <link>\n");
            }
        } else if (strcmp(cmd, "echo") == 0) {
            char content[MAX_LINE_LEN], target_path[MAX_PATH_LEN];
            int append;
            if (parse_echo(args, content, sizeof(content), target_path, sizeof(target_path), &append) == 0) {
                fs_write_content(cwd, target_path, content, append, user);
            } else {
                printf("uso: echo \"conteudo\" > arquivo   ou   echo \"conteudo\" >> arquivo\n");
            }
        } else {
            printf("comando desconhecido: %s (digite 'help')\n", cmd);
        }

        disk_sync(); // persiste as alteracoes apos cada comando
    }
}
