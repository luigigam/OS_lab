#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

FILE *ptr;

pid_t leaves[10];
int leafTerm = 0;
int n;

int isNumber(char number[]) {
    int i = 0;
    for (; number[i] != 0; i++) {
        if (!isdigit(number[i])) return FALSE;
    }
    return TRUE;
}

void leafHandler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGUSR1) {
        kill(info->si_value.sival_int, SIGUSR2);
        printf("Sent SIGUSR2 to %d\n", info->si_value.sival_int);
        fflush(stdout);
    }
}

void managerHandler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGUSR1) {
        if (leafTerm < n) {
            union sigval value;
            value.sival_int = info->si_pid;
            sigqueue(leaves[leafTerm], SIGUSR1, value);
            printf("Received SIGUSR1 from %d, redirecting to Leaf %d\n",
                   info->si_pid, leaves[leafTerm]);
            fflush(stdout);
            sleep(1);
            printf("Terminating leave:%d\n", leaves[leafTerm]);
            fflush(stdout);
            kill(leaves[leafTerm], SIGKILL);
            leafTerm++;
        } else {
            printf("Terminating Manager");
            fflush(stdout);
            exit(0);
        }
    } else if (signo == SIGTERM) {
        printf("Terminating %d leaves\n", n - leafTerm);
        fflush(stdout);
        while (leafTerm < n) {
            printf("Terminating %d..\n", leaves[leafTerm]);
            fflush(stdout);
            kill(leaves[leafTerm], SIGKILL);
            leafTerm++;
        }
        printf("Terminating Manager\n");
        fflush(stdout);
        exit(0);
    }
}

void createLeaves(int n) {
    for (int i = 0; i < n; i++) {
        int isLeaf = fork();
        if (isLeaf == 0) {
            struct sigaction sa;
            sa.sa_sigaction = leafHandler;
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGCHLD);
            sigaddset(&sa.sa_mask, SIGCONT);
            sa.sa_flags = SA_SIGINFO;
            sigaction(SIGUSR1, &sa, NULL);
            fprintf(ptr, "%d\n", getpid());
            fflush(ptr);
            while (1)
                ;
        } else {
            leaves[i] = isLeaf;
        }
    }
}

int main(int argc, void **argv) {
    if (argc != 3) {
        fprintf(stderr, "?ERROR, wrong number of arguments\n");
        exit(3);
    }

    if (isNumber(argv[2])) {
        n = atoi(argv[2]);
        if (n < 1 || n > 10) {
            fprintf(stderr, "?ERROR, n is not between 1 and 10\n");
            exit(4);
        }
    } else {
        fprintf(stderr, "?ERROR, n is not a number\n");
        exit(4);
    }

    ptr = fopen(argv[1], "r");
    if (ptr != NULL) {
        fprintf(stderr, "?ERROR, path exists already\n");
        fclose(ptr);
        exit(5);
    }

    ptr = fopen(argv[1], "w");

    fprintf(ptr, "%d*\n", getpid());
    fflush(ptr);

    int isManager = fork();
    if (isManager != 0) {
        fprintf(ptr, "%d\n", isManager);
        fflush(ptr);
    } else {
        struct sigaction sa;
        sa.sa_sigaction = managerHandler;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGALRM);
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        createLeaves(n);
        while (1)
            ;
    }

    return 0;
}