/* Author: John Nemeth
   Description: main file for anagram program
   Sources: class material, lab0, lab1
*/

#include "anagram.h"

int main(int argc, char *argv[]) {
    FILE *input = NULL, *output = NULL;

    if (argc > 3 || argc < 1) {
        printf("usage is:\n\t'./program' - read from stdin, write stdout\n");
        printf("\t'./program input' - read from input, write stdout\n");
        printf("\t'./program inputfile outputfile' - read from input, write to output\n\tExiting...\n");
        exit(1);
    }

    if (argc == 2 || argc == 3) {				// for setting input file or stdin
        input = fopen(argv[1], "r");
        if (!input) {
            printf("%s input file not found! exiting...\n", argv[1]);
            exit(1);
        }
    }
    else
        input = stdin;

    if (argc == 3)						// for setting output file or stdout
        output = fopen(argv[2], "w");
    else
        output = stdout;

    struct AnagramList *AList = NULL;
    char buffer[256];
    char fromStream;
    int i = 0;
    while ((fromStream = getc(input)) != EOF) {			// loop for getting input
        if (fromStream == '\n') {
            buffer[i] = '\0';					// places null terminator in place of newline
            i = 0;						// reset index for input in buffer
            AddWordAList(&AList, buffer);			// add the word to the anagramlist
        }
        else {
            buffer[i] = fromStream;				// place charcter in the buffer
            i++;
        }
    }
    PrintAList(output, AList);
    FreeAList(&AList);
    fclose(input);
    fclose(output);

    return 0;
}
