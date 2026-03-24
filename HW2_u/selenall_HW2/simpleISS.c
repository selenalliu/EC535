#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// define GP registers
static uint8_t registers[6]; // R1-R6, need to also handle signed values when needed (ADD)


static uint8_t cmpEqual = 1; // set 0 if equal, reset after 1 cycle?
static int execInstrCount = 0;
static int cycleCount = 0;
static int localMemHits = 0; // multiply memHits by 43 and subtract from cycle count?
static int loadStoreCount = 0;


static int instrAddrOffset = 0; // offset for instruction addresses (line # vs. actual address)
static int currentInstr = 0; // current instruction index (in term of line #), for looping through instructions later

typedef struct InstructionMem {
    unsigned int instrAddr;
    char instruction[25];
} InstructionMem;

void MOV(uint8_t Rn, int8_t num); // MOV Rn, <num> -- 1 clock cycle
void ADD_R(uint8_t Rn, uint8_t Rm); // ADD Rn, Rm (R-type) -- 1 clock cycle
void ADD_I(uint8_t Rn, int8_t num); // ADD Rn, <num> (I-type) -- 1 clock cycle
void CMP(uint8_t Rn, uint8_t Rm); // CMP Rn, Rm (set an equality flag) -- 1 clock cycle
void JE(uint16_t address); // JE <Address> if last comparison resulting in equality -- 1 clock cycle
void JMP(uint16_t address); // JMP <Address> -- 1 clock cycle
void LD(uint8_t Rn, uint8_t Rm); // LD Rn, [Rm] (loads from address stored in Rm into Rn) -- if 
void ST(uint8_t Rm, uint8_t Rn); // ST [Rm], Rn (stores contents of Rn into mem addr in Rm)

int checkRegNum(char *operand); // check which register is being used

/* Implement simple cache */
/* 256B, byte addressing */
static uint8_t cache[256][2]; // 256B cache, with "dirty" flag (already accessed before), 0 = clean, 1 = dirty

int main(int argc, char *argv[]) {

    if (argc != 2) {
        return -1;
    }

    char line[20];

    unsigned int arrSize = 100;
    InstructionMem * instrMem = (InstructionMem *)malloc(sizeof(InstructionMem) * arrSize);
    if (instrMem == NULL) {
        fputs("Memory allocation failed.\n", stderr);
        return -1;
    }

    FILE *asmFile;
    asmFile = fopen(argv[1], "r");

    if (asmFile == NULL) {
        fputs("File not found.\n", stderr);
		return -1;
	}


    /* Read through file first line by line, copying to a large instruction buffer, indexed by instruction type */
    /* Parse insturctions*/
    /* Format: [Instr Addr] [Instr Type] [Operand(s)]*/

    int lineCount = 0;
    while (fgets(line, sizeof(line), asmFile) != NULL) {    
        // sscanf(line, "%u %[^\n]", &instrMem[lineCount].instrAddr, instrMem[lineCount].instruction); // read line into struct
        instrMem[lineCount].instrAddr = atoi(strtok(line, " \t")); // read instruction address
        strcpy(instrMem[lineCount].instruction, strtok(NULL, "\n")); // read instruction
        if (lineCount >= arrSize) {
            // reallocate array of instrMem structs
            arrSize *= 2; // double size of array
            instrMem = (InstructionMem *)realloc(instrMem, sizeof(InstructionMem) * arrSize);
            if (instrMem == NULL) {
                fputs("Memory reallocation failed.\n", stderr);
                return -1;
            }
        }
        lineCount++;
    }
    
    // calculate instruction offset
    instrAddrOffset = instrMem[0].instrAddr;

    /* Read through instruction buffer*/
    /* Identify instruction type (ADD will require checking operands to determine if R/I-type)*/
    /* Call respective functions to instruction call */
    /* Keep track of instruction address for jumps */    

    while (currentInstr < lineCount) {
        // parse instruction
        char instructionCopy[50]; // copy of instruction for parsing
        char instructionCopyCopy[50]; // copy of instruction for error printing
        strcpy(instructionCopy, instrMem[currentInstr].instruction);
        strcpy(instructionCopyCopy, instructionCopy);
        char *instrType = strtok(instructionCopy, " \t");
        char *operand1 = strtok(NULL, " \t"); // contine parsing same string (\t for tabs)
        char *operand2 = strtok(NULL, " \t");

        // // reset cmpEqual flag if not a JE instruction
        // if (strcmp(instrType, "JE") != 0) {
        //     cmpEqual = 1; // reset cmpEqual flag to not equal
        // }

        if (strcmp(instrType, "MOV") == 0) {
            // get Rn and num
            int8_t Rn = checkRegNum(operand1);
            if (Rn == -1) {
                unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                printf("%d %s\n", instrAddr, instructionCopyCopy);
                return -1;
            }
            int8_t num = atoi(operand2);
            MOV(Rn, num);
        } else if (strcmp(instrType, "ADD") == 0) {
            if (operand2[0] != 'R') {
                int8_t Rn = checkRegNum(operand1);
                if (Rn == -1) {
                    unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                    printf("%d %s\n", instrAddr, instructionCopyCopy);
                    return -1;
                }
                int8_t num = atoi(operand2);
                ADD_I(Rn, num);
            } else {
                operand2[2] = '\0';
                int8_t Rn = checkRegNum(operand1);
                int8_t Rm = checkRegNum(operand2);
                if (Rn == -1 || Rm == -1) {
                    unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                    printf("%d %s\n", instrAddr, instructionCopyCopy);
                    return -1;
                }
                ADD_R(Rn, Rm);
            }
        } else if (strcmp(instrType, "CMP") == 0) {
            operand2[2] = '\0';
            int8_t Rn = checkRegNum(operand1);
            int8_t Rm = checkRegNum(operand2);
            if (Rn == -1 || Rm == -1) {
                unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                printf("%d %s\n", instrAddr, instructionCopyCopy);
                return -1;
            }
            CMP(Rn, Rm);
        } else if (strcmp(instrType, "JE") == 0) {
            uint16_t address = atoi(operand1);
            JE(address);
        } else if (strcmp(instrType, "JMP") == 0) {
            uint16_t address = atoi(operand1);
            JMP(address);
        } else if (strcmp(instrType, "LD") == 0) {
            operand2[4] = '\0';
            int8_t Rn = checkRegNum(operand1);
            int8_t Rm = checkRegNum(operand2);   

            if (Rn == -1 || Rm == -1) {
                unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                printf("%d %s\n", instrAddr, instructionCopyCopy);
                return -1;
            }
            LD(Rn, Rm);
        } else if (strcmp(instrType, "ST") == 0) {
            operand2[2] = '\0';
            int8_t Rm = checkRegNum(operand1);
            int8_t Rn = checkRegNum(operand2);
            
            if (Rm == -1 || Rn == -1) {
                unsigned int instrAddr = instrMem[currentInstr].instrAddr;
                printf("%d %s\n", instrAddr, instructionCopyCopy);
                return -1;
            }
            ST(Rm, Rn);
        } else {
            unsigned int instrAddr = instrMem[currentInstr].instrAddr;
            printf("%d %s\n", instrAddr, instructionCopyCopy);
            return -1;
        }
        currentInstr++;
    }

    // free(instrMem);
    fclose(asmFile);

    printf("Total number of executed instructions: %d\n", execInstrCount);
    printf("Total number of clock cycles: %d\n", cycleCount);
    printf("Number of hits to local memory: %d\n", localMemHits);
    printf("Total number of executed LD/ST instructions: %d\n", loadStoreCount);

    return 0;
}

void MOV(uint8_t Rn, int8_t num) {
    registers[Rn] = num;
    execInstrCount++;
    cycleCount++;
}

void ADD_R(uint8_t Rn, uint8_t Rm) {
    registers[Rn] += registers[Rm];
    execInstrCount++;
    cycleCount++;
}

void ADD_I(uint8_t Rn, int8_t num) {
    registers[Rn] += num;
    execInstrCount++;
    cycleCount++;
}

void CMP(uint8_t Rn, uint8_t Rm) {
    if (registers[Rn] == registers[Rm]) {
        cmpEqual = 0;
    } else {
        cmpEqual = 1;
    }
    execInstrCount++;
    cycleCount++;
}

void JE(uint16_t address) {
    if (cmpEqual != 0) {
        cycleCount++;
        execInstrCount++;
    } else {
        JMP(address);
    }
    cmpEqual = 1; // reset cmpEqual flag to not equal
}

void JMP(uint16_t address) {
    // jump to address
    currentInstr = address - instrAddrOffset - 1; // subtract 1 to account for incrementing at end of loop
    execInstrCount++;
    cycleCount++;
}

void LD(uint8_t Rn, uint8_t Rm) {
    uint8_t addr = registers[Rm];

    if (cache[addr][1] != 0) {
        localMemHits++;
        cycleCount += 2;
    } else {
        cache[addr][1] = 1;
        cycleCount += 45;
    }

    registers[Rn] = cache[addr][0]; // load value from memory address stored in Rm into Rn
    loadStoreCount++;
    execInstrCount++;
}

void ST(uint8_t Rm, uint8_t Rn) {
    uint8_t addr = registers[Rm];

    if (cache[addr][1] == 0) {
        cache[addr][1] = 1; // set dirty flag
        cycleCount += 45;
    } else {
        localMemHits++;
        cycleCount += 2;
    }

    cache[addr][0] = registers[Rn]; // store value from Rn into memory address stored in Rm
   
    loadStoreCount++;
    execInstrCount++;
}

int checkRegNum(char *operand) {
    if ((strcmp(operand, "R1,") == 0) || (strcmp(operand, "R1") == 0) || (strcmp(operand, "[R1]") == 0) || (strcmp(operand, "[R1],") == 0)) {
        return 0;
    } else if ((strcmp(operand, "R2,") == 0) || (strcmp(operand, "R2") == 0) || (strcmp(operand, "[R2]") == 0) || (strcmp(operand, "[R2],") == 0)) {
        return 1;
    } else if ((strcmp(operand, "R3,") == 0) || (strcmp(operand, "R3") == 0) || (strcmp(operand, "[R3]") == 0) || (strcmp(operand, "[R3],") == 0)) {
        return 2;
    } else if ((strcmp(operand, "R4,") == 0) || (strcmp(operand, "R4") == 0) || (strcmp(operand, "[R4]") == 0) || (strcmp(operand, "[R4],") == 0)) {
        return 3;
    } else if ((strcmp(operand, "R5,") == 0) || (strcmp(operand, "R5") == 0) || (strcmp(operand, "[R5]") == 0) || (strcmp(operand, "[R5],") == 0)) {
        return 4;
    } else if ((strcmp(operand, "R6,") == 0) || (strcmp(operand, "R6") == 0) || (strcmp(operand, "[R6]") == 0) || (strcmp(operand, "[R6],") == 0)) {
        return 5;
    } else {
        return -1;
    }
}
