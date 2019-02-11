/*
    Author: John Nemeth 
    Description: pub file for scenario 5
    Sources: online linux manuals
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "Qheader.h"

/*  actual function which sends publisher message to server using linux pipes
		msg: actual message to be sent
		readEnd: the end of a linux pipe for publisher to read message from server
		writeEnd: the end of a linux pipe that publisher can write to that where server
					reads the pipe
*/
int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];									// temp buffer to read receiving message into
    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);			// write the determined response to server
        read(readEnd, buff, 1024);						// read response from server (synchronous msg passing)

        fprintf(stdout, "\n---[PUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {				// if server rejects msg, immediately send again
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {			// server accepted attempted connection
            break;
        }
        else if (strcmp(buff, "retry") == 0) {			// servers asks to retry, wait for some time and resend
            usleep(100000);								// sleep for .1 secs - to reduce output spam
            continue;
        }
        else if (strcmp(buff, "successful") == 0) {		// server communicates successful generic message
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {		// server sends termination message
            return 0;									// function sends false when given 'signal' to terminate
        }
        else {											// unknown message sent, terminate or send next message
            fprintf(stdout, "PUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());	
            break;
        }
    }
    return 1;											// function sends 1 to continue sending messages
}

/*	simulates the operation of a publisher process - is executed as virtual process from Qserver.c
		- arguments passed pipe end values and topics published to
*/
int main(int argc, char * argv[]){

	// terminate if bad arguments sent
    if(argc != 4) {
        fprintf(stderr, "PUBLISHER error, bad args");
        exit(0);
    }

	// ends of linux pipes - see linux manuals
    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);

	// the specific topic this publisher publishes to
    int topic = atoi(argv[3]);

    fprintf(stdout, "\n---[PUB PID %d] using topic %d\n", (int)getpid(), topic);
    fprintf(stderr, "\n---[PUB PID %d] using topic %d\n", (int)getpid(), topic);

    // generate connect message
    char msgString[1024];
    char msg[QUACKSIZE + 1];
    char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };			// allowed chars for message
    int i, j, v;												// indices for loops

	// construct connection request message
    sprintf(msgString, "pub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

	// use the address of declared int variable as seed for random number generator
    srand((uintptr_t) &writeEnd);

    // repeat 20 times (where each iteration is a second)
    for (i = 0; i < 20; i++) {
        // post 20 entries per second
        for (j = 0; j < 20; j++) {
            // choose random-length & random-char msg up to QUACKSIZE size
            for (v = 0; v < QUACKSIZE; v++) {
                msg[v] = toUse[rand() %28];						// want to include null ptr for variable size msgs
            }
            msg[QUACKSIZE] = '\0';								// just in case they're all not null ptr

            // only posts to one topic that was passed as argument in argv[3]
            sprintf(msgString, "topic %d %s", topic, msg);
            sendMessage(msgString, readEnd, writeEnd);
            sendMessage("end", readEnd, writeEnd);
        }
        sleep(1);												// sleep 1 second
    }

	// after messages completed, send terminate    
    sendMessage("terminate", readEnd, writeEnd);

	// for succesfull termination
    return 0;
}
