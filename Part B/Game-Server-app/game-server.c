/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: game-server.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 * 
 * Description:
 * Code that handles game server application. Calls game.logic.c and space-display.c
 */

#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <zmq.h>
#include <pthread.h>
#include <ncurses.h>
#include "../src/config.h" 
#include "../src/game-logic.h" 
#include "../src/space-display.h" 

// ZeroMQ context and sockets
void* context;
void* responder;  // For REQ/REP with astronauts
void* publisher;  // For PUB/SUB with display
void* scores_publisher;  // For PUB/SUB with scores

// Flags to indicate thread ending
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
bool thread_server_finished = false;
bool thread_display_finished = false;

void cleanup() {
    zmq_close(responder);
    zmq_close(publisher);
    zmq_close(scores_publisher);
    zmq_ctx_destroy(context);
    zmq_ctx_term(context);
    pthread_mutex_destroy(&server_lock);
    pthread_mutex_destroy(&display_lock);
    pthread_mutex_destroy(&lock);
    endwin();
}

void* thread_server_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    // Initialize ZeroMQ context
    context = zmq_ctx_new();

    // Set up REQ/REP socket for astronaut clients
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, SERVER_ENDPOINT_REQ);

    // Set up PUB socket for display client
    publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher, SERVER_ENDPOINT_PUB);

    scores_publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind(scores_publisher, SERVER_ENDPOINT_SCORES);

    // Start the game logic
    game_logic(responder, publisher, scores_publisher);
    
    // Note; no need to close the socket or context, as the cleanup function will handle it

    // End thread
    pthread_mutex_lock(&lock);
    thread_server_finished = true;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

void* thread_display_data_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        pthread_mutex_lock(&lock);
        if (thread_display_finished) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        char buffer[BUFFER_SIZE];

        // Copy the game state string in the server to a buffer
        // This is to release the lock the fastest possible
        pthread_mutex_lock(&server_lock);
        strcpy(buffer, game_state_server);
        pthread_mutex_unlock(&server_lock);


        // Copy the game state string in the server to the display
        // Display will then parse it to get the state
        // This bypasses the use of sockets for the display created by the game server
        pthread_mutex_lock(&display_lock);
        strcpy(game_state_display, buffer);
        pthread_mutex_unlock(&display_lock);

    }

    // End thread
    pthread_exit(NULL);
}

void* thread_display_routine(void* arg) {
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

void* thread_input_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            if (ch == 'q' || ch == 'Q') {
                // Set flag to notify game over
                pthread_mutex_lock(&server_lock);
                game_over_server = 1;
                pthread_mutex_unlock(&server_lock);
                break;
            }

            pthread_mutex_lock(&lock);
            if (thread_display_finished || thread_server_finished) {
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
        }
    }

    while(1) {
        // Exit the program if both game over flags are set
        // Both flags are checked to ensure that the game over state was processed by the display
        pthread_mutex_lock(&lock);
        if (thread_display_finished && thread_server_finished) {
            cleanup();
            exit(0);
        }
        pthread_mutex_unlock(&lock);
    }
}

/**
 * @brief Main function to start the game server and display.
 *
 * This function creates a new process using fork(). The child process is responsible for
 * the game display, while the parent process handles the game logic. 
 * The parent process sets up REQ/REP and PUB sockets for communication
 * with astronaut clients and the display client, respectively.
 *
 * @return int Returns 0 on successful execution, 1 on failure to fork.
 */
int main() {
    pthread_t thread_server;
    pthread_t thread_display_data;
    pthread_t thread_display;
    pthread_t thread_input;
    int ret;

    // Initialize the mutexes
    if (pthread_mutex_init(&display_lock, NULL) != 0) {
        perror("Mutex init failed");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&server_lock, NULL) != 0) {
        perror("Mutex init failed");
        return EXIT_FAILURE;
    }

    // Create the threads
    ret = pthread_create(&thread_server, NULL, thread_server_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_server");
        return 1;
    }
    ret = pthread_create(&thread_display_data, NULL, thread_display_data_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_display_data");
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

    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_server, NULL);
    pthread_join(thread_display_data, NULL);
    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);

    cleanup();
    exit(0);
}