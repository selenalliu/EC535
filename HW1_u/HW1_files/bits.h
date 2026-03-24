/* EC535 HW 1
    Selena Liu*/

#ifndef BITS_H
#define BITS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

typedef struct Node {
    uint32_t inputNum;
    uint32_t bigEndian;
    char *encryptNum;
    struct Node *next;
    struct Node *prev;
} Node;

Node *createHead(uint32_t, uint32_t, char *);
Node *insertNodeSort(Node *, uint32_t, uint32_t, char *);

uint32_t ConvertEndianess(uint32_t);
char *EncryptXOR(uint32_t, char *); 

#endif