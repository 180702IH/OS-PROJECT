#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#define NL printf("\n")

// Message Structure Definition
typedef struct message_buffer
{
    long type;
    char text[100];
} message;

int main()
{
    // Define Connection
    key_t key;
    int msg_id;

    // Create Shared Message Queue
    key = ftok("progfile", 65);
    msg_id = msgget(key, 0666 | IPC_CREAT);

    // Error Handling
    if (msg_id == -1)
    {
        perror("Server could not create message queue");
        exit(1);
    }

    while (true)
    {
        NL;
        printf("Server is Listening");
        NL;
        message m;

        // Recieve Message
        int fetchRes = msgrcv(msg_id, &m, sizeof(m), 0, 0);

        // Error Handling
        if (fetchRes == -1)
        {
            perror("Server could not recieve message");
            exit(1);
        }

        if (m.type == __INT_MAX__)
        {
            NL;
            printf("Waiting for Child Servers to Close.....");
            NL;

            while (wait(NULL) > 0)
            {
            }

            NL;
            printf("Exiting.....");
            NL;

            // Close Message Queue
            int msgexit = msgctl(msg_id, IPC_RMID, NULL);

            // Error Handling
            if (msgexit == -1)
            {
                perror("Server could not remove message queue.");
                exit(1);
            }

            // Exit the server
            exit(0);
        }

        NL;
        printf("Received: \"%s\"", &m.text[1]);
        NL;
        printf("From: %ld", m.type);
        NL;

        char fname[100];
        strcpy(fname, "");

        // Create Child server
        int pid = fork();

        // Error Handling
        if (pid == -1)
        {
            perror("Child server could not be created.");
            exit(1);
        }
        else if (pid == 0)
        {
            // Inside Child Server

            // Switch based on client choice
            switch (m.text[0])
            {
            case '1':
                strcpy(m.text, "hello");

                // Send Message to Client
                int sendRes = msgsnd(msg_id, &m, sizeof(m), 0);

                // Error Handling
                if (sendRes == -1)
                {
                    perror("Message could not be sent by child server");
                    exit(1);
                }
                NL;
                printf("Sent by child server: \"%s\"", m.text);
                NL;
                printf("To: %ld", m.type);
                NL;

                // Exit Child Server
                exit(0);

                break;
            case '2':
                strcpy(fname, m.text);

                // Create Pipe
                int pipefd[2];

                // Error Handling
                if (pipe(pipefd) == -1)
                {
                    perror("Child Server could not create pipe");
                    exit(1);
                }

                // Create GrandChild Server
                int pid = fork();

                // Error Handling
                if (pid == -1)
                {
                    perror("GrandChild Server could not be created");
                    exit(1);
                }
                else if (pid == 0)
                {
                    // In GrandChild Server

                    // Close the read end of the pipe
                    close(pipefd[0]);

                    // Redirect stdout to the write end of the pipe
                    dup2(pipefd[1], STDOUT_FILENO);

                    // Close unused file descriptors
                    close(pipefd[1]);

                    // Execute the instruction
                    execlp("find", "find", "-name", &fname[1], (char *)NULL);

                    // Error Handling
                    perror("Command could not be executed");
                    exit(1);
                }
                else
                {
                    // Close the write end of the pipe
                    close(pipefd[1]);

                    // Redirect read end of the pipe to stdin
                    dup2(pipefd[0], STDIN_FILENO);

                    // Read the output of execlp from the pipe
                    char *buffer = (char *)malloc(sizeof(char) * 100);
                    int bytesRead = read(pipefd[0], buffer, sizeof(buffer));

                    // Close the read end of the pipe
                    close(pipefd[0]);

                    // Error Handling
                    if (bytesRead == -1)
                    {
                        perror("Child Server could not read pipe");
                        exit(1);
                    }

                    buffer[2] = '\0';
                    char *str = "./";

                    // Check if file found
                    if (strcmp(buffer, str) == 0)
                        strcpy(m.text, "File Found");
                    else
                        strcpy(m.text, "File not Found");

                    // Send Message
                    int sendRes = msgsnd(msg_id, &m, sizeof(m), 0);

                    // Error Handling
                    if (sendRes == -1)
                    {
                        perror("Child Server could not send message");
                        exit(1);
                    }
                    NL;
                    printf("Sent by child server: \"%s\"", m.text);
                    NL;
                    printf("To: %ld", m.type);
                    NL;
                    // Free allocated memory
                    free(buffer);

                    // Wait for GrandChild Server to exit
                    wait(NULL);

                    // Exit Child Server
                    exit(0);
                }
                break;
            case '3':
                strcpy(fname, m.text);

                // Create Pipe
                int pipefd2[2];

                // Error Handling
                if (pipe(pipefd2) == -1)
                {
                    perror("Child Server could not create pipe");
                    exit(1);
                }

                // Create GrandChild Server
                int pid2 = fork();

                // Error Handling
                if (pid2 == -1)
                {
                    perror("GrandChild Server could not be created");
                    exit(1);
                }
                else if (pid2 == 0)
                {
                    // Close the read end of the pipe
                    close(pipefd2[0]);

                    // Redirect stdout to the write end of the pipe
                    dup2(pipefd2[1], STDOUT_FILENO);

                    // Close unused file descriptors
                    close(pipefd2[1]);

                    // Execute the command
                    execlp("wc", "wc", "-w", &fname[1], (char *)NULL);

                    // Error Handling
                    perror("Command could not be executed");
                    exit(1);
                }
                else
                {
                    // Close the write end of the pipe
                    close(pipefd2[1]);

                    // Redirect the read end of the pipe to stdin
                    dup2(pipefd2[0], STDIN_FILENO);

                    // Read the output of execlp from the pipe
                    char *buffer = (char *)malloc(sizeof(char) * 100);
                    int bytesRead = read(pipefd2[0], buffer, sizeof(buffer));

                    // Close the read end of the pipe
                    close(pipefd2[0]);

                    // Count Number of Words
                    char *words = strtok(buffer, " ");

                    // Error Handling
                    if (bytesRead == -1)
                    {
                        perror("Child Server could not read from pipe");
                        exit(1);
                    }
                    if (buffer[0] >= '0' && buffer[0] <= '9')
                        strcpy(m.text, words);
                    else
                        strcpy(m.text, "File Not Found");

                    // Send Message
                    int sendRes = msgsnd(msg_id, &m, sizeof(m), 0);

                    // Error Handling
                    if (sendRes == -1)
                    {
                        perror("Child server could not send message");
                        exit(1);
                    }
                    NL;
                    printf("Sent by child server: \"%s\"", m.text);
                    NL;
                    printf("To: %ld", m.type);
                    NL;

                    // Free allocated memory
                    free(buffer);

                    // Wait for GrandChild Server to exit
                    wait(NULL);

                    // Exit Child Server
                    exit(0);
                }
                break;
            default:
                NL;
                printf("Recieved Message in Incorrect Format");
                NL;
                exit(0);
                break;
            }
        }
    }
}
