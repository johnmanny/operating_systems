/* Author: John Nemeth
   Description: implements master control program, simulates process scheduling,
		launches workload
   Sources: linux manual entries, project material
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "p1fxns.h"

/* struct for a process */
struct Process
{
    struct Process* next;
    char * command;
    char ** args;
    pid_t childID;
};

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

/* gets the command and argument strings for a process and return the relevant node pointer */
struct Process *  GetCommandAndArgs(struct Process **node, char * buff) {

    struct Process * curNode = NULL;

    if (*node == NULL) {						// if process list is empty
        *node = (struct Process *)malloc(sizeof(struct Process));
        (*node)->command = NULL;
        (*node)->args = NULL;
        (*node)->next = NULL;
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

    return curNode;
}

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
        free(*head);
    }
    else {
        return;
    }
}

/* launch command and return child process id */
pid_t LaunchCom(char * com, char ** args) {

    pid_t comChildID;

    comChildID = fork();

    if (comChildID == 0) {		// is a child process
        // bad should not return, so execution beyond it is result of an error
        int bad = execvp(com, args);
        fprintf(stderr, "warning! execvp failed on command: '%s' - it returned: '%d'\n", com, bad);
        exit(0); 
    }
    if (comChildID < 0) {
        fprintf(stderr, "warning! creation of child process unsuccessful!\n");
    }
    if (comChildID > 0) {
        fprintf(stderr, "child process '%s' forked successfully! inside parent process...\n", com);
    }
    return comChildID;
}

/* halt parent process until child processes are finished */
void WaitProcesses(struct Process **head) {

    struct Process *curProcess = *head;

    while (curProcess != NULL) {
        int childStatus;
        waitpid(curProcess->childID, &childStatus, 0);
        fprintf(stderr, "Child process '%s', id '%d'  exited with status '%d'\n", curProcess->command, curProcess->childID, childStatus);
        curProcess = curProcess->next;
    }
}

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
    struct Process *head = NULL, *curNode = NULL;
    // NEED TO CHANGE INPUT TO ACCOUNT FOR ENDING OF FILE WITH NO NEWLINE? 
    while ((fromStream = getc(input)) != EOF) {				// loop for getting input
        if (fromStream == '\n') {
            buffer[buffIndex] = '\0';					// places null terminator in place of newline
            curNode = GetCommandAndArgs(&head, buffer);			// get com and args per line
            LaunchCom(curNode->command, curNode->args);			// launch commands as they're input

            buffIndex = 0;						// reset index for input in buffer
        }
        else {
            buffer[buffIndex] = fromStream;				// place charcter in the buffer
            buffIndex++;
        }
    }

    //PrintProcessList(head);
    WaitProcesses(&head);
    FreeProcessList(&head);

    fclose(input);

    return 0;
}
