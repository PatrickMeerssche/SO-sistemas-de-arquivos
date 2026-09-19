#include "shell.h"
#include "../disk_manager/disk.h"
#include "../filesystem/dir_ops.h"
#include "../filesystem/file_ops.h"
#include "../filesystem/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The maximum length of a command line input, including the command and its arguments.
#define MAX_LINE_LEN 1024

// Print the help message with the list of available commands.
static void print_help(void) {
    printf("Available commands:\n");
    printf("  touch <file>                  Create an empty file\n");
    printf("  rm <file>                     Remove a file\n");
    printf("  echo \"text\" > <file>          Create/overwrite a file with text\n");
    printf("  echo \"text\" >> <file>         Create/append text to a file\n");
    printf("  cat <file>                    Read the content of a file\n");
    printf("  cp <source> <destination>     Copy a file\n");
    printf("  mv <source> <destination>     Rename/move a file or directory\n");
    printf("  ln -s <target> <link>         Create a symbolic link for a file or directory\n");
    printf("  mkdir <directory>             Create a directory\n");
    printf("  rmdir <directory>             Remove an empty directory\n");
    printf("  ls <directory>                List the contents of a directory\n");
    printf("  cd <directory>                Change the current directory\n");
    printf("  pwd                           Show the current directory\n");
    printf("  exit | quit                   Exit the program\n");
}

/* Parse the arguments of an echo command, extracting the content to write, the target file path, and whether to append or overwrite.
 *      Args:
 *          args: the string containing everything after the "echo" command.
 *          content: buffer to store the extracted content.
 *          content_sz: size of the content buffer.
 *          path: buffer to store the extracted file path.
 *          path_sz: size of the path buffer.
 *          append: pointer to store whether to append (1) or overwrite (0).
 *      Returns 0 on success, -1 if no redirection operator is found.
 */
static int parse_echo(const char *args, char *content, size_t content_sz, char *path, size_t path_sz, int *append) {
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

    // content = everything before the operator, without trailing spaces and without surrounding quotes  
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

    // path = everything after the operator, without trailing spaces
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

// Run the interactive loop (read command, execute, show result) until the user types "exit"/"quit" or closes the standard input.
void shell_run(void) {
    char user[MAX_USER_LEN];
    const char *env_user = getenv("USER");
    strncpy(user, env_user ? env_user : "user", MAX_USER_LEN - 1);
    user[MAX_USER_LEN - 1] = '\0';

    uint32_t cwd = disk_superblock()->root_inode;
    char line[MAX_LINE_LEN];
    char prompt_path[MAX_PATH_LEN];

    printf("Filesystem shell started. Type 'help' for a list of commands.\n");
    for (;;) {
        // Display the prompt with the current user and directory
        path_absolute(cwd, prompt_path, sizeof(prompt_path));
        printf("%s:%s$ ", user, prompt_path);
        fflush(stdout);

        // Read a line of input from the user
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Skip leading spaces and extract the command and its arguments
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

        // Execute the command based on the parsed input
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
            }
        } else if (strcmp(cmd, "mv") == 0) {
            char a[MAX_PATH_LEN], b[MAX_PATH_LEN];
            if (sscanf(args, "%511s %511s", a, b) == 2) {
                fs_mv(cwd, a, b, user);
            }
        } else if (strcmp(cmd, "ln") == 0) {
            char flag[16], a[MAX_PATH_LEN], b[MAX_PATH_LEN];
            if (sscanf(args, "%15s %511s %511s", flag, a, b) == 3 && strcmp(flag, "-s") == 0) {
                fs_ln(cwd, a, b, user);
            }
        } else if (strcmp(cmd, "echo") == 0) {
            char content[MAX_LINE_LEN], target_path[MAX_PATH_LEN];
            int append;
            if (parse_echo(args, content, sizeof(content), target_path, sizeof(target_path), &append) == 0) {
                fs_write_content(cwd, target_path, content, append, user);
            }
        } else {
            printf("Unknown command: %s (type 'help' for a list of commands)\n", cmd);
        }

        // Ensure that all changes are written to disk after each command execution
        disk_sync(); 
    }
}
