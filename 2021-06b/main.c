#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_MAXLEN 60
#define CHILDREN_MAX 10
#define PIDLEN 8

#define TRUE 1
#define FALSE 0

#define READ 0
#define WRITE 1

pid_t parent;
FILE* ptr;
int logfile;

int queue;

typedef struct msg {
    long mtype;
    char mtext[100];
} Msg_packet;

// verifica se la stringa in unput è un numero
int isNumber(char number[]) {
    int i = 0;
    for (; number[i] != 0; i++) {
        if (!isdigit(number[i])) return FALSE;
    }
    return TRUE;
}

void myHandler(int sigNum) {
    if (sigNum == SIGUSR1) {
        write(logfile, "SIGUSR1\n", 8);
    } else if (sigNum == SIGUSR2) {
        Msg_packet msg_sig;
        char toSend_sig[PIDLEN];
        sprintf(toSend_sig, "%d", getpid());
        strcpy(msg_sig.mtext, toSend_sig);
        msg_sig.mtype = 1;
        msgsnd(queue, &msg_sig, sizeof(msg_sig.mtext), 0);
    }
}

int main(int argc, void** argv) {
    if (argc != 3) {
        fprintf(stderr, "!ERROR: wrong number of parameters\n");
        exit(1);
    }

    char path[PATH_MAXLEN];
    strcpy(path, argv[1]);
    int n;

    if (isNumber(argv[2])) {
        n = atoi(argv[2]);
    } else {
        fprintf(stderr, "!ERROR: second parameter must be a number\n");
        exit(2);
    }

    if (n > 10 || n < 1) {
        fprintf(stderr, "!ERROR: n must be between 1 and 10\n");
        exit(3);
    }

    struct stat st;
    if (stat(argv[1], &st)) {
        fprintf(stderr, "!ERROR: path non existent\n");
        exit(4);
    }

    strcat(path, "/info/\0");
    char pathInfo[PATH_MAXLEN];
    strcat(pathInfo, path);

    if (mkdir(pathInfo, 0755)) {
        fprintf(stderr,
                "an ERROR occured during the creation of folder /info/\n");
        exit(5);
    }

    strcat(path, "key.txt");
    ptr = fopen(path, "w+");
    chmod(path, 0755);
    parent = getpid();
    fprintf(ptr, "%d\n", getpid());
    printf("%d\n", getpid());

    char logName[PATH_MAXLEN];
    char logPath[PATH_MAXLEN];
    key_t keyQueue;

    keyQueue = ftok(path, 32);
    queue = msgget(keyQueue, IPC_CREAT);
    msgctl(queue, IPC_RMID, NULL);
    queue = msgget(keyQueue, IPC_CREAT);
    Msg_packet msg;
    char toSend[PIDLEN];
    sprintf(toSend, "%d", getpid());
    strcpy(msg.mtext, toSend);
    msg.mtype = 1;
    msgsnd(queue, &msg, sizeof(msg.mtext), 0);

    for (int i = 0; i < n; i++) {
        int isChild = !fork();
        if (isChild) {
            sprintf(logName, "%d", getpid());
            strcat(logName, ".txt");
            strcat(pathInfo, logName);
            printf("%d ", getpid());
            fflush(stdout);
            logfile = open(pathInfo, O_CREAT | O_RDWR);
            chmod(pathInfo, 0755);

            struct sigaction sa;
            sigemptyset(&sa.sa_mask);
            sa.sa_handler = myHandler;
            sigaction(SIGUSR1, &sa, NULL);
            sigaction(SIGUSR2, &sa, NULL);

            while (1);

            break;
        } else {
            sleep(1);
        }
    }

    printf("\n");

    return 0;
}