/*
 * PSIS 2024/2025 - Project Part 2
 *
 * Filename: outer-space-display.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Code that handles gameplay display. Calls space-display.c
 */

#include <zmq.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include "../src/config.h" 
#include "../src/space-display.h" 

// ZeroMQ subscriber socket
void* context;
void* subscriber_gamestate;
void* subscriber_heartbeat;

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
    endwin();
    zmq_close(subscriber_gamestate);
    zmq_close(subscriber_heartbeat);
    pthread_mutex_destroy(&lock);
}

/**
 * @brief Thread routine to handle communication with the server.
 *
 * This function continuously reads messages from the server using ZeroMQ and updates the display grid.
 * It checks for termination conditions and handles various ZeroMQ errors appropriately.
 * The function will exit the thread when the display is finished or if a critical error occurs.
 *
 * @param arg Unused argument.
 * @return None.
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
        int recv_size = zmq_recv(subscriber_gamestate, buffer, sizeof(buffer) - 1, 0);
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
 * This function serves as the entry point for the display thread. It starts
 * the display by calling `display_main()`.
 * It will exit the thread when the display is finished.
 *
 * @param arg Unused argument.
 */
void *thread_display_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the display
    display_main();

    // End thread
    pthread_mutex_lock(&lock);
    thread_display_finished = true;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

/**
 * @brief Thread routine to handle user input.
 *
 * This function continuously reads user input from the terminal. If the user
 * presses 'q' or 'Q', the program will exit. It also checks if the display
 * thread has finished and exits the program if so.
 *
 * @param arg Unused argument.
 */
void *thread_input_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            if (ch == 'q' || ch == 'Q') {
                // Exit the program
                cleanup();
                exit(0);
            }
        }

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
 * This function continuously reads heartbeat messages from the server using ZeroMQ.
 * If a heartbeat is not received or an invalid heartbeat is received, the program exits.
 *
 * @param arg Unused argument.
 */
void* thread_heartbeat_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        char buffer[2];
        int rc = zmq_recv(subscriber_heartbeat, buffer, 1, 0);
        if (rc == -1) {
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
 * @brief Main function for the outer space display application.
 *
 * This function initializes the ZeroMQ context and sockets, connects to the game server,
 * and creates threads to handle communication, display, user input, and heartbeats.
 *
 * @return int Exit status of the program.
 */
int main() {
    // Initialize the mutex
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init failed");
        cleanup();
        exit(1);
    }

    // Initialize ZeroMQ
    context = zmq_ctx_new();
    subscriber_gamestate = zmq_socket(context, ZMQ_SUB);
    subscriber_heartbeat = zmq_socket(context, ZMQ_SUB);

    // Connect to server's PUB socket
    if (zmq_connect(subscriber_gamestate, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        cleanup();
        exit(1);
    }

    // Subscribe to all messages
    zmq_setsockopt(subscriber_gamestate, ZMQ_SUBSCRIBE, "", 0);

    // Connect to server's heartbeat PUB socket
    if (zmq_connect(subscriber_heartbeat, CLIENT_CONNECT_HEARTBEAT) != 0) {
        perror("Failed to connect to game server");
        cleanup();
        exit(1);
    }

    // Subscribe to heartbeat messages
    zmq_setsockopt(subscriber_heartbeat, ZMQ_SUBSCRIBE, "", 0);
    int timeout = HEARTBEAT_FREQUENCY*2*1000; // Accepting one missed heartbeat
    zmq_setsockopt(subscriber_heartbeat, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    // Initialize ncurses
    initscr();
    noecho();
    curs_set(FALSE); // Hide the cursor
    cbreak();
    keypad(stdscr, TRUE);
    start_color();

    // Create the threads
    pthread_t thread_comm;
    pthread_t thread_display;
    pthread_t thread_input;
    pthread_t thread_heartbeat;
    int ret;
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
    pthread_join(thread_comm, NULL);
    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);

    cleanup();
    exit(0);
}