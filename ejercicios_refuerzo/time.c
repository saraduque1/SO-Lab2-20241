#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Si el argumento es menor a 2 caracteres, entonces se imprime el error
    if (argc < 2 || (strlen(argv[1]) < 2)) {
        printf("Incorrect execution\n");
        fprintf(stderr, "Please, follow this structure: %s <command> [argument]\n", argv[0]);
        exit(1);
    }
    
    struct timeval start, end;
    pid_t pid;

    gettimeofday(&start, NULL);

    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Proceso hijo
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else {
        // Proceso padre
        int status;
        wait(&status);

        gettimeofday(&end, NULL);

        double elapsed = (end.tv_sec - start.tv_sec) + 
                         (end.tv_usec - start.tv_usec) / 1000000.0;

        printf("\nElapsed time: %.5f seconds\n", elapsed);
    }

    return 0;
}