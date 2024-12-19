#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXDIM 100

FILE *ptr;
int queue;

struct Msg_packet {
    char mtext[MAXDIM];
    long mtype;
} msg;

void handler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGUSR1) {
        printf("Signal received from %d\nSending signal SIGUSR1 back\n",
               info->si_pid);
        kill(info->si_pid, SIGUSR1);
        sleep(3);
        fprintf(ptr, "%d-%d\n", info->si_pid, SIGUSR1);
    } else if (signo == SIGUSR2) {
        int isChild;
        isChild = fork();
        if (isChild == 0) {
            printf("Sending SIGUSR2 to %d from child pid %d\n", info->si_pid,
                   getpid());
            kill(info->si_pid, SIGUSR2);
            sleep(3);
            fprintf(ptr, "%d-%d\n", info->si_pid, SIGUSR2);
        } else {
            while (wait(NULL) > 0)
                ;
        }
    } else if (signo == SIGINT) {
        fprintf(ptr, "stop\n");
        fclose(ptr);
        exit(0);
    }
}

void *queueCheck(void *param) {
    while (1) {
        while (msgrcv(queue, &msg, sizeof(msg.mtext), 0, 0) == -1) {
            kill(atoi(msg.mtext), SIGALRM);
        }
    }
}

int main(int argc, void **argv) {
    if (argc != 2) {
        fprintf(stderr, "!ERROR, wrong number of parameters\n");
        exit(1);
    }

    printf("PID: %d\n", getpid());

    struct sigaction sa;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    ptr = fopen("logFile", "a");

    creat("/tmp/queue", 0777);
    key_t keyQueue = ftok(argv[1], 1);
    queue = msgget(keyQueue, 077 | IPC_CREAT);

    pthread_t t_id;
    pthread_create(&t_id, NULL, queueCheck, NULL);

    while (1) {
        pause();
    }
}