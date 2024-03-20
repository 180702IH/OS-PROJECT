#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdbool.h>

#define SHARED_MEMORY_SIZE 1024
#define MAX_NODES 10

typedef struct
{
    int nodes;
    int adjacencyMatrix[MAX_NODES][MAX_NODES];
} SharedData;

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
    // Define Connection
    key_t key;
    int msgid;

    // Get Shared Message Queue
    key = ftok("msgq", 65);
    msgid = msgget(key, 0666);

    //    key_t shared_memory_key = ftok("shmfile", 65);
    if (msgid == -1)
    {
        perror("Client could not get message queue");
        exit(1);
    }

    while (true)
    {
        Message m;

        // Set the MessageType
        m.mtype = 1;

        // int shared_memory_id;

        // Display menu options
        printf("\n\n");
        printf("1. Add a new graph to the database\n");
        printf("2. Modify an existing graph of the database\n");
        printf("3. Perform DFS on an existing graph of the database\n");
        printf("4. Perform BFS on an existing graph of the database\n");
        printf("\n\n");

        // Get and store user input
        int sequence_number;
        int operation_number;
        char graph_file_name[1024];

        printf("Enter Sequence Number: ");
        scanf("%d", &sequence_number);
        printf("Enter Operation Number: ");
        scanf("%d", &operation_number);
        printf("Enter Graph File Name: ");
        scanf("%s", graph_file_name);

        Payload p;
        p.operation_number = operation_number;
        strcpy(p.graph_file_name, graph_file_name);
        p.sequence_number = sequence_number;
        m.payload = p;

        // / Generate a key for the shared memory segment
        key_t shkey = ftok("shmfile", sequence_number);

        // Create a shared memory segment
        int shmid = shmget(shkey, sizeof(SharedData), IPC_CREAT | 0666);

        // Attach the shared memory segment to the process
        SharedData *data = (SharedData *)shmat(shmid, NULL, 0);

        if (operation_number == 1 || operation_number == 2)
        {

            // For write operations, prompt for additional information
            // Input: Number of nodes
            printf("Enter the number of nodes: ");
            scanf("%d", &data->nodes);

            // Input: Adjacency Matrix
            printf("Enter the adjacency matrix (%d x %d):\n", data->nodes, data->nodes);
            for (int i = 0; i < data->nodes; i++)
            {
                for (int j = 0; j < data->nodes; j++)
                {
                    scanf("%d", &data->adjacencyMatrix[i][j]);
                }
            }

            shmdt(data);
        }
        else if (operation_number == 3 || operation_number == 4)
        {
            // For read operations, prompt for the starting vertex
            printf("Enter starting vertex: ");
            scanf("%d", &data->nodes);
        }

        int sendRes = msgsnd(msgid, &m, sizeof(m.payload), 0);

        // Error Handling
        if (sendRes == -1)
        {
            perror("Message could not be sent by client");
            exit(1);
        }

        printf(
            "\nSent message with: \nMessage Type: %d\nSequence Number:%d \nOperation Number:%d \nFile Name:%s\n", m.mtype, m.payload.sequence_number, m.payload.operation_number, m.payload.graph_file_name);

        if(operation_number == 1 || operation_number == 2){
            Message reply;
            // Receive Message;
            int fetchRes = msgrcv(msgid, &reply, sizeof(reply.payload), 5, 0);

            // Error Handling
            if (fetchRes == -1)
            {
                perror("Client could not receive message");
                exit(1);
            }

            printf("%s",reply.payload.graph_file_name);
        }
        else if (operation_number == 3 || operation_number == 4)
        {
            Message reply;
            // Receive Message;
            int fetchRes = msgrcv(msgid, &reply, sizeof(reply.payload), 5, 0);

            // Error Handling
            if (fetchRes == -1)
            {
                perror("Client could not receive message");
                exit(1);
            }

            if (reply.payload.operation_number == 3)
            {
                printf("Vertices at end of all paths of DFS are:\n");
                for (int i = 0; i < reply.payload.sequence_number; i++)
                {
                    printf("%d ", reply.payload.result[i]);
                }
                printf("\n");
            }
            else if (reply.payload.operation_number == 4)
            {
                printf("BFS traversal:\n");
                for (int i = 0; i < reply.payload.sequence_number; i++)
                {
                    printf("%d ", reply.payload.result[i]);
                }
                printf("\n");
            }
        }
    }

    return 0;
}
