#include <stdio.h>
#include <stdlib.h>

#define NUM_CYLINDERS 5000
#define NUM_REQUESTS 1000
#define INITIAL_POS 330

int compare(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// Function to perform FCFS (First-Come-First-Serve) scheduling
int fcfs(int initial, int requests[]) {
    int headMovement = 0;
    int current = initial;

    for (int i = 0; i < NUM_REQUESTS; i++) {
        headMovement += abs(current - requests[i]);
        current = requests[i];
    }

    return headMovement;
}

// Function to perform SSTF (Shortest Seek Time First) scheduling
int sstf(int initial, int requests[]) {
    int headMovement = 0;
    int current = initial;

    for (int i = 0; i < NUM_REQUESTS; i++) {
        int minDist = NUM_CYLINDERS + 1;
        int minIndex = -1;

        // Look for shortest seek time.
        for (int j = 0; j < NUM_REQUESTS; j++) {
            int distance = abs(current - requests[j]);
            if (distance < minDist) {
                minDist = distance;
                minIndex = j;
            }
        }

        headMovement += minDist;
        current = requests[minIndex];
        requests[minIndex] = NUM_CYLINDERS + 1; // Mark as visited
    }

    return headMovement;
}

// Function to look for next request upwards on the queue.
int lookUpwards(int current, const int requests[], int upperbound) {
    int j = 0;
    while(j < NUM_REQUESTS && (requests[j] <= upperbound || requests[j] == -1)) {
        if(requests[j] >= current && requests[j] != -1)
            return j;
        j++;
    }

    return -1;
}

// Function to look for next request downwards on the queue.
int lookDownwards(int current, const int requests[], int lowerbound) {

    int j = NUM_REQUESTS - 1;
    while(j >= 0 && (requests[j] >= lowerbound || requests[j] == -1)) {
        if(requests[j] <= current && requests[j] != -1)
            return j;

        j--;
    }

    return -1;
}

// Function to perform SCAN scheduling
int scan(int initial, int requests[]) {
    int headMovement = 0;
    int current = initial;
    int direction = 0; // Assume we start going downwards.

    // Requests ordered.
    qsort(requests, NUM_REQUESTS, sizeof(int), compare);

    for (int i = 0; i < NUM_REQUESTS; i++) {

        int index;
        if(direction == 0) { // Going downwards
            index = lookDownwards(current, requests, 0);
            if(index == -1) { // No request found in downwards direction.
                direction = 1;
                headMovement += current;
                index = lookUpwards(current, requests, NUM_CYLINDERS);
            }
        }

        else { // Going upwards
            index = lookUpwards(current, requests, NUM_CYLINDERS);
            if(index == -1) { // No request found in upwards direction.
                direction = 0;
                headMovement += NUM_CYLINDERS - current;
                index = lookDownwards(current, requests, 0);
            }
        }

        if(index != -1) {
            headMovement += abs(current - requests[index]);
            current = requests[index];
            requests[index] = -1; // Mark as visited.
        }

        else {
            perror("Index = -1 at scan");
            exit(1);
        }
    }

    return headMovement;
}

// Function to perform CSCAN scheduling
int cscan(int initial, int requests[]) {
    int headMovement = 0;
    int current = initial;

    qsort(requests, NUM_REQUESTS, sizeof(int), compare);

    for (int i = 0; i < NUM_REQUESTS; i++) {
        int index;
        int turn = 0; // Did the head move to end of disk?

        index = lookDownwards(current, requests, 0);
        if(index == -1) {
            turn = 1;
            index = lookDownwards(NUM_CYLINDERS - 1, requests, 0);

            if(index == -1) {
                perror("Index = -1 at cscan");
                exit(1);
            }
        }

        if(turn == 0) // Next request found without having to go up the disk.
            headMovement += abs(current - requests[index]);
        else // We went up the disk:
             // (Going down to 0 + going up to right corner) + (going from the right corner to requests[index]).
            headMovement += (current + NUM_CYLINDERS) + (NUM_CYLINDERS - requests[index]);

        current = requests[index];
        requests[index] = -1; // Mark as visited.
    }

    return headMovement;
}

// Function to perform LOOK scheduling
int look(int initial, int requests[]) {

    int headMovement = 0;
    int current = initial;
    int direction = 0;

    // Requests ordered.
    qsort(requests, NUM_REQUESTS, sizeof(int), compare);

    int lowerbound = requests[0];
    int upperbound = requests[NUM_REQUESTS - 1];

    for (int i = 0; i < NUM_REQUESTS; i++) {

        int index;

        if(direction == 0) {
            index = lookDownwards(current, requests, lowerbound);
            if(index == -1) {
                direction = 1;

                // Obtain new lowerbound
                for(int j = 0; j < NUM_REQUESTS; j++)
                    if(requests[j] != -1) {
                        lowerbound = requests[j];
                        break;
                    }

                //headMovement += abs(current - lowerbound); // 2* because of going down and up to current again.
                index = lookUpwards(current, requests, upperbound);

                if(index == -1) {
                    perror("Index = -1 at look. Direction 0");
                    exit(1);
                }
            }
        }

        else {
            index = lookUpwards(current, requests, upperbound);
            if(index == -1) {
                direction = 0;

                for(int j = NUM_REQUESTS; j >= 0; j--)
                    if(requests[j] != -1) {
                        upperbound = requests[j];
                        break;
                    }

                //headMovement += abs(upperbound - current); // 2* because of going down and up to current again.
                index = lookDownwards(current, requests, lowerbound);

                if(index == -1) {
                    perror("Index = -1 at look. Direction 1");
                    printf("Current: %d", current);
                    exit(1);
                }
            }
        }

        headMovement += abs(current - requests[index]);
        current = requests[index];

        requests[index] = -1; // Mark as visited.

    }

    return headMovement;
}

// Function to perform C-LOOK scheduling
int clook(int initial, int requests[]) {
    int headMovement = 0;
    int current = initial;

    qsort(requests, NUM_REQUESTS, sizeof(int), compare);

    int lowerbound = requests[0];

    for (int i = 0; i < NUM_REQUESTS; i++) {
        int index;
        int turn = 0; // Did the head move to end of disk?

        index = lookDownwards(current, requests, lowerbound);
        if(index == -1) {
            turn = 1;
            index = lookDownwards(NUM_CYLINDERS - 1, requests, lowerbound);

            if(index == -1) {
                perror("Index = -1 at clook");
                exit(1);
            }
        }

        if(turn == 0) // Next request found without having to go up.
            headMovement += abs(current - requests[index]);
        else // (Going down to lowerbound + going up to right corner) + (going from the right corner to requests[index]).
            headMovement += (current - lowerbound + NUM_CYLINDERS - lowerbound) + (NUM_CYLINDERS - requests[index]);

        current = requests[index];
        requests[index] = -1; // Mark as visited.
    }

    return headMovement;
}

void copy(const int requests[], int copyOfReq[]) {
    for(int i = 0; i < NUM_REQUESTS; i++)
        copyOfReq[i] = requests[i];
}

int main() {

    int initial = INITIAL_POS;
    int requests[NUM_REQUESTS];
    int copyOfReq[NUM_REQUESTS];

    // Generate random cylinder requests
    for (int i = 0; i < NUM_REQUESTS; i++) {
        requests[i] = rand() % NUM_CYLINDERS;
        copyOfReq[i] = requests[i];
    }

    // Perform scheduling and print results
    printf("Total head movement:\n");
    printf("FCFS: %d cylinders\n", fcfs(initial, copyOfReq));
    copy(requests, copyOfReq);
    printf("SSTF: %d cylinders\n", sstf(initial, copyOfReq));
    copy(requests, copyOfReq);
    printf("SCAN: %d cylinders\n", scan(initial, copyOfReq));
    copy(requests, copyOfReq);
    printf("C-SCAN: %d cylinders\n", cscan(initial, copyOfReq));
    copy(requests, copyOfReq);
    printf("LOOK: %d cylinders\n", look(initial, copyOfReq));
    copy(requests, copyOfReq);
    printf("C-LOOK: %d cylinders\n", clook(initial, copyOfReq));

    return 0;
}
