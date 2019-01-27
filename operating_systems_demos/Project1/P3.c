/* Author: John Nemeth
   Description: addition of features: schedules process execution
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

int StartChild = 0;			// changes to 1 from signal SIGUSR1
int ExitMCP = 0;
int processCount = 0;			// total number of processes
int remainingProc = 0;			// remaining processes in queue
struct Process * runnProc  = NULL;
struct Process *PCBHead = NULL;		// create process control block head

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
/* signal handler to start children processes */
void BeginChild(int sig) {

    StartChild = 1;
}

/////////////////////////////////////////////////
/* signal handler to end MCP */
void EndMCP(int sig) {
    sigprocmask(SIG_BLOCK, &signalSet, NULL);
    fprintf(stderr, "MCP got SIGUSR2(exit)\n");

    ExitMCP = 1;

    sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
}

/////////////////////////////////////////////////
/* used for execute or cleanup */
void WaitAndExec(struct Process * cur) {

        while (StartChild == 0) {		// wait until told to run
            usleep(1);
        }
        fprintf(stderr, "PROCESS %s STARTING\n", cur->command);
        int bad = execvp(cur->command, cur->args);	// bad shouldn't return
        //*cur->status = EXIT;
        fprintf(stderr, "ERROR: execvp failed on command: '%s' - it returned: '%d'\n", cur->command, bad);
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
        fprintf(stderr, "   Child process '%s' forked successfully!\n", cur->command);
    }
    return comChildID;
}

/////////////////////////////////////////////////
/* signal handler for parent - uses alarms for scheduling */
void ParentSignalHandler (int sig) {


    sigprocmask(SIG_BLOCK, &signalSet, NULL);

    int childStatus = -100;
    int numsExited = 0;


    // evaluate currently running process
    fprintf(stderr, "\n---CURRENT PROCESS: ");
    switch(*runnProc->status) {

        case RUN:
            waitpid(*runnProc->childID, &childStatus, WNOHANG);
            // check if still running or if exited
            if (WIFEXITED(childStatus)) {
                fprintf(stderr, "%s - Exited during timeslice with status '%d'\n", runnProc->command, childStatus);
                *runnProc->status = EXIT;
                numsExited++;
                remainingProc--;
            }
            else {
                if (remainingProc == 1) {
                    fprintf(stderr, "%s - final remaining process (will not sent signal)\n", runnProc->command);
                    sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
                    return;
                }
                kill(*runnProc->childID, SIGSTOP);
                *runnProc->status = PAUSE;
                fprintf(stderr, "%s - sent stop\n", runnProc->command);
            }
            break;

        case PAUSE:
            //kill(PCB[curProcess].childID, SIGUSR1);
            fprintf(stderr, "'%s' THIS SHOULD NOT PRINT\n", runnProc->command);
            break;

        case EXIT:
            fprintf(stderr, "%s THIS SHOULD NOT PRINT\n", runnProc->command);
            numsExited++;
            break;

        case READY:			// for the first process
            fprintf(stderr, "NONE\n");
            fprintf(stderr, "   NEXT PROCESS:    %s - sent start\n", runnProc->command);
            *runnProc->childID = LaunchCom(runnProc);
            *runnProc->status = RUN;
             kill(*runnProc->childID, SIGUSR1);
            sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
            return;

        default:
            break;
    }

    fprintf(stderr, "   NEXT PROCESS:    ");
    // loop to find next process to run
    while (1) {
        if (numsExited >= processCount) {
            fprintf(stderr, "NONE - no process left\n");
            kill(getpid(), SIGUSR2);
            break;
        }

        if (runnProc->next == NULL) {
            runnProc = PCBHead;
        }
        else {
            runnProc = runnProc->next;
        }

        if (*runnProc->status == EXIT) {
            numsExited++;
            continue;
        }

        if (*runnProc->status == PAUSE) {
            kill(*runnProc->childID, SIGCONT);
            fprintf(stderr, "%s - sent continue\n", runnProc->command);
            *runnProc->status = RUN;
            break;
        }
        if (*runnProc->status == READY) {
            fprintf(stderr, "%s - sent start (new process)\n", runnProc->command);
            *runnProc->childID = LaunchCom(runnProc);
            *runnProc->status = RUN;
            kill(*runnProc->childID, SIGUSR1);
            break;
        }

    }

    sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
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
        fprintf(stderr, "\tstatus: %d, childID: %d", *cur->status, *cur->childID);
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
    *curNode->childID = (pid_t) 0;
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
/*  */
void WaitForChildren(struct Process **head) {

    struct Process * curProcess = *head;

    while (curProcess != NULL) {
        int childStatus;
        waitpid(*curProcess->childID, &childStatus, WUNTRACED | WCONTINUED);
        fprintf(stderr, "process: %s status: %d - ", curProcess->command, *curProcess->status);
        curProcess = curProcess->next;
    }
    fprintf(stderr, "\n\tParent continuing after receiving continue/stop/exit status from children\n\n");
}

/////////////////////////////////////////////////
/* runs the processes according to the schedules */
void RunProcessSch() {

    while (ExitMCP == 0) {
        alarm(2);
        pause();
    }
}

/////////////////////////////////////////////////
/*  */
void ExecuteProcesses() {

    sigemptyset(&signalSet);		// initialize signal set
    sigaddset(&signalSet, SIGCONT);	// 
    sigaddset(&signalSet, SIGUSR2);	// add SIGUSR1 to signal set
    sigaddset(&signalSet, SIGALRM);
    
    if (signal(SIGUSR1, BeginChild) == SIG_ERR) {
        fprintf(stderr, "Coudn't tie SIGUSR1 to the signal handler\n");
        exit(1);
    }
    if (signal(SIGUSR2, EndMCP) == SIG_ERR) {
        fprintf(stderr, "Coudn't tie SIGUSR2 to the signal handler\n");
        exit(1);
    }
    if (signal(SIGALRM, ParentSignalHandler) == SIG_ERR) {
        fprintf(stderr, "Coudn't tie SIGARLM to the signal handler\n");
        exit(1);
    }

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
    while ((fromStream = getc(input)) != EOF) {				// loop for getting input
        if (fromStream == '\n') {
            buffer[buffIndex] = '\0';					// places null terminator in place of newline
            processCount++;
            GetCommandAndArgs(&PCBHead, buffer);			// get com and args per line
            buffIndex = 0;						// reset index for input in buffer
        }
        else {
            buffer[buffIndex] = fromStream;				// place charcter in the buffer
            buffIndex++;
        }
    }

    runnProc = PCBHead;
    remainingProc = processCount;

    ExecuteProcesses();
    RunProcessSch();

    sigemptyset(&signalSet);

    FreeProcessList(&PCBHead);
    fclose(input);

    return 0;
}
