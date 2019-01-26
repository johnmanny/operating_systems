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

#include "QheaderScen3.h"

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];
    char toSend[1024];
    strcpy(toSend, msg);

    while (1) {
        write(writeEnd, toSend, strlen(toSend) + 1);
        read(readEnd, buff, 1023);

        fprintf(stdout, "\n---[SUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strstr(buff, "topic") != NULL) {

            // take entry and output it
            char entry[QUACKSIZE+1];
            int topic;
            sscanf(buff, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topic, entry);

            // send success message back
            strcpy(toSend, "successful");
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {
            exit(0);	// exit if send termination signal
        }
        else {
            fprintf(stdout, "SUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());
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

    char msgString[1024];
    int subid = (int) getpid();
    sprintf(msgString, "sub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);    
    sendMessage("end" , readEnd, writeEnd);   

    int i;
    // subscribe to all topics
    for (i = 0; i < MAXTOPICS; i++) {
        sprintf(msgString, "sub %d topic %d", subid, i);
        sendMessage(msgString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
     }

    sendMessage("read", readEnd, writeEnd);
    sendMessage("end", readEnd, writeEnd);
    sendMessage("terminate", readEnd, writeEnd);


    return 0;
}
