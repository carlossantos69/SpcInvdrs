/*
 * PSIS 2024/2025 - Project Part 2
 *
 * Filename: client-logic.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Logic for the client code.
 */

#include <zmq.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "client-logic.h"

// ZeroMQ socket
void* req;

// Flag to indicate if ncurses is being used
int show_ncurses;

// Client state
char player_id = '\0';
int player_score = 0;
char session_token[33]; // To store the session token received from the server

// Key input and condition variable for client inputs to process
int input_ch;
int input_ready = 0; // Flag indicating there is an input ready to be processed
pthread_mutex_t client_lock; // Mutex lock for client input
pthread_cond_t input_cond;


/**
 * @brief Constructs an error message based on the provided error code.
 *
 * This function takes an error code and a message buffer, and constructs
 * a descriptive error message by appending the appropriate error message
 * string to the buffer.
 *
 * @param code The error code indicating the type of error.
 * @param msg A buffer to store the constructed error message.
 */
void find_error(int code, char *msg) {
    strcpy(msg, "Error: ");
    switch (code) {
        case ERR_UNKNOWN_CMD: // Move up
            strcat(msg, ERR_UNKNOWN_CMD_MSG);
            break;
        case ERR_TOLONG: // Move down
            strcat(msg, ERR_TOLONG_MSG);
            break;
        case ERR_FULL: // Move left
            strcat(msg, ERR_FULL_MSG);
            break;
        case ERR_INVALID_TOKEN: // Move right
            strcat(msg, ERR_INVALID_TOKEN_MSG);
            break;
        case ERR_INVALID_PLAYERID: // Move right
            strcat(msg, ERR_INVALID_PLAYERID_MSG);
            break;
        case ERR_STUNNED: // Move right
            strcat(msg, ERR_STUNNED_MSG);
            break;
        case ERR_INVALID_MOVE: // Move right
            strcat(msg, ERR_INVALID_MOVE_MSG);
            break;
        case ERR_INVALID_DIR: // Move right
            strcat(msg, ERR_INVALID_DIR_MSG);
            break;
        case ERR_LASER_COOLDOWN: // Move right
            strcat(msg, ERR_LASER_COOLDOWN_MSG);
            break;
        default:
            strcat(msg, "Unknown error");
    }
}


/**
 * @brief Sends a connect message to the server and processes the response.
 *
 * This function sends a connect message using ZeroMQ, waits for the server's response,
 * and processes it. 
 * It then sends a message to the ncurses thread with the appropriate text to display.
 * 
 * @note This function is not thread-safe.
 * 
 * Returns 0 if the connection was successful, or -1 if an error occurred.
 */
int send_connect_message() {
    // Send connect message
    char msg[2];
    msg[0] = CMD_CONNECT;
    msg[1] = '\n';
    zmq_send(req, msg, strlen(msg), 0);

    // Receive response from server
    char buffer[BUFFER_SIZE];
    int response = ERR_UNKNOWN_CMD;
    int recv_size = zmq_recv(req, buffer, sizeof(buffer) - 1, 0);
    if (recv_size != -1) {
        buffer[recv_size] = '\0';

        // Process response
        if (sscanf(buffer, "%d %c %32s", &response, &player_id, session_token) == 3) {

            // Check is status is error
            if (response != RESP_OK) {
                // Parse error
                char error_msg[BUFFER_SIZE];

                // Update screen
                if (show_ncurses) {
                    move(0, 0);
                    clrtoeol();
                    mvprintw(0, 0, "%s", error_msg);
                    move(2, 0);
                    clrtoeol();
                    mvprintw(2, 0, " ");
                    refresh();
                }

                return -1;
            } 

            // Update screen
            if (show_ncurses) {
                move(0, 0);
                clrtoeol();
                mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
                move(2, 0);
                clrtoeol();
                mvprintw(2, 0, " ");
                refresh();
            }

        }

    } else {
        return -1;
    }
    return 0;
}



/**
 * @brief Handles user key input and communicates with the server.
 *
 * Waits for text output to be displayed, processes user input, and sends commands to the server.
 * Updates player's score based on server's response.
 * 
 * @note This function is not thread-safe.
 * 
 * @return 1 if client exits, 0 if client continues, or -1 if an error occurs.
 */
int handle_key_input() {
    // Process user input
    char buffer[BUFFER_SIZE];
    switch (input_ch) {
        case KEY_UP:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_UP);
            zmq_send(req, buffer, strlen(buffer), 0);
            break;
        case KEY_DOWN:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_DOWN);
            zmq_send(req, buffer, strlen(buffer), 0);
            break;
        case KEY_LEFT:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_LEFT);
            zmq_send(req, buffer, strlen(buffer), 0);
            break;
        case KEY_RIGHT:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_RIGHT);
            zmq_send(req, buffer, strlen(buffer), 0);
            break;
        case ' ':
            snprintf(buffer, sizeof(buffer), "%c %c %s", MSG_ZAP, player_id, session_token);
            zmq_send(req, buffer, strlen(buffer), 0);
            break;
        case 'q':
        case 'Q':
            snprintf(buffer, sizeof(buffer), "%c %c %s", CMD_DISCONNECT, player_id, session_token);
            zmq_send(req, buffer, strlen(buffer), 0);
            return 1;
            break;
        default:
            // Ignore other keys
            return 0;
    }

    // Receive response from the server
    int recv_size = zmq_recv(req, buffer, sizeof(buffer) - 1, 0);
    if (recv_size != -1) {
        buffer[recv_size] = '\0';
        
        int response;
        int new_score;
        if (sscanf(buffer, "%d %d", &response, &new_score) >= 1) {
            player_score = new_score;
            if (response != RESP_OK) {
                // Parse error
                char error_msg[BUFFER_SIZE-25];
                find_error(response, error_msg);

                // Update screen
                if (show_ncurses) {
                    move(0, 0);
                    clrtoeol();
                    mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
                    move(2, 0);
                    clrtoeol();
                    mvprintw(2, 0, "Last action failed: %s ", error_msg);
                    refresh();
                }
                return 0;
            }

            // Update screen
            if (show_ncurses) {
                move(0, 0);
                clrtoeol();
                mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
                move(2, 0);
                clrtoeol();
                mvprintw(2, 0, " ");
                refresh();
            }

            return 0;
        }


    } else {
        return -1;
    } 

    return -1;
}

/**
 * @brief Handles input key events.
 *
 * This function locks the client mutex, sets the input character and marks it as ready,
 * signals the condition variable to notify other threads, and then unlocks the mutex.
 *
 * It is used by main programs to send characters to the client logic
 * 
 * @param ch The input character to be processed.
 */
void input_key(int ch) {
    pthread_mutex_lock(&client_lock);
    input_ch = ch;
    input_ready = 1;
    pthread_cond_signal(&input_cond);
    pthread_mutex_unlock(&client_lock);
}

/**
 * @brief Main function for the client logic.
 *
 * This function initializes the necessary synchronization primitives, sends a connect message to the server,
 * and enters the main client loop to handle key inputs. It uses a mutex and condition variable to synchronize
 * input handling. The function will exit if there is a failure in initialization, connection, or if the key
 * input handling indicates to stop.
 *
 * @param requester A pointer to the requester object.
 * @param ncurses An integer flag indicating whether ncurses mode is enabled.
 */
void client_main(void* requester, int ncurses) {
    req = requester;
    show_ncurses = ncurses;

    // Initialize mutex and condition variables
    if (pthread_mutex_init(&client_lock, NULL) != 0) {
        perror("Client Mutex init failed");
        return;
    }
    if (pthread_cond_init(&input_cond, NULL) != 0) {
        perror("Client input Condition variable init failed");
        return;
    }


    // Send connect message and receive player ID
    int ret = send_connect_message();
    if (ret == -1) {
        perror("Failed to connect to server");
        return;
    }
    
    // Main client loop
    while(1) {
        pthread_mutex_lock(&client_lock);
        while (!input_ready) {
            pthread_cond_wait(&input_cond, &client_lock);
        }
        input_ready = 0;


        ret = handle_key_input();
        pthread_mutex_unlock(&client_lock);
        if (ret == 1 || ret == -1) {
            return; // Void return, no error code
        }
    }

    // Cleanup
    pthread_mutex_destroy(&client_lock);
    pthread_cond_destroy(&input_cond);

    return;
}