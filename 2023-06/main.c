#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXDIM 255

struct Msg_packet {
    long mtype;
    char mtext[MAXDIM];
} msg;

typedef struct {
    pid_t target;
    int signo;
    int canSend;
} PayloadKill;

typedef struct {
    char *key_param;
    struct Msg_packet msg;
    int canSend;
} PayloadQueue;

typedef struct {
    char fifoname[MAXDIM];
    char fifomsg[MAXDIM];
    int canSend;
} PayloadFifo;

FILE *ptr;
int queue, fifo;
int nextCommad = 0;

void *threadKill(void *param) {
    PayloadKill *payload = (PayloadKill *)param;
    while (1) {
        if (payload->canSend == 1) {
            kill(payload->target, payload->signo);
            payload->canSend = 0;
            sleep(1);
        }
    }
}
void *threadQueue(void *param) {
    PayloadQueue *payload = (PayloadQueue *)param;

    key_t keyQueue = ftok(payload->key_param, 1);
    queue = msgget(keyQueue, 0777 | IPC_CREAT);

    while (1) {
        if (payload->canSend == 1) {
            msgsnd(queue, &payload->msg, sizeof(payload->msg.mtext), 0);
            payload->canSend = 0;
            sleep(1);
        }
    }
}
void *threadFifo(void *param) {
    PayloadFifo *payload = (PayloadFifo *)param;
    while (1) {
        if (payload->canSend == 1) {
            int fifo_create = mkfifo(payload->fifoname, 0666);
            fifo = open(payload->fifoname, O_WRONLY);
            write(fifo, payload->fifomsg, strlen(payload->fifomsg));
            close(fifo);
            payload->canSend = 0;
            sleep(1);
        }
    }
}

void handler(int signo, siginfo_t *info, void *empty) {
    if (signo == SIGUSR1) {
        nextCommad = 1;
    }
}

int main(int argc, void **argv) {
    if (argc != 2) {
        fprintf(stderr, "?ERROR, wrong number of parameters\n");
        exit(1);
    }

    ptr = fopen(argv[1], "r");
    char buf[3][MAXDIM];

    PayloadKill argK = {0, 0, 0};
    PayloadQueue argQ = {argv[1], {-1, ""}, 0};
    PayloadFifo argF = {"", "", 0};

    pthread_t th_k, th_q, th_f;
    pthread_create(&th_k, NULL, threadKill, (void *)&argK);
    pthread_create(&th_q, NULL, threadQueue, (void *)&argQ);
    pthread_create(&th_f, NULL, threadFifo, (void *)&argF);

    struct sigaction sa;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);

    while (!feof(ptr)) {
        if (nextCommad == 1) {
            fscanf(ptr, "%s %s %s", buf[0], buf[1], buf[2]);
            if (strcmp(buf[0], "kill") == 0) {
                argK.signo = atoi(buf[1]);
                argK.target = atoi(buf[2]);
                argK.canSend = 1;
                // kill(atoi(buf[2]), atoi(buf[1]));
            } else if (strcmp(buf[0], "queue") == 0) {
                argQ.msg.mtype = atoi(buf[1]);
                strncpy(argQ.msg.mtext, buf[2], sizeof(argQ.msg.mtext));
                // key_t keyQueue = ftok(argv[1], 1);
                // queue = msgget(keyQueue, 0777 | IPC_CREAT);
                // msg.mtype = atoi(buf[1]);
                // strcpy(msg.mtext, buf[2]);
                // msgsnd(queue, &msg, sizeof(msg.mtext), 0);
                argQ.canSend = 1;
            } else if (strcmp(buf[0], "fifo") == 0) {
                strncpy(argF.fifoname, buf[1], sizeof(argF.fifoname));
                strncpy(argF.fifomsg, buf[2], sizeof(argF.fifomsg));
                argF.canSend = 1;
                // mkfifo(buf[1], 0666);
                // write(fifo, buf[2], strlen(buf[2]));
                // close(fifo);
            } else {
                fprintf(stderr, "?ERROR, unrecognise command: %s\n", buf[0]);
                exit(2);
            }
            nextCommad = 0;
        }
    }

    return 0;
}