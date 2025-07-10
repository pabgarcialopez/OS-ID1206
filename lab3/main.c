#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PAGE_SIZE 256
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 256
#define PHYSICAL_MEMORY_SIZE 65536

const char *IN_FILE = "/Users/pablogarcialopez/CLionProjects/lab3/addresses.txt";
const char *OUT_FILE = "/Users/pablogarcialopez/CLionProjects/lab3/out.txt";
const char *CORRECT_FILE = "/Users/pablogarcialopez/CLionProjects/lab3/correct.txt";
const char *BIN_FILE = "/Users/pablogarcialopez/CLionProjects/lab3/BACKING_STORE.bin";

// Assume a global array to represent physical memory
// unit8_t represents a byte.
uint8_t physicalMemory[PHYSICAL_MEMORY_SIZE];

// TLB entry structure
struct TLBEntry {
    int pageNumber;
    int frameNumber;
    int FIFO;
};

// Page Table entry structure
struct PageTableEntry {
    int frameNumber;
    bool valid;
};

// TLB and Page Table arrays
struct TLBEntry TLB[TLB_SIZE];
struct PageTableEntry pageTable[PAGE_TABLE_SIZE];

void displayStatistics();

// Where to allocate page from virtual memory into physical memory
int frameAllocation = 0;

// Statistics variables
int pageFaultCount = 0;
int tlbHitCount = 0;
int totalTranslations = 0;

// Function to compare results with the correct output
bool compareResults() {
    FILE *outFile = fopen(OUT_FILE, "r");
    if (outFile == NULL) {
        perror("Error opening out.txt for comparison");
        return false;
    }

    FILE *correctFile = fopen(CORRECT_FILE, "r");
    if (correctFile == NULL) {
        perror("Error opening correct.txt for comparison");
        fclose(outFile);
        return false;
    }

    int isEqual = true;
    char outChar, correctChar;

    // Compare each character in the files
    while ((outChar = fgetc(outFile)) != EOF &&
           (correctChar = fgetc(correctFile)) != EOF) {

        if (outChar != correctChar) {
            isEqual = false;
            break;
        }
    }

    // Check for any remaining characters in either file
    if (fgetc(outFile) != EOF || fgetc(correctFile) != EOF)
        isEqual = false;

    // Close the files
    fclose(outFile);
    fclose(correctFile);

    return isEqual;
}

// Function to initialize TLB and Page Table
void initialize() {
    // Initialize TLB and Page Table entries
    for (int i = 0; i < TLB_SIZE; i++) {
        TLB[i].pageNumber = -1; // Invalid entry
        TLB[i].frameNumber = -1;
        TLB[i].FIFO = 0;
    }

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        pageTable[i].frameNumber = -1; // Invalid entry
        pageTable[i].valid = false;
    }
}

// Function that returns the index from the TLB that must be replaced (FIFO)
int fifoIndex() {

    int i;
    for(i = 0; i < TLB_SIZE; i++) {
        if(TLB[i].FIFO == 0)
            break;
    }

    if(i == TLB_SIZE) {
        perror("Error with FIFO index");
        exit(1);
    }

    return i;
}

// Updates TLB at entry specified by index.
void updateTLB(int index, int pageNumber, int frameNumber) {

    for(int i = 0; i < TLB_SIZE; i++)
        if(TLB[i].FIFO != 0)
            TLB[i].FIFO--;

    TLB[index].pageNumber = pageNumber;
    TLB[index].frameNumber = frameNumber;
    TLB[index].FIFO = TLB_SIZE - 1;
}

// Handles page faults
void handlePageFault(int pageNumber) {

    FILE *backingStore = fopen(BIN_FILE, "rb");
    if (backingStore == NULL) {
        perror("Error opening BACKING_STORE.bin");
        exit(1);
    }

    // Seek to the appropriate position in the BACKING_STORE.bin file
    fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);

    // Read the 256-byte page from the BACKING_STORE.bin file
    fread(&physicalMemory[pageNumber * PAGE_SIZE], sizeof(uint8_t), PAGE_SIZE, backingStore);

    // Close the BACKING_STORE.bin file
    fclose(backingStore);

    // Update the page table and TLB with the new entry.
    // We allocate the page from virtual memory into the first available frame.
    pageTable[pageNumber].frameNumber = frameAllocation % PAGE_TABLE_SIZE;
    pageTable[pageNumber].valid = true;
    updateTLB(fifoIndex(), pageNumber, frameAllocation);

    frameAllocation++;

    // Increment page fault count
    pageFaultCount++;
}

// Checks whether the frame number for page number exists in the TLB.
int checkTLB(int pageNumber) {

    int frameNumber = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].pageNumber == pageNumber) {
            // TLB hit
            frameNumber = TLB[i].frameNumber;
            tlbHitCount++;
            break;
        }
    }

    return frameNumber;
}

// Function that translates logical address into physical address.
int translateAddress(int logicalAddress) {
    // Extract page number and offset from logical address
    int pageNumber = (logicalAddress >> 8) & 0xFF; // Rightmost 8 bits
    int pageOffset = logicalAddress & 0xFF;        // Leftmost 8 bits

    // Check TLB for the frame number
    int frameNumber = checkTLB(pageNumber);

    // TLB miss -> consult Page Table
    if (frameNumber == -1) {

        // Check Page Table for the page number
        if (pageTable[pageNumber].valid) {

            // Page Table hit
            frameNumber = pageTable[pageNumber].frameNumber;

            // Update TLB
            updateTLB(fifoIndex(), pageNumber, frameNumber);

        } else {

            // Page fault - Handle page fault
            handlePageFault(pageNumber);

            // Now, frame number is obtained from Page Table
            frameNumber = pageTable[pageNumber].frameNumber;
        }
    }

    // Calculate physical address using frame number and offset
    int physicalAddress = (frameNumber << 8) | pageOffset;

    // Increment total translations count
    totalTranslations++;

    return physicalAddress;
}

// Displays page fault and TLB hit rates.
void displayStatistics() {
    double pageFaultRate = (double)pageFaultCount / totalTranslations * 100;
    double TLBHitRate = (double)tlbHitCount / totalTranslations * 100;
    printf("Page-fault rate: %.2f%%\n", pageFaultRate);
    printf("TLB hit rate: %.2f%%\n", TLBHitRate);
}

// Prints TLB table
void printTLB() {
    printf("\nTLB Contents:\n");
    printf("+-----+-----------+------------+------+\n");
    printf("| IDX | Page Num  | Frame Num  | FIFO |\n");
    printf("+-----+-----------+------------+------+\n");

    for (int i = 0; i < TLB_SIZE; i++) {
        printf("| %-3d | %-9d | %-10d | %-4d |\n", i, TLB[i].pageNumber, TLB[i].frameNumber, TLB[i].FIFO);
    }

    printf("+-----+-----------+------------+------+\n");
}

int main() {

    // Initialize TLB and Page Table
    initialize();

    // Open the inFile containing logical addresses
    FILE *inFile = fopen(IN_FILE, "r");
    if (inFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    // Open the output inFile
    FILE *outputFile = fopen(OUT_FILE, "w");
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inFile);
        return 1;
    }

    // Read and translate logical addresses
    int logicalAddress;
    while (fscanf(inFile, "%d", &logicalAddress) != EOF) {
        int physicalAddress = translateAddress(logicalAddress);
        printf("Virtual address: %d Physical address: %d Value: %d", logicalAddress, physicalAddress, physicalMemory[physicalAddress]);
        printTLB();
        // Output the values to the inFile
        fprintf(outputFile, "Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, physicalMemory[physicalAddress]);
    }

    // Close the files
    fclose(inFile);
    fclose(outputFile);

    displayStatistics();

    if (compareResults())
        printf("Results match the correct output.\n");
    else
        printf("Results do not match the correct output.\n");

    return 0;
}


