/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description: sub file for scenario 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "QheaderScen4.h"

// globals used for scenario specs
int entsToRead;
int secsToWait;

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
            sscanf(buff, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topic, entry);
            entriesRead++;
            // send success message back
            strcpy(toSend, "successful");

            if ((entriesRead % entsToRead) == 0) {
                sleep(secsToWait);
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

    if(argc != 4) {
        fprintf(stderr, "SUBSCRIBER error, bad args");
        exit(0);
    }
    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);
    int setNum = atoi(argv[3]);
    int subid = (int) getpid();
    char msgString[1024];


    // get number of entries to read and number of seconds to wait from header   
    if (setNum == 1) {
        entsToRead = 1;
        secsToWait = 0;
    }
    else if (setNum == 2) {
        entsToRead = 10;
        secsToWait = 5;
    }
    else {
        entsToRead = 50;
        secsToWait = 10;
    }

    fprintf(stderr, "\n---[SUB PID %d] IS SET TO %d secs to wait, %d entires to read\n", getpid(), secsToWait, entsToRead);
    fprintf(stdout, "\n---[SUB PID %d] IS SET TO %d secs to wait, %d entires to read\n", getpid(), secsToWait, entsToRead);


    sprintf(msgString, "sub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);    
    sendMessage("end" , readEnd, writeEnd);   

    srand((uintptr_t) &writeEnd);

    // subscribe to only 1 (random) topic
    sprintf(msgString, "sub %d topic %d", subid, (rand() % MAXTOPICS));
    sendMessage(msgString, readEnd, writeEnd);
    sendMessage("end", readEnd, writeEnd);


    sendMessage("read", readEnd, writeEnd);
    sendMessage("end", readEnd, writeEnd);
    sendMessage("terminate", readEnd, writeEnd);


    return 0;
}
