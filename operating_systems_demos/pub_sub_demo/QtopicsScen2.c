/*
    Revision Author: John Nemeth
    Original Author(s): UO CIS415 Departartment Staff and Instructors
    Description: topics server file for scenario 2 of project2 
*/

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "QheaderScen2.h"

pthread_cond_t allConnected;
pthread_cond_t readyForTermination;
pthread_mutex_t lock;

void pubt_print(int tid, int pid, char * str);
void subt_print(int tid, int pid, char * str);
void* pubProxy (void * pubEntry);
void* subProxy (void * subEntry);
void* del();
void printTopic(int index);
void printEntry(int index, struct topicentry * cur);
int enqueue(int topic, struct topicentry *new);

struct queue topicQs[MAXTOPICS];

int allTerm;

////////////////////////////////////////////////////////////////////
int topicServer(struct pub_process pubs[], int TOTAL_PUBS, struct sub_process subs[], int TOTAL_SUBS) {

    int i, errorFlag;

    // semaphore for deletion protocl
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
    pthread_join(deletor, NULL);

    fprintf(stderr, "\n[-TOPICS SERVER-] all pub & sub proxy threads & deletor have been joined\n");
    fprintf(stdout, "\n[-TOPICS SERVER-] all pub & sub proxy threads & deletor have been joined\n");

    // destroy the locks
    for(i = 0; i < MAXTOPICS; i++) {
        pthread_mutex_destroy(&(topicQs[i].topicLock));
    }

    return 0;
}
////////////////////////////////////////////////////////////////////
int enqueue(int topic, struct topicentry * new)
{
    pthread_mutex_lock(&topicQs[topic].topicLock);

    // check for full buffer
    if (((topicQs[topic].in + 1) % MAXENTRIES) == topicQs[topic].out) {
        printf("Buffer Full! For Topic %d - in: %d out: %d Max Allowed: %d\n", topic, topicQs[topic].in, topicQs[topic].out, MAXENTRIES);
        // unlock & return signal
        pthread_mutex_unlock(&topicQs[topic].topicLock);
        return -1;
    }

    int in = topicQs[topic].in;
    topicQs[topic].topicCounter += 1;
    topicQs[topic].circBuffer[in].entryNum = topicQs[topic].topicCounter;
    topicQs[topic].circBuffer[in].pubID = new->pubID;
    gettimeofday(&topicQs[topic].circBuffer[in].timestamp, NULL);       // set timestamp
    strcpy(topicQs[topic].circBuffer[in].message, new->message);        // give it a message

    topicQs[topic].in = (topicQs[topic].in + 1) % MAXENTRIES;  
    printf("\t=== Entry Successfull on topic %d ===\n", topic);
    printTopic(topic);
    printEntry(in, &topicQs[topic].circBuffer[in]);

    pthread_mutex_unlock(&(topicQs[topic].topicLock));
    return 0;
}

////////////////////////////////////////////////////////////////////
/* i: index of a topic in the topic queue */
int dequeue(int i) {

    pthread_mutex_lock(&topicQs[i].topicLock);

    // check for empty buffer
    if (topicQs[i].in == topicQs[i].out) {
        pthread_mutex_unlock(&topicQs[i].topicLock);
        return 0;
    }

    int allowedDiff = 14;        // delete entries older than 14 seconds
    struct timeval now;
    int timediff;

    gettimeofday(&now, NULL);
    timediff = (int)difftime(now.tv_sec, topicQs[i].circBuffer[topicQs[i].out].timestamp.tv_sec);

    if ((topicQs[i].circBuffer[topicQs[i].out].pubID != -1) && (timediff > allowedDiff)) {

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

    pthread_mutex_unlock(&topicQs[i].topicLock);
    return 1;
}

////////////////////////////////////////////////////////////////////
int getEntry(int topic, int lastEntry, struct topicentry * entry) {

    pthread_mutex_lock(&topicQs[topic].topicLock);

    // -1 is no entry assigned
    if ((topicQs[topic].circBuffer[lastEntry+1].pubID > 0) && (lastEntry < (MAXENTRIES-1))) {
        *entry = topicQs[topic].circBuffer[lastEntry+1];
        pthread_mutex_unlock(&topicQs[topic].topicLock);
        return 1;
    }

    int i, curEntry;

    for (i = 0; i < MAXENTRIES; i++) {

        curEntry = (lastEntry + i) % MAXENTRIES;

        if (topicQs[topic].circBuffer[curEntry].pubID > 0) {
            // found an active entry - assign and send back negative entry num value
            *entry = topicQs[topic].circBuffer[curEntry];
            pthread_mutex_unlock(&topicQs[topic].topicLock);
            return curEntry;
        }
    }

    // if reached here then the queue is empty. the 'in-1 % max' is the last pos of enqueued
    int check = (topicQs[topic].in - 1) % MAXENTRIES;
    if (check < 0) {		// check for if last queue position was zero
        curEntry = MAXENTRIES + check;
    }
    else {
        curEntry = check;
    }

    pthread_mutex_unlock(&topicQs[topic].topicLock);
    return -curEntry;
}

////////////////////////////////////////////////////////////////////
void* pubProxy (void * pubEntry) {

    struct pub_process * pub = pubEntry;

    int tid = (int)pthread_self();
    int pid = (int)pub->pid;

    char buffer[QUACKSIZE];
    char output[256];
    char response[256];

    // close irrelevant ends of pipe for this child
    close(pub->quacker_to_pub[0]);
    close(pub->pub_to_quacker[1]);

    pubt_print(tid, pid, "Pub thread running.");
    sprintf(output, "Listening for publisher %d", pub->pid);
    pubt_print(tid, pid, output);
    read(pub->pub_to_quacker[0], buffer, 1024);
    sprintf(output, "got message: %s", buffer);
    pubt_print(tid, pid, output);

    // establish connection
    if (strstr(buffer, "connect") != NULL) {
        pubt_print(tid, pid, "sending 'accept'");
        char pubid[1024];
        sscanf(buffer, "%*s %s %*s", pubid);
        pub->pubid = atoi(pubid);
        write(pub->quacker_to_pub[1], "accept", 7);
    }
    else {	// not valid connection signal
        pubt_print(tid, pid, "sending 'reject' for wrong connect msg");
        write(pub->quacker_to_pub[1], "reject", 7);
        close(pub->quacker_to_pub[1]);
        close(pub->pub_to_quacker[0]);
        return NULL;
    }

    // main loop for messages
    do {
        pubt_print(tid, pid, "waiting for message");
        read(pub->pub_to_quacker[0], buffer, 1024);     // read from input of entry
	sprintf(output, "got message: %s", buffer);
        pubt_print(tid, pid, output);

        if (strcmp(buffer, "end") == 0) {		// end of last communication
            sprintf(response, "accept");
            sprintf(output, "Sending Response: %s", response);
            pubt_print(tid, pid, output);
            // wait for threads to connect if they're not
            if (pub->connected != 1) {
                pub->connected = 1;
                pthread_mutex_lock(&lock);
                pthread_cond_wait(&allConnected, &lock);
                pthread_mutex_unlock(&lock);
            }
        }
        else if (strstr(buffer, "topic") != NULL) {     // topic establishment protocol

            int topicID;
            char topicEntry[QUACKSIZE + 1];

            // use c preprossesor to insert value to sscan argument (from stackexchange)
            sscanf(buffer, "%*s %d %"STR(QUACKSIZE)"[^\n]", &topicID, topicEntry);

            struct topicentry newEntry;
            strcpy(newEntry.message, topicEntry);
            newEntry.pubID = pub->pubid;
            sprintf(output, "Attempting  entry of message '%s' on topic %d", newEntry.message, topicID);
            pubt_print(tid, pid, output);

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

            // add topic to connection record for pub
            pub->topicList[topicID] = 1;           

            pubt_print(tid, pid, output);
        }
        else if (strcmp(buffer, "terminate") == 0) {	// for termination
            break;
        }
        else {						// bad input 
            sprintf(response, "reject");
            sprintf(output, "Sending Response: %s (not a valid msg)", response);
            pubt_print(tid, pid, output);
        }

        write(pub->quacker_to_pub[1], response, strlen(response)+1);

    } while (1);

    pub->terminated = 1;
    sprintf(response, "terminate");
    sprintf(output, "Sending response %s (once all other threads ready to terminate)", response);
    pubt_print(tid, pid, output);

    // halts thread until all other threads ready to terminate (per project spec)
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&readyForTermination, &lock);
    pthread_mutex_unlock(&lock);

    write(pub->quacker_to_pub[1], response, (strlen(response)+1));
    close(pub->quacker_to_pub[1]);
    close(pub->pub_to_quacker[0]);

    return NULL;
}

////////////////////////////////////////////////////////////////////
void* subProxy(void * subEntry) {

    struct sub_process * sub = subEntry;

    int i, j;
    int tid = (int)pthread_self();
    int pid = sub->pid;
    char buffer[QUACKSIZE];
    char output[256];		// server display msg
    char response[256];
    struct topicentry readRecord[MAXTOPICS][MAXENTRIES];	// represents read record for the topic

    struct timeval limit;	// time limit for 'success' response

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
    read(sub->sub_to_quacker[0], buffer, 1024);     // read from sub for connect message
    sprintf(output, "sub proxy got message: %s", buffer);
    subt_print(tid, pid, output);

    // connect to subscriber process
    if (strstr(buffer, "connect") != NULL) {
        subt_print(tid, pid, "sending accept");
        char subid[1024];
        sscanf(buffer, "%*s %s %*s", subid);
        sub->subid = atoi(subid);
        write(sub->quacker_to_sub[1], "accept", 7);
    }
    else {		// bad connection msg, end thread
        subt_print(tid, pid, "sending reject (bad connection msg)");
        write(sub->quacker_to_sub[1], "reject", 7);
        close(sub->quacker_to_sub[1]);
        close(sub->sub_to_quacker[0]);
        return NULL;
    }

    // main loop for communication 
    do {
        subt_print(tid, pid,  "waiting for msg");
        read(sub->sub_to_quacker[0], buffer, 1024);     // read from input of entry
        sprintf(output, "sub proxy got message: %s", buffer);
        subt_print(tid,pid, output);

        if (strcmp(buffer, "end") == 0) {
            sprintf(response, "accept");
            sprintf(output, "Sending response: %s", response);
            subt_print(tid, pid, output);
            if (sub->connected != 1) {
                subt_print(tid, pid, "Halting thread to wait on all other threads being connected");
                sub->connected = 1;
                pthread_mutex_lock(&lock);		// need lock for conditional wait on threads
                pthread_cond_wait(&allConnected, &lock);        // thread wait for condition of all connected
                pthread_mutex_unlock(&lock);
            }
        }
        else if (strstr(buffer, "topic") != NULL) {		// msg subscription 
            sprintf(response, "accept");
            sprintf(output, "Sending response: %s", response);
            subt_print(tid,pid, output);

            int topicID;
            sscanf(buffer, "%*s %*s %*s %d", &topicID);
            topicID = topicID % MAXTOPICS;

            sprintf(output, "Wants to subscribe to topic %d, adding to connection record", topicID);
            subt_print(tid, pid, output);

            // assign iterested topics to its connection record
            sub->topicList[topicID] = 1;
        }
        else if (strcmp(buffer, "read") == 0) {	// reads all items that are sub'd 

            struct topicentry curEntry;					// temp entry to store return msg
            struct timeval findEntry, now;

            gettimeofday(&findEntry, NULL);
            gettimeofday(&now, NULL);

            // stop reading when over 10 seconds of no new entries
            while (difftime(now.tv_sec, findEntry.tv_sec) < 11) {

                // loop for cycling through NUMBER of topics, actual indices are contained in sub struct
                for (i = 0; i < MAXTOPICS; i++) {

                    // make sure topic subscribed to
                    if (sub->topicList[i] != 0) {
                        int lastRead = -1;

                        // loop a max amount of time, break earlier if need
                        for (j = 0; j < MAXENTRIES; j++) {

                            // find index and message of next entry
                            int retVal;
                            curEntry.pubID = -3;
                            retVal = getEntry(i, lastRead, &curEntry);

                            if (retVal == 1) {
                                lastRead++;
                            }
                            else if (curEntry.pubID != -3) {	
                                lastRead = retVal;
                            }
                            else {
                                // empty queue, move to next topic
                                lastRead = -retVal;
                                break;
                            }

                            // cur not init and there is a diff from record, send to sub
                            if (strcmp(readRecord[i][lastRead].message, curEntry.message) != 0) {

                                // get time of finding an entry
                                gettimeofday(&findEntry, NULL);
                                strcpy(readRecord[i][lastRead].message, curEntry.message);

                                sprintf(response, "topic %d %s", i, curEntry.message);
                                sprintf(output, "Found unread topic on %d - Sending response: %s",i ,response);
                                subt_print(tid, pid, output);
                                printTopic(i);
                                printEntry(lastRead, &curEntry);

                                write(sub->quacker_to_sub[1], response, strlen(response)+1);

                                // wait till sub response or till time limit is passed
                                int subRespond;
                                subRespond = select(1, &readIn, NULL, NULL, &limit);

                                if (subRespond == -1) {
                                    subt_print(tid, pid, "ERROR USING SELECT() FOR TIMED RESPONSE");
                                }
                                else if (subRespond == 0) {		// sub sent a response
                                    read(sub->sub_to_quacker[0], buffer, 1024);
                                    sprintf(output, "SUB responded with '%s'", buffer);
                                    subt_print(tid, pid, output);
                                }
                                else {				// sub hasn't sent a response
                                }
                                
                            } // if the item is different from the record
                        } // for all items in circbuffer of topic
                    } // if sub is subb'd to topic
                } // for all topics
                gettimeofday(&now, NULL);
            } // while haven't found an entry for over 10 seconds
            sprintf(output, "Sub has had no entry to read for at least 11 seconds - sending terminate");
            subt_print(tid, pid, output);

            strcpy(buffer, "terminate");
            strcpy(response, "accept");
        }
        else if (strcmp(buffer, "terminate") == 0) {	
            break;
        }
        else {		// msg not recognized
            sprintf(response, "reject");
            sprintf(output, "Sending response: %s (msg not recognized)", response);
            subt_print(tid, pid, output);
        }

        write(sub->quacker_to_sub[1], response, strlen(response) + 1);

    } while (strcmp(buffer, "terminate") != 0);

    sprintf(response, "terminate");
    sprintf(output, "Sending response: %s (once all other threads are ready to terminate)", response);
    subt_print(tid, pid, output);
    sub->terminated = 1;

    // wait for other threads ready for termination
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&readyForTermination, &lock);
    pthread_mutex_unlock(&lock);

    write(sub->quacker_to_sub[1], response, (strlen(response)+1));
    close(sub->quacker_to_sub[1]);
    close(sub->sub_to_quacker[0]);

    return NULL;
}

////////////////////////////////////////////////////////////////////
void* del() {

    printf("\n[Delete Protocol] Beginning timed deletetion protocol\n");
    int i;

    struct timeval now, lastDelete;
    int numsDeleted = 0;

    gettimeofday(&lastDelete, NULL);
    gettimeofday(&now, NULL);

    // keep running until 30 secords without deletion or, as a safety, all children aren't terminated
    while ((difftime(now.tv_sec, lastDelete.tv_sec) < 30) || (allTerm != 1)) {
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
void printTopic(int index) {

    printf("===== Printing Topic ID: %d (max unique: %d)\n\tin position of buffer: %d\n\tout position of buffer: %d", index, MAXTOPICS, topicQs[index].in, topicQs[index].out);
    printf("\n\tnumber of entries(topiccounter): %d\n", topicQs[index].topicCounter);

}

// prints entry
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
    printf("=====================\n");
}

void pubt_print(int tid, int pid, char * str)
{
    printf("\n[Pub thread %d (for PID: %d)] %s\n", tid, pid, str);
}

void subt_print(int tid, int pid, char * str)
{
    printf("\n[Subscriber thread %d (for PID: %d)] %s\n", tid, pid, str);
}

