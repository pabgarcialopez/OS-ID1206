#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>


#define QUEUE_NAME "/mymq"
#define MAX_MSG_SIZE 1024

int main(){

    mqd_t mq;
    pid_t pid;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    /*
    O_RDWR opens the queue for both receiving and sending messages if exists with that name
    O_CREAT creates a que with specified name - requires two more arguments
    mode, which is of type mode_t, and attr, which is a pointer to a mq_attr structure
    O_NONBLOCK makes the mq_receive function return -1 if queue is empty
    S_IRUSR read permission for the owner, S_IWUSR write permission for the owner
    */

    mq = mq_open(QUEUE_NAME, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    pid = fork();
    if(pid==-1) {
        perror("fork");
        mq_close(mq);
        mq_unlink(QUEUE_NAME);
        return 1;
    }

    if(pid == 0) {

        char msg[MAX_MSG_SIZE];
        FILE *file = fopen("file.txt","r");

        // The mq_send() function adds the message pointed to by the
        // argument msg_ptr to the message queue specified by mqdes

        while(fgets(msg,MAX_MSG_SIZE,file) != NULL) {
            if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
                perror("mq_send");
                mq_close(mq);
                exit(1);
            }
        }

        fclose(file);
        exit(0);

    }

    else {

        wait(NULL);

        char message[MAX_MSG_SIZE];
        int totalWords = 0;
        bool newWord = false;

        while(true) {

            // mq_receive returns the length of the message in bytes.
            if (mq_receive(mq, message, MAX_MSG_SIZE, NULL) == -1) {
                // No more messages
                printf("No more msg\n");
                break;
            }

            int i = 0;

            while(message[i] != '\0') {

                if(isspace(message[i]))
                    newWord = true;

                else if(newWord) {
                    newWord = false;
                    totalWords++;
                }

                i++;
            }

        }

        printf("Number of words is: %d\n",totalWords);
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    return 0;


}