#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

int buffer = 0;
const int NUM_MODIFICATIONS = 15;
pthread_mutex_t lock;

struct Solution {
    unsigned long TID;
    int mod;
};

void * thread_function() {

    struct Solution *sol = malloc(sizeof *sol);
    sol->TID = (unsigned long) pthread_self();
    sol->mod = 0;

    int done = 0;
    while (!done) {
        pthread_mutex_lock(&lock);

        done = (buffer >= NUM_MODIFICATIONS);
        if (!done) {
            printf("TID: %lu, PID: %d, Buffer: %d\n", sol->TID, getpid(), buffer);
            buffer++; sol->mod++;
        }

        pthread_mutex_unlock(&lock);

        // sleep(0.5) will generate uneven distribution as well.
    }

    return sol;
}

int main() {

    pthread_t t1, t2, t3;
    pthread_mutex_init(&lock, NULL);

    pthread_create(&t1, NULL, thread_function, NULL);
    pthread_create(&t2, NULL, thread_function, NULL);
    pthread_create(&t3, NULL, thread_function, NULL);

    void * out_void1, * out_void2, * out_void3;
    struct Solution *out1, *out2, *out3;

    pthread_join(t1, &out_void1);
    pthread_join(t2, &out_void2);
    pthread_join(t3, &out_void3);

    out1 = out_void1;
    out2 = out_void2;
    out3 = out_void3;

    printf("TID %lu worked on the buffer %d times\n", out1->TID, out1->mod);
    printf("TID %lu worked on the buffer %d times\n", out2->TID, out2->mod);
    printf("TID %lu worked on the buffer %d times\n", out3->TID, out3->mod);

    printf("Total buffer accesses: %d\n", out1->mod + out2->mod + out3->mod);

    free(out_void1);
    free(out_void2);
    free(out_void3);

    pthread_mutex_destroy(&lock);
}

