/* Author: John Nemeth
   Description: implements methods for controlling execution of designated programs
   Sources: class material, lab0, lab1, lab2
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "p1fxns.h"
#include <signal.h>

#define READY 0
#define RUN 1
#define PAUSE 2
#define EXIT 3
sigset_t signalSet;

int RunIt = 0;	// changes to 1 from signal SIGUSR1
int StopIt = 0;

/* struct for a process */
struct Process
{
    struct Process* next;
    char * command;
    char ** args;
    pid_t *childID;
    int  *status;
};


/////////////////////////////////////////////////
void SignalHandler (int sig) {

    switch(sig) {
        case SIGUSR1:
            //fprintf(stderr, "\t%d got SIGUSR1\n", getpid());
            RunIt = 1;
            break;
        case SIGCONT:
            //fprintf(stderr, "\t%d got SIGCONT\n", getpid());
            break;
        default:
            fprintf(stderr, "\t%d got default signal handle\n", getpid());
            break;
    }

    return;  
}
/////////////////////////////////////////////////
/* print process information */
void PrintProcessList(struct Process *node) {
    struct Process *cur = node;
    while (cur != NULL) {
        fprintf(stderr, "command: %s\n", cur->command);

        int argIndex = 0;						// print argument information
        for (argIndex = 0; cur->args[argIndex] != NULL; argIndex++) {
            fprintf(stderr, "\targ %d: %s\n", argIndex, cur->args[argIndex]);
        }
        fprintf(stderr, "\n");
        cur = cur->next;
    }
}

/////////////////////////////////////////////////
/* gets the command and argument strings for a process and return the relevant node pointer */
void GetCommandAndArgs(struct Process **node, char * buff) {

    struct Process * curNode = NULL;

    if (*node == NULL) {						// if process list is empty
        *node = (struct Process *)malloc(sizeof(struct Process));
        (*node)->command = NULL;
        (*node)->args = NULL;
        (*node)->next = NULL;
        (*node)->status = NULL;
        (*node)->childID = NULL;
        curNode = *node;
    }
    else {
        curNode = *node;
        struct Process * prevNode = NULL;
        while (curNode != NULL) {
            prevNode = curNode;
            curNode = curNode->next;
        }
        curNode = (struct Process *)malloc(sizeof(struct Process));
        prevNode->next = curNode;
        curNode->command = NULL;
        curNode->args = NULL;
        curNode->next = NULL;
        curNode->status = NULL;
        curNode->childID = NULL;
    }

    // determine if command has arguments
    char space = ' ';
    int spaceLoc = p1strchr(buff, space);			// to determine if command has arguments
    int i;
    if (spaceLoc != -1) {					// if has arguments
        curNode->command = (char *)malloc(sizeof(char) * (spaceLoc + 1));
        for (i = 0; i < spaceLoc; i++) {
            curNode->command[i] = buff[i];
        }
        curNode->command[i] = '\0';
    }
    else {							// buffer has no arguments
        curNode->command = (char *)malloc(sizeof(char) * (p1strlen(buff) + 1));
        p1strcpy(curNode->command, buff);			// copy entire buffer to command (no args)
    }

    // calculate variables for memory allocation
    int argumentCount = 1, maxArgSize = 0, curArgSize = 0;	// used for memory allocation counts
    char * spaceChecker = buff;					// pointer used for checking spaces
    while (*spaceChecker != '\0') {
        curArgSize++;
        if (curArgSize > maxArgSize)
            maxArgSize = curArgSize;
        if (*spaceChecker == ' ') {
            curArgSize = 0;
            argumentCount++;
        }
        spaceChecker++;						// increment char pointer by 1
    }

    // allocate and assign arguments to process->args - always place NULL pointer as final arg
    curNode->args = (char **)malloc(sizeof(char *) * (argumentCount + 1));
    int argPosTracker = 0;
    for (i = 0; i < argumentCount; i++) {
        curNode->args[i] = (char *)malloc(sizeof(char) * (maxArgSize + 1));
        argPosTracker = p1getword(buff, argPosTracker, curNode->args[i]);
    }
    curNode->args[argumentCount] = NULL;
    curNode->childID = (pid_t*)malloc(sizeof(pid_t));
    curNode->status = (int*)malloc(sizeof(int));
    *curNode->status = READY;

}

/////////////////////////////////////////////////
/* recursively free memory allocated to process list */
void FreeProcessList(struct Process **head) {
    if (*head != NULL) {
        struct Process *next = (*head)->next; 
        FreeProcessList(&next);
        int argIndex = 0;						// print argument information
        for (argIndex = 0; (*head)->args[argIndex] != NULL; argIndex++) {
            free((*head)->args[argIndex]);
        }
        free((*head)->args);
        free((*head)->command);
        free((*head)->status);
        free((*head)->childID);
        free(*head);
    }
    else {
        return;
    }
}


/////////////////////////////////////////////////
/* used for execute or cleanup */
void WaitAndExec(struct Process * cur) {

        while (RunIt == 0) {		// wait while global isn't changed by signal
            usleep(1);
        }
        fprintf(stderr, "process %s, id: %d is starting\n", cur->command, getpid());
        *(cur->status) = RUN;
        int bad = execvp(cur->command, cur->args);	// bad shouldn't return
        fprintf(stderr, "warning! execvp failed on command: '%s' - it returned: '%d'\n", cur->command, bad);
        exit(0); 

}

/////////////////////////////////////////////////
/* launch command and return child process id 
     NOTE: because all memory is dynamically allocated,
       we only need to release memory at the end of main 
       and not in the children in case of error*/

pid_t LaunchCom(struct Process * cur) {

    pid_t comChildID;

    comChildID = fork();

    if (comChildID == 0) {		// is a child process

        WaitAndExec(cur);

    }
    if (comChildID < 0) {
        fprintf(stderr, "warning! creation of child process unsuccessful!\n");
    }
    if (comChildID > 0) {
        fprintf(stderr, "child process '%s' forked successfully! inside parent process...\n", cur->command);
    }
    return comChildID;
}


/////////////////////////////////////////////////
/* starts all forked children children */
void StartChildren(struct Process ** head) {

    struct Process * cur = *head;
//    fprintf(stderr, "processes ");
    while (cur != NULL) {
        kill(*cur->childID, SIGUSR1);
        fprintf(stderr, "%s, ", cur->command);
        cur = cur->next;
    }
    fprintf(stderr, "were sent SIGUSER1\n");
}

/////////////////////////////////////////////////
/* stops all forked children children */
void StopChildren(struct Process ** head) {

    struct Process * cur = *head;
    fprintf(stderr, "processes ");
    while (cur != NULL) {

        kill(*cur->childID, SIGSTOP);
        fprintf(stderr, "%s, ", cur->command);
        *cur->status = PAUSE;
        cur = cur->next;
    }
    fprintf(stderr, "were sent SIGSTOP\n");
}

/////////////////////////////////////////////////
/* continues all forked children children */
void ContChildren(struct Process ** head) {

    struct Process * cur = *head;
    fprintf(stderr, "processes ");
    while (cur != NULL) {

        kill(*cur->childID, SIGCONT);
        fprintf(stderr, "%s, ", cur->command);
        *cur->status = RUN;
        cur = cur->next;
    }
    fprintf(stderr, "were sent SIGCONT\n");
}

/////////////////////////////////////////////////
/*  */
void WaitForChildren(struct Process **head) {

    struct Process * curProcess = *head;

    while (curProcess != NULL) {
        int childStatus;
        waitpid(*curProcess->childID, &childStatus, WUNTRACED | WCONTINUED);
        //fprintf(stderr, "'%s', id '%d' status '%d' - ", curProcess->command, *curProcess->childID, childStatus);
        curProcess = curProcess->next;
    }
    fprintf(stderr, "\n\tParent continuing after receiving continue/stop/exit status from children\n\n");
}

/////////////////////////////////////////////////
/*  */
void ExecuteProcesses(struct Process **head) {

    sigemptyset(&signalSet);		// initialize signal set
    sigaddset(&signalSet, SIGUSR1);	// add SIGUSR1 to signal set
    
    if (signal(SIGUSR1, SignalHandler) == SIG_ERR) {
        fprintf(stderr, "Coudn't tie SIGUSR1 to the signal handler\n");
        exit(1);
    }
    if (signal(SIGCONT, SignalHandler) == SIG_ERR) {
        fprintf(stderr, "Coudn't tie SIGUSR1 to the signal handler\n");
        exit(1);
    }

    struct Process *curProcess = *head;

    while (curProcess != NULL) {
        *curProcess->childID = LaunchCom(curProcess);
        curProcess = curProcess->next;
    }

    sigemptyset(&signalSet);
}

/////////////////////////////////////////////////
/* main program - can take input from stdin or specified file */
int main(int argc, char *argv[]) {

    FILE *input = NULL;

    if (argc > 2 || argc < 1) {
        printf("usage is:\n\t'./program' - read from stdin (use <)\n");
        printf("\t'./program input' - read from input\n");
        exit(1);
    }

    if (argc == 2) {				// for setting input file or stdin
        input = fopen(argv[1], "r");
        if (!input) {
            printf("%s input file not found! exiting...\n", argv[1]);
            exit(1);
        }
    }
    else
        input = stdin;

    // perform input operations
    char buffer[256];
    char fromStream;
    int buffIndex = 0;
    struct Process *head = NULL;
    // NEED TO CHANGE INPUT TO ACCOUNT FOR ENDING OF FILE WITH NO NEWLINE? 
    while ((fromStream = getc(input)) != EOF) {				// loop for getting input
        if (fromStream == '\n') {
            buffer[buffIndex] = '\0';					// places null terminator in place of newline
            GetCommandAndArgs(&head, buffer);			// get com and args per line

            buffIndex = 0;						// reset index for input in buffer
        }
        else {
            buffer[buffIndex] = fromStream;				// place charcter in the buffer
            buffIndex++;
        }
    }

    ExecuteProcesses(&head);

    StartChildren(&head);
    StopChildren(&head);
    WaitForChildren(&head);
    ContChildren(&head);
    WaitForChildren(&head);

    FreeProcessList(&head);

    fclose(input);

    return 0;
}
