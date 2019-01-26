/* Author: John Nemeth
   DuckID: jnemeth
   Assignment: Project0
   Description: implementation file for anagram.h
   Sources: geeks4geeks & wikipedia (for quicksort breakdown)
*/
 
#include "anagram.h"

/* uncapitalizes letters in array and places in newWord array */
void CopyLowercase(char * origWord, char * newWord) {
    int i = 0;
    for (i = 0; origWord[i]; i++) {
        newWord[i] = tolower(origWord[i]);
    }
}

/* quicksort - referenced from wikipedia and geeks4geeks */
void QuickSort(char * word,int i, int j) {
    if (i < j) {
        int v = Partition(word, i, j);		// find partition
        QuickSort(word, i, v - 1);		// recursive sort both sides of partition (and sub-partitions)
        QuickSort(word, v + 1, j);
    }
}

/* used to move pivot & swap values in quicksort - move values larger than
   pivot to position that will eventually be behind pivot by swapping values
   incrementally */
int Partition(char * word,int l,int h) {
    char pivot = word[h];			// arbitrarily choose ending value as pivot
    int i = l - 1, j;
    for (j = l; j < h; j++) {		// loop to highest index in partitioned sub-array
        if (word[j] < pivot) {
            i++;
            char toSave = word[j];
            word[j] = word[i];
            word[i] = toSave;            
        }
    }
    char toSave = word[h];			// swap pivot with last value moved (higher than pivot)
    word[h] = word[i + 1];
    word[i + 1] = toSave;

    return (i + 1);				// return index start position of items larger than pivot
}

/* allocates memory for new stringlist struct */
struct StringList *MallocSList(char *word) {
    struct StringList *SList = (struct StringList*)malloc(sizeof(struct StringList));
    SList->Word = (char *)malloc(sizeof(char) * (strlen(word) + 1));
    SList->Next = NULL;
    strcpy(SList->Word, word);

    return SList;
};

/* allocates memory for new anagramlist struct */
struct AnagramList* MallocAList(char *word) {
    struct AnagramList *AList = (struct AnagramList*)malloc(sizeof(struct AnagramList));
    AList->Next = NULL;

    AList->Anagram = (char*)malloc((1 + strlen(word)) * sizeof(char));		// create copy and arrange in sortable order
    strcpy(AList->Anagram, word);
    CopyLowercase(word, AList->Anagram);
    QuickSort(AList->Anagram, 0, strlen(word) - 1);

    AList->Words = MallocSList(word);						// create list of words for anagram

    return AList;
};

/* appends stringlist with new stringlist node */
void AppendSList(struct StringList **head, struct StringList *node) {
    if (*head == NULL) {
        *head = node;
    }
    else {
        struct StringList * curNode = *head;
        while (curNode->Next != NULL) {
            curNode = curNode->Next;
        }
        curNode->Next = node;
    }
}

/* iteratively frees all malloc'd memory in a stringlist - iterative to reduce function calls */
void FreeSList(struct StringList **node) {
    if ((*node) == NULL)
        return;
    struct StringList * curNode = *node;
    struct StringList * nextNode = NULL;
    do {
        nextNode = curNode->Next;
        free(curNode->Word);
        free(curNode);
        curNode = nextNode;
    } while (curNode != NULL);
}

/* iteratively prints all words in a stringList */
void PrintSList(FILE *file,struct StringList *node) {
    if (node != NULL) {
        struct StringList * curNode = node;
        while (curNode != NULL) {
            fprintf(file, "\t%s\n", curNode->Word);
            curNode = curNode->Next;
        }
    }
    else {
        return;
    }
}

/* returns count of strings in a stringlist */
int SListCount(struct StringList *node) {
    struct StringList * curNode = node;
    int i = 0;
    while (curNode) {
        i++;
        curNode = curNode->Next;
    }
    return i;
}

/* iteratively frees all malloc'd memory - iterative to reduce function calls */
void FreeAList(struct AnagramList **node) {
    if ((*node) == NULL)
        return;
    struct AnagramList *nextNode = NULL;
    struct AnagramList *curNode = *node;
    do {
        nextNode = curNode->Next;
        free(curNode->Anagram);
        FreeSList(&curNode->Words);
        free(curNode);
        curNode = nextNode;
    } while (curNode != NULL);
}

/* iteratively prints anagram list and stringlist */
void PrintAList(FILE *file,struct AnagramList *node) {
    if (node == NULL)
        return;
    struct AnagramList * curNode = node;
    int wordCount = SListCount(curNode->Words);
    while (curNode != NULL) {
        wordCount = SListCount(curNode->Words);
        if (wordCount > 1) {						// ensure anagrams printed have more than 1 word
            fprintf(file, "%s:%d\n", curNode->Anagram, wordCount);
            PrintSList(file, curNode->Words);
        }
        curNode = curNode->Next;
    }
}

/* takes first item in AList, iterates using pointers until correct position found*/
void AddWordAList(struct AnagramList **node, char *word) {
    if (*node != NULL) {
        char copyForSearch[256];
        strcpy(copyForSearch, word);					// copy to avoid uninitialized memory error
        CopyLowercase(word, copyForSearch);				// transform to lowercase
        QuickSort(copyForSearch, 0, (strlen(copyForSearch) - 1));	// sort using quicksort

        struct AnagramList * curNode = (*node);				// create placeholder nodes for finding positions
        struct AnagramList * prevNode = NULL;
        int len = strlen(copyForSearch) - 1;
        int len2 = strlen(curNode->Anagram) - 1;
        int i;

        /* this loop is designed to be faster than strcmp when finding matching anagrams and is accomplished by
           immediately moving to the next node to be analyzed when either 1. the lengths are different or 2. the
           first non-matching letter is found. it should take time off of analyzing the A-Z dictionary. */
        for (i = 0; i <= len; i++) {
            if ((len != len2) || (copyForSearch[i] != curNode->Anagram[i])) {
                prevNode = curNode;
                curNode = curNode->Next;
                if (curNode != NULL) {
                    len2 = strlen(curNode->Anagram) - 1;
                    i = -1;
                }
                else {							// at bottom of AList, so create new anagram
                    prevNode->Next = MallocAList(word);
                    return;
                }
            }
                    
        }	
                 
        struct StringList * curString = MallocSList(word);		// if above loop didn't return, then curNode is correct anagram
        AppendSList(curNode->Words, curString);
    }
    else {								// if no anagrams, create the first one
        *node = MallocAList(word);
    }
}
