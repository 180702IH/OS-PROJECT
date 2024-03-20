#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>

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

pthread_mutex_t writeLock[20];
pthread_mutex_t readLock[20];
int readerCount[20] = {0};

void *HandleRequest(void *params)
{
	argParams arguments = *(argParams *)params;
	Message m = arguments.m;
	int msg_id = arguments.msg_id;
	Payload load = m.payload;

	int sequenceNumber = load.sequence_number;
	int operationNumber = load.operation_number;
	char *GraphFileName = load.graph_file_name;

	int reader;

	int len = strlen(GraphFileName);
	int val;
	if (len == 7)
	{
		char num[3];
		num[0] = *(GraphFileName + 1);
		num[1] = *(GraphFileName + 2);
		num[2] = '\0';
		val = atoi(num);
	}
	else
	{
		char num[2];
		num[0] = *(GraphFileName + 1);
		num[1] = '\0';
		val = atoi(num);
	}

	// if (operationNumber == 1 || operationNumber == 2)
	// {
	// 	reader = 0;
	// 	pthread_mutex_lock(&writeLock[val]);
	// 	m.mtype = 2;
	// }
	// else
	// {
	// 	reader = 1;
	// 	pthread_mutex_lock(&readLock[val]);
	// 	readerCount[val]++;
	// 	if (readerCount[val] == 1)
	// 		pthread_mutex_lock(&writeLock[val]);
	// 	pthread_mutex_unlock(&readLock[val]);

	// 	if (sequenceNumber % 2 == 0)
	// 		m.mtype = 3;
	// 	else
	// 		m.mtype = 4;
	// }

	// m.mtype = 2;

	// Send To Server (Either Primary or Secondary)
	int sendRes = msgsnd(msg_id, &m, sizeof(m), 0);
	// Error Handling
	if (sendRes == -1)
	{
		perror("Load Balancer could not send message");
		exit(1);
	}

	// // Receive Message
	// int fetchRes = msgrcv(msg_id, &m, sizeof(m), ToLoadReceiver, 0);

	// // Error Handling
	// if (fetchRes == -1)
	// {
	// 	perror("Server could not receive message");
	// 	exit(1);
	// }

	// if (!reader)
	// 	pthread_mutex_unlock(&writeLock);
	// else
	// {
	// 	pthread_mutex_lock(&readLock);
	// 	readerCount--;
	// 	if (readerCount == 0)
	// 		pthread_mutex_unlock(&writeLock);
	// 	pthread_mutex_unlock(&readLock);
	// }

	return 0;
}

int main(int argc, char *argv[])
{

	// Define Connection
	key_t key;
	int msg_id;

	// Create Shared Message Queuep
	key = ftok("msgq", 65);
	msg_id = msgget(key, 0666 | IPC_CREAT);

	// Error Handling
	if (msg_id == -1)
	{
		perror("Server could not create message queue");
		exit(1);
	}

	printf("Load Balancer created Message Queue.\n\n");

	for (int i = 0; i < 20; i++)
	{
		pthread_mutex_init(&writeLock[i], NULL);
		pthread_mutex_init(&readLock[i], NULL);
	}

	while (true)
	{
		Message m;

		// Receive Message;
		int fetchRes = msgrcv(msg_id, &m, sizeof(m.payload), 1, 0);

		// Error Handling
		if (fetchRes == -1)
		{
			perror("Load Balancer could not receive message");
			exit(1);
		}

		printf(
			"\nRecieved message with: \nMessage Type: %d\nSequence Number:%d \nOperation Number:%d \nFile Name:%s\n", m.mtype, m.payload.sequence_number, m.payload.operation_number, m.payload.graph_file_name);

		pthread_t thread;
		pthread_attr_t thread_attr;

		argParams params;
		params.m = m;
		params.msg_id = msg_id;

		pthread_create(&thread, NULL, (void *)HandleRequest, (void *)&params);
		pthread_join(thread, NULL);
	}

	return 0;
}