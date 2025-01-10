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
void* publisher_gamestate;  // For PUB/SUB with display
void* publisher_scores;  // For PUB/SUB with scores
void* publisher_heartbeat;  // For PUB/SUB with heartbeat

// Flags to indicate thread ending
pthread_mutex_t lock;
bool thread_server_finished = false;
bool thread_display_finished = false;

void cleanup() {
    zmq_close(responder);
    zmq_close(publisher_gamestate);
    zmq_close(publisher_scores);
    zmq_close(publisher_heartbeat);
    zmq_ctx_destroy(context);
    zmq_ctx_term(context);
    pthread_mutex_destroy(&lock);
    endwin();
}

void* thread_server_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the game logic
    int ret = server_logic(responder, publisher_gamestate, publisher_scores);
    if (ret != 0) {
        perror("Error in server_logic");
        cleanup();
        exit(1);
    }
    
    // Note; no need to close the socket or context, as the cleanup function will handle it

    // End thread
    pthread_mutex_lock(&lock);
    thread_server_finished = true;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

void* thread_heartbeat_routine(void* arg) {
    // Avoid unused argument warning
    (void)arg;

    while (1) {
        // Send a heartbeat message every second
        zmq_send(publisher_heartbeat, "H", 1, 0);
        sleep(HEARTBEAT_FREQUENCY);

        pthread_mutex_lock(&lock);
        if (thread_server_finished) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
    }

    // End thread
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


        // Copy the game state string in the server to the display
        // Display will then parse it to get the state
        // This bypasses the use of sockets for the display created by the game server
        char buffer[BUFFER_SIZE];
        get_server_game_state(buffer);
        set_display_game_state(buffer);

        // Note: No need to sleep here, function will not active wait
        //  because it will be blocked in get_server_game_state or set_display_game_state and only unblocks when data is received/sent

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
                // End game logic
                end_server_logic();
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
    pthread_t thread_heartbeat;
    pthread_t thread_display_data;
    pthread_t thread_display;
    pthread_t thread_input;
    int ret;

    // Initialize zeroMQ context
    context = zmq_ctx_new();

    // Set up REQ/REP socket for astronaut clients
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, SERVER_ENDPOINT_REQ);

    // Set up PUB socket for display client
    publisher_gamestate = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_gamestate, SERVER_ENDPOINT_PUB);

    // Set up PUB socket for scores
    publisher_scores = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_scores, SERVER_ENDPOINT_SCORES);

    // Set up PUB socket for heartbeat
    publisher_heartbeat = zmq_socket(context, ZMQ_PUB);
    zmq_bind(publisher_heartbeat, SERVER_ENDPOINT_HEARTBEAT);


    // Initialize the mutex
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init failed");
        cleanup();
        exit(1);
    }

    // Initialize ncurses mode
    initscr();
    noecho();
    curs_set(FALSE); // Hide the cursor
    cbreak();
    keypad(stdscr, TRUE);
    start_color();

    // Create the threads
    ret = pthread_create(&thread_server, NULL, thread_server_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_server");
        cleanup();
        exit(1);
    }
    ret = pthread_create(&thread_heartbeat, NULL, thread_heartbeat_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_heartbeat");
        cleanup();
        exit(1);
    }
    ret = pthread_create(&thread_display_data, NULL, thread_display_data_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_display_data");
        cleanup();
        exit(1);
    }
    ret = pthread_create(&thread_display, NULL, thread_display_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_display");
        cleanup();
        exit(1);
    }
    ret = pthread_create(&thread_input, NULL, thread_input_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_input");
        cleanup();
        exit(1);
    }

    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_server, NULL);
    pthread_join(thread_display_data, NULL);
    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);

    cleanup();
    exit(0);
}