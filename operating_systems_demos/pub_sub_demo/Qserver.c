/*
    Author: John Nemeth
    Description: Main control file for project. Recieves arguments as the number of publishers and
					subscribers that will connect. 
    Sources: academic material, project specifications
*/

#include "Qheader.h"

////////////////////////////////////////////////////////////////////
int main(int argc, char * argv[]){

	// error check for bad argument counts
    if(argc != 3) {
        fprintf(stderr, "Error, bad args.\n USAGE: startQuacker [num_publisheds] [num_subscribers]\n");
        exit(0);
    }

	// grab pub/sub counts from arguments
    int TOTAL_PUBS = atoi(argv[1]);
    int TOTAL_SUBS = atoi(argv[2]);
    printf("publishers passed as argument: %d\nsubscribers passed as argument: %d\n", TOTAL_PUBS, TOTAL_SUBS);

	// create arrays of passed pub and subs
    struct pub_process pubs[TOTAL_PUBS];
    struct sub_process subs[TOTAL_SUBS];

    pid_t pid;											// define server program process ID

    // parent creates pubs
    int i;
    for (i  = 0; i < TOTAL_PUBS; i++) {

		// pipe sets file descriptors ptq[0] read end, ptq[1] write end
        pipe(pubs[i].pub_to_quacker);
        pipe(pubs[i].quacker_to_pub);
        pid = fork();

        if (pid < 0) {									// error code returned
            fprintf(stderr, "Fork Failed.\n");
            exit(0);
        }
        else if (pid > 0) {								// if parent process
            printf("Forked pub with ID : %d\n", pid);
            pubs[i].pid = pid;							// assign process ID for forked process
            pubs[i].connected = 0;						// flag for when a pub sent connection signal and server accepted it
            pubs[i].terminated = 0;						// flag for when a pub has terminated
            pubs[i].pubid = 0;							// for assigning another unique id for publisher
            // init all topic entries to indicator number for which topic a certain publisher publishes to
            int j;
            for (j = 0; j < MAXTOPICS; j++) {
                pubs[i].topicList[j] = 0;
            }
        }
        else if (pid == 0) {							// if child process 
            close(pubs[i].quacker_to_pub[1]);			// close writing to child for child
            close(pubs[i].pub_to_quacker[0]);			// close reading from child output
            char *pubExec = "./pub";					// set program to execute
            char *args[5];
            char fdarg[10];
            char parg[10];
            char setNum[10];
            sprintf(fdarg, "%d", pubs[i].quacker_to_pub[0]);	// send read end of pipe as arg to pub
            sprintf(parg, "%d", pubs[i].pub_to_quacker[1]);		// send write end

            // set argument as unique publishing topic
            sprintf(setNum, "%d", i);

            args[0] = pubExec;
            args[1] = fdarg;
            args[2] = parg;
            args[3] = setNum;
            args[4] = NULL;
                         
            execvp(pubExec, args);						// execute publisher program with args
			// if successfull execution, should not reach this point
            fprintf(stderr, "ERROR, publisher %d not executed with return code %d\n", getpid(), errno);
			exit(0);
        }
    }

    if (pid > 0) {										// a check for ensuring current process is the parent
        for (i  = 0; i < TOTAL_SUBS; i++) {

			// create pipes and set file descriptors
            pipe(subs[i].sub_to_quacker);
            pipe(subs[i].quacker_to_sub);
            pid = fork();

            if (pid < 0) {								// error code returned
                fprintf(stderr, "Fork Failed.\n");
                exit(0);
            }
            else if (pid > 0) {							// parent process
                printf("Forked sub with ID : %d\n", pid);
                subs[i].pid = pid;						// assign id of child process to record
                subs[i].connected = 0;					// flag for setting when sub has sent connection signal and is connected
                subs[i].terminated = 0;					// flag for checking if sub is terminated
                subs[i].subid = 0;   					// secondary unique ID for subscriber

                // init all topic entries to 0
                int j;
                for (j = 0; j < MAXTOPICS; j++) {
                    subs[i].topicList[j] = 0;
                }
            }
            else if (pid == 0) {						// current process is a child process
                char *subExec = "./sub";				// name for subscriber executable
                char *args[4];
                char parg[10];
                char fdarg[10];
                sprintf(fdarg, "%d", subs[i].quacker_to_sub[0]);	// send read end of pipe as arg to sub
                sprintf(parg, "%d", subs[i].sub_to_quacker[1]);		// send write end of pipe as arg to sub

                args[0] = subExec;
                args[1] = fdarg;
                args[2] = parg;
                args[3] = NULL;
                         
                execvp(subExec, args);								// try to execute subscriber process and send args
				// if subscriber successfully executes, process should not reach this point
                fprintf(stderr, "ERROR, subscriber %d not executed with return code %d\n", getpid(), errno);
				exit(0);
            }
        }
        fprintf(stderr, "\n");										// formatting possible output
    }

    pid_t quacker_pid;
    // fork to create topic store/server process
    if (pid > 0) {													// ensures parent process used

        quacker_pid = fork();								
        if (quacker_pid == 0) {										// is child process, launch backend thread proxy routine 
            topicServer(pubs, TOTAL_PUBS, subs, TOTAL_SUBS);
            return 0;
        }
        else if (quacker_pid > 0) {
            // parent proc waits for children to be done
        }
        else if (quacker_pid < 0) {
            fprintf(stderr, "ERROR: quacker server couldn't be created!\n");
        }
    }

    // Wait for all child processes to complete for termination of server
    if (pid > 0) {
        int status = 0;
        pid_t childPid;
        while ((childPid = wait(&status)) > 0);
        fprintf(stdout, "\n[starter server] all pub and sub processes terminated\n");
    }

    return 0;
}


////////////////////////////////////////////////////////////////////
/* print functions - not currently used */
void print_pubs(struct pub_process pubs[], int num_pubs)
{
    fprintf(stderr,"\n---Printing Publishers:\n");
    int i;
    for(i = 0; i < num_pubs; i++) {
        fprintf(stdout, "For pub record %d:\n\tpid: %d\n\tpubid:  %d\n\tconnected: %d\n\tterminated: %d\n\tthreadID: %d\n", i, (int)pubs[i].pid, pubs[i].pubid, pubs[i].connected, pubs[i].terminated, (int)pubs[i].pubTID);
        fprintf(stdout, "\ttopicsList: ");
        int j;
        for (j = 0; j < MAXTOPICS; j++) {
            if (pubs[i].topicList[j] != -1) {
                fprintf(stdout, "%d ", pubs[i].topicList[j]);
            }
            printf("\n");
            break;
        }
    }

    printf("\n");
}

void print_subs(struct sub_process subs[], int num_subs)
{
    fprintf(stderr, "\n---Printing Subscribers:\n");
    int i;
    for(i = 0; i < num_subs; i++) {
        fprintf(stdout, "For sub record %d:\n\tpid: %d\n\tsubid: %d\n\tconnected: %d\n\tterminated: %d\n\tthreadID: %d\n", i, (int)subs[i].pid, subs[i].subid, subs[i].connected, subs[i].terminated, (int)subs[i].subTID);
        fprintf(stdout, "\ttopicsList: ");
        int j;
        for (j = 0; j < MAXTOPICS; j++) {
            if (subs[i].topicList[j] != -1) {
                fprintf(stdout, "%d ", subs[i].topicList[j]);
            }
            printf("\n");
            break;
        }
    }
    printf("\n");
}
