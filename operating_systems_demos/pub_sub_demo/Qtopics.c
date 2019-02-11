/*
	Author: John Nemeth
    Description: Source file for proxy thread creation for each publisher and subscriber.
					Acts as direct communication for all subscribers and publishers to 
					improve server performance. Each publisher and subscriber have their own
					'Proxy' thread created which monitors the associated pipes created in
					Qserver.c. Proxy threads all run in parallel unless they enter a critical section.
					pthread mutex locks and semaphores are used to maintain correct data flow and 
					synchronization. A deletor routine runs in parallel to other threads to maintain
					all topic entries by removing them based on how old they are (and other criteria
					in prior project versions). The deletor routine ends after a specified amount
					of time. The topicServer routine ends when all threads are done. 
	Sources: linux manuals and APIs, pthread docs, academic material from UO, and other specific material
				cited in comments
*/

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "Qheader.h"

// pthread object declarations used to maintain synchronization
pthread_cond_t allConnected;
pthread_cond_t readyForTermination;
pthread_mutex_t lock;

void pubt_print(int tid, int pid, char * str);						// declaration for generic publisher log message
void subt_print(int tid, int pid, char * str);						// declaration for generic subscriber log message
void* pubProxy (void * pubEntry);									// routine to run a publisher proxy thread
void* subProxy (void * subEntry);									// routine to run subscriber proxy thread
void* del();														// deletion from queue (requires mutex lock)
void printTopic(int index);											// prints topic (queue struct) information
void printEntry(int index, struct topicentry * cur);				// prints entry information using pointer
int enqueue(int topic, struct topicentry *new);						// add to queue (requires mutex lock)

// declare array of queues, 1 queue for each topic
struct queue topicQs[MAXTOPICS];

// used as a flag in different subroutines (all processes have terminated)
int allTerm;

////////////////////////////////////////////////////////////////////
int topicServer(struct pub_process pubs[], int TOTAL_PUBS, struct sub_process subs[], int TOTAL_SUBS) {

    int i, errorFlag;

    // initialize deletion protocol flag
    allTerm = 0;

    // init topics list
    for(i = 0; i < MAXTOPICS; i++) {
        topicQs[i].in = 0;
        topicQs[i].out = 0;
        topicQs[i].topicCounter = 0;

        int j;
        for (j = 0; j < MAXENTRIES; j++) {
            topicQs[i].circBuffer[j].entryNum = 0;
            topicQs[i].circBuffer[j].pubID = -1;
            gettimeofday(&topicQs[i].circBuffer[j].timestamp, NULL);
            strcpy(topicQs[i].circBuffer[j].message, "initialized entry");
        }
    }

	// creates a mutex lock for each queue
    printf("\n[-TOPICS SERVER-] Creating the topic locks.\n");
    for(i = 0; i < MAXTOPICS; i++) {
        if(pthread_mutex_init(&(topicQs[i].topicLock), NULL) != 0) {
            printf("\n mutex init has failed\n");
        }
    }

    // create pub proxy threads
    for (i = 0; i < TOTAL_PUBS; i++){

        errorFlag = pthread_create(&pubs[i].pubTID, NULL, &pubProxy, &pubs[i]);
        if (errorFlag != 0) {
            fprintf(stderr, "ERROR! Pub thread couldn't be created\n");
        }
    }
    // create sub proxy threads
    for (i = 0; i < TOTAL_SUBS; i++) {
        errorFlag = pthread_create(&subs[i].subTID, NULL, &subProxy, &subs[i]);
        if (errorFlag != 0) {
            fprintf(stderr, "ERROR! Sub thread couldn't be created\n");
        }
    }

    // create deletion thread
    pthread_t deletor;

    errorFlag = pthread_create(&deletor, NULL, &del, NULL);
    if (errorFlag != 0) {
        fprintf(stderr, "ERROR! Deletion thread couldn't be created\n");
    }
    

    // now wait for all to be connected before triggering flag to start 'termination'
    int totalConnected = 0;
    while (totalConnected != (TOTAL_PUBS + TOTAL_SUBS)) {

        totalConnected = 0;
        for (i = 0; i < TOTAL_SUBS; i++) {
            if (subs[i].connected == 1) {
                totalConnected++;
            }
        }
        for (i = 0; i < TOTAL_PUBS; i++) {
            if (pubs[i].connected == 1) {
                totalConnected++;
            }
        }
    }

    // after loop above is passed, trigger condition variable to proceed threads
    fprintf(stderr, "\n[-TOPICS SERVER-] all pubs and subs connected, proceeding to topics\n");
    fprintf(stdout, "\n[-TOPICS SERVER-] all pubs and subs connected, proceeding to topics\n");
    pthread_cond_broadcast(&allConnected);

    // wait till all threads ready to terminate
    int totalReady4Term = 0;
    while (totalReady4Term != (TOTAL_PUBS + TOTAL_SUBS)) {

        totalReady4Term = 0;
        for (i = 0; i < TOTAL_PUBS; i++) {
            if (pubs[i].terminated == 1) {
                totalReady4Term++;
            }
        }
        for (i = 0; i < TOTAL_SUBS; i++) {
            if (subs[i].terminated == 1) {
                totalReady4Term++;
            }
        }
    }

    // all threads ready to terminate
    fprintf(stderr, "\n[-TOPICS SERVER-] all pubs and subs ready to terminate, proceeding with termination\n");
    fprintf(stdout, "\n[-TOPICS SERVER-] all pubs and subs ready to terminate, proceeding with termination\n");
    allTerm = 1;    
    pthread_cond_broadcast(&readyForTermination);

    // join all threads back together
    for (i = 0; i < TOTAL_PUBS; i++) {
        if (pubs[i].terminated != 0) {
            pthread_join(pubs[i].pubTID, NULL);
        }
    }
    for (i = 0; i < TOTAL_SUBS; i++) {
        if (subs[i].terminated != 0) {
            pthread_join(subs[i].subTID, NULL);
        }
    }
    // join deletor protocol thread
    pthread_join(deletor, NULL);

    fprintf(stderr, "\n[-TOPICS SERVER-] all pub and sub proxy threads & deletor have been joined\n");
    fprintf(stdout, "\n[-TOPICS SERVER-] all pub and sub proxy threads & deletor have been joined\n");

    // destroy the locks
    for(i = 0; i < MAXTOPICS; i++) {
        pthread_mutex_destroy(&(topicQs[i].topicLock));
    }

    return 0;
}

////////////////////////////////////////////////////////////////////
/* enqueues a new entry into a passed topic queue
		topic: identifier of which topic queue to make entry
		new:	pointer to new entry created
*/
int enqueue(int topic, struct topicentry * new)
{
    pthread_mutex_lock(&topicQs[topic].topicLock);						// lock all other threads out of this topic
    // check for full buffer
    if (((topicQs[topic].in + 1) % MAXENTRIES) == topicQs[topic].out) {
        printf("Buffer Full! For Topic %d - in: %d out: %d Max Allowed: %d\n", topic, topicQs[topic].in, topicQs[topic].out, MAXENTRIES);
        // unlock & return signal
        pthread_mutex_unlock(&topicQs[topic].topicLock);				// unlock mutex
        return -1;														// return full buffer flag
    }

    int in = topicQs[topic].in;											// save current in position for changing values
    topicQs[topic].topicCounter += 1;									// record of how many entries
    topicQs[topic].circBuffer[in].entryNum = topicQs[topic].topicCounter;
    topicQs[topic].circBuffer[in].pubID = new->pubID;
    gettimeofday(&topicQs[topic].circBuffer[in].timestamp, NULL);       // set timestamp
    strcpy(topicQs[topic].circBuffer[in].message, new->message);        // give it a message

    topicQs[topic].in = (topicQs[topic].in + 1) % MAXENTRIES;  			// find new in position for topic circular buffer
    printf("\t=== Entry Successfull on topic %d ===\n", topic);	
    printTopic(topic);													// prints new relevant topic information
    printEntry(in, &topicQs[topic].circBuffer[in]);						// prints new entry information

    pthread_mutex_unlock(&(topicQs[topic].topicLock));					// unlock mutex so other threads can access
    return 0;
}

////////////////////////////////////////////////////////////////////
/* removes entries from a topic based on how old the entry is (>4 seconds)
		i: index of a topic in the topic queue 
*/
int dequeue(int i) {

    pthread_mutex_lock(&topicQs[i].topicLock);							// lock mutex for this topic

    // check for empty buffer or if buffer position does not contain valid entry
    if ((topicQs[i].in == topicQs[i].out) || (topicQs[i].circBuffer[topicQs[i].out].pubID == -1)) {
        pthread_mutex_unlock(&topicQs[i].topicLock);					// unlock mutex
        return 0;														// return flag for nothing deleted
    }

	// get time values for comparison
    int allowedDiff = 4;        										// set allowance of time difference (4 seconds)
    struct timeval now;
    int timediff;

    gettimeofday(&now, NULL);
    timediff = (int)difftime(now.tv_sec, topicQs[i].circBuffer[topicQs[i].out].timestamp.tv_sec);

	// compare time values and make sure entry not older than 4 seconds
    if (timediff > allowedDiff) {

        // "removes" entries from the queue by reinitializing the entry
        printf("===========================\n");
        printf("[Delete Protocol (TID: %d)]\n", (int)pthread_self());
        printf("Entry About to be Dequed in topic %d (in: %d out: %d)\n", i, topicQs[i].in, topicQs[i].out);
        printEntry(topicQs[i].out, &topicQs[i].circBuffer[topicQs[i].out]);

        topicQs[i].circBuffer[topicQs[i].out].entryNum = 0;
        topicQs[i].circBuffer[topicQs[i].out].pubID = -1;
        strcpy(topicQs[i].circBuffer[topicQs[i].out].message, "initialized entry");
        topicQs[i].out = (topicQs[i].out + 1) % MAXENTRIES;
    }

    pthread_mutex_unlock(&topicQs[i].topicLock);					// unlock mutex
    return 1;														// return flag for item deleted
}

////////////////////////////////////////////////////////////////////
/* sends stored message in topic queue to subscriber in entry struct
	and returns index of next valid entry in the queue.
		topic: index of topic in topic queue
		lastEntry: index of last valid entry in circular buffer within topic
		entry: object for placing valid entry information for subscriber thread to send to subscriber process
*/
int getEntry(int topic, int lastEntry, struct topicentry * entry) {

    pthread_mutex_lock(&topicQs[topic].topicLock);					// lock topic queue

    // if lastentry+1 is valid entry
    if ((topicQs[topic].circBuffer[lastEntry+1].pubID != -1) && (lastEntry < (MAXENTRIES - 1))) {
        *entry = topicQs[topic].circBuffer[lastEntry+1];
        pthread_mutex_unlock(&topicQs[topic].topicLock);
        return 1;													// return flag for next entry was valid
    }

    int i, curEntry;
    for (i = 0; i < MAXENTRIES; i++) {

        curEntry = (lastEntry + i) % MAXENTRIES;

        if (topicQs[topic].circBuffer[curEntry].pubID != -1) {
            // found an active entry - assign and send back negative entry num value
            *entry = topicQs[topic].circBuffer[curEntry];
            pthread_mutex_unlock(&topicQs[topic].topicLock);
            return curEntry;										// return index of first valid entry found
        }
    }

    // if reached here then the queue is empty. the 'in-1 % max' is the last pos of enqueued 
    int check = (topicQs[topic].in - 1) % MAXENTRIES;
    if (check < 0) {												// if last position in queue was zero (to avoid modulus error)
        curEntry = MAXENTRIES + check;
    }
    else {
        curEntry = check;
    }

    pthread_mutex_unlock(&topicQs[topic].topicLock);				// end of critical section
    return -curEntry;												// returns negative flag to signal empty queue 
}

////////////////////////////////////////////////////////////////////
/* publisher proxy thread. one is created for each connected publisher process 
	4 receiving are possible. 	"connect" - connection msg, sent only once and required
								"end" - signals end of last communication
								"topic 'number' " 	- signals which topic publisher wants to publish to
														next message will be entry
								"terminate" 		- signals termination of publisher process
		pubEntry: passed object of publisher process information from array
*/
void* pubProxy (void * pubEntry) {

	// assign object to pub struct
    struct pub_process * pub = pubEntry;

	// get IDs
    int tid = (int)pthread_self();
    int pid = (int)pub->pid;

	// define buffers for messages
    char buffer[QUACKSIZE];
    char output[256];
    char response[256];

	// close irrelevant pipe ends
    close(pub->quacker_to_pub[0]);
    close(pub->pub_to_quacker[1]);

	// establish connection
    pubt_print(tid, pid, "Pub thread running.");
    sprintf(output, "Listening for publisher %d", pub->pid);
    pubt_print(tid, pid, output);
    read(pub->pub_to_quacker[0], buffer, 1024);						// wait for connection message
    sprintf(output, "got message: %s", buffer);
    pubt_print(tid, pid, output);

	// check for message received
    if (strstr(buffer, "connect") != NULL) {						// send accept message
        pubt_print(tid, pid, "sending 'accept'");
        char pubid[1024];
        sscanf(buffer, "%*s %s %*s", pubid);
        pub->pubid = atoi(pubid);
        write(pub->quacker_to_pub[1], "accept", 7);
    }
    else {															// not valid connection signal
        pubt_print(tid, pid, "sending 'reject' for wrong connect msg");
        write(pub->quacker_to_pub[1], "reject", 7);
        close(pub->quacker_to_pub[1]);
        close(pub->pub_to_quacker[0]);
        return NULL;												// end thread
    }

    // main loop for messages
    do {
        pubt_print(tid, pid, "waiting for message");
        read(pub->pub_to_quacker[0], buffer, 1024);     			// read from publisher using pipe
		sprintf(output, "got message: %s", buffer);
        pubt_print(tid, pid, output);

        if (strcmp(buffer, "end") == 0) {							// end of last communication
            sprintf(response, "accept");
            sprintf(output, "Sending Response: %s", response);
            pubt_print(tid, pid, output);
            // wait for all threads to be connected before reading new messages (project requirements)
            if (pub->connected != 1) {
                pub->connected = 1;
                pthread_mutex_lock(&lock);
                pthread_cond_wait(&allConnected, &lock);
                pthread_mutex_unlock(&lock);
            }
        }
        else if (strstr(buffer, "topic") != NULL) {     			// topic establishment protocol

            int topicID;
            char topicEntry[QUACKSIZE + 1];

            // use c preprossesor to insert value to sscan argument
            sscanf(buffer, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topicID, topicEntry);

			// prepare reading of new entry from publisher
            struct topicentry newEntry;								
            strcpy(newEntry.message, topicEntry);
            newEntry.pubID = pub->pubid;
            sprintf(output, "Attempting  entry of message '%s' on topic %d", newEntry.message, topicID);
            pubt_print(tid, pid, output);

			// attempt enqueuing of message into topic queue
            int tester = enqueue(topicID, &newEntry);
            if (tester < 0) {
                sprintf(response, "retry");
                sprintf(output, "Sending Response: %s (entry failed - full buffer)", response);
                pthread_yield();
            }
            else {
                sprintf(response, "successful");
                sprintf(output, "Sending Response: %s (entry made successfully)", response);
            }

            // set flag for active topic publishing for connection record
            pub->topicList[topicID] = 1;
            pubt_print(tid, pid, output);
        }
        else if (strcmp(buffer, "terminate") == 0) {	// for termination
            break;
        }
        else {											// bad input 
            sprintf(response, "reject");				// send reject msg
            sprintf(output, "Sending Response: %s (not a valid msg)", response);
            pubt_print(tid, pid, output);
        }

		// send reponse to publisher
        write(pub->quacker_to_pub[1], response, strlen(response)+1);

    } while (1);

    pub->terminated = 1;								// set this pub terminated
    sprintf(response, "terminate");
    sprintf(output, "Sending response %s (once all other threads ready to terminate)", response);
    pubt_print(tid, pid, output);

    // halts thread until all other threads ready to terminate (per project spec)
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&readyForTermination, &lock);
    pthread_mutex_unlock(&lock);

	// send final msg to publisher in case hasn't terminated
    write(pub->quacker_to_pub[1], response, (strlen(response)+1));
	// close pipes opened for communication
    close(pub->quacker_to_pub[1]);
    close(pub->pub_to_quacker[0]);

    return NULL;										// end thread
}

////////////////////////////////////////////////////////////////////
/* proxy thread for subscriber processes. 
		subProxy: subscriber object passed as argument to routine
*/
void* subProxy(void * subEntry) {

	// assign passed object as subscriber struct
    struct sub_process * sub = subEntry;

	// declare loop indices, IDs, and message buffers
    int i, j;
    int tid = (int)pthread_self();
    int pid = sub->pid;
    char buffer[QUACKSIZE];
    char output[256];           						// server display msg
    char response[256];

	// declare read record for topic - needed to determine which read entries are old
    struct topicentry readRecord[MAXTOPICS][MAXENTRIES];

    struct timeval limit;       						// time limit for 'success' response

    // used for watching when input bit switched on file descriptors in 'read'
    fd_set readIn;
    FD_ZERO(&readIn);
    FD_SET(sub->sub_to_quacker[0], &readIn);

    // time limit to wait for response is 3 seconds
    limit.tv_sec =  3;
    limit.tv_usec = 0;

    // init read record
    for (i = 0; i < MAXTOPICS; i++) {
        for (j = 0; j < MAXENTRIES; j++) {
            strcpy(readRecord[i][j].message, "initialized entry");
        }
    }

    // close unused pipes
    close(sub->quacker_to_sub[0]);
    close(sub->sub_to_quacker[1]);

    subt_print(tid, pid, "Sub thread running");
    sprintf(output, "Listening for subscriber %d", sub->pid);
    subt_print(tid, pid, output);
    read(sub->sub_to_quacker[0], buffer, 1024);     // wait for connection signal from subscriber process
    sprintf(output, "sub proxy got message: %s", buffer);
    subt_print(tid, pid, output);

    // check for first message being connection msg
    if (strstr(buffer, "connect") != NULL) {
        subt_print(tid, pid, "sending accept");
        char subid[1024];
        sscanf(buffer, "%*s %s %*s", subid);
        sub->subid = atoi(subid);
        write(sub->quacker_to_sub[1], "accept", 7);
    }
    else {              // bad connection msg, end thread
        subt_print(tid, pid, "sending reject (bad connection msg)");
        write(sub->quacker_to_sub[1], "reject", 7);
        close(sub->quacker_to_sub[1]);
        close(sub->sub_to_quacker[0]);
        return NULL;
    }

    // main loop for communication 
    do {

        subt_print(tid, pid,  "waiting for msg");
        read(sub->sub_to_quacker[0], buffer, 1024);  	   		// read from input of entry
        sprintf(output, "sub proxy got message: %s", buffer);
        subt_print(tid,pid, output);

        if (strcmp(buffer, "end") == 0) {						// whenever end is sent, accept response
            sprintf(response, "accept");
            sprintf(output, "Sending response: %s", response);
            subt_print(tid, pid, output);
            if (sub->connected != 1) {							// wait for all other threads to be connected (project requirements)
                subt_print(tid, pid, "Halting thread to wait on all other threads being connected");
                sub->connected = 1;
                pthread_mutex_lock(&lock);						// no shared data
                pthread_cond_wait(&allConnected, &lock);		// thread wait for condition of all connected
                pthread_mutex_unlock(&lock);
            }
        }
        else if (strstr(buffer, "topic") != NULL) {            	// subscriber wants to subscribe to a tpic
            sprintf(response, "accept");
            sprintf(output, "Sending response: %s", response);
            subt_print(tid,pid, output);

            int topicID;										// topicID value was sent in same string as msg 'topic ...'
            sscanf(buffer, "%*s %*s %*s %d", &topicID);
            topicID = topicID % MAXTOPICS;

            sprintf(output, "Wants to subscribe to topic %d, adding to connection record", topicID);
            subt_print(tid, pid, output);

            // assign iterested topics to its connection record
            sub->topicList[topicID] = 1;
        }
        else if (strcmp(buffer, "read") == 0) { 				// reads all items that are sub'd 

            struct topicentry curEntry;                         // temp entry to store return msg
            struct timeval findEntry, now;
            int rri;                 						   	// topic index used for round-robin reading
            int lastRead[MAXTOPICS];    						// keeps track of last read by topic index

            // init all entries to -1
            for (i = 0; i < MAXTOPICS; i++) {
                lastRead[i] = -1;
            }

            gettimeofday(&findEntry, NULL);
            gettimeofday(&now, NULL);

            // stop reading when nothing new to read for 5 seconds
            while (difftime(now.tv_sec, findEntry.tv_sec) < 5) {

                // loop for cycling through NUMBER of topics, actual indices are contained in sub struct
                for (i = 0; i < MAXTOPICS; i++) {

                    // look through all possible entries
                    for (j = 0; j < MAXENTRIES; j++) {
                        rri = (i + j) % MAXTOPICS;      // find round robin index

                        // make sure subbed to topic
                        if (sub->topicList[rri] != 0) {

                            // find index and message of next entry
                            int retVal;
                            curEntry.pubID = -3;
                            retVal = getEntry(rri, lastRead[rri], &curEntry);

                            if (retVal == 1) {
                                lastRead[rri] += 1;
                            }
                            else if (curEntry.pubID > 0) {
                                lastRead[rri] = retVal;
                            }
                            else {
                                // empty queue, move to next topic
                                lastRead[rri] = -retVal;
                                break;
                            }

                            // cur not init and there is a diff from record, send to sub
                            if (strcmp(readRecord[rri][lastRead[rri]].message, curEntry.message) != 0) {

                                // get time of finding an entry
                                gettimeofday(&findEntry, NULL);
                                strcpy(readRecord[rri][lastRead[rri]].message, curEntry.message);

                                sprintf(response, "topic %d %s", rri, curEntry.message);
                                sprintf(output, "Found unread topic on %d - Sending response: %s", rri ,response);
                                subt_print(tid, pid, output);
                                printTopic(rri);
                                printEntry(lastRead[rri], &curEntry);

                                write(sub->quacker_to_sub[1], response, strlen(response)+1);

                                int subRespond = select(1, &readIn, NULL, NULL, &limit);
                                if (subRespond == -1) {
                                    subt_print(tid, pid, "ERROR USING SELECT() FOR TIMED RESPONSE");
                                }
                                else if (subRespond == 0) {             // sub sent a response
                                    read(sub->sub_to_quacker[0], buffer, 1024);
                                    sprintf(output, "SUB responded with '%s'", buffer);
                                    subt_print(tid, pid, output);
                                }
                                else {                          // sub hasn't sent a response
                                }
                            } // if the item is different from the record
                        } // for all items in circbuffer of topic
                    } // if sub is subb'd to topic
                } // for all topics
                gettimeofday(&now, NULL);
            } // while haven't found an entry for over 10 seconds
            sprintf(output, "Sub has had no entry to read for more than 10 seconds - sending terminate");
            subt_print(tid, pid, output);

            strcpy(buffer, "terminate");
            strcpy(response, "accept");
        }
        else if (strcmp(buffer, "terminate") == 0) {
            break;
        }
        else {          // msg not recognized
            sprintf(response, "reject");
            sprintf(output, "Sending response: %s (msg not recognized)", response);
            subt_print(tid, pid, output);
        }

        write(sub->quacker_to_sub[1], response, strlen(response) + 1);

    } while (strcmp(buffer, "terminate") != 0);

	// subscriber process termination has been determined
    sprintf(response, "terminate");
    sprintf(output, "Sending response: %s (once all other threads are ready to terminate)", response);
    subt_print(tid, pid, output);
    sub->terminated = 1;

    // wait for other threads ready for termination
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&readyForTermination, &lock);
    pthread_mutex_unlock(&lock);

	// send termination signal to subscriber in case it hasn't terminated
    write(sub->quacker_to_sub[1], response, (strlen(response)+1));
    close(sub->quacker_to_sub[1]);
    close(sub->sub_to_quacker[0]);

    return NULL;
}


////////////////////////////////////////////////////////////////////
/* deletor routine used for deletion thread. ends when no deleting has occured
	for 10 seconds and after all subscriber and publisher threads have terminated.
*/
void* del() {

    printf("\n[Delete Protocol] Beginning timed deletetion protocol\n");

	// declare variables for tracking deletions and time passed
    struct timeval now, lastDelete;
    int numsDeleted = 0, i;

    gettimeofday(&lastDelete, NULL);
    gettimeofday(&now, NULL);

    // terminate after 10 seconds of no deleteing and when all children are terminated (for safety - no retry loops)
    while ((difftime(now.tv_sec, lastDelete.tv_sec) < 11) || (allTerm != 1)) {

        // for all topics that have items in it
        for (i = 0; i < MAXTOPICS; i++) {

            numsDeleted = dequeue(i);

            if (numsDeleted != 0) {      // reset timer everytime something gets deleted
                gettimeofday(&lastDelete, NULL);
            }
        }
        gettimeofday(&now, NULL);
    }

    printf("\n[Delete Protocol] All processes have terminated and time limit reached. Ending timed delete protocol\n");
    return NULL;
}

////////////////////////////////////////////////////////////////////
/* utility function for printing general topic information
		index: index of topic in the topic queue array
*/
void printTopic(int index) {

    printf("===== Printing Topic ID: %d (max unique: %d)\n\tin position of buffer: %d\n\tout position of buffer: %d", index, MAXTOPICS, topicQs[index].in, topicQs[index].out);
    printf("\n\tnumber of entries(topiccounter): %d\n", topicQs[index].topicCounter);

}

//////////////////////////////////////////////////////////////////
/* prints general entry information
		i: index of entry in ciruclar buffer
		cur: pointer of entry
*/
void printEntry(int i, struct topicentry * cur) {

    struct timeval now;
    int timediff;

    gettimeofday(&now, NULL);
    timediff = (int)difftime(now.tv_sec, cur->timestamp.tv_sec);

    printf("---for entry at circbuffer index %d\n", i);
    printf("entryNum: %d\n", cur->entryNum);
    printf("seconds old: %d\n", timediff);
    printf("pubID: %d\n", cur->pubID);
    printf("message: %s\n", cur->message);
    printf("====================\n");
}

/////////////////////////////////////////////////////////////////
// general templates for printing publisher and subscriber thread information
void pubt_print(int tid, int pid, char * str)
{
    printf("\n[Pub thread %d (for PID: %d)] %s\n", tid, pid, str);
}

void subt_print(int tid, int pid, char * str)
{
    printf("\n[Subscriber thread %d (for PID: %d)] %s\n", tid, pid, str);
}
