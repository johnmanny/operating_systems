/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description: part 1 to project 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

struct pub_process{
    pid_t pid;
    pthread_t pubTID;
    int pub_to_quacker[2];
    int quacker_to_pub[2];
    int connected;
    int terminated;
    int pubid;
};

struct sub_process{
    pid_t pid;
    pthread_t subTID;
    int sub_to_quacker[2];
    int quacker_to_sub[2];
    int connected;
    int terminated;
    int subid;

};

pthread_cond_t allConnected;
pthread_mutex_t lock;

void print_pubs(struct pub_process pubs[], int num_pubs);
void print_subs(struct sub_process subs[], int num_subs);
void* pubProxy(void * pubEntry);
void* subProxy(void * subEntry);

int main(int argc, char * argv[]){

    if(argc != 3) {
        fprintf(stderr, "Error, bad args.\n USAGE: startQuacker [num_publisheds] [num_subscribers]\n");
        exit(0);
    }

    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "ERROR: mutex init has failed");
    }
    int TOTAL_PUBS = atoi(argv[1]);
    printf("total pubs : %d\n", TOTAL_PUBS);
    int TOTAL_SUBS = atoi(argv[2]);
    printf("total subs : %d\n", TOTAL_SUBS);

    struct pub_process pubs[TOTAL_PUBS];
    struct sub_process subs[TOTAL_SUBS];

    pid_t pid;

    // parent creates pubs
    int i;
    for (i  = 0; i < TOTAL_PUBS; i++) {

	// pipe sets file descriptors ptq[0] read end, ptq[1] write end
        pipe(pubs[i].pub_to_quacker);
        pipe(pubs[i].quacker_to_pub);
        pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Fork Failed.\n");
            exit(0);
        }
        else if (pid > 0) {
            printf("Forked child with ID : %d\n", pid);
            pubs[i].pid = pid;
            pubs[i].connected = 0;
            pubs[i].terminated = 0;
            pubs[i].pubid = 0;
        }
	// parent gets the PID of the child and the PID value in the child is 0
        else if (pid == 0) {
            close(pubs[i].quacker_to_pub[1]);			// close writing to child for child
            close(pubs[i].pub_to_quacker[0]);			// close reading from child output
            char *pubExec = "./pubpart1-2";
            char *args[4];
            char fdarg[10];
            char parg[10];
            sprintf(fdarg, "%d", pubs[i].quacker_to_pub[0]);	// send read end of pipe as arg to pub
            sprintf(parg, "%d", pubs[i].pub_to_quacker[1]);
            args[0] = pubExec;
            args[1] = fdarg;
            args[2] = parg;
            args[3] = NULL;
                         
            execvp(pubExec, args);
            fprintf(stderr, "ERROR, publisher %d not executed with return code %d\n", getpid(), errno);
            break;
        }
    }

    // parent creates subs
    if (pid > 0) {

        for (i  = 0; i < TOTAL_SUBS; i++) {

            pipe(subs[i].sub_to_quacker);
            pipe(subs[i].quacker_to_sub);
            pid = fork();

            if (pid < 0) {
                fprintf(stderr, "Fork Failed.\n");
                exit(0);
            }
            else if (pid > 0) {
                printf("Forked child with ID : %d\n", pid);
                subs[i].pid = pid;
                subs[i].connected = 0;
                subs[i].terminated = 0;
                subs[i].subid = 0;   
            }
            else if (pid == 0) {
                char *subExec = "./subpart1-2";
                char *args[3];
                char parg[10];
                char fdarg[10];
                sprintf(fdarg, "%d", subs[i].quacker_to_sub[0]);	// send read end of pipe as arg to pub
                sprintf(parg, "%d", subs[i].sub_to_quacker[1]);
                args[0] = subExec;
                args[1] = fdarg;
                args[2] = parg;
                args[3] = NULL;
                         
                execvp(subExec, args);
                fprintf(stderr, "ERROR, subscriber %d not executed with return code %d\n", getpid(), errno);
                break;
            }
        }
        fprintf(stderr, "\n");
    }

    // all have been loaded so create proxy threads
    if (pid > 0) {
        int i, errorFlag;
        for (i = 0; i < TOTAL_PUBS; i++){

            errorFlag = pthread_create(&pubs[i].pubTID, NULL, &pubProxy, &pubs[i]);
            if (errorFlag != 0) {
                fprintf(stderr, "ERROR! Thread couldn't be created\n");
            }
        }
        for (i = 0; i < TOTAL_SUBS; i++) {
            errorFlag = pthread_create(&subs[i].subTID, NULL, &subProxy, &subs[i]);
            if (errorFlag != 0) {
                fprintf(stderr, "ERROR! Thread couldn't be created\n");
            }
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
	fprintf(stderr, "\tSERVER: all pubs and subs connected, beginning termination\n");
        pthread_cond_broadcast(&allConnected);

        // join all threads back together
	for (i = 0; i < TOTAL_PUBS; i++) {
            pthread_join(pubs[i].pubTID, NULL);
	}
        // print pubs after all terminated
        print_pubs(pubs, TOTAL_PUBS);
	for (i = 0; i < TOTAL_SUBS; i++) {
            pthread_join(subs[i].subTID, NULL);
	}
        print_subs(subs, TOTAL_SUBS);
    }

    // Wait for all child procs to complete to complete
    if (pid > 0) {
        int status = 0;
        pid_t childPid;
        while ((childPid = wait(&status)) > 0);
        fprintf(stdout, "\tALL PUBS & SUBS TERMINATED\n");

    }

    return 0;
}

void* pubProxy (void * pubEntry) {
 
    struct pub_process * pub = pubEntry;

    pthread_t tid = pthread_self();
    char buffer[1024];
    char acc[] = "accept";
    // char rej[] = "reject";
    char term[] = "terminate";
    // 'connection' protocol
    while (1) {
    
        read(pub->pub_to_quacker[0], buffer, 1024);	// read from input of entry
        fprintf(stderr, "\tSERVER: PUB thread %d got '%s' from PUB %d - ", (int)tid, buffer, pub->pid);

        if (strstr(buffer, "connect") != NULL) {
            char pubid[1024];
            sscanf(buffer, "%*s %s %*s", pubid);
            pub->pubid = atoi(pubid);
            write(pub->quacker_to_pub[1], acc, strlen(acc) + 1);
            fprintf(stderr, "sent accept\n");
        }
        else if (strcmp(buffer, "end") == 0) {
            write(pub->quacker_to_pub[1], acc, strlen(acc) + 1);
            fprintf(stderr, "sent accept\n");
            pub->connected = 1;
            fprintf(stderr, "\tSERVER: PUB thread %d waiting on connection signal\n", (int)tid);
            pthread_mutex_lock(&lock);		// this doesn't qualify as a critical section
            pthread_cond_wait(&allConnected, &lock);	// thread wait for condition of all connected
            pthread_mutex_unlock(&lock);
            break;
        }
        else {
            fprintf(stderr, "ERROR: something went wrong with reading pipe\n");
        }
    }

    // 'termination' protocol
    while (1) {
        read(pub->pub_to_quacker[0], buffer, 1024);
        fprintf(stderr, "\tSERVER: PUB thread %d got '%s' from PUB %d - ", (int)tid, buffer, pub->pid);
        if (strcmp(buffer, "terminate") == 0) {
            pub->terminated = 1;
            write(pub->quacker_to_pub[1], term, (strlen(term)+1));
            fprintf(stderr, "sent terminate\n");
            close(pub->quacker_to_pub[0]);
            close(pub->quacker_to_pub[1]);
            close(pub->pub_to_quacker[0]);
            close(pub->pub_to_quacker[1]);
            fprintf(stderr, "\tSERVER: PUB thread %d closed pipe connections to pub %d\n", (int)tid, pub->pid);
            break;
        }
        else {
            fprintf(stderr, "ERROR - msg sent not valid\n");
        }
    }


    return NULL;
}

void* subProxy(void * subEntry) {

    struct sub_process * sub = subEntry;

    pthread_t tid = pthread_self();
    char buffer[1024];
    char acc[] = "accept";
    // char rej[] = "reject";
    char term[] = "terminate";
    // 'connection' protocol
    while (1) {
    
        read(sub->sub_to_quacker[0], buffer, 1024);	// read from input of entry
        fprintf(stderr, "\tSERVER: SUB thread %d got '%s' from SUB %d - ", (int)tid, buffer, sub->pid);

        if (strstr(buffer, "connect") != NULL) {
            char subid[1024];
            sscanf(buffer, "%*s %s %*s", subid);
            sub->subid = atoi(subid);
            write(sub->quacker_to_sub[1], acc, strlen(acc) + 1);
            fprintf(stderr, "sent accept\n");
        }
        else if (strcmp(buffer, "end") == 0) {
            write(sub->quacker_to_sub[1], acc, strlen(acc) + 1);
            fprintf(stderr, "sent accept\n");
            sub->connected = 1;
            fprintf(stderr, "\tSERVER: SUB thread %d waiting on connection signal\n", (int)tid);
            pthread_mutex_lock(&lock);		// no shared data
	    pthread_cond_wait(&allConnected, &lock);	// thread wait for condition of all connected
            pthread_mutex_unlock(&lock);
            break;
        }
        else {
            fprintf(stderr, "ERROR: something went wrong with reading pipe\n");
        }
    }

    // 'termination' protocol
    while (1) {
        read(sub->sub_to_quacker[0], buffer, 1024);
        fprintf(stderr, "\tSERVER: SUB thread %d got '%s' from SUB %d - ", (int)tid, buffer, sub->pid);
        if (strcmp(buffer, "terminate") == 0) {
            sub->terminated = 1;
            write(sub->quacker_to_sub[1], term, (strlen(term)+1));
            fprintf(stderr, "sent terminate\n");
            close(sub->quacker_to_sub[0]);
            close(sub->quacker_to_sub[1]);
            close(sub->sub_to_quacker[0]);
            close(sub->sub_to_quacker[1]);
            fprintf(stderr, "\tSERVER: SUB thread %d closed pipe connections to sub %d\n", (int)tid, sub->pid);
            break;
        }
        else {
            fprintf(stderr, "ERROR - msg sent not valid\n");
        }
    }

    return NULL;
}


void print_pubs(struct pub_process pubs[], int num_pubs)
{
    fprintf(stderr,"\n---Printing Publishers:\n");
    int i;
    for(i = 0; i < num_pubs; i++) {
        fprintf(stdout, "For pub record %d:\n\tpid: %d\n\tpubid:  %d\n\tconnected: %d\n\tterminated: %d\n\tthreadID: %d\n", i, (int)pubs[i].pid, pubs[i].pubid, pubs[i].connected, pubs[i].terminated, (int)pubs[i].pubTID);
    }
    printf("\n");
}

void print_subs(struct sub_process subs[], int num_subs)
{
    fprintf(stderr, "\n---Printing Subscribers:\n");
    int i;
    for(i = 0; i < num_subs; i++) {
        fprintf(stdout, "For sub record %d:\n\tpid: %d\n\tpubid: %d\n\tconnected: %d\n\tterminated: %d\n\tthreadID: %d\n", i, (int)subs[i].pid, subs[i].subid, subs[i].connected, subs[i].terminated, (int)subs[i].subTID);
    }
    printf("\n");
}

