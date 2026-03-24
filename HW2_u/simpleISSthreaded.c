// an attempt to implement pthreading

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

// #include <time.h> // temp for testing

// define GP registers
static uint8_t registers[6]; // R1-R6, need to also handle signed values when needed (ADD)


static uint8_t cmpEqual = 1; // set 0 if equal, reset after 1 cycle?
static int execInstrCount = 0;
static int cycleCount = 0;
static uint8_t localMemHits = 0; // multiply memHits by 43 and subtract from cycle count?
static uint8_t loadStoreCount = 0;


static int instrAddrOffset = 0; // offset for instruction addresses (line # vs. actual address)
static int lineCount = 0;   // number of lines in the file
static int currentInstr = 0; // current instruction index (in term of line #), for looping through instructions later

typedef struct InstructionMem {
    int instrAddr;
    char instruction[50];
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

pthread_mutex_t reg_mutexes[6]; // mutexes for registers
pthread_mutex_t cache_mutexes[256]; // mutex for cache
pthread_mutex_t instr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t instr_barrier;

void *execute_instructions(void *arg);

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        return -1;
    }

    pthread_t exec_thread;

    // init mutexes and barrier
    for (int i = 0; i < 6; i++) {
        pthread_mutex_init(&reg_mutexes[i], NULL);
    }
    for (int i = 0; i < 256; i++) {
        pthread_mutex_init(&cache_mutexes[i], NULL);
    }
    pthread_barrier_init(&instr_barrier, NULL, 2);
    
    // file reading
    char line[50];

    unsigned int arrSize = 100;
    InstructionMem *instrMem = (InstructionMem *)malloc(sizeof(InstructionMem) * arrSize);
    if (instrMem == NULL) {
        fputs("Memory allocation failed.\n", stderr);
        return -1;
    }

    FILE *asmFile = fopen(argv[1], "r");
    if (asmFile == NULL) {
        fputs("File open failed.\n", stderr);
        return -1;
    }
 
    while (fgets(line, sizeof(line), asmFile)) {
        sscanf(line, "%d %[^\n]", &instrMem[lineCount].instrAddr, instrMem[lineCount].instruction);
        if (lineCount >= arrSize) {
            // reallocate array of instrMem structs
            arrSize *= 2; // double size of array
            instrMem = (InstructionMem *)realloc(instrMem, sizeof(InstructionMem) * arrSize);
            if (instrMem == NULL) {
                fputs("Memory reallocation failed.\n", stderr);
                return -1;
            }
        }
        lineCount++; // increment line count
    }

    fclose(asmFile);
    instrAddrOffset = instrMem[0].instrAddr;

    pthread_t threads[8];
    for (int i = 0; i < 8; i++) {
        pthread_create(&threads[i], NULL, execute_instructions, instrMem);
    }
    for (int i = 0; i < 8; i++) {
        pthread_join(threads[i], NULL);
    }

    // // start execution in a separate thread
    // pthread_create(&exec_thread, NULL, execute_instructions, instrMem);
    // pthread_join(exec_thread, NULL);

    free(instrMem);

    // destroy mutexes and barrier
    for (int i = 0; i < 6; i++) {
        pthread_mutex_destroy(&reg_mutexes[i]);
    }
    for (int i = 0; i < 256; i++) {
        pthread_mutex_destroy(&cache_mutexes[i]);
    }
    pthread_barrier_destroy(&instr_barrier);

    printf("Executed Instructions: %d\n", execInstrCount);
    printf("Total Clock Cycles: %d\n", cycleCount);
    printf("Memory Hits: %d\n", localMemHits);
    printf("Executed LD/ST Instructions: %d\n", loadStoreCount);

    return 0;
}

void *execute_instructions(void *arg) {
    InstructionMem *instrMem = (InstructionMem *)arg;

    while (currentInstr < lineCount) {
        pthread_mutex_lock(&instr_mutex);
        // parse instruction
        char instructionCopy[50]; // copy of instruction for parsing
        char instructionCopyCopy[50]; // copy of instruction for error printing
        strcpy(instructionCopy, instrMem[currentInstr].instruction);
        strcpy(instructionCopyCopy, instructionCopy);
        char *instrType = strtok(instructionCopy, " \t");
        char *operand1 = strtok(NULL, " \t"); // contine parsing same string (\t for tabs)
        char *operand2 = strtok(NULL, " \t");

        int instrAddr = instrMem[currentInstr].instrAddr;

        printf("Instruction address: %d\n", instrAddr);
        printf("Instruction: %s %s %s\n", instrType, operand1, operand2);

        pthread_mutex_unlock(&instr_mutex);

        if (strcmp(instrType, "MOV") == 0) {
            // get Rn and num
            int8_t Rn = checkRegNum(operand1);
            if (Rn == -1) {
                printf("Invalid register number.\n");
                exit(-1);
            }
            int8_t num = atoi(operand2);
            MOV(Rn, num);
        } else if (strcmp(instrType, "ADD") == 0) {
            if (operand2[0] != 'R') {
                int8_t Rn = checkRegNum(operand1);
                if (Rn == -1) {
                    printf("Invalid register number.\n");
                    exit(-1);
                }
                int8_t num = atoi(operand2);
                ADD_I(Rn, num);
            } else {
                operand2[strlen(operand2) - 1] = '\0';
                int8_t Rn = checkRegNum(operand1);
                int8_t Rm = checkRegNum(operand2);
                if (Rn == -1 || Rm == -1) {
                    printf("Invalid register number.\n");
                    exit(-1);
                }
                ADD_R(Rn, Rm);
            }
        } else if (strcmp(instrType, "CMP") == 0) {
            operand2[strlen(operand2) - 1] = '\0';
            int8_t Rn = checkRegNum(operand1);
            int8_t Rm = checkRegNum(operand2);
            if (Rn == -1 || Rm == -1) {
                printf("Invalid register number.\n");
                exit(-1);
            }
            CMP(Rn, Rm);
        } else if (strcmp(instrType, "JE") == 0) {
            uint16_t address = atoi(operand1);
            pthread_barrier_wait(&instr_barrier);
            JE(address);
        } else if (strcmp(instrType, "JMP") == 0) {
            uint16_t address = atoi(operand1);
            pthread_barrier_wait(&instr_barrier);
            JMP(address);
        } else if (strcmp(instrType, "LD") == 0) {
            operand2[strlen(operand2) - 1] = '\0';
            int8_t Rn = checkRegNum(operand1);
            int8_t Rm = checkRegNum(operand2);     
            if (Rn == -1 || Rm == -1) {
                printf("Invalid register number.\n");
                exit(-1);
            }
            LD(Rn, Rm);
        } else if (strcmp(instrType, "ST") == 0) {
            operand2[strlen(operand2) - 1] = '\0';
            int8_t Rm = checkRegNum(operand1);
            int8_t Rn = checkRegNum(operand2);
            if (Rm == -1 || Rn == -1) {
                printf("Invalid register number.\n");
                exit(-1);
            }
            ST(Rm, Rn);
        } else {
            // int instrAddr = instrMem[currentInstr].instrAddr;
            printf("%d %s\n", instrAddr, instructionCopyCopy);
            exit(0);
        }        
        pthread_mutex_lock(&instr_mutex);
        currentInstr++;
        pthread_mutex_unlock(&instr_mutex);
    }

    return NULL;
}

void MOV(uint8_t Rn, int8_t num) {
    pthread_mutex_lock(&reg_mutexes[Rn]);
    registers[Rn] = num;
    execInstrCount++;
    cycleCount++;
    pthread_mutex_unlock(&reg_mutexes[Rn]);
}


void ADD_R(uint8_t Rn, uint8_t Rm) {
    pthread_mutex_lock(&reg_mutexes[Rn]);
    pthread_mutex_lock(&reg_mutexes[Rm]);
    registers[Rn] += registers[Rm];
    execInstrCount++;
    cycleCount++;
    pthread_mutex_unlock(&reg_mutexes[Rn]);
    pthread_mutex_unlock(&reg_mutexes[Rm]);
}

void ADD_I(uint8_t Rn, int8_t num) {
    pthread_mutex_lock(&reg_mutexes[Rn]);
    registers[Rn] += num;
    execInstrCount++;
    cycleCount++;
    pthread_mutex_unlock(&reg_mutexes[Rn]);
}

void CMP(uint8_t Rn, uint8_t Rm) {
    pthread_mutex_lock(&reg_mutexes[Rn]);
    pthread_mutex_lock(&reg_mutexes[Rm]);
    if (registers[Rn] == registers[Rm]) {
        cmpEqual = 0;
    } else {
        cmpEqual = 1;
    }
    execInstrCount++;
    cycleCount++;
    pthread_mutex_unlock(&reg_mutexes[Rn]);
    pthread_mutex_unlock(&reg_mutexes[Rm]);
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
    pthread_mutex_lock(&reg_mutexes[Rm]);
    uint8_t addr = registers[Rm];
    pthread_mutex_unlock(&reg_mutexes[Rm]);

    pthread_mutex_lock(&cache_mutexes[addr]); // lock cache
    pthread_mutex_lock(&reg_mutexes[Rn]); // lock register
    // load from memory
    registers[Rn] = cache[registers[Rm]][0]; // load value from memory address stored in Rm into Rn

    // printf("Memory address: %d\n", registers[Rm]);
    // printf("Value loaded: %d\n", registers[Rn]);

    if (cache[addr][1] == 0) {
        cache[addr][1] = 1; // set dirty flag
        cycleCount += 45;
    } else {
        localMemHits++;
        cycleCount += 2;
    }
    execInstrCount++;
    loadStoreCount++;
    pthread_mutex_unlock(&cache_mutexes[addr]); // unlock cache
    pthread_mutex_unlock(&reg_mutexes[Rn]); // unlock register
}

void ST(uint8_t Rm, uint8_t Rn) {
    pthread_mutex_lock(&reg_mutexes[Rm]);
    uint8_t addr = registers[Rm];
    pthread_mutex_unlock(&reg_mutexes[Rm]);

    pthread_mutex_lock(&cache_mutexes[addr]); // lock cache
    pthread_mutex_lock(&reg_mutexes[Rn]); // lock register
    // store to memory
    cache[addr][0] = registers[Rn]; // store value from Rn into memory address stored in Rm
   
    // printf("Memory address: %d\n", registers[Rm]);
    // printf("Value stored: %d\n", cache[registers[Rm]][0]);

    if (cache[addr][1] == 0) {
        cache[addr][1] = 1; // set dirty flag
        cycleCount += 45;
    } else {
        localMemHits++;
        cycleCount += 2;
    }
    execInstrCount++;
    loadStoreCount++;
    pthread_mutex_unlock(&cache_mutexes[addr]); // unlock cache
    pthread_mutex_unlock(&reg_mutexes[Rn]); // unlock register
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

