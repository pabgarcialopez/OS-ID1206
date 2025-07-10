#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h> // For O_CREAT, O_EXCL flags

#define SHM_KEY 1234
#define SEM_KEY_WRITER "/my_semaphore_writer"
#define SEM_KEY_READER "/my_semaphore_reader"

const int MAX = 5;

typedef struct {
    int var;
    int readers_count;
} SharedData;

void writer(sem_t *sem_writer, sem_t *sem_reader, SharedData *sharedData, pid_t writer_pid) {
    while (1) {
        // Lock semaphore when no readers
        sem_wait(sem_writer);

        sharedData->var++;
        if (sharedData->var > MAX)
            break;

        printf("The writer acquires the lock\n");
        printf("The writer (%d) writes the value %d\n", writer_pid, sharedData->var);
        printf("The writer releases the lock.\n");

        // Let readers know they can read now.
        sem_post(sem_reader);
        sem_post(sem_writer);
        sleep(1);
    }
}

void reader(sem_t *sem_reader, sem_t *sem_writer, SharedData *sharedData, pid_t reader_pid) {

    while (1) {
        // Wait until writer is done.
        sem_wait(sem_reader);

        // One more reader is reading.
        sharedData->readers_count++;

        if (sharedData->var > MAX)
            break;

        // The first reader acquires the lock and lets other readers read too.
        if (sharedData->readers_count == 1) {
            sem_wait(sem_writer);
            sem_post(sem_reader);
            printf("The first reader (%d) acquires the lock and reads %d.\n", reader_pid, sharedData->var);
        }

        else if (sharedData->readers_count == 2) {
            printf("The second reader (%d) reads the value %d too.\n", reader_pid, sharedData->var);
            printf("The last reader (%d) releases the lock.\n", reader_pid);
            sharedData->readers_count = 0;
            sem_post(sem_writer);
        }

        sleep(1);
    }
}

int main() {

    int shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);

    // Create semaphore with initial value of 1 and 0.
    sem_t *sem_writer = sem_open(SEM_KEY_WRITER, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem_reader = sem_open(SEM_KEY_READER, O_CREAT | O_EXCL, 0666, 0);

    // Initialize shared memory
    SharedData *sharedData = shmat(shm_id, NULL, 0);
    sharedData->var = 0;
    sharedData->readers_count = 0;

    pid_t writer_pid = fork();

    if (writer_pid == 0)
        writer(sem_writer, sem_reader, sharedData, getpid());

    else {
        pid_t reader_pid1 = fork();

        if (reader_pid1 == 0)
            reader(sem_reader, sem_writer, sharedData, getpid());

        else {
            pid_t reader_pid2 = fork();

            if (reader_pid2 == 0)
                reader(sem_reader, sem_writer, sharedData, getpid());

            else {
                // Wait for all processes to finish
                wait(NULL);

                // Cleanup
                sem_close(sem_writer);
                sem_close(sem_reader);
                sem_unlink(SEM_KEY_WRITER);
                sem_unlink(SEM_KEY_READER);
                shmdt(sharedData);
                shmctl(shm_id, IPC_RMID, NULL);
            }
        }
    }

    return 0;
}

