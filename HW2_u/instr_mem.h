/* EC535 HW 2 -- Selena Liu 
    Implementation of instruction memory + linked list data structure
    for storing instructions & maintaining reasonable access time */

#ifndef INSTR_MEM
#define INSTR_MEM

#include <stdio.h>

typedef struct instrMem {
    int instrAddr;
    char instruction[50];
} instrMem;

// linked list implementation for instruction buffer
typedef struct node {
    instrMem *instruction;
    struct node *next;
    struct node *prev;
} node;

typedef struct linkedList {
    node *head;
    node *tail;
    int size;
} linkedList;

#endif

