#ifndef ANAGRAM_H
#define ANAGRAM_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct StringList
{
	struct	StringList* Next;
	char *Word;
};

struct AnagramList
{
	struct StringList *Words;
	struct AnagramList *Next;
	char *Anagram;
};

struct StringList *MallocSList(char *word);

void AppendSList(struct StringList **head, struct StringList *node);

void FreeSList(struct StringList **node);

void PrintSList(FILE *file,struct StringList *node);

int SListCount(struct StringList *node);

struct AnagramList* MallocAList(char *word);

void FreeAList(struct AnagramList **node);

void PrintAList(FILE *file,struct AnagramList *node);

void AddWordAList(struct AnagramList **node, char *word);

#endif
