/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description:
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];

    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);
        read(readEnd, buff, 1023);

        fprintf(stderr, "SUB %d RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {
            return 0;	// sends false when given 'signal' to terminate
        }
        else {
            fprintf(stderr, "SUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());
            break;
        }
    }
    return 1;
}

int main(int argc, char * argv[]){

    if(argc != 3) {
        fprintf(stderr, "SUBSCRIBER error, bad args");
        exit(0);
    }
    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);

    srand((uintptr_t) &readEnd);
    int fakeRandom = rand() % 100;
    char fakeNum[20];
    sprintf(fakeNum, "%d", fakeRandom);
    char connectString[1024];
    strcpy(connectString, "sub ");
    strcat(connectString, fakeNum);
    strcat(connectString, " connect");
    sendMessage(connectString, readEnd, writeEnd);    


    sendMessage("end" , readEnd, writeEnd);   
    sendMessage("terminate", readEnd, writeEnd);

    return 0;
}
