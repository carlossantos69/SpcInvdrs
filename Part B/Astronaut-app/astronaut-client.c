/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: astronaut-cient.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Code that handles astronaut client application.
 */


#include <zmq.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/config.h"
#include "../src/client-logic.h"

// ZeroMQ socket
void* context;
void* requester;
void* heartbeat_subscriber;

void cleanup() {
    endwin();
    zmq_close(requester);
    zmq_close(heartbeat_subscriber);
    //zmq_ctx_destroy(context);
    //zmq_ctx_term(context);
}

void *thread_client_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the client
    client_main(requester, 1);

    // Kill the program
    cleanup();
    exit(0);
}

void *thread_input_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    while(1) {
        int ch = getch();
        if (ch == ERR) {
            ch = 'q';
        }
        input_key(ch);
    }


    // Exit thread
    // Note: program should not reach this point
    pthread_exit(NULL);
}

void* thread_heartbeat_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        char buffer[2];
        int rc = zmq_recv(heartbeat_subscriber, buffer, 1, 0);
        if (rc == -1) {
            perror("Failed to receive heartbeat");
            cleanup();
            exit(1);
        }
        buffer[rc] = '\0';
        if (strcmp(buffer, "H") != 0) {
            fprintf(stderr, "Invalid heartbeat received\n");
            cleanup();
            exit(1);
        }
    }

    // End thread
    // Note: program should not reach this point
    pthread_exit(NULL);
}

/**
 * @brief Main function for the astronaut client application.
 * 
 * This function initializes the ncurses library for handling terminal input/output,
 * sets up a ZeroMQ context and socket for communication with the server, and enters
 * the main game loop where it handles key input and sends messages to the server.
 * thread_client_routine
 * @return int Exit status of the program.
 */
int main() {
    // Initialize ZeroMQ
    context = zmq_ctx_new();
    requester = zmq_socket(context, ZMQ_REQ);
    heartbeat_subscriber = zmq_socket(context, ZMQ_SUB);
    
    // Connect to server's REQ/REP socket
    if (zmq_connect(requester, CLIENT_CONNECT_REQ) != 0) {
        perror("Failed to connect to game server");
        cleanup();
        exit(1);
    }

    // Connect to server's heartbeat PUB socket
    if (zmq_connect(heartbeat_subscriber, CLIENT_CONNECT_HEARTBEAT) != 0) {
        perror("Failed to connect to game server");
        cleanup();
        exit(1);
    }

    // Subscribe to heartbeat messages
    zmq_setsockopt(heartbeat_subscriber, ZMQ_SUBSCRIBE, "", 0);
    int timeout = HEARTBEAT_FREQUENCY*2*1000; // Accepting one missed heartbeat
    zmq_setsockopt(heartbeat_subscriber, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    // Create threads
    pthread_t thread_client;
    pthread_t thread_input;
    pthread_t thread_heartbeat;
    int ret;

    ret = pthread_create(&thread_client, NULL, thread_client_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_client");
        return 1;
    }
    ret = pthread_create(&thread_input, NULL, thread_input_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_ncurses");
        return 1;
    }
    ret = pthread_create(&thread_heartbeat, NULL, thread_heartbeat_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_heartbeat");
        return 1;
    }

    // Wait for threads to finish
    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_client, NULL);
    pthread_join(thread_input, NULL);

    cleanup();
    exit(0);
}