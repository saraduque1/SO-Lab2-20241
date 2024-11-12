#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATH 1024
#define MAX_ARGS 20

char error_message[30] = "An error has occurred\n";
char *path = NULL;

void print_error()
{
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *find_command(const char *command)
{
    if (path == NULL)
        return NULL;

    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");
    char *full_path = malloc(MAX_PATH);

    while (dir != NULL)
    {
        snprintf(full_path, MAX_PATH, "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0)
        {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    free(full_path);
    return NULL;
}

void cd_command(char **args)
{
    if (args[1] == NULL || args[2] != NULL)
    {
        print_error();
        return;
    }

    if (chdir(args[1]) != 0)
    {
        print_error();
    }
}

void path_command(char **args)
{
    free(path);
    path = NULL;

    int i = 1;
    size_t total_len = 0;
    while (args[i] != NULL)
    {
        total_len += strlen(args[i]) + 1;
        i++;
    }

    if (total_len > 0)
    {
        path = malloc(total_len);
        path[0] = '\0';

        i = 1;
        while (args[i] != NULL)
        {
            if (i > 1)
                strcat(path, ":");
            strcat(path, args[i]);
            i++;
        }
    }
}

void echo_command(char **args, char *output_file)
{
    int stdout_copy = -1;
    FILE *file = NULL;

    if (output_file != NULL)
    {
        stdout_copy = dup(STDOUT_FILENO);
        file = fopen(output_file, "w");
        if (file == NULL)
        {
            print_error();
            return;
        }
        if (dup2(fileno(file), STDOUT_FILENO) == -1)
        {
            print_error();
            fclose(file);
            return;
        }
    }

    for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
            printf(" ");
    }
    printf("\n");

    if (output_file != NULL)
    {
        fflush(stdout);
        if (dup2(stdout_copy, STDOUT_FILENO) == -1)
        {
            print_error();
        }
        close(stdout_copy);
        fclose(file);
    }
}

void cat_command(char **args)
{
    if (args[1] == NULL)
    {
        print_error();
        return;
    }

    for (int i = 1; args[i] != NULL; i++)
    {
        FILE *file = fopen(args[i], "r");
        if (file == NULL)
        {
            print_error();
            continue;
        }

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL)
        {
            printf("%s", buffer);
        }

        fclose(file);
    }
}

void execute_commands(char *command)
{
    char *commands[MAX_ARGS];
    int command_count = 0;
    char *cmd = command;

    while ((cmd = strsep(&command, "&")) != NULL && command_count < MAX_ARGS)
    {
        commands[command_count++] = cmd;
    }

    pid_t pids[MAX_ARGS];
    for (int i = 0; i < command_count; i++)
    {
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *output_file = NULL;
        char *token;

        while ((token = strsep(&commands[i], " \t\n")) != NULL && arg_count < MAX_ARGS - 1)
        {
            if (strcmp(token, ">") == 0)
            {
                token = strsep(&commands[i], " \t\n");
                if (token != NULL)
                {
                    output_file = token;
                    break;
                }
                else
                {
                    print_error();
                    return;
                }
            }
            if (*token != '\0')
            {
                args[arg_count++] = token;
            }
        }
        args[arg_count] = NULL;

        if (arg_count == 0)
            continue;

        if (strcmp(args[0], "exit") == 0)
        {
            if (arg_count > 1)
            {
                print_error();
                continue;
            }
            exit(0);
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            cd_command(args);
            continue;
        }
        else if (strcmp(args[0], "path") == 0)
        {
            path_command(args);
            continue;
        }
        else if (strcmp(args[0], "echo") == 0)
        {
            echo_command(args, output_file);
            continue;
        }
        else if (strcmp(args[0], "cat") == 0)
        {
            cat_command(args);
            continue;
        }

        char *cmd_path = find_command(args[0]);

        if (cmd_path == NULL)
        {
            print_error();
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            if (output_file != NULL)
            {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1)
                {
                    print_error();
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            execv(cmd_path, args);
            print_error();
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            pids[i] = pid;
        }
        else
        {
            print_error();
        }

        free(cmd_path);
    }

    for (int i = 0; i < command_count; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
    }
}

int main(int argc, char *argv[])
{
    path = strdup("/bin:/usr/bin");

    if (argc > 2)
    {
        print_error();
        exit(1);
    }

    if (argc == 2)
    {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL)
        {
            print_error();
            exit(1);
        }

        char *command = NULL;
        size_t len = 0;

        while (getline(&command, &len, file) != -1)
        {
            execute_commands(command);
        }

        free(command);
        fclose(file);
        free(path);
        exit(0);
    }

    char *command = NULL;
    size_t len = 0;
    while (1)
    {
        printf("wish> ");
        if (getline(&command, &len, stdin) == -1)
        {
            free(path);
            exit(0);
        }
        execute_commands(command);
    }

    free(command);
    free(path);
    exit(0);
}