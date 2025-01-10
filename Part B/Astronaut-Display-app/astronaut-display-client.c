/*
 * PSIS 2024/2025 - Project Part 2
 *
 * Filename: astronaut-display-cient.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Code that handles astronaut client application with display.
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
#include "../src/space-display.h"

// ZeroMQ subscriber socket
void* context;
void* requester;
void* subscriber_gamestate;
void* heartbeat_subscriber;

// Flags to indicate thread ending
pthread_mutex_t lock;
bool thread_display_finished = false;

/**
 * @brief Cleans up resources used by the application.
 * 
 * This function closes the ZeroMQ subscriber, destroys the ZeroMQ context,
 * terminates the ZeroMQ context, destroys the display mutex, and ends the
 * ncurses window session.
 */
void cleanup() {
    // We need to wait for the display thread to finish message processing
    endwin();
    zmq_close(requester);
    zmq_close(subscriber_gamestate);
    zmq_close(heartbeat_subscriber);
    pthread_mutex_destroy(&lock);
}

/**
 * @brief Thread routine for the client.
 *
 * This function serves as the entry point for a client thread. It starts the client
 * by calling `client_main` and then performs cleanup before terminating the program.
 *
 * @param arg Unused argument.
 */
void *thread_client_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the client
    client_main(requester, 0);

    // Kill the program
    cleanup();
    exit(0);
}

/**
 * @brief Thread routine for communication with the server.
 *
 * This function runs in a separate thread and continuously reads messages
 * from the server to update the display grid. It handles various errors
 * that may occur during message reception and ensures proper cleanup
 * before exiting.
 *
 * @param arg Unused argument.
 * @return void* Always returns NULL.
 */
void *thread_comm_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Read messages from the server and update the display grid
    while (1) {
        pthread_mutex_lock(&lock);
        if (thread_display_finished) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);


        char buffer[BUFFER_SIZE];
        int recv_size = zmq_recv(subscriber_gamestate, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        if (recv_size != -1) {
            buffer[recv_size] = '\0';
            // Copy the message to display
            set_display_game_state(buffer);
        } else {
            int err = zmq_errno();
            if (err == EAGAIN) {
                // No message received, continue
            } else if (err == ETERM || err == ENOTSOCK) {
                // The context was terminated or socket invalid, exit program
                cleanup();
                exit(1);
            } else {
                // Exit program on other errors
                cleanup();
                exit(1);
            }
        }
    }

    // Note: no need to close the socket or context, as the cleanup function will handle it

    // End thread
    pthread_exit(NULL);
}

/**
 * @brief Thread routine to handle the display.
 *
 * This function serves as the entry point for a thread that manages the display.
 * It is the main signal to exit the application, as it will only finish after the
 * game over screen is displayed.
 *
 * @param arg Unused argument.
 * @return None.
 */
void *thread_display_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the display
    display_main();

    sleep(1); // This a workaround to avoid the program exiting before the next getch(). Improve later

    // End thread
    pthread_mutex_lock(&lock);
    thread_display_finished = true;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

/**
 * @brief Thread routine to handle input from the user.
 *
 * This function runs in an infinite loop, continuously reading input characters
 * using the `getch()` function. The input character is then passed to the `input_key()` function.
 *
 * It blocks in getch() until a key is pressed.
 * It blocks in input_key() until the client thread process the key request
 * 
 * @param arg Unused argument.
 */
void *thread_input_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        int ch = getch();
        if (ch == ERR) {
            ch = 'q';
        }
        input_key(ch);

        // Exit the program if display thread has finished
        // Display thread finishes after game over screen is displayed
        pthread_mutex_lock(&lock);
        if (thread_display_finished) {
            cleanup();
            exit(0);
        }
        pthread_mutex_unlock(&lock);
        
    }
}

/**
 * @brief Thread routine to handle heartbeat messages.
 *
 * This function runs in an infinite loop, receiving heartbeat messages
 * from a ZeroMQ subscriber socket. If a message is received that is not
 * the expected heartbeat ("H"), the function logs an error, performs
 * cleanup, and exits the program.
 *
 * @param arg Unused argument.
 * @return This function does not return.
 */
void* thread_heartbeat_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        char buffer[2];
        int rc = zmq_recv(heartbeat_subscriber, buffer, 1, 0);
        if (rc == -1) {
            // Do not exit if game is over, will wait for input
            pthread_mutex_lock(&lock);
            if (thread_display_finished) {
                // Do not timeout, server closed and we want to keep game over screen
                pthread_mutex_unlock(&lock);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&lock);
            perror("Failed to receive heartbeat");
            cleanup();
            exit(1);
        }
        buffer[rc] = '\0';
        if (strcmp(buffer, "H") != 0) {
            // Do not exit if game is over, will wait for input
            pthread_mutex_lock(&lock);
            if (thread_display_finished) {
                // Do not timeout, server closed and we want to keep game over screen
                pthread_mutex_unlock(&lock);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&lock);
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
 * 
 * @return int Exit status of the program.
 */
int main() {
    // Initialize the mutexes
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init failed");
        cleanup();
        exit(1);
    }

    // Initialize ZeroMQ
    context = zmq_ctx_new();
    requester = zmq_socket(context, ZMQ_REQ);
    subscriber_gamestate = zmq_socket(context, ZMQ_SUB);
    heartbeat_subscriber = zmq_socket(context, ZMQ_SUB);
    
    // Connect to server's REQ/REP socket
    if (zmq_connect(requester, CLIENT_CONNECT_REQ) != 0) {
        perror("Failed to connect to server");
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        exit(1);
    }

    // Connect to server's PUB socket
    if (zmq_connect(subscriber_gamestate, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        cleanup();
        exit(1);
    }

    // Subscribe to all messages
    zmq_setsockopt(subscriber_gamestate, ZMQ_SUBSCRIBE, "", 0);

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
    curs_set(FALSE); // Hide the cursor
    cbreak();
    keypad(stdscr, TRUE);
    start_color();

    // Create the threads
    pthread_t thread_client;
    pthread_t thread_comm;
    pthread_t thread_display;
    pthread_t thread_input;
    pthread_t thread_heartbeat;
    int ret;

    ret = pthread_create(&thread_client, NULL, thread_client_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_client");
        return 1;
    }
    ret = pthread_create(&thread_comm, NULL, thread_comm_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_comm");
        return 1;
    }
    ret = pthread_create(&thread_display, NULL, thread_display_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_display");
        return 1;
    }
    ret = pthread_create(&thread_input, NULL, thread_input_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_input");
        return 1;
    }
    ret = pthread_create(&thread_heartbeat, NULL, thread_heartbeat_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_heartbeat");
        return 1;
    }

    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_client, NULL);
    pthread_join(thread_comm, NULL);
    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);

    cleanup();
    exit(0);

    return 0;
}