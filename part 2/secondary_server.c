#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>

#define MAX_NODES 10

typedef struct Payload
{
    int sequence_number;
    int operation_number;
    char graph_file_name[50];
    int result[50];
} Payload;

typedef struct
{
	int nodes;
	int adjacencyMatrix[MAX_NODES][MAX_NODES];
} SharedData;

// Message Structure Definition
typedef struct Message
{
    long mtype;
    Payload payload;
} Message;

int **allocateMatrix(int n){
    int **matrix = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++){
        matrix[i] = (int *)malloc (n * sizeof(int));
    }
    return matrix;
}

int** read_file(char *file_name, int *n){

    FILE *file = fopen(file_name, "r");

    if(file==NULL){
        perror("Error opening file");
        exit(1);
    }

    fscanf(file, "%d", n);

    int **Adj = allocateMatrix(*n);


    for (int i = 0; i < (*n); i++)
    {
        for (int j = 0; j < (*n); j++)
        {
            fscanf(file, "%d", &Adj[i][j]);
        }
    }

    fclose(file);

    return Adj;
}

void printAdj(int **Adj, int n){
    printf("\n");
    fflush(stdout);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            printf("%d ", Adj[i][j]);
        fflush(stdout);

        }
    printf("\n");
        fflush(stdout);

    }
    return;
}

typedef struct Util {
    int **Adj;
    int *Vis;
    int *res;
    int n;
    int u;
} Util;

void *recursion(void *params)
{
    Util *util = (Util *)params;
    int *Vis = util->Vis;
    int **Adj = util->Adj;
    int n = util->n;
    int u = util->u;
    int *res = util->res;

    // printf("Visited: %d\n", u);
    Vis[u] = 1;

    int flag = true;
    for (int v = 0; v < n; v++)
    {
        if(Adj[u][v] && !Vis[v]){
            flag = false;
            pthread_t tid;
            Util *new_util = (Util *)malloc(sizeof(Util));
            new_util->Vis = Vis;
            new_util->Adj = Adj;
            new_util->u = v;
            new_util->n = n;
            new_util->res = res;
            fflush(stdout);
            pthread_create(&tid, NULL, recursion, (void *)new_util);
            pthread_join(tid, NULL);
        }
    }


    if(flag){
        for (int i = 0; i < n; i++)
        {
            if (util->res[i] == -1)
            {
                util->res[i] = u + 1;
                break;
            }
        }
    }

    fflush(stdout);
}

typedef struct dfs_utils {
    Message *m;
    long msg_id;
} dfs_utils;

void *dfs(void *params){

    dfs_utils *utils = (dfs_utils *)params;
    Message *m = utils->m;
    long msg_id = utils->msg_id;

    // Generate a key for the shared memory segment
	key_t shkey = ftok("shmfile", m->payload.sequence_number);

	// Get the shared memory segment
	int shmid = shmget(shkey, sizeof(SharedData), 0666);

	// Attach the shared memory segment to the process
	SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
    int start = data->nodes;
    start--;

    int n;
    int **Adj = read_file(m->payload.graph_file_name, &n);

    Util *util = (Util *)malloc(sizeof(Util));
    int *vis = (int *)malloc(n * sizeof(int));
    util->Vis = vis;
    for (int i = 0; i < n; i++){
        util->Vis[i] = 0;
    }
    util->Adj = Adj;
    util->u = start;
    util->n = n;
    int *res = malloc(n * sizeof(int));
    util->res = res;
    for (int i = 0; i < n; i++)
    {
        util->res[i] = -1;
    }

    fflush(stdout);

    pthread_t tid;
    pthread_create(&tid, NULL, recursion, (void *) util);

    pthread_join(tid, NULL);

    int count = 0;
    for (int i = 0; i < n; i++)
    {
        if(res[i]!=-1){
            count++;
        }
    }

    Message send;
    send.mtype = 5;
    send.payload.operation_number = 3;
    send.payload.sequence_number = count;
    for (int i = 0; i < n; i++)
    {
        send.payload.result[i] = res[i];
    }

    int sendRes = msgsnd(msg_id, &send, sizeof(send.payload), 0);

    // Error Handling
    if (sendRes == -1)
    {
        perror("Message could not be sent by client");
        exit(1);
    }

    printf(
        "\nSent message with: \nMessage Type: %d\nSequence Number:%d \nOperation Number:%d \nFile Name:%s\n",send.mtype ,send.payload.sequence_number, send.payload.operation_number, send.payload.graph_file_name);
    printf("Res: \n");
    for (int i = 0; i < send.payload.sequence_number; i++){
        printf("%d ", send.payload.result[i]);
    }

        fflush(stdout);
}


typedef struct bfsprops {
    int *queue;
    int *frontBack;
    int **Adj;
    int *Vis;
    int *res;
    int n;
} bfsprops;

void *handleBfs(void *props){

    bfsprops *BfsProps = (bfsprops *)props;
    int *queue = BfsProps->queue;
    int *frontBack = BfsProps->frontBack;

    int **Adj = BfsProps->Adj;
    int *Vis = BfsProps->Vis;
    int *res = BfsProps->res;
    int n = BfsProps->n;

    // Visit
    int u = queue[frontBack[1]];

    for (int i = 0; i < n; i++)
    {
        if(res[i]==-1){
            res[i] = u + 1;
            break;
        }
    }

    frontBack[1] += 1;


    for (int v = 0; v < n; v++)
    {
        if(Adj[u][v] && !Vis[v]){


            // Push
            queue[frontBack[0]] = v;
            frontBack[0] += 1;
            Vis[v] = 1;

        }
    }

    fflush(stdout);
}

void *bfs(void *params){
    dfs_utils *utils = (dfs_utils *)params;
    Message *m = utils->m;
    long msg_id = utils->msg_id;

    // Generate a key for the shared memory segment
	key_t shkey = ftok("shmfile", m->payload.sequence_number);

	// Get the shared memory segment
	int shmid = shmget(shkey, sizeof(SharedData), 0666);

	// Attach the shared memory segment to the process
	SharedData *data = (SharedData *)shmat(shmid, NULL, 0);

    int start = data->nodes;
    start--;

    int n;
    int **Adj = read_file(m->payload.graph_file_name, &n);

    int *Vis = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++){
        Vis[i] = 0;
    }

    int *res = malloc(n * sizeof(int));
    
    for (int i = 0; i < n; i++)
    {
        res[i] = -1;
    }

    int *queue = (int *)malloc(n * sizeof(int));
    int *frontBack = (int *)malloc(2 * sizeof(int));
    frontBack[0] = 0;
    frontBack[1] = 0;
    queue[frontBack[0]] = start;
    frontBack[0] += 1;
    Vis[start] = 1;

    int idx = 0;

    while(frontBack[0] != frontBack[1]){

        int len = frontBack[0] - frontBack[1];

        pthread_t threads[len];

        for (int i = 0; i < len; i++)
        {
            fflush(stdout);
            bfsprops *props = (bfsprops*) malloc(sizeof(bfsprops));
            props->Adj = Adj;
            props->frontBack = frontBack;
            props->res = res;
            props->Vis = Vis;
            props->queue = queue;
            props->n = n;
            pthread_create(&threads[i], NULL, handleBfs, (void *)props);
        }

        for (int i = 0; i < len; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

    for (int i = 0; i < n; i++)
    {
        printf("%d ", res[i]);
    }

    Message send;
    send.mtype = 5;
    send.payload.operation_number = 4;
    send.payload.sequence_number = n;
    for (int i = 0; i < n; i++)
    {
        send.payload.result[i] = res[i];
    }

    int sendRes = msgsnd(msg_id, &send, sizeof(send.payload), 0);

    // Error Handling
    if (sendRes == -1)
    {
        perror("Message could not be sent by client");
        exit(1);
    }

    printf(
        "\nSent message with: \nMessage Type: %d\nSequence Number:%d \nOperation Number:%d \nFile Name:%s\n",send.mtype ,send.payload.sequence_number, send.payload.operation_number, send.payload.graph_file_name);
    printf("Res: \n");
    for (int i = 0; i < send.payload.sequence_number; i++){
        printf("%d ", send.payload.result[i]);
    }

    fflush(stdout);
}

int main(int argc, char *argv[])
{
    // Check if the CL arg is present or not
    if (argc != 2)
    {
        fprintf(stderr, "ERROR: COMMAND LINE ARGUMENT NOT GIVEN PROPERLY\n");
        fprintf(stderr, "USAGE: ./secondary_server.bin <secondary_server_number>\n");
        exit(0);
    }

    // Get the Secondary Server Number
    int SERVER_NUMBER = atoi(argv[1]);
    if (SERVER_NUMBER != 1 && SERVER_NUMBER != 2)
    {
        fprintf(stderr, "ERROR: INVALID SECONDARY SERVER NUMBER! ONLY 1 or 2 ALLOWED.\n");
        exit(0);
    }

    // Key for the message queue (make sure it matches the key used by the load balancer)
    key_t key = ftok("msgq", 65);

    // Create or get the message queue
    int msg_id = msgget(key, 0666);
    if (msg_id == -1)
    {
        perror("ERROR: Couldn't find the Message Queue!");
        exit(1);
    }

    printf("Secondary Server %d Unleashing Pure Power 💥💻🔥\n", SERVER_NUMBER);

    pthread_t threads[1000];
    int idx = 0;

    // Main Loop
    while (true)
    {
        fflush(stdout);
        // Receive a message from the message queue
        Message m;
        // Receive Message;
		int fetchRes = msgrcv(msg_id, &m, sizeof(m.payload), SERVER_NUMBER + 2, 0);

		// Error Handling
		if (fetchRes == -1)
		{
			perror("Secondary Server could not receive message");
			exit(1);
		}

        if(m.payload.sequence_number == INT_MAX){
            for (int i = 0; i < idx; i++)
            {
                pthread_join(threads[i], NULL);
            }
            exit(0);
        }

        if(m.payload.operation_number == 3){
            pthread_t tid;
            dfs_utils dfsutils;
            dfsutils.m = &m;
            dfsutils.msg_id = msg_id;
            pthread_create(&tid, NULL, dfs, (void *)&dfsutils);
            threads[idx++] = tid;
        }
        else if(m.payload.operation_number == 4){
            pthread_t tid;
            dfs_utils dfsutils;
            dfsutils.m = &m;
            dfsutils.msg_id = msg_id;
            pthread_create(&tid, NULL, bfs, (void *)&dfsutils);
            threads[idx++] = tid;
        }
    }

    return 0;
}