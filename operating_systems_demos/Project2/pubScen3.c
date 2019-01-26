/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description: scenario 3 pub file
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

    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);
        read(readEnd, buff, 1023);

        fprintf(stdout, "\n---[PUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "retry") == 0) {
            sleep(1);	// to avoid output spam
            continue;
        }
        else if (strcmp(buff, "successful") == 0) {
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {
            return 0;	// sends false when given 'signal' to terminate
        }
        else {
            fprintf(stdout, "PUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());
            break;
        }
    }
    return 1;
}

int main(int argc, char * argv[]){

    if(argc != 3) {
        fprintf(stderr, "PUBLISHER error, bad args");
        exit(0);
    }

    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);
    char connectString[1024];
    char topicString[1024];
    char msg[QUACKSIZE + 1];
    char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };	// allowed chars for message

    // generate connect message
    sprintf(connectString, "pub %d connect", (int)getpid());
    sendMessage(connectString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

    // use randomish seed
    srand((uintptr_t) &writeEnd);

    int i, j, x, v;

    // repeat 10 times
    for (j = 0; j < 10; j++) {

            // for each topic
	    for (i = 0; i < MAXTOPICS; i++) {

                // post 50 entries
                for (x = 0; x < 50; x++) {
			// generate chars to put in message to send of size QUACKSIZE
			for (v = 0; v < QUACKSIZE; v++) {
			    // want to include the null pointer to allow for string to terminate at any point
			    msg[v] = toUse[rand() % 28];
			}
			msg[QUACKSIZE] = '\0';		// just in case they're all not the null ptr

                        sprintf(topicString, "topic %d %s", (i + x) % MAXTOPICS, msg);	// add changing offset to round-robin
			sendMessage(topicString, readEnd, writeEnd);
			sendMessage("end", readEnd, writeEnd);
                }
	    }
            // 50 posts per each topic is posted every second
            sleep(1);
    }

    sendMessage("terminate", readEnd, writeEnd);
       

    return 0;
}
