/* EC535 HW 1
    Selena Liu*/

#include "bits.h"

int main (int argc, char* argv[]) {
    // check for correct number of arguments
    if (argc != 4) {
        printf("Invalid input.\n");
        return -1;
    }

    // assign arguments to variables
    char * key = argv[3];

    FILE * infile;
    infile = fopen(argv[1], "r");

    FILE * outfile;
    outfile = fopen(argv[2], "w");

    // check if files were created successfully
    if (infile == NULL || outfile == NULL) {
        printf("Error in creating file");
        return -1;
    }

    char line[50];

    Node *head = NULL;

    // read input file and insert into linked list
    while (fgets(line, sizeof(line), infile) != NULL) {    
        uint32_t inputNum;
        sscanf(line, " %u", &inputNum);
        uint32_t bigEndian = ConvertEndianess(inputNum);
        char *encryptNum = EncryptXOR(bigEndian, key);

        head = insertNodeSort(head, inputNum, bigEndian, encryptNum);
    }

    // print linked list to output file
    while (head != NULL) {
        Node *temp = head;
        fprintf(outfile, "%u %08x %s\n\n", temp->inputNum, temp->bigEndian, temp->encryptNum);
        head = head->next;
        free(temp->encryptNum);
        free(temp);
    }

    fclose(infile);
    fclose(outfile);

    return 0;
}
