#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <limits.h>
#define NL printf("\n")

typedef struct Payload
{
    int sequence_number;
    int operation_number;
    char graph_file_name[50];
    int result[50];
} Payload;

// Message Structure Definition
typedef struct Message
{
    long mtype;
    Payload payload;
} Message;

int main()
{
    char terminate[100];

    // Define Connection
    key_t key;
    int msgid;

    key = ftok("msgq", 65);
    msgid = msgget(key, 0666);

    // Error Handling
    if (msgid == -1)
    {
        perror("Cleanup could not get message queue.");
        exit(1);
    }

    while (true)
    {
        NL;
        printf("Do you want the server to terminate?\nPress Y for Yes and N for No.");
        NL;

        scanf("%s", terminate);
        if (strcmp(terminate, "Y") == 0)
        {
            Message m;
            m.mtype = 1;
            m.payload.sequence_number = INT_MAX;

            // Send message
            int sendRes = msgsnd(msgid, &m, sizeof(m), 0);

            // Error Handling
            if (sendRes == -1)
            {
                perror("Cleanup could not send message to message queue.");
                exit(1);
            }
            
            exit(0);
        }
        else if (strcmp(terminate, "N") == 0)
        {
            continue;
        }
        else
        {
            printf("Please Choose a valid option");
            NL;
        }
    }
}