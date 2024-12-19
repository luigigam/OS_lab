#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

#define READ 0
#define WRITE 1

#define MAXDIM 255

extern int errno;
FILE *ptr;
char filename[255];
int n, queue, pipe_lavoratori[10][2];
pid_t lavoratori[10];

struct Msg_packet {
    long mtype;
    char mtext[MAXDIM];
} msg;

int isNumber(char number[]) {
    int i = 0;
    for (; number[i] != 0; i++) {
        if (!isdigit(number[i])) return FALSE;
    }
    return TRUE;
}

void handler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGUSR1) {
        printf("Received signal from%d, sending it back just this time\n",
               info->si_pid);
        fflush(stdout);
        kill(info->si_pid, SIGUSR1);
    } else if (signo == SIGUSR2) {
        printf("sending SIGUSR2 back to %d\n", info->si_pid);
        fflush(stdout);
        kill(info->si_pid, SIGUSR2);
    }
}

void terminateLavoratori() {
    for (int i = 0; i < n; i++) {
        close(pipe_lavoratori[i][WRITE]);
        if (!(errno == ERANGE || kill(lavoratori[i], 0) == -1)) {
            printf("terminating %d\n", lavoratori[i]);
            fflush(stdout);
            kill(lavoratori[i], SIGKILL);
        }
    }
    printf("terminating PostOffice\n");
    exit(0);
}

void writeQueue() {
    int count = 0;
    while (!feof(ptr)) {
        char toSend[MAXDIM];
        fscanf(ptr, "%s", toSend);
        write(pipe_lavoratori[count++ % n][WRITE], toSend, strlen(toSend) + 1);
        sleep(1);
    }
    printf("File's over, terminating everything\n");
    fflush(stdout);
    for (int i = 0; i < n; i++) {
        write(pipe_lavoratori[i][WRITE], "0", 2);
    }
    terminateLavoratori();
}

void sigwinch_handler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGWINCH) {
        key_t keyQueue = ftok(filename, getpid());
        queue = msgget(keyQueue, 0777 | IPC_CREAT);
        writeQueue();
        fclose(ptr);
    }
}

void creaLavoratore(pid_t target) {
    for (int i = 0; i < n; i++) {
        pipe(pipe_lavoratori[i]);
        int isLavoratore = fork();
        if (isLavoratore == 0) {
            close(pipe_lavoratori[i][WRITE]);

            struct sigaction sa1, sa2;

            sa1.sa_sigaction = handler;
            sigemptyset(&sa1.sa_mask);
            sa1.sa_flags = SA_RESETHAND | SA_SIGINFO;
            sigaction(SIGUSR1, &sa1, NULL);

            sa2.sa_sigaction = handler;
            sigemptyset(&sa2.sa_mask);
            sa2.sa_flags = SA_SIGINFO;
            sigaction(SIGUSR2, &sa2, NULL);

            printf("I'm %d, sending SIGTERM to:%d\n", getpid(), target);
            fflush(stdout);
            kill(target, SIGTERM);
            char buf[MAXDIM];
            while (1) {
                int r = read(pipe_lavoratori[i][READ], buf, MAXDIM);
                if (r > 0) {
                    if (strcmp(buf, "0") != 0) {
                        strncpy(msg.mtext, buf, sizeof(msg.mtext));
                        msg.mtype = getpid();
                        printf("I'm %d, Writing to queue %s\n", getpid(),
                               msg.mtext);
                        fflush(stdout);
                        int sending = msgsnd(queue, &msg, sizeof(msg.mtext), 0);
                    } else {
                        close(pipe_lavoratori[i][WRITE]);
                    }
                }
            }

        } else {
            close(pipe_lavoratori[i][READ]);
            lavoratori[i] = isLavoratore;
        }
    }
}

int main(int argc, void **argv) {
    if (argc != 4) {
        fprintf(stderr, "?ERROR, wrong number of parameters\n");
        exit(1);
    }

    if (isNumber(argv[1])) {
        n = atoi(argv[1]);
        if (n < 1 || n > 10) {
            fprintf(stderr, "?ERROR, n is not between 1 and 10\n");
            exit(2);
        }
    } else {
        fprintf(stderr, "?ERROR, n is not a number\n");
        exit(2);
    }

    ptr = fopen(argv[2], "r");
    if (ptr == NULL) {
        fprintf(stderr, "?ERROR, file does not exist\n");
        exit(3);
    }
    strcpy(filename, argv[2]);

    pid_t pidInput = strtol(argv[3], NULL, 10);

    if (errno == ERANGE || kill(pidInput, 0) == -1) {
        fprintf(stderr, "PID non valido\n");
        exit(4);
    }

    printf("PostOffice %d\n", getpid());
    fflush(stdout);

    creaLavoratore(pidInput);

    struct sigaction sa_sigwinch;
    sa_sigwinch.sa_sigaction = sigwinch_handler;
    sigemptyset(&sa_sigwinch.sa_mask);
    sa_sigwinch.sa_flags = SA_SIGINFO;
    sigaction(SIGWINCH, &sa_sigwinch, NULL);

    while (1)
        ;

    return 0;
}