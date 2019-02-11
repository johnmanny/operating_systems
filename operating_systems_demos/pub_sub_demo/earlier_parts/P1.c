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

struct pub_process{
    pid_t pid;
    int pubid;
    int pub_to_quacker[2];
    int quacker_to_pub[2];
    int terminated;
};

struct sub_process{
    pid_t pid;
    int subid;
    int sub_to_quacker[2];
    int quacker_to_sub[2];
    int terminated;
};

void print_pubs(struct pub_process pubs[], int num_pubs);
void print_subs(struct sub_process subs[], int num_subs);
void listen(int TOTAL_PUBS, int TOTAL_SUBS, struct pub_process pubs[], struct sub_process subs[]);

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
            printf("Forked child with ID : %d\n", pid);
            pubs[i].pid = pid;
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

        for (i = 0; i < TOTAL_SUBS; i++) {

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
                         
                //fprintf(stderr, "SUBID %d READ END IN PARENT:: %d \n", getpid(), subs[i].quacker_to_sub[0]);
                execvp(subExec, args);
                fprintf(stderr, "ERROR, subscriber %d not executed with return code %d\n", getpid(), errno);
                break;
            }
        }
    }

    // Wait for all child procs to complete to complete
    if (pid > 0) {
        listen(TOTAL_PUBS, TOTAL_SUBS, pubs, subs);
        int status = 0;
        pid_t childPid;
        while ((childPid = wait(&status)) > 0);
        fprintf(stdout, "\tALL PROCESSES TERMINATED\n");
    }

    return 0;
}

// there is a chance that children send their message so fast the concurrent threads pick it up 
//   in the same if...else if...else if...conditional statements. not a HUGE problem though (yet)
void listen(int TOTAL_PUBS, int TOTAL_SUBS, struct pub_process pubs[], struct sub_process subs[]) {

    char buffer[1024];
    char acc[] = "accept";
    //char rej[] = "reject";
    char term[] = "terminate";
    int i, totalTerminated;
    while (1) {
        totalTerminated = 0;

        // loop for pubs
        for (i = 0; i < TOTAL_PUBS; i++) {

            if (pubs[i].terminated == 1) {
                totalTerminated++;
                continue;
            }

            read(pubs[i].pub_to_quacker[0], buffer, 1024); 	// read from server input
            fprintf(stderr, "\tSERVER: received '%s' from PUB %d - ", buffer, pubs[i].pid);

            if (strcmp(buffer, "terminate") == 0) {
                pubs[i].terminated = 1;
                write(pubs[i].quacker_to_pub[1], term, (strlen(term)+1));
                fprintf(stderr, "sent terminate\n");
                close(pubs[i].quacker_to_pub[1]);
                close(pubs[i].quacker_to_pub[0]);
                close(pubs[i].pub_to_quacker[1]);
                close(pubs[i].pub_to_quacker[0]);
                fprintf(stderr, "\tSERVER: CLOSED ITS PIPE CONNECTIONS TO PUB %d\n", pubs[i].pid);
                totalTerminated++;
            }
            else if (strstr(buffer, "connect") != NULL) {
                char pubid[1024];
                sscanf(buffer, "%*s %s %*s", pubid);
                pubs[i].pubid = atoi(pubid);
            //else if (strcmp(buffer, "pub pubid connect") == 0) {
                write(pubs[i].quacker_to_pub[1], acc, (strlen(acc)+1));
                fprintf(stderr, "sent accept\n");
            }
            else if (strcmp(buffer, "end") == 0) {
                write(pubs[i].quacker_to_pub[1], acc, (strlen(acc)+1));
                fprintf(stderr, "sent accept\n");
            }
            else {
                fprintf(stderr, "\nERROR! SOMETHING WENT WRONG READING BUFFER FROM PIPES\n");
            }
        }

        // loop for checking subs
        for (i = 0; i < TOTAL_SUBS; i++) {

            if (subs[i].terminated == 1) {
                totalTerminated++;
                continue;
            }

            read(subs[i].sub_to_quacker[0], buffer, 1024);
            fprintf(stderr, "\tSERVER: received '%s' from SUB %d - ", buffer, subs[i].pid);

            if (strcmp(buffer, "terminate") == 0) {
                subs[i].terminated = 1;
                write(subs[i].quacker_to_sub[1], term, (strlen(term)+1));
                fprintf(stderr, "sent terminate\n");
                close(subs[i].quacker_to_sub[1]);
                close(subs[i].quacker_to_sub[0]);
                close(subs[i].sub_to_quacker[1]);
                close(subs[i].sub_to_quacker[0]);
                fprintf(stderr, "\tSERVER: CLOSED ITS PIPE CONNECTIONS TO SUB %d\n", subs[i].pid);
                totalTerminated++;
            }
            else if (strstr(buffer, "connect") != NULL) {
                char subid[1024];
                sscanf(buffer, "%*s %s %*s", subid);
                subs[i].subid = atoi(subid);
                write(subs[i].quacker_to_sub[1], acc, (strlen(acc)+1));
                fprintf(stderr, "sent accept\n");
            }
            else if (strcmp(buffer, "end") == 0) {
                write(subs[i].quacker_to_sub[1], acc, (strlen(acc)+1));
                fprintf(stderr, "sent accept\n");
            }
            else {
                fprintf(stderr, "SOMETHING WENT WRONG READING BUFFER FROM PIPES\n");
            }
        } 
        if (totalTerminated == (TOTAL_PUBS + TOTAL_SUBS)) {
            break;
        }
    }
}

void print_pubs(struct pub_process pubs[], int num_pubs)
{
    int i;
    for(i = 0; i < num_pubs; i++) {
        fprintf(stdout, "For pub record %d:\n\tpid: %d\n\tpubid:  %d\n\tterminated: %d\n",
                 i, (int)pubs[i].pid, pubs[i].pubid, pubs[i].terminated);
    }
    printf("\n");
}

void print_subs(struct sub_process subs[], int num_subs)
{
    int i;
    for(i = 0; i < num_subs; i++) {
        fprintf(stdout, "For sub record %d:\n\tpid: %d\n\tpubid: %d\n\tterminated: %d\n",
                 i, (int)subs[i].pid, subs[i].subid, subs[i].terminated);
    }
    printf("\n");
}

