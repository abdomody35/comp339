#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int write_all(int fd, const char *buf);

int main(int argc, char **argv)
{
    int fds[2];

    if (pipe(fds) == -1)
    {
        perror("pipe failed");
        return 1;
    }

    int pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        return 2;
    }

    if (!pid)
    {
        close(fds[0]);

        if (dup2(fds[1], 1) == -1)
        {
            perror("dup2 failed");
            close(fds[1]);
            return 3;
        }

        if (dup2(fds[1], 2) == -1)
        {
            perror("dup2 failed");
            close(fds[1]);
            return 3;
        }

        execlp("ls", "ls", NULL);

        perror("execlp failed");
        close(fds[1]);
        return 4;
    }

    close(fds[1]);

    wait(NULL);

    if (dup2(fds[0], 0) == -1)
    {
        perror("dup2 failed");
        close(fds[0]);
        return 3;
    }

    char buffer[1024];

    while (1)
    {
        char filename[255] = {};

        scanf("%s", filename);

        int length = strlen(filename);

        if (!length)
        {
            break;
        }

        if (!(filename[length - 2] == '.' && filename[length - 1] == 'c'))
        {
            continue;
        }

        int len = strlen(filename);
        char output[len];

        strcpy(output, filename);
        output[len - 2] = '\0';

        if (strstr(argv[0], output))
        {
            continue;
        }

        if (pipe(fds) == -1)
        {
            perror("pipe failed");
            return 1;
        }

        int pid = fork();

        if (pid == -1)
        {
            perror("fork failed");
            return 2;
        }

        if (!pid)
        {
            close(fds[0]);

            if (dup2(fds[1], 2) == -1)
            {
                perror("dup2 failed");
                close(fds[1]);
                return 3;
            }

            execlp("gcc", "gcc", "-o", output, filename, NULL);

            perror("Compiler failed");
            close(fds[1]);
            return 5;
        }

        close(fds[1]);

        if (dup2(fds[0], 0) == -1)
        {
            perror("dup2 failed");
            close(fds[0]);
            return 3;
        }

        int status;
        wait(&status);

        char log[len];

        strcpy(log, output);
        strcat(log, ".txt");

        int fd = creat(log, 0644);

        if (fd == -1)
        {
            perror("open failed");
            return 6;
        }

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status))
            {
                if (write_all(fd, "Compilation Failed.\n\nError :\n\n") == -1)
                {
                    perror("write failed");
                    close(fd);
                    return 7;
                }

                int size;
                char buffer[4096];

                while ((size = read(0, buffer, 4096)) > 0)
                {
                    if (write_all(fd, buffer) == -1)
                    {
                        perror("write failed");
                        close(fd);
                        return 7;
                    }
                }

                continue;
            }

            if (write_all(fd, "Compiled Successfully.\n\nOutput :\n\n") == -1)
            {
                perror("write failed");
                close(fd);
                return 7;
            }
        }

        pid = fork();

        if (pid == -1)
        {
            perror("fork failed");
            close(fd);
            return 2;
        }

        if (!pid)
        {
            if (dup2(fd, 1) == -1)
            {
                perror("dup2 failed");
                return 3;
            }

            if (dup2(fd, 2) == -1)
            {
                perror("dup2 failed");
                return 3;
            }

            close(fd);

            int argsCount = argc > 1 ? argc + 1 : 2;

            char *args[argsCount];

            args[0] = output;

            for (int i = 1; i < argc; i++)
            {
                args[i] = argv[i];
            }

            args[argsCount - 1] = NULL;

            execv(output, args);

            perror("execl failed");
            return 8;
        }

        wait(&status);

        if (!WIFEXITED(status))
        {
            if (write_all(fd, "Program Crashed.\n") == -1)
            {
                perror("write failed");
                close(fd);
                return 7;
            }
        }

        close(fd);
    }

    close(fds[0]);
    return 0;
}

int write_all(int fd, const char *buf)
{
    int count = strlen(buf), written;

    while ((written = write(fd, buf, count)) > 0)
    {
        count -= written;
        buf += written;
    }

    if (written == -1)
    {
        perror("write failed");
        return -1;
    }

    return 0;
}