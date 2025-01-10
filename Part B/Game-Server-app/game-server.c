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

/**
 * @brief Thread routine for the game server.
 *
 * This function serves as the entry point for the game server thread. It starts the game logic
 * by calling the server_logic function and handles any errors that occur. Upon completion,
 * it sets the thread_server_finished flag to true and exits the thread.
 *
 * @param arg Unused argument.
 * @return None.
 */
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

/**
 * @brief Thread routine to send heartbeat messages at regular intervals.
 *
 * This function runs in a separate thread and sends a heartbeat message
 * every second to indicate that the server is alive. It continues to send
 * these messages until the `thread_server_finished` flag is set to true.
 *
 * @param arg Unused argument.
 * @return void* Always returns NULL when the thread exits.
 */
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

/**
 * @brief Thread routine to display game state data.
 *
 * This function runs in a separate thread and continuously updates the display
 * with the current game state from the server. It locks a mutex to check if the
 * display thread should finish, copies the game state string from the server to
 * the display, and then unlocks the mutex. The function will block in 
 * get_server_game_state or set_display_game_state until data is received or sent.
 *
 * @param arg Unused argument.
 * @return None.
 */
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


/**
 * @brief Thread routine to start the display and mark the display thread as finished.
 *
 * This function serves as the entry point for a thread that initializes and starts the display
 * by calling the `display_main()` function. Once the display is started, it marks the display
 * thread as finished by setting the `thread_display_finished` flag to true and then exits the thread.
 *
 * @param arg Unused argument.
 */
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

/**
 * @brief Thread routine to handle input from the standard input.
 *
 * This function runs in a loop, reading a single character from the standard input.
 * If the character is 'q' or 'Q', it triggers the end game logic and breaks the loop.
 * It also checks if either the display or server threads have finished and exits the loop if so.
 * After exiting the input loop, it enters another loop to check if both game over flags are set,
 * and if so, it performs cleanup and exits the program.
 *
 * @param arg Unused argument.
 * @return None.
 */
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
 * @brief Main function to initialize and run the game server.
 * 
 * This function sets up the ZeroMQ context and various sockets for communication
 * with astronaut clients, display clients, and for broadcasting game state, scores,
 * and heartbeat signals. It also initializes ncurses for terminal display and creates
 * multiple threads to handle server operations, heartbeat signals, display data, 
 * display updates, and user input. The function ensures proper cleanup and resource 
 * deallocation in case of errors.
 * 
 * @return int Exit status of the program.
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