#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

int isNumber(char number[]) {
    int i = 0;
    for (; number[i] != 0; i++) {
        if (!isdigit(number[i])) return FALSE;
    }
    return TRUE;
}

int main(int argc, void **argv) {
    if (argc != 3) {
        fprintf(stderr, "?ERROR, numero sbagliato di parametri in entrata\n");
        exit(1);
    }

    int n;

    if (isNumber(argv[2])) {
        n = atoi(argv[2]);
    } else {
        fprintf(stderr, "?ERROR, il secondo input deve essere un numero\n");
        exit(3);
    }

    if (n < 0 || n > 10) {
        fprintf(stderr,
                "?ERROR, il secondo input deve essere un numero compreso tra 0 "
                "e 10\n");
        exit(3);
    }

    mkfifo(argv[1], 0666);
    int fd = open(argv[1], O_RDONLY);

    char buffer[10];
    int i = 0;

    int r = read(fd, buffer, n);

    for (int i = 0; i < n; i++) {
        printf("%c\n", buffer[i]);
    }

    if (r < n) {
        return 1;
    }
    return 0;
}