#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>

#define SHARED_MEMORY_SIZE 1024
#define MAX_NODES 10

// Structure to hold data in shared memory

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

typedef struct Argument
{
	int msg_id;
	Message m;
} argParams;

typedef struct
{
	int nodes;
	int adjacencyMatrix[MAX_NODES][MAX_NODES];
} SharedData;

void *HandleRequest(void *params)
{
	argParams arguments = *(argParams *)params;
	Message m = arguments.m;
	int msg_id = arguments.msg_id;
	Payload load = m.payload;

	int sequenceNumber = load.sequence_number;
	int operationNumber = load.operation_number;
	char *GraphFileName = load.graph_file_name;

	// Generate a key for the shared memory segment
	key_t shkey = ftok("shmfile", sequenceNumber);

	// Get the shared memory segment
	int shmid = shmget(shkey, sizeof(SharedData), 0666);

	// Attach the shared memory segment to the process
	SharedData *data = (SharedData *)shmat(shmid, NULL, 0);

	if (operationNumber == 1)
	{
		FILE *file = fopen(GraphFileName, "w");
		if (file == NULL)
		{
			perror("Error opening file");
			exit(1);
		}

		// Write the received data to the file
		fprintf(file, "%d\n", data->nodes);
		for (int i = 0; i < data->nodes; i++)
		{
			for (int j = 0; j < data->nodes; j++)
			{
				fprintf(file, "%d ", data->adjacencyMatrix[i][j]);
			}
			fprintf(file, "\n");
		}

		// Close the file
		fclose(file);
		strcpy(m.payload.graph_file_name, "File Successfully Added");
	}
	else if (operationNumber == 2)
	{
		FILE *file = fopen(GraphFileName, "w+");
		if (file == NULL)
		{
			perror("Error opening file");
			exit(1);
		}

		// Write the received data to the file
		fprintf(file, "%d\n", data->nodes);
		for (int i = 0; i < data->nodes; i++)
		{
			for (int j = 0; j < data->nodes; j++)
			{
				fprintf(file, "%d ", data->adjacencyMatrix[i][j]);
			}
			fprintf(file, "\n");
		}

		// Close the file
		fclose(file);
		strcpy(m.payload.graph_file_name, "File Successfully Modified");
	}
	else
	{
		printf("Invalid Operation Number\n");
		exit(0);
	}

	// / Detach the shared memory segment
	shmdt(data);

	// Remove the shared memory segment (optional)
	shmctl(shmid, IPC_RMID, NULL);

	m.mtype = 5;
	int sendRes = msgsnd(msg_id, &m, sizeof(m.payload), 0);

	// Error Handling
	if (sendRes == -1)
	{
		perror("Message could not be sent by client");
		exit(1);
	}

	printf("Sent message: %s", m.payload.graph_file_name);
}

int main(int argc, char *argv[])
{
	// Define Connection
	key_t key;
	int msg_id;

	// Create Shared Message Queue
	key = ftok("msgq", 65);
	msg_id = msgget(key, 0666);

	// Error Handling
	if (msg_id == -1)
	{
		perror("Server could not create message queue");
		exit(1);
	}

	while (true)
	{
		Message m;
		// Receive Message
		int fetchRes = msgrcv(msg_id, &m, sizeof(m), 2, 0);

		// Error Handling
		if (fetchRes == -1)
		{
			perror("Server could not receive message");
			exit(1);
		}

		printf("\nReceived message with: \nMessage Type: %d\nSequence Number:%d \nOperation Number:%d \nFile Name:%s\n", m.mtype, m.payload.sequence_number, m.payload.operation_number, m.payload.graph_file_name);

		pthread_t thread;
		pthread_attr_t thread_attr;

		argParams params;
		params.m = m;
		params.msg_id = msg_id;

		pthread_create(&thread, NULL, (void *)HandleRequest, (void *)&params);
		pthread_join(thread, NULL);
	}
}