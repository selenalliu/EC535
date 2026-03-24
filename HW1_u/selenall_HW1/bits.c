/* EC535 HW 1
    Selena Liu*/

#include "bits.h"

// create head node for linked list
Node *createHead(uint32_t inputNum, uint32_t bigEndian, char *encryptNum) {
    Node *head = (Node *)malloc(sizeof(Node));
    head->inputNum = inputNum;
    head->bigEndian = bigEndian;
    head->encryptNum = encryptNum;
    head->next = NULL;
    head->prev = NULL;
    return head;
}

// insert node in sorted order, return new head node
Node *insertNodeSort(Node *head, uint32_t inputNum, uint32_t bigEndian, char *encryptNum) {
    if (head == NULL) {
        return createHead(inputNum, bigEndian, encryptNum);
    }
    
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        return NULL;
    }
    newNode->inputNum = inputNum;
    newNode->bigEndian = bigEndian;
    newNode->encryptNum = encryptNum;

    Node *current = head;

    // check for duplicates because it is somehow printing duplicates only when i compile on eng grid and idk why
    while (current != NULL) {
        if (strcmp(encryptNum, current->encryptNum) == 0) {
            free(newNode); 
            return head;
        }
        current = current->next;
    }

    // reset current to head after checking for dupes
    current = head;

    // check for insert at start of list
    if (strcmp(encryptNum, current->encryptNum) < 0) {
        newNode->next = current;
        current->prev = newNode;
        return newNode; // return new head
    }

    // traverse the list to find the correct position
    while (current->next != NULL && strcmp(encryptNum, current->encryptNum) > 0) {
        current = current->next;
    }

    // insert before the current node
    if (strcmp(encryptNum, current->encryptNum) > 0) {
        current->next = newNode;
        newNode->prev = current;
    } else { // insert after current node
        newNode->next = current;
        newNode->prev = current->prev;
        if (current->prev != NULL) {
            current->prev->next = newNode;
        }
        current->prev = newNode;
    }

    return head;
}

// convert unsigned 32-bit integer in little-endian to hexadecimal big-endian format (byte-endianess)
uint32_t ConvertEndianess(uint32_t littleEnd) {
    uint32_t bigEnd = 0;
    
    // bit masking by byte with bitwise AND, setting appropriate bytes in bigEnd to bit masked value by using bitwise shift & OR
    bigEnd = ((littleEnd & 0x000000FF) << 24) | ((littleEnd & 0x0000FF00) << 8) | ((littleEnd & 0x00FF0000) >> 8) | ((littleEnd & 0xFF000000) >> 24);

    return bigEnd;    
    
}

// encrypt big-endian 32-bit integer with 4-byte key using XOR operation
char *EncryptXOR(uint32_t bigEnd, char *key) {
    char asciiRep[4 + 1];

    // convert every 8 bits to their corresponding ASCII representation
    sprintf(&asciiRep[0], "%c", (bigEnd & 0xFF000000) >> 24);
    sprintf(&asciiRep[1], "%c", (bigEnd & 0x00FF0000) >> 16);
    sprintf(&asciiRep[2], "%c", (bigEnd & 0x0000FF00) >> 8);
    sprintf(&asciiRep[3], "%c", bigEnd & 0x000000FF);

    char *output = (char *)malloc(4 + 1);
    
    for (int i = 0; i < 4; i++) {
        output[i] = asciiRep[i] ^ key[i];
    }
    output[4] = '\0';

    return output;
}