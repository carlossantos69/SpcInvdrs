/*
 * PSIS 2024/2025 - Project Part 1
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
void* subscriber;

/**
 * @brief Updates the game grid and player statuses based on the provided update message.
 *
 * This function clears the current grid and player statuses, then parses the update message
 * to update the grid with new player positions, alien positions, laser beams, and player scores.
 * It also sets a flag if the game is over.
 *
 * @param msg A string containing the update information, with each line representing
 *            a different update command (e.g., player positions, alien positions, laser beams, scores).
 *
 * @note This function is not thread safe and a mutex must be locked before the function is called.
 */
void update_grid(char* msg) {
    // Clear the grid first
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
        }
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        players_disp[i].active = 0;
    }

     // Parse the message line by line
    char* line = strtok(msg, "\n");
    while (line != NULL) {
        if (line[0] == CMD_GAME_OVER) {
            game_over_display = 1; // Set a flag to indicate the game is over


        } else if (line[0] == CMD_PLAYER) {
            char id;
            int x, y;
            sscanf(line, "%*c %c %d %d", &id, &x, &y);
            
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = id;
                // Update player status
                int idx = id - 'A';
                if (idx >= 0 && idx < MAX_PLAYERS) {
                    players_disp[idx].id = id;
                    players_disp[idx].active = 1;
                }
            }
        } else if (line[0] == CMD_ALIEN) {
            // Format: ALIEN <x> <y>
            int x, y;
            sscanf(line, "%*c %d %d", &x, &y);
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                grid[y][x].ch = '*'; // Represent aliens with '*'
            }
        } else if (line[0] == CMD_LASER) {
            int x, y;
            int zone;
            sscanf(line, "%*c %d %d %d", &x, &y, &zone);
            
            time_t now = time(NULL);
            
            if (zone == ZONE_A || zone == ZONE_H) {
                for (int i = x; i < GRID_WIDTH; i++) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                    grid[y][i].laser_time = now;
                }
            } else if (zone == ZONE_D || zone == ZONE_F) {
                for (int i = x; i >= 0; i--) {
                    grid[y][i].ch = LASER_HORIZONTAL;
                    grid[y][i].laser_time = now;
                }
            }
            if (zone == ZONE_E || zone == ZONE_G) {
                for (int i = y; i < GRID_HEIGHT; i++) {
                    grid[i][x].ch = LASER_VERTICAL;
                    grid[i][x].laser_time = now;
                }
            } else if (zone == ZONE_B || zone == ZONE_C) {
                for (int i = y; i >= 0; i--) {
                    grid[i][x].ch = LASER_VERTICAL;
                    grid[i][x].laser_time = now;
                }
            }
            
        } else if (line[0] == CMD_SCORE) {
            char id;
            int player_score;
            sscanf(line, "%*c %c %d", &id, &player_score);
            
            int idx = id - 'A';
            if (idx >= 0 && idx < MAX_PLAYERS) {
                players_disp[idx].active = 1;
                players_disp[idx].score = player_score;
            }
        }
        // Move to the next line
        line = strtok(NULL, "\n");
    }
}

void *thread_comm_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Initialize the ZeroMQ context and subscriber socket
    context = zmq_ctx_new();
    subscriber = zmq_socket(context, ZMQ_SUB);

    // Connect to server's PUB socket
    if (zmq_connect(subscriber, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        exit(1);
    }

    // Subscribe to all messages
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // Read messages from the server and update the display grid
    while (!game_over_display) {
        char buffer[BUFFER_SIZE];
        int recv_size = zmq_recv(subscriber, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        if (recv_size != -1) {
            buffer[recv_size] = '\0';

            // Update the game grid based on the received message
            pthread_mutex_lock(&display_lock);
            update_grid(buffer);
            pthread_mutex_unlock(&display_lock);
        } else {
            int err = zmq_errno();
            if (err == EAGAIN) {
                // No message received, continue
            } else if (err == ETERM || err == ENOTSOCK) {
                // The context was terminated or socket invalid, exit program
                exit(1);
            } else {
                // Exit program on other errors
                exit(1);
            }
        }
    }

    // Note: no need to close the socket or context, as the cleanup function will handle it

    pthread_exit(NULL);
}


void *thread_display_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the display
    display_main();

    // Exit the program
    exit(0);
}

void *thread_input_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // TODO: In the future, when a new thread handles communication and data is in shared memory using mutex, this function can read game_over state and exit on any key press.

    while (1) {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            if (ch == 'q' || ch == 'Q' || game_over_display) {
                // Exit the program
                exit(0);
            }
        }
    }
}

// Function to be called on exit
void cleanup() {
    // Destroy the mutex
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    zmq_ctx_term(context);
    pthread_mutex_destroy(&display_lock);
    endwin();
}

int main() {
    pthread_t thread_comm;
    pthread_t thread_display;
    pthread_t thread_input;
    int ret;

    // Register the cleanup function to be called at exit
    atexit(cleanup);

    // Initialize the mutex
    if (pthread_mutex_init(&display_lock, NULL) != 0) {
        perror("Mutex init failed");
        return EXIT_FAILURE;
    }

    // Initialize grid to empty spaces
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            grid[y][x].ch = ' ';
        }
    }

    // Initialize player scores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players_disp[i].id = '\0';
        players_disp[i].score = 0;
    }

    // Create the threads
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

    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_comm, NULL);
    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);

    exit(0);
}

// TODO: use atexit() to destroy the mutex and do other stuff