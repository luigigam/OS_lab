#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXDIM 32

pid_t parent;
pid_t pid_in;
int id_coda;
int esito;

struct Msg_packet {
    long mtype;
    char mtext[100];
} msgSnd, msgRcv;

int main(int argc, void **argv) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "!ERROR: wrong numbers of parameters\n");
        exit(1);
    }

    char nome[MAXDIM];
    char valore[MAXDIM];
    char azione;
    pid_in = atoi(argv[argc - 1]);
    strcat(nome, argv[1]);

    creat(argv[1], 0777);
    key_t keyQueue;
    keyQueue = ftok(argv[1], 1);
    if (id_coda == -1) {
        esito = 0;
    } else {
        esito = 1;
    }
    if (strcmp(argv[2], "new") == 0) {
        // sleep(1);
        id_coda = msgget(keyQueue, 0777 | IPC_CREAT);
        printf("QUEUE creata con ID %d\n", id_coda);
    } else if (strcmp(argv[2], "put") == 0) {
        id_coda = msgget(keyQueue, 0777 | IPC_CREAT);
        msgSnd.mtype = 1;
        strcpy(msgSnd.mtext, argv[3]);
        msgsnd(id_coda, &msgSnd, sizeof(msgSnd.mtext), 0);
    } else if (strcmp(argv[2], "get") == 0) {
        id_coda = msgget(keyQueue, 0777 | IPC_CREAT);
        msgrcv(id_coda, &msgRcv, sizeof(msgRcv.mtext), 1, IPC_NOWAIT);
        printf("%s\n", msgRcv.mtext);
        esito = 1;
    } else if (strcmp(argv[2], "del") == 0) {
        if (msgctl(id_coda, IPC_RMID, NULL) == -1) {
            esito = 0;
        }
    } else if (strcmp(argv[2], "emp") == 0) {
        esito = 1;
        id_coda = msgget(keyQueue, 0777 | IPC_CREAT);
        while (msgrcv(id_coda, &msgRcv, sizeof(msgRcv.mtext), 1, IPC_NOWAIT) !=
               -1) {
            printf("%s\n", msgRcv.mtext);
        }
    } else if (strcmp(argv[2], "mov") == 0) {
        esito = 1;
        id_coda = msgget(keyQueue, 0777 | IPC_CREAT);
        if (id_coda == -1) {
            esito = 0;
        } else {
            int count = 0;
            key_t keyQueue2 = ftok(argv[3], 1);
            int id_coda2 = msgget(keyQueue2, 0777 | IPC_CREAT);
            while (msgrcv(id_coda, &msgRcv, sizeof(msgRcv.mtext), 1,
                          IPC_NOWAIT) != -1) {
                printf("%s\n", msgRcv.mtext);
                msgRcv.mtype = 1;
                msgsnd(id_coda2, &msgRcv, sizeof(msgRcv.mtext), 0);
                count++;
            }
            printf("Number of message recived: %d\n", count);
            msgctl(id_coda, IPC_RMID, NULL);
        }
    } else {
        printf("? Wrong command, try again\n");
    }

    if (esito == 1) {
        kill(pid_in, SIGUSR1);
    } else {
        kill(pid_in, SIGUSR2);
        exit(2);
    }

    return 0;
}