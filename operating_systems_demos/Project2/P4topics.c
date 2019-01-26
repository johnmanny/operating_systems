/*
    Original Author(s): UO CIS415 Departartment Staff and Instructors
    Heavy Revision Author: John Nemeth

*/

#include <stdio.h>
#include <string.h>
#include "P4header.h"

pthread_cond_t allConnected;
pthread_cond_t readyForTermination;
pthread_mutex_t lock;

void pubt_print(int tid, int pid, char * str);
void subt_print(int tid, int pid, char * str);
void* pubProxy (void * pubEntry);
void* subProxy (void * subEntry);
void* del();
void printTopicQueue(int index);
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
        //struct timeval start = gettimeofday(&start, NULL);
        int j;
        for (j = 0; j < MAXENTRIES; j++) {
            topicQs[i].circBuffer[j].entryNum = -1;
            topicQs[i].circBuffer[j].pubID = -1;
            gettimeofday(&topicQs[i].circBuffer[j].timestamp, NULL);
            //topicQs[i].circBuffer[j].timestamp = start;
            strcpy(topicQs[i].circBuffer[j].message, "this is the initialized entry");
        }
    }

    printf("\n[-TOPICS SERVER-] Creating the topic locks.\n");
    for(i = 0; i < MAXTOPICS; i++) {
        if(pthread_mutex_init(&(topicQs[i].topicLock), NULL) != 0) {
            printf("\n mutex init has failed\n");
        }
    }

    // create sub proxy threads
    for (i = 0; i < TOTAL_SUBS; i++) {
        errorFlag = pthread_create(&subs[i].subTID, NULL, &subProxy, &subs[i]);
        if (errorFlag != 0) {
            fprintf(stderr, "ERROR! Sub thread couldn't be created\n");
        }
    }
    // create pub proxy threads
    for (i = 0; i < TOTAL_PUBS; i++){

        errorFlag = pthread_create(&pubs[i].pubTID, NULL, &pubProxy, &pubs[i]);
        if (errorFlag != 0) {
            fprintf(stderr, "ERROR! Pub thread couldn't be created\n");
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
        for (i = 0; i < TOTAL_PUBS; i++) {
            if (pubs[i].connected == 1) {
                totalConnected++;
            }
        }
        for (i = 0; i < TOTAL_SUBS; i++) {
            if (subs[i].connected == 1) {
                totalConnected++;
            }
        }
    }

    // after loop above is passed, trigger condition variable to proceed threads
    fprintf(stderr, "\n[-TOPICS SERVER-] all pubs and subs connected, proceeding to topics\n");
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
    allTerm = 1;    
    pthread_cond_broadcast(&readyForTermination);

    // join all threads back together
    for (i = 0; i < TOTAL_PUBS; i++) {
        pthread_join(pubs[i].pubTID, NULL);
    }
    for (i = 0; i < TOTAL_SUBS; i++) {
        pthread_join(subs[i].subTID, NULL);
    }
    // join deletor
    pthread_join(deletor, NULL);


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
        printf("\nTopic Num: %d\nin: %d\nout: %d", topic, topicQs[topic].in, topicQs[topic].out);
        printf("\nMax allowed: %d\n", MAXENTRIES);
        // unlock & return signal
        pthread_mutex_unlock(&topicQs[topic].topicLock);
        return -1;
    }

    topicQs[topic].topicCounter += 1;

    int in = topicQs[topic].in;
    topicQs[topic].circBuffer[in].entryNum = topicQs[topic].topicCounter;
    topicQs[topic].circBuffer[in].pubID = new->pubID;
    gettimeofday(&topicQs[topic].circBuffer[in].timestamp, NULL);	// set timestamp
    strcpy(topicQs[topic].circBuffer[in].message, new->message);	// give it a message

    topicQs[topic].in = (topicQs[topic].in + 1) % MAXENTRIES;   
    
    pthread_mutex_unlock(&(topicQs[topic].topicLock));
    return 0;
}

////////////////////////////////////////////////////////////////////
/* i: index of a topic in the topic queue */
int dequeue(int i) {

    pthread_mutex_lock(&topicQs[i].topicLock);
    if (topicQs[i].in == topicQs[i].out) {
        // buffer is empty, allow caller to try again
        pthread_mutex_unlock(&topicQs[i].topicLock);
        return -1;
    }


    int allowedDiff = 50000;	// .05 seconds

    int j, curIndex = topicQs[i].out;	// start from head of the queue
    for (j = 0; j < MAXENTRIES; j++) {

        struct timeval now;
        int timediff;

        gettimeofday(&now, NULL);

        timediff = difftime(now.tv_usec, topicQs[i].circBuffer[curIndex].timestamp.tv_usec);

        if ((topicQs[i].circBuffer[curIndex].pubID != -1) && (timediff > allowedDiff)) {
            // "removes" entries from the queue by reinitializing the entry

            printf("\n=== Entry About to be Dequed in topic %d", i);
            printEntry(curIndex, &topicQs[i].circBuffer[curIndex]);

            topicQs[i].circBuffer[curIndex].entryNum = 0;
            topicQs[i].circBuffer[curIndex].pubID = -1;
            strcpy(topicQs[i].circBuffer[curIndex].message, "this entry has been dequeued");
            topicQs[i].out = (topicQs[i].out + 1) % MAXENTRIES;
            topicQs[i].topicCounter--;
        }
        curIndex = (curIndex + 1) % MAXENTRIES;

        // check if done, curindex or out catch up to in
        if (curIndex == topicQs[i].in) {
            break;
        }

    }
    pthread_mutex_unlock(&topicQs[i].topicLock);
    return 0;
}

////////////////////////////////////////////////////////////////////
int getEntry(int topic, int lastEntry, struct topicentry * entry) {

    pthread_mutex_lock(&topicQs[topic].topicLock);

    // -1 is no entry assigned
    if ((topicQs[topic].circBuffer[lastEntry+1].pubID > 0) && (lastEntry < MAXENTRIES)) {
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
    char response[8];

    // char rej[] = "reject";
    // 'connection' protocol

    // close irrelevant ends of pipe
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
            sprintf(response, "accept");
            sprintf(output, "Sending Response: %s", response);
            pubt_print(tid, pid, output);

            char topicStr[1024];
            sscanf(buffer, "%*s %*s %*s %s", topicStr);
            int topicID = atoi(topicStr) % MAXTOPICS;

            struct topicentry newEntry;
            strcpy(newEntry.message, "test msg");
            newEntry.pubID = pub->pubid;
            sprintf(output, "Attempting  entry of message '%s' on topic %d", newEntry.message, topicID);
            pubt_print(tid, pid, output);

            int tester = enqueue(topicID, &newEntry);
            while (tester < 0) {
                pubt_print(tid, pid, "Entry failed, buffer is full");
                pthread_yield();
                tester = enqueue(topicID, &newEntry);
            }
            pubt_print(tid, pid, "TopicEntry made succesfully");
            printTopicQueue(topicID);
            //printTopicQueue(topicID);

            int i;
            for (i = 0; i < MAXTOPICS; i++) {
                if (pub->topicList[i] != -1) {
                    continue;
                }
                pub->topicList[i] = topicID;
                break;
            }
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

    int tid = (int)pthread_self();
    int pid = sub->pid;
    int lastEntryRead = 0;	// self explanatory
    char buffer[QUACKSIZE];
    char output[256];		// server display msg
    char response[8];
   
    close(sub->quacker_to_sub[0]);
    close(sub->sub_to_quacker[1]);

    subt_print(tid, pid, "Sub thread running");
    sprintf(output, "Listening for subscriber %d", sub->pid);
    subt_print(tid, pid, output);
    read(sub->sub_to_quacker[0], buffer, 1024);     // read from input of entry
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
                pthread_mutex_lock(&lock);              // no shared data
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

            sprintf(output, "Wants to subscribe to topic %d, retrieving posts:", topicID);
            subt_print(tid, pid, output);
            // assign iterested topics to its connection record
            int i;
            for (i = 0; i < MAXTOPICS; i++) {
                if (sub->topicList[i] != -1) {
                    continue;
                }
                sub->topicList[i] = topicID;
                break;
            }

            // proceed to read the 'last read entry' of the topic for this subscriber
            struct topicentry entryToRead;
            entryToRead.pubID = -3;			// used to indicate empty queue

            int retVal = getEntry(topicID, lastEntryRead, &entryToRead);
            if (retVal == 1) {
                // read value that is lastentry+1
                sprintf(output, "The next entry had a valid topic to read at %d+1", lastEntryRead);
                subt_print(tid, pid, output);
                printEntry((lastEntryRead+1), &entryToRead);
                lastEntryRead++;
            }
            else if (entryToRead.pubID != -3) {	// entrytoread is modified if found entry
                // read value - ret value is index of entry
                sprintf(output, "Next entry available to read was at buffer index %d", retVal);
                subt_print(tid, pid, output);
                printEntry(retVal, &entryToRead);
                lastEntryRead = retVal;

            }
            else {
                // ret val should be negative
                sprintf(output, "No valid entry in the topics queue, the last position was %d", -retVal);
                subt_print(tid, pid, output);
                printEntry(-retVal, &topicQs[topicID].circBuffer[-retVal]);
                lastEntryRead = -retVal;
            }
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

    } while (1);

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


    while (allTerm != 1) {

        for (i = 0; i < MAXTOPICS; i++) {
            if (topicQs[i].topicCounter != 0) {
                //while (repeatFlag == -1) {
                dequeue(i);
                //}
            }
        }
    }

    printf("\n[Delete Protocol] All processes have terminated. Ending timed delete protocol\n");
    /* Activate the deque function. */
    return NULL;
}

////////////////////////////////////////////////////////////////////
void printTopicQueue(int index) {

    printf("\n===== Printing Topic ID: %d\n\tin position of buffer: %d\n\tout position of buffer: %d", index, topicQs[index].in, topicQs[index].out);
    printf("\n\tnumber of entries: %d\n", topicQs[index].topicCounter);
    int i;
    for (i = 0; i < MAXENTRIES; i++) {
        if (topicQs[index].circBuffer[i].entryNum != -1) {
            printEntry(i, &topicQs[index].circBuffer[i]);
        }
    }

}

// prints entry
void printEntry(int i, struct topicentry * cur) {

    struct timeval now;
    int timediff;

    gettimeofday(&now, NULL);
    timediff = difftime(now.tv_usec, cur->timestamp.tv_usec);

    printf("---for entry at circbuffer index %d\n", i);
    printf("entryNum: %d\n", cur->entryNum);
    printf("microseconds old: %d\n", timediff);
    printf("pubID: %d\n", cur->pubID);
    printf("message: %s\n", cur->message);
}

void pubt_print(int tid, int pid, char * str)
{
    printf("\n[Pub thread %d (for PID: %d)] %s\n", tid, pid, str);
}

void subt_print(int tid, int pid, char * str)
{
    printf("\n[Subscriber thread %d (for PID: %d)] %s\n", tid, pid, str);
}

