#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAXCHILD 10
#define MAXCOUNTER 10

typedef enum {
    KP_ECHO_OFF,
    KP_ECHO_ON,
} kp_echo_t;

int keypress(const kp_echo_t echo) {
    struct termios savedState, newState;
    unsigned char echo_bit;  // flag
    int c;
    if (-1 == tcgetattr(STDIN_FILENO, &savedState)) {
        return EOF;
    };  // error
    newState = savedState;
    if (KP_ECHO_OFF == echo) {
        echo_bit = ECHO;
    } else {
        echo_bit = 0;
    };
    /* canonical input + set echo with minimal input as 1. */
    newState.c_lflag &= ~(echo_bit | ICANON);
    newState.c_cc[VMIN] = 1;
    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &newState)) {
        return EOF;
    };             // error
    c = getchar(); /* block until key press */
    if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &savedState)) {
        return EOF;
    };  // error
    return c;
}

FILE *ptr;
int child_num = 0;
pid_t children[MAXCHILD];

void createChild() {
    int isChild = fork();
    if (isChild == 0) {
        fprintf(ptr, "+%d\n", getpid());
        fflush(ptr);
        printf("[server] +%d\n", getpid());
        fflush(stdout);
        while (1)
            ;
    } else {
        children[child_num] = isChild;
        child_num++;
    }
}

void deleteChild() {
    if (child_num - 1 >= 0) {
        kill(children[child_num - 1], SIGKILL);
        fprintf(ptr, "-%d\n", children[child_num - 1]);
        fflush(ptr);
        printf("[server] -%d\n", children[child_num - 1]);
        fflush(stdout);
        child_num--;
    } else {
        fprintf(ptr, "0\n");
        fflush(ptr);
        printf("[server] 0\n");
        fflush(stdout);
    }
}

void handler(int signo, siginfo_t *info, void *empty) {
    // printf("Received signal from%d\n", info->si_pid);
    if (signo == SIGUSR1) {
        createChild();
    } else if (signo == SIGUSR2) {
        deleteChild();
    } else if (signo == SIGINT) {
        fprintf(ptr, "%d\n", child_num);
        fclose(ptr);
        exit(0);
    }
}

int main(int argc, void **argv) {
    if (argc != 3) {
        fprintf(stderr, "?ERROR, wrong number of paramenters\n");
        exit(1);
    }

    if (strcmp(argv[1], "server") == 0) {
        ptr = fopen(argv[2], "r");
        if (ptr == NULL) {
            fprintf(stderr, "?ERROR, path doesn't exists\n");
            exit(2);
        }
        fclose(ptr);

        struct sigaction sa;
        sa.sa_sigaction = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGUSR2, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);

        ptr = fopen(argv[2], "w");
        fprintf(ptr, "%d\n", getpid());
        fflush(ptr);
        printf("[server:%d]\n", getpid());

        while (1)
            ;

    } else if (strcmp(argv[1], "client") == 0) {
        while (ptr == NULL) {
            ptr = fopen(argv[2], "r");
        }
        pid_t server_pid;
        fscanf(ptr, "%d", &server_pid);

        printf("[client] server: %d\n", server_pid);

        char c;
        int counter = 0;
        while (1) {
            c = keypress(KP_ECHO_OFF);
            if (c == '+') {
                if (counter < MAXCOUNTER) {
                    //kill(server_pid, SIGUSR1);
                    counter++;
                }
            } else if (c == '-') {
                if (counter > 0) {
                    //kill(server_pid, SIGUSR2);
                    counter--;
                }
            } else if (c == '\n') {
                while (counter != 0) {
                    //kill(server_pid, SIGUSR2);
                    counter--;
                    printf("[client] %d\n", counter);
                    sleep(1);
                }
                break;
            }

            printf("[client] %d\n", counter);
        }
    } else {
        fprintf(stderr, "?ERROR, wrong first paramenter\n");
        exit(3);
    }

    return 0;
}