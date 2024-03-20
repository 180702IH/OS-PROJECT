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
    long client_id;

    // Define Connection
    key_t key;
    int msgid;

    // Get Shared Message Queue
    key = ftok("progfile", 65);
    msgid = msgget(key, 0666);

    // Error Handling
    if (msgid == -1)
    {
        perror("Client could not get message queue");
        exit(1);
    }

    // Take Client ID input
    printf("Enter Client-ID:");
    NL;
    scanf("%ld", &client_id);

    while (true)
    {
        bool flag = true;

        message m;
        strcpy(m.text, "");

        // Set Message type as Client ID
        m.type = client_id;
        NL;

        // Display Menu
        printf("1. Enter 1 to contact the Ping Server:");
        NL;
        printf("2. Enter 2 to contact the File Search Server:");
        NL;
        printf("3. Enter 3 to contact the File Word Counter Server:");
        NL;
        printf("4. Enter 4 if this Client wishes to exit:");
        NL;

        // Get Client Selection
        int choice;
        printf("Your Choice:\n> ");
        scanf("%d", &choice);

        // Switch based on the choice
        switch (choice)
        {
        case 1:
            strcpy(m.text, "1");
            strcat(m.text, "hi");
            break;
        case 2:
            strcpy(m.text, "2");
            NL;
            printf("Enter File Name:\n> ");
            scanf("%s", &m.text[1]);
            NL;
            break;
        case 3:
            strcpy(m.text, "3");
            NL;
            printf("Enter File Name:\n> ");
            scanf("%s", &m.text[1]);
            NL;
            break;
        case 4:
            NL;
            printf("Exiting.....");
            NL;
            // Exit Client Process
            exit(0);
        default:
            printf("Please Choose a valid option");
            flag = false;
            NL;
            break;
        }

        if (!flag)
            continue;

        // Send Message
        int sendRes = msgsnd(msgid, &m, sizeof(m), 0);

        // Error Handling
        if (sendRes == -1)
        {
            perror("Message could not be sent by client");
            exit(1);
        }

        NL;
        printf("Sent \"%s\"", &m.text[1]);
        NL;

        {

            // Get Reply
            message reply;
            int fetchRes = msgrcv(msgid, &reply, sizeof(reply), client_id, 0);

            // Error Handling
            if (fetchRes == -1)
            {
                perror("Message could not be recieved by client");
                exit(1);
            }
            printf("Reply from Server: \"%s\"", reply.text);
            NL;
        }
    }

    return 0;
}