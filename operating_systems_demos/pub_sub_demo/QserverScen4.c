/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description: part 1 to project 2
*/

#include "QheaderScen4.h"

void print_pubs(struct pub_process pubs[], int num_pubs);
void print_subs(struct sub_process subs[], int num_subs);

////////////////////////////////////////////////////////////////////
int main(int argc, char * argv[]){

    if(argc != 3) {
        fprintf(stderr, "Error, bad args.\n USAGE: startQuacker [num_publisheds] [num_subscribers]\n");
        exit(0);
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
            printf("Forked pub with ID : %d\n", pid);
            pubs[i].pid = pid;
            pubs[i].connected = 0;
            pubs[i].terminated = 0;
            pubs[i].pubid = 0;
            // init all topic entries to indicator number
            int j;
            for (j = 0; j < MAXTOPICS; j++) {
                pubs[i].topicList[j] = 0;
            }
        }
	// parent gets the PID of the child and the PID value in the child is 0
        else if (pid == 0) {
            close(pubs[i].quacker_to_pub[1]);			// close writing to child for child
            close(pubs[i].pub_to_quacker[0]);			// close reading from child output
            char *pubExec = "./pubScen4";
            char *args[5];
            char fdarg[10];
            char parg[10];
            char setNum[10];
            sprintf(fdarg, "%d", pubs[i].quacker_to_pub[0]);	// send read end of pipe as arg to pub
            sprintf(parg, "%d", pubs[i].pub_to_quacker[1]);
            // determine what topics this pub will post to give iteration of creation (based on scenario4)
            if (i == 0)
                sprintf(setNum, "%d", 1);
            else if (i == 1)
                sprintf(setNum, "%d", 2);
            else 
                sprintf(setNum, "%d", 3);

            args[0] = pubExec;
            args[1] = fdarg;
            args[2] = parg;
            args[3] = setNum;
            args[4] = NULL;
                         
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
                printf("Forked sub with ID : %d\n", pid);
                subs[i].pid = pid;
                subs[i].connected = 0;
                subs[i].terminated = 0;
                subs[i].subid = 0;   
                // init all topic entries to 0
                int j;
                for (j = 0; j < MAXTOPICS; j++) {
                    subs[i].topicList[j] = 0;
                }
            }
            else if (pid == 0) {
                char *subExec = "./subScen4";
                char *args[3];
                char parg[10];
                char fdarg[10];
                char setNum[10];
                sprintf(fdarg, "%d", subs[i].quacker_to_sub[0]);	// send read end of pipe as arg to pub
                sprintf(parg, "%d", subs[i].sub_to_quacker[1]);
                if ((i == 0) || (i == 1))
                    sprintf(setNum, "%d", 1);
                else if ((i == 2) || (i == 3))
                    sprintf(setNum, "%d", 2);
                else 
                    sprintf(setNum, "%d", 3);

                args[0] = subExec;
                args[1] = fdarg;
                args[2] = parg;
                args[3] = setNum;
                args[4] = NULL;
                         
                execvp(subExec, args);
                fprintf(stderr, "ERROR, subscriber %d not executed with return code %d\n", getpid(), errno);
                break;
            }
        }
        fprintf(stderr, "\n");
    }

    pid_t quacker_pid;
    // fork to create topic store/server process
    if (pid > 0) {
        quacker_pid = fork();

        if (quacker_pid == 0) {
            // quacker proc created
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


    // Wait for all child procs to complete to complete
    if (pid > 0) {
        int status = 0;
        pid_t childPid;
        while ((childPid = wait(&status)) > 0);
        fprintf(stdout, "\n[starter server] all pub and sub processes terminated\n");
    }

    return 0;
}

////////////////////////////////////////////////////////////////////
/* print functions */
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
