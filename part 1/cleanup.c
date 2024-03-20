#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>
#define NL printf("\n")

// Message Structure Definition
typedef struct message_buffer
{
    long type;
    char text[100];
} message;

int main()
{
    char terminate[100];

    // Define Connection
    key_t key;
    int msgid;

    key = ftok("progfile", 65);
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
        printf("Do you want the server to terminate?\nPress Y for Yes and N for No.\n");
        NL;
        scanf("%s", terminate);
        if (strcmp(terminate, "Y") == 0)
        {
            message m;
            m.type = __INT_MAX__;
            strcpy(m.text, "");
            strcpy(m.text, "4");

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
