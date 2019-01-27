/*
    Author: John Nemeth
    Sources: class material, given lab examples
    Description: sub file for scenario 5
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "QheaderScen5.h"

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];
    char toSend[1024];
    char entry[QUACKSIZE+1];
    int entriesRead = 0;
    int topic;
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
            sscanf(buff, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topic, entry);
            entriesRead++;

            // send success message back
            strcpy(toSend, "successful");
            if ((entriesRead % 30) == 0) {	// read 30 topics every second
                sleep(1);
            }
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
    int subid = (int) getpid();
    int i;

    char msgString[1024];

    sprintf(msgString, "sub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);    
    sendMessage("end" , readEnd, writeEnd);   

    // subscribe to all topics
    for (i = 0; i < MAXTOPICS; i++) {
        sprintf(msgString, "sub %d topic %d", subid, i);
        sendMessage(msgString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
     }

    // before sleeping, sub connected and notified of interested topics sleep for 15 seconds
    sleep(15);

    sendMessage("read", readEnd, writeEnd);
    sendMessage("end", readEnd, writeEnd);
    sendMessage("terminate", readEnd, writeEnd);


    return 0;
}
