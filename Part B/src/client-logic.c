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
pthread_mutex_t ncurses_lock = PTHREAD_MUTEX_INITIALIZER;

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


void send_connect_message() {
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
                char error_msg[BUFFER_SIZE-10];

                // Send error message to ncurses thread
                pthread_mutex_lock(&ncurses_lock);
                sprintf(output_buffer_line1, "Error: %s", error_msg);
                sprintf(output_buffer_line2, " ");
                output_ready = 1;
                pthread_mutex_unlock(&ncurses_lock);


                // TODO: Decide if this is the correct way to handle this
                return;
            } 

            // Send success message to ncurses thread
            pthread_mutex_lock(&ncurses_lock);
            sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
            sprintf(output_buffer_line2, " ");
            output_ready = 1;
            pthread_mutex_unlock(&ncurses_lock);

        }

    } else {
        // TODO: Decide if this is the correct way to handle this
        endwin();
        exit(1);
    }
}



void handle_key_input() {
    // Wait for text output to be displayed
    // This is necessary to prevent locking in getch() without displaying the output first
    while(1) {
        pthread_mutex_lock(&ncurses_lock);
        if (output_ready == 0) {
            break;
        }
        pthread_mutex_unlock(&ncurses_lock);
    }


    // Mark input as ready
    input_ready = 1;
    pthread_mutex_unlock(&ncurses_lock);


    // Lock until ncurse thread sends input
    while(1) {
        pthread_mutex_lock(&ncurses_lock);

        if (input_ready == 0) {
            break;
        }
        pthread_mutex_unlock(&ncurses_lock);
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
            endwin();
            exit(0); // TODO: Decide if this is the correct way to handle this
            break;
        default:
            // Ignore other keys
            return;
    }

    pthread_mutex_unlock(&ncurses_lock);

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
                pthread_mutex_lock(&ncurses_lock);
                sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
                sprintf(output_buffer_line2, "Last action failed: %s ", error_msg);
                output_ready = 1;
                pthread_mutex_unlock(&ncurses_lock);
                return;
            }

            // Send new text to ncurses thread
            pthread_mutex_lock(&ncurses_lock);
            sprintf(output_buffer_line1, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", player_id, player_score);
            sprintf(output_buffer_line2, " ");
            output_ready = 1;
            pthread_mutex_unlock(&ncurses_lock);
        }


    } else {
        perror("Failed to receive response from server");
    } 
}

void client_main(void* requester) {

    req = requester;

    // Send connect message and receive player ID
    send_connect_message();
    
    // Main game loop
    while(1) {
        handle_key_input();
        usleep(10000); // 10ms delay to prevent busy waiting
    }

}