/*
    Author: John Nemeth
    Description: subscriber process file
    Sources: linux manual pages
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "Qheader.h"

/* sends a message to subscriber proxy thread in topics.c. uses linux pipes
		msg: message to send
		readEnd: read end of pipe to server
		writeEnd: write end of pipe to server
*/
int sendMessage(char * msg, int readEnd, int writeEnd) {

    // declare message buffers
    char buff[1024];
    char toSend[1024];
    char entry[QUACKSIZE+1];
    int entriesRead = 0;
    int topic;
    strcpy(toSend, msg);

    while (1) {
        write(writeEnd, toSend, strlen(toSend) + 1);	// send message to server
        read(readEnd, buff, 1023);			// wait for response (synchronous

        fprintf(stdout, "\n---[SUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {		// received signal to resend msg
            continue;
        }
        else if (strstr(buff, "topic") != NULL) {	// received signal of message to read from topic
            // take entry and output it
            sscanf(buff, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topic, entry);
            entriesRead++;

            // send success message back
            strcpy(toSend, "successful");
            if ((entriesRead % 30) == 0) {		// read 30 topics every second
                sleep(1);
            }
        }
        else if (strcmp(buff, "accept") == 0) {		// server accepted message, end function
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {	// recieved termination signal
            exit(0);					// exit if send termination signal
        }
        else {
            fprintf(stdout, "SUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());	// default case
            break;
        }
    }
    return 1;
}

/* subscriber process. passed arguments are the linux pipes.
*/
int main(int argc, char * argv[]){

    // check for correct argument format
    if(argc != 3) {
        fprintf(stderr, "SUBSCRIBER error, bad args");
        exit(0);
    }

    // declare linux pipe file descriptors
    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);

    int subid = (int) getpid();
    int i;

    // string for msg sending
    char msgString[1024];

    // send connection message
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
