#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

//===================================================================
// HELPER FUNCTIONS - Memory Management (PROVIDED)
//===================================================================

/**
 * alloc_cmd_buff - Allocate memory for cmd_buff internal buffer
 *
 * This function is provided for you. It allocates the _cmd_buffer
 * that will store the command string.
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_buff - Free cmd_buff internal buffer
 *
 * This function is provided for you. Call it when done with a cmd_buff.
 */
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    return OK;
}

/**
 * clear_cmd_buff - Reset cmd_buff without freeing memory
 *
 * This function is provided for you.
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_list - Free all cmd_buffs in a command list
 *
 * This function is provided for you. It frees all allocated memory
 * in a command_list_t structure.
 */
int free_cmd_list(command_list_t *cmd_lst)
{
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    cmd_lst->num = 0;
    return OK;
}

//===================================================================
// PARSING FUNCTIONS - REUSE FROM PARTS 1 & 2
//===================================================================

/**
 * build_cmd_buff - Parse a single command string into cmd_buff_t
 *
 * REUSE YOUR PART 2 IMPLEMENTATION!
 *
 * This function takes a single command string and parses it into
 * argc/argv format, handling quotes correctly.
 *
 * @param cmd_line: Command string to parse
 * @param cmd_buff: Allocated cmd_buff_t to populate
 * @return: OK on success, error code on failure
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    if (cmd_line == NULL || cmd_buff == NULL || cmd_buff->_cmd_buffer == NULL) return ERR_MEMORY;
    clear_cmd_buff(cmd_buff);

    int n = (int)strlen(cmd_line);
    int i = 0, w = 0;
    while (i < n && isspace((unsigned char)cmd_line[i])) i++;
    if (i >= n) return WARN_NO_CMDS;
    while (n > 0 && isspace((unsigned char)cmd_line[n - 1])) n--;
    if (n <= i) return WARN_NO_CMDS;

    while (i < n) {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1) break;
        cmd_buff->argv[cmd_buff->argc] = &cmd_buff->_cmd_buffer[w];
        if (cmd_line[i] == '"' || cmd_line[i] == '\'') {
            char q = cmd_line[i++];
            while (i < n && cmd_line[i] != q) cmd_buff->_cmd_buffer[w++] = cmd_line[i++];
            if (i < n && cmd_line[i] == q) i++;
        } else {
            while (i < n && !isspace((unsigned char)cmd_line[i])) cmd_buff->_cmd_buffer[w++] = cmd_line[i++];
        }

        cmd_buff->_cmd_buffer[w++] = '\0';
        cmd_buff->argc++;
        while (i < n && isspace((unsigned char)cmd_line[i])) i++;
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    if (cmd_buff->argc == 0) return WARN_NO_CMDS;
    return OK;
}

/**
 * build_cmd_list - Parse command line with pipes into command_list_t
 *
 * REUSE YOUR PART 1 IMPLEMENTATION!
 *
 * This function splits input by pipes and creates a command_list_t.
 *
 * @param cmd_line: Full command line from user input
 * @param clist: Command list to populate
 * @return: OK on success, error code on failure
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || clist == NULL) return WARN_NO_CMDS;

    clist->num = 0;

    while (*cmd_line && isspace((unsigned char)*cmd_line)) cmd_line++;
    if (*cmd_line == '\0') return WARN_NO_CMDS;

    int end = (int)strlen(cmd_line);
    while (end > 0 && isspace((unsigned char)cmd_line[end - 1])) {
        cmd_line[end - 1] = '\0';
        end--;
    }
    if (end <= 0) return WARN_NO_CMDS;

    int pipes = 0;
    for (int i = 0; cmd_line[i] != '\0'; i++) {
        if (cmd_line[i] == PIPE_CHAR) pipes++;
    }
    if (pipes + 1 > CMD_MAX) return ERR_TOO_MANY_COMMANDS;

    char *save = NULL;
    char *seg = strtok_r(cmd_line, PIPE_STRING, &save);

    while (seg != NULL) {
        while (*seg && isspace((unsigned char)*seg)) seg++;

        int slen = (int)strlen(seg);
        while (slen > 0 && isspace((unsigned char)seg[slen - 1])) {
            seg[slen - 1] = '\0';
            slen--;
        }
        if (slen <= 0) {
            free_cmd_list(clist);
            return WARN_NO_CMDS;
        }

        int rc = alloc_cmd_buff(&clist->commands[clist->num]);
        if (rc != OK) {
            free_cmd_list(clist);
            return rc;
        }

        rc = build_cmd_buff(seg, &clist->commands[clist->num]);
        if (rc != OK) {
            free_cmd_buff(&clist->commands[clist->num]);
            free_cmd_list(clist);
            return rc;
        }
        clist->num++;
        if (clist->num > CMD_MAX) {
            free_cmd_list(clist);
            return ERR_TOO_MANY_COMMANDS;
        }

        seg = strtok_r(NULL, PIPE_STRING, &save);
    }

    if (clist->num == 0) return WARN_NO_CMDS;
    return OK;
}

//===================================================================
// BUILT-IN COMMAND FUNCTIONS - REUSE FROM PART 2
//===================================================================

/**
 * match_command - Check if input is a built-in command
 *
 * This function is provided for you.
 */
Built_In_Cmds match_command(const char *input)
{
    if (strcmp(input, "exit") == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "dragon") == 0) return BI_CMD_DRAGON;
    return BI_NOT_BI;
}

/**
 * exec_built_in_cmd - Execute built-in commands
 *
 * REUSE YOUR PART 2 IMPLEMENTATION!
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);

    switch (bi_cmd) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;

        case BI_CMD_DRAGON:
            #ifdef DRAGON_IMPLEMENTED
            print_dragon();
            #else
            printf("Dragon not implemented\n");
            #endif
            return BI_EXECUTED;

        case BI_CMD_CD:
            if (cmd->argc < 2) return BI_EXECUTED;
            if (chdir(cmd->argv[1]) != 0) perror("cd");
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// EXECUTION FUNCTIONS
//===================================================================

/**
 * exec_cmd - Execute single command using fork/exec
 *
 * REUSE YOUR PART 2 IMPLEMENTATION!
 *
 * This function implements the fork/exec pattern to run one command.
 *
 * @param cmd: Command to execute
 * @return: Child's exit code on success, error code on failure
 */
int exec_cmd(cmd_buff_t *cmd)
 {
     if (!cmd || cmd->argc == 0) return ERR_EXEC_CMD;

     pid_t pid = fork();
     if (pid < 0) {
         perror("fork");
         return ERR_EXEC_CMD;
     }

     if (pid == 0) {
         execvp(cmd->argv[0], cmd->argv);
         perror("execvp");
         exit(1);
     }

     int status = 0;
     if (waitpid(pid, &status, 0) < 0) {
         perror("waitpid");
         return ERR_EXEC_CMD;
     }

     if (WIFEXITED(status)) return WEXITSTATUS(status);
     return ERR_EXEC_CMD;
 }

/**
 * execute_pipeline - Execute piped commands
 *
 * YOU NEED TO IMPLEMENT THIS FUNCTION FOR PART 3!
 *
 * This is the main function for Part 3. It creates pipes, forks
 * children for each command, connects them with dup2(), and waits
 * for all to complete.
 *
 * Algorithm for N commands in pipeline:
 *
 * 1. CREATE PIPES
 *    - Need N-1 pipes for N commands
 *    - pipe_fds[N-1][2] array
 *    - Each pipe has [0]=read, [1]=write
 *
 * 2. FORK AND SETUP EACH COMMAND
 *    For each command i from 0 to N-1:
 *    a. Fork child process
 *    b. In child:
 *       - If NOT first command (i > 0):
 *           dup2(pipe_fds[i-1][0], STDIN_FILENO)
 *           (read from previous pipe)
 *       - If NOT last command (i < N-1):
 *           dup2(pipe_fds[i][1], STDOUT_FILENO)
 *           (write to next pipe)
 *       - Close ALL pipe file descriptors
 *         (both read and write ends of all pipes)
 *       - execvp(command)
 *    c. In parent:
 *       - Store child PID for later wait
 *
 * 3. PARENT CLOSES ALL PIPES
 *    - Close all read and write ends
 *    - Critical: if parent keeps write end open,
 *      last command never gets EOF!
 *
 * 4. WAIT FOR ALL CHILDREN
 *    - Loop N times calling wait() or waitpid()
 *    - This prevents zombie processes
 *
 * Example for "ls | grep txt | wc -l":
 *
 *   Pipes created:
 *     pipe_fds[0]: ls → grep
 *     pipe_fds[1]: grep → wc
 *
 *   Command 0 (ls):
 *     - stdin: terminal (no dup2)
 *     - stdout: pipe_fds[0][1]
 *     - Close: all pipe fds
 *
 *   Command 1 (grep):
 *     - stdin: pipe_fds[0][0]
 *     - stdout: pipe_fds[1][1]
 *     - Close: all pipe fds
 *
 *   Command 2 (wc):
 *     - stdin: pipe_fds[1][0]
 *     - stdout: terminal (no dup2)
 *     - Close: all pipe fds
 *
 * CRITICAL: Close ALL pipe ends!
 * - Child must close all pipes after dup2
 * - Parent must close all pipes after forking
 * - If you don't, processes hang waiting for EOF
 *
 * Starter code structure:
 *
 *   int num_commands = clist->num;
 *   int num_pipes = num_commands - 1;
 *   int pipe_fds[num_pipes][2];
 *   pid_t pids[num_commands];
 *
 *   // Step 1: Create all pipes
 *   for (int i = 0; i < num_pipes; i++) {
 *       if (pipe(pipe_fds[i]) < 0) {
 *           perror("pipe");
 *           return ERR_EXEC_CMD;
 *       }
 *   }
 *
 *   // Step 2: Fork and setup each command
 *   for (int i = 0; i < num_commands; i++) {
 *       pid_t pid = fork();
 *
 *       if (pid < 0) {
 *           perror("fork");
 *           return ERR_EXEC_CMD;
 *       }
 *
 *       if (pid == 0) {  // Child process
 *           // TODO: Setup stdin from previous pipe (if i > 0)
 *
 *           // TODO: Setup stdout to next pipe (if i < num_commands-1)
 *
 *           // TODO: Close ALL pipe file descriptors
 *           // CRITICAL: Close both ends of ALL pipes!
 *
 *           // TODO: Execute command
 *           execvp(clist->commands[i].argv[0], clist->commands[i].argv);
 *           perror("execvp");
 *           exit(EXIT_FAILURE);
 *       } else {  // Parent process
 *           pids[i] = pid;
 *       }
 *   }
 *
 *   // Step 3: Parent closes all pipes
 *   // TODO: Close all pipe file descriptors
 *   // Both read and write ends of all pipes
 *
 *   // Step 4: Wait for all children
 *   // TODO: Loop through all children and wait
 *   for (int i = 0; i < num_commands; i++) {
 *       waitpid(pids[i], NULL, 0);
 *   }
 *
 *   return OK;
 *
 * @param clist: Command list to execute as pipeline
 * @return: OK on success, error code on failure
 */
int execute_pipeline(command_list_t *clist)
{
    if (clist == NULL || clist->num < 2) return ERR_EXEC_CMD;

    int ncmd = clist->num;
    int npipes = ncmd - 1;

    int pipe_fds[CMD_MAX - 1][2];
    pid_t pids[CMD_MAX];

    for (int i = 0; i < npipes; i++) {
        if (pipe(pipe_fds[i]) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < ncmd; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pid == 0) {
            if (i > 0) dup2(pipe_fds[i - 1][0], STDIN_FILENO);
            if (i < ncmd - 1) dup2(pipe_fds[i][1], STDOUT_FILENO);

            for (int j = 0; j < npipes; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(1);
        }

        pids[i] = pid;
    }

    for (int i = 0; i < npipes; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }

    for (int i = 0; i < ncmd; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

//===================================================================
// MAIN SHELL LOOP
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 *
 * This combines work from Parts 1, 2, and 3.
 *
 * Changes from Part 2:
 *   - Parse into command_list_t (not just cmd_buff_t)
 *   - If single command: use exec_cmd()
 *   - If multiple commands: use execute_pipeline()
 *
 * Algorithm:
 *
 *   1. Loop forever
 *   2. Print prompt
 *   3. Read input
 *   4. Check for exit
 *   5. Parse into command_list_t
 *   6. If first command is built-in, execute it
 *   7. Otherwise:
 *      - If num == 1: exec_cmd(&commands[0])
 *      - If num > 1: execute_pipeline(&clist)
 *   8. Free memory
 *   9. Loop back
 *
 * Starter code:
 *
 *   char cmd_line[SH_CMD_MAX];
 *   command_list_t clist;
 *   int rc;
 *
 *   while (1) {
 *       printf("%s", SH_PROMPT);
 *
 *       if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
 *           printf("\n");
 *           break;
 *       }
 *
 *       cmd_line[strcspn(cmd_line, "\n")] = '\0';
 *
 *       if (strcmp(cmd_line, EXIT_CMD) == 0) {
 *           printf("exiting...\n");
 *           break;
 *       }
 *
 *       // TODO: Parse into command_list_t
 *       rc = build_cmd_list(cmd_line, &clist);
 *
 *       if (rc != OK) {
 *           if (rc == WARN_NO_CMDS) {
 *               printf(CMD_WARN_NO_CMD);
 *           } else if (rc == ERR_TOO_MANY_COMMANDS) {
 *               printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
 *           }
 *           continue;
 *       }
 *
 *       // TODO: Check if first command is built-in
 *       Built_In_Cmds bi = exec_built_in_cmd(&clist.commands[0]);
 *       if (bi == BI_CMD_EXIT) {
 *           printf("exiting...\n");
 *           free_cmd_list(&clist);
 *           break;
 *       }
 *       if (bi == BI_EXECUTED) {
 *           free_cmd_list(&clist);
 *           continue;
 *       }
 *
 *       // TODO: Execute command(s)
 *       if (clist.num == 1) {
 *           exec_cmd(&clist.commands[0]);
 *       } else {
 *           execute_pipeline(&clist);
 *       }
 *
 *       free_cmd_list(&clist);
 *   }
 *
 *   return OK;
 *
 * @return: OK on success
 */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;
    clist.num = 0;

    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        int rc = build_cmd_list(cmd_line, &clist);

        if (rc != OK) {
            if (rc == WARN_NO_CMDS) {
                printf(CMD_WARN_NO_CMD);
            } else if (rc == ERR_TOO_MANY_COMMANDS) {
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            }
            continue;
        }

        Built_In_Cmds bi = exec_built_in_cmd(&clist.commands[0]);

        if (bi == BI_CMD_EXIT) {
            printf("exiting...\n");
            free_cmd_list(&clist);
            break;
        }

        if (bi == BI_EXECUTED) {
            free_cmd_list(&clist);
            continue;
        }

        if (clist.num == 1) {
            exec_cmd(&clist.commands[0]);
        } else {
            execute_pipeline(&clist);
        }

        free_cmd_list(&clist);
    }

    return OK;
}
