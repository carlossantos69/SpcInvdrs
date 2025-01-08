/*
 * PSIS 2024/2025 - Project Part 1
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

// Ncurses buffers and flags
int input_buffer;
char output_buffer_line1[BUFFER_SIZE];
char output_buffer_line2[BUFFER_SIZE];
int input_ready = 0; // 1 if client is ready to receive keypress input
int output_ready = 0; // 1 if client has output to display
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER; //TODO: rename to client_lock

// Client state
char player_id = '\0';
int player_score = 0;
char session_token[33]; // To store the session token received from the server

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

                // Send error message to ncurses thread
                pthread_mutex_lock(&client_lock);
                sprintf(output_buffer_line1, "%s", error_msg);
                sprintf(output_buffer_line2, " ");
                output_ready = 1;
                pthread_mutex_unlock(&client_lock);

                return -1;
            } 

            // Send success message to ncurses thread
            pthread_mutex_lock(&client_lock);
            sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
            sprintf(output_buffer_line2, " ");
            output_ready = 1;
            pthread_mutex_unlock(&client_lock);

        }

    } else {
        return -1;
    }
    return 0;
}



/**
 * @brief Handles user key input and communicates with the server.
 *
 * This function waits for the text output to be displayed, marks the input as ready,
 * and processes the user input based on the key pressed. It sends the appropriate
 * command to the server using ZeroMQ and updates the player's score based on the server's response.
 * 
 * Returns 1 if client exits, 0 if client continues, or -1 if an error occurs(invalid move/zap etc.. is not considered an error).
 */
int handle_key_input() {
    // Wait for text output to be displayed
    // This is necessary to prevent locking in getch() without displaying the output first
    while(1) {
        pthread_mutex_lock(&client_lock);
        if (output_ready == 0) {
            break;
        }
        pthread_mutex_unlock(&client_lock);
    }


    // Mark input as ready
    input_ready = 1;
    pthread_mutex_unlock(&client_lock);


    // Lock until ncurse thread sends input
    while(1) {
        pthread_mutex_lock(&client_lock);

        if (input_ready == 0) {
            // No mutex unlock here, as we will continue doing more work
            break;
        }
        pthread_mutex_unlock(&client_lock);
    }

    // Process user input
    char buffer[BUFFER_SIZE];
    switch (input_buffer) {
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
            pthread_mutex_unlock(&client_lock); // Should not be needed because client_main will return aswell
            return 1;
            break;
        default:
            // Ignore other keys
            pthread_mutex_unlock(&client_lock); // Unlock mutex before returning
            return 0;
    }

    pthread_mutex_unlock(&client_lock);

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

                // Send error message to ncurses thread
                pthread_mutex_lock(&client_lock);
                sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
                sprintf(output_buffer_line2, "Last action failed: %s ", error_msg);
                output_ready = 1;
                pthread_mutex_unlock(&client_lock);
                return 0;
            }

            // Send new text to ncurses thread
            pthread_mutex_lock(&client_lock);
            sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
            sprintf(output_buffer_line2, " ");
            output_ready = 1;
            pthread_mutex_unlock(&client_lock);

            return 0;
        }


    } else {
        return -1;
    } 

    return -1;
}

void client_main(void* requester) {
    req = requester;

    int ret;

    // Send connect message and receive player ID
    ret = send_connect_message();
    if (ret == -1) {
        return; // Void return, no error code
    }
    
    // Main client loop
    while(1) {
        ret = handle_key_input();
        if (ret == 1 || ret == -1) {
            return; // Void return, no error code
        }
        usleep(10000); // 10ms delay to prevent busy waiting
    }

}