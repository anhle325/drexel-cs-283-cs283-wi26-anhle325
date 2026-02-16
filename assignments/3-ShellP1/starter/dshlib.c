#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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
// PARSING FUNCTIONS - YOU IMPLEMENT THESE
//===================================================================

/**
 * build_cmd_buff - Parse a single command string into cmd_buff_t
 * 
 * YOU NEED TO IMPLEMENT THIS FUNCTION!
 * 
 * This function takes a single command string (no pipes) and parses
 * it into argc/argv format.
 * 
 * Steps:
 *   1. Copy cmd_line into cmd_buff->_cmd_buffer
 *   2. Split the string by spaces into tokens
 *   3. Store each token pointer in cmd_buff->argv[]
 *   4. Set cmd_buff->argc to number of tokens
 *   5. Ensure cmd_buff->argv[argc] is NULL (required for execvp later)
 * 
 * Example:
 *   Input:  "ls -la /tmp"
 *   Output: argc=3, argv=["ls", "-la", "/tmp", NULL]
 * 
 * Hints:
 *   - Use strcpy() to copy cmd_line to _cmd_buffer
 *   - Use strtok() to split by spaces
 *   - Remember to trim leading/trailing whitespace
 *   - Handle multiple consecutive spaces
 * 
 * @param cmd_line: Command string to parse
 * @param cmd_buff: Allocated cmd_buff_t to populate
 * @return: OK on success, error code on failure
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    if (cmd_line == NULL || cmd_buff == NULL) return WARN_NO_CMDS;

    while (*cmd_line && isspace((unsigned char)*cmd_line)) cmd_line++;
    if (*cmd_line == '\0') return WARN_NO_CMDS;

    int len = (int)strlen(cmd_line);
    while (len > 0 && isspace((unsigned char)cmd_line[len - 1])) {
        cmd_line[len - 1] = '\0';
        len--;
    }
    if (len <= 0) return WARN_NO_CMDS;

    strcpy(cmd_buff->_cmd_buffer, cmd_line);

    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) cmd_buff->argv[i] = NULL;

    char *tok = strtok(cmd_buff->_cmd_buffer, " \t");
    while (tok != NULL) {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1) break;
        cmd_buff->argv[cmd_buff->argc] = tok;
        cmd_buff->argc++;
        tok = strtok(NULL, " \t");
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;

    if (cmd_buff->argc == 0) return WARN_NO_CMDS;
    return OK;
}


/**
 * build_cmd_list - Parse command line with pipes into command_list_t
 * 
 * YOU NEED TO IMPLEMENT THIS FUNCTION! This is the main parsing function.
 * 
 * This function:
 *   1. Checks if input is empty/whitespace only
 *   2. Splits input by pipe character '|'
 *   3. For each segment, creates a cmd_buff_t
 *   4. Stores all cmd_buffs in command_list_t
 * 
 * Algorithm:
 *   1. Trim leading/trailing whitespace from cmd_line
 *   2. If empty, return WARN_NO_CMDS
 *   3. Count pipe characters
 *      - If count > CMD_MAX-1, return ERR_TOO_MANY_COMMANDS
 *   4. Split cmd_line by '|' character
 *      - For each segment:
 *        a. Allocate cmd_buff using alloc_cmd_buff()
 *        b. Parse segment using build_cmd_buff()
 *        c. Store in clist->commands[i]
 *   5. Set clist->num to number of commands
 *   6. Return OK
 * 
 * Example inputs and outputs:
 * 
 *   Input:  "cmd"
 *   Output: num=1, commands[0]={argc=1, argv=["cmd", NULL]}
 * 
 *   Input:  "cmd arg1 arg2"
 *   Output: num=1, commands[0]={argc=3, argv=["cmd", "arg1", "arg2", NULL]}
 * 
 *   Input:  "cmd1 | cmd2"
 *   Output: num=2, 
 *           commands[0]={argc=1, argv=["cmd1", NULL]}
 *           commands[1]={argc=1, argv=["cmd2", NULL]}
 * 
 *   Input:  "ls | grep txt | wc -l"
 *   Output: num=3,
 *           commands[0]={argc=1, argv=["ls", NULL]}
 *           commands[1]={argc=2, argv=["grep", "txt", NULL]}
 *           commands[2]={argc=2, argv=["wc", "-l", NULL]}
 * 
 * Error cases:
 *   Input:  ""  or "   "
 *   Return: WARN_NO_CMDS
 * 
 *   Input:  "c1|c2|c3|c4|c5|c6|c7|c8|c9"  (9 commands)
 *   Return: ERR_TOO_MANY_COMMANDS
 * 
 * Hints:
 *   - Use strchr() to find pipe characters
 *   - Use strtok() with PIPE_STRING to split by pipes
 *   - Remember to trim spaces around each command segment
 *   - Don't forget to set clist->num!
 *   - Call alloc_cmd_buff() for each command before build_cmd_buff()
 * 
 * Standard library functions you might use:
 *   - strlen(), strcpy(), strcmp()
 *   - strtok(), strchr()
 *   - isspace(), isalpha()
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
// BUILT-IN COMMAND FUNCTIONS (PROVIDED FOR PART 1)
//===================================================================

/**
 * match_command - Check if input is a built-in command
 * 
 * This function is provided for you.
 */
Built_In_Cmds match_command(const char *input)
{
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

/**
 * exec_built_in_cmd - Execute built-in commands
 * 
 * This function is provided for you, but incomplete.
 * You can add dragon command here for extra credit.
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            // Exit is handled in main loop
            return BI_CMD_EXIT;

        case BI_CMD_DRAGON: {
            static const char *d[] = {
"                                                                        @%%%%                       ",
"                                                                     %%%%%%                         ",
"                                                                    %%%%%%                          ",
"                                                                 % %%%%%%%           @              ",
"                                                                %%%%%%%%%%        %%%%%%%           ",
"                                       %%%%%%%  %%%%@         %%%%%%%%%%%%@    %%%%%%  @%%%%        ",
"                                  %%%%%%%%%%%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%%%%%%%%%%%%          ",
"                                %%%%%%%%%%%%%%%%%%%%%%%%%%   %%%%%%%%%%%% %%%%%%%%%%%%%%%           ",
"                               %%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%     %%%            ",
"                             %%%%%%%%%%%%%%%%%%%%%%%%%%%%@ @%%%%%%%%%%%%%%%%%%        %%            ",
"                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%                ",
"                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              ",
"                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%@%%%%%%@              ",
"      %%%%%%%%@           %%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%      %%                ",
"    %%%%%%%%%%%%%         %%@%%%%%%%%%%%%           %%%%%%%%%%% %%%%%%%%%%%%      @%                ",
"  %%%%%%%%%%   %%%        %%%%%%%%%%%%%%            %%%%%%%%%%%%%%%%%%%%%%%%                        ",
" %%%%%%%%%       %         %%%%%%%%%%%%%             %%%%%%%%%%%%@%%%%%%%%%%%                       ",
"%%%%%%%%%@                % %%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%                     ",
"%%%%%%%%@                 %%@%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  ",
"%%%%%%%@                   %%%%%%%%%%%%%%%           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              ",
"%%%%%%%%%%                  %%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      %%%%  ",
"%%%%%%%%%@                   @%%%%%%%%%%%%%%         %%%%%%%%%%%%@ %%%% %%%%%%%%%%%%%%%%%   %%%%%%%%",
"%%%%%%%%%%                  %%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%% %%%%%%%%%",
"%%%%%%%%%@%%@                %%%%%%%%%%%%%%%%@       %%%%%%%%%%%%%%     %%%%%%%%%%%%%%%%%%%%%%%%  %%",
" %%%%%%%%%%                  % %%%%%%%%%%%%%%@        %%%%%%%%%%%%%%   %%%%%%%%%%%%%%%%%%%%%%%%%% %%",
"  %%%%%%%%%%%%  @           %%%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  %%% ",
"   %%%%%%%%%%%%% %%  %  %@ %%%%%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%% ",
"    %%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%           @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%%%%%% ",
"     %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              %%%%%%%%%%%%%%%%%%%%%%%%%%%%        %%%   ",
"      @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  %%%%%%%%%%%%%%%%%%%%%%%%%               ",
"        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%%%%%%  %%%%%%%          ",
"           %%%%%%%%%%%%%%%%%%%%%%%%%%                           %%%%%%%%%%%%%%%  @%%%%%%%%%         ",
"              %%%%%%%%%%%%%%%%%%%%           @%@%                  @%%%%%%%%%%%%%%%%%%   %%%        ",
"                  %%%%%%%%%%%%%%%        %%%%%%%%%%                    %%%%%%%%%%%%%%%    %         ",
"                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%            ",
"                %%%%%%%%%%%%%%%%%%%%%%%%%%  %%%% %%%                      %%%%%%%%%%  %%%@          ",
"                     %%%%%%%%%%%%%%%%%%% %%%%%% %%                          %%%%%%%%%%%%%@          ",
"                                                                                 %%%%%%%@           "
            };
            int n = (int)(sizeof(d) / sizeof(d[0]));
            for (int i = 0; i < n; i++) puts(d[i]);
            return BI_EXECUTED;
        }
        case BI_CMD_CD:
            // CD will be implemented in Part 2
            printf("cd not implemented yet!\n");
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// MAIN SHELL LOOP - YOU IMPLEMENT THIS
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 * 
 * YOU NEED TO IMPLEMENT THIS FUNCTION! This is your shell's main loop.
 * 
 * This function should:
 *   1. Loop forever (while(1))
 *   2. Print the shell prompt (SH_PROMPT)
 *   3. Read a line of input using fgets()
 *   4. Remove trailing newline
 *   5. Check for exit command - if found, print "exiting..." and break
 *   6. Parse the command line using build_cmd_list()
 *   7. Handle return codes:
 *      - WARN_NO_CMDS: print CMD_WARN_NO_CMD
 *      - ERR_TOO_MANY_COMMANDS: print CMD_ERR_PIPE_LIMIT with CMD_MAX
 *      - OK: print the parsed commands (see below)
 *   8. Free the command list using free_cmd_list()
 *   9. Loop back to step 2
 * 
 * Output format when commands are parsed successfully:
 * 
 *   First line:
 *     printf(CMD_OK_HEADER, clist.num);
 * 
 *   For each command:
 *     printf("<%d> %s", i+1, clist.commands[i].argv[0]);
 *     if (clist.commands[i].argc > 1) {
 *         printf(" [");
 *         for (int j = 1; j < clist.commands[i].argc; j++) {
 *             printf("%s", clist.commands[i].argv[j]);
 *             if (j < clist.commands[i].argc - 1) {
 *                 printf(" ");
 *             }
 *         }
 *         printf("]");
 *     }
 *     printf("\n");
 * 
 * Examples of expected output:
 * 
 *   dsh> cmd
 *   PARSED COMMAND LINE - TOTAL COMMANDS 1
 *   <1> cmd
 * 
 *   dsh> cmd arg1 arg2
 *   PARSED COMMAND LINE - TOTAL COMMANDS 1
 *   <1> cmd [arg1 arg2]
 * 
 *   dsh> cmd1 | cmd2 | cmd3
 *   PARSED COMMAND LINE - TOTAL COMMANDS 3
 *   <1> cmd1
 *   <2> cmd2
 *   <3> cmd3
 * 
 *   dsh> 
 *   warning: no commands provided
 * 
 *   dsh> c1|c2|c3|c4|c5|c6|c7|c8|c9
 *   error: piping limited to 8 commands
 * 
 *   dsh> exit
 *   exiting...
 * 
 * Starter code to help you get going:
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
 *       // Remove trailing newline
 *       cmd_line[strcspn(cmd_line, "\n")] = '\0';
 *       
 *       // Check for exit command
 *       // TODO: implement exit check
 *       
 *       // Parse the command line
 *       rc = build_cmd_list(cmd_line, &clist);
 *       
 *       // Handle return codes and print output
 *       // TODO: implement return code handling
 *       
 *       // Free memory
 *       if (rc == OK) {
 *           free_cmd_list(&clist);
 *       }
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
    int rc;

    while (1) {
        printf("%s", SH_PROMPT);

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        char *p = cmd_line;
        while (*p && isspace((unsigned char)*p)) p++;

        int plen = (int)strlen(p);
        while (plen > 0 && isspace((unsigned char)p[plen - 1])) {
            p[plen - 1] = '\0';
            plen--;
        }

        if (*p == '\0') {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        if (strcmp(p, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        rc = build_cmd_list(p, &clist);

        if (rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }
        if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }
        if (rc != OK) {
            continue;
        }

        if (clist.num == 1 && clist.commands[0].argc == 1 &&
            strcmp(clist.commands[0].argv[0], "dragon") == 0) {
            exec_built_in_cmd(&clist.commands[0]);
            free_cmd_list(&clist);
            continue;
        }

        printf(CMD_OK_HEADER, clist.num);

        for (int i = 0; i < clist.num; i++) {
            printf("<%d> %s", i + 1, clist.commands[i].argv[0]);

            if (clist.commands[i].argc > 1) {
                printf(" [");
                for (int j = 1; j < clist.commands[i].argc; j++) {
                    printf("%s", clist.commands[i].argv[j]);
                    if (j < clist.commands[i].argc - 1) printf(" ");
                }
                printf("]");
            }
            printf("\n");
        }

        free_cmd_list(&clist);
    }

    return OK;
}
//===================================================================
// EXECUTION FUNCTIONS - For future assignments
//===================================================================

/**
 * exec_cmd - Execute a single command (Part 2)
 * 
 * This will be implemented in Part 2 using fork/exec
 */
int exec_cmd(cmd_buff_t *cmd)
{
    printf("exec_cmd not implemented yet (Part 2)\n");
    return OK;
}

/**
 * execute_pipeline - Execute piped commands (Part 3)
 * 
 * This will be implemented in Part 3 using pipes
 */
int execute_pipeline(command_list_t *clist)
{
    printf("execute_pipeline not implemented yet (Part 3)\n");
    return OK;
}