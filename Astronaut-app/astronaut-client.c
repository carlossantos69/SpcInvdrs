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
 * Code that handles game server application.
 */


#include <ncurses.h>
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/config.h" 

// ZeroMQ context and socket
void* context;
void* requester;

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
 * @brief Sends a connect message to the server and handles the response.
 * 
 * This function sends a connect message to the server using ZeroMQ, waits for the response,
 * and processes it. If the response indicates an error, it displays the error message and exits.
 * Otherwise, it initializes the display with the player ID and session token.
 */
void send_connect_message() {
    char msg[2];
    msg[0] = CMD_CONNECT;
    msg[1] = '\n';
    zmq_send(requester, msg, strlen(msg), 0);

    // Receive response from server
    char buffer[BUFFER_SIZE];
    int response = ERR_UNKNOWN_CMD;
    int recv_size = zmq_recv(requester, buffer, sizeof(buffer) - 1, 0);
    if (recv_size != -1) {
        buffer[recv_size] = '\0';
        
        if (sscanf(buffer, "%d %c %32s", &response, &player_id, session_token) == 3) {

            // Check is status is error
            if (response != RESP_OK) {
                char error_msg[BUFFER_SIZE];
                find_error(response, error_msg);
                // Clear screen and display error
                clear();
                mvprintw(LINES/2, (COLS-strlen(buffer))/2, "%s", buffer);
                mvprintw(LINES/2 + 1, (COLS-35)/2, "%s", error_msg);
                refresh();
                
                // Wait for keypress before exiting
                nodelay(stdscr, FALSE);  // Make getch() blocking
                getch();
                
                // Clean up and exit gracefully
                endwin();
                zmq_close(requester);
                zmq_ctx_destroy(context);
                exit(0);
            } 

            // Initialize display with player ID and session token
            move(0, 0);
            clrtoeol();
            mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", 
                    player_id, player_score);
            move(2, 0);
            clrtoeol(); 
            refresh();
        
        }
    } else {
        perror("Failed to receive player ID");
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        exit(1);
    }
}


/**
 * @brief Handles key input from the user and sends appropriate commands to the server.
 * 
 * This function captures key presses and sends corresponding commands to the server
 * using ZeroMQ. It handles movement commands (arrow keys), a zap command (space bar),
 * and a disconnect command ('q' or 'Q'). It also processes the server's response to
 * update the player's score and display status messages.
 */
void handle_key_input() {
    int ch = getch();
    if (ch == ERR) return; // No key pressed

    char buffer[BUFFER_SIZE];
    switch (ch) {
        case KEY_UP:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_UP);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_DOWN:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_DOWN);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_LEFT:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_LEFT);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_RIGHT:
            snprintf(buffer, sizeof(buffer), "%c %c %s %c", CMD_MOVE, player_id, session_token, MOVE_RIGHT);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case ' ':
            snprintf(buffer, sizeof(buffer), "%c %c %s", MSG_ZAP, player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case 'q':
        case 'Q':
            snprintf(buffer, sizeof(buffer), "%c %c %s", CMD_DISCONNECT, player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            // Clean up and exit
            endwin();
            zmq_close(requester);
            zmq_ctx_destroy(context);
            exit(0);
            break;
        default:
            // Ignore other keys
            return;
    }

    // Receive response from the server
    int recv_size = zmq_recv(requester, buffer, sizeof(buffer) - 1, 0);
    if (recv_size != -1) {
        buffer[recv_size] = '\0';
        
        // Parse response to get status and score
        int response;
        int new_score;
        if (sscanf(buffer, "%d %d", &response, &new_score) >= 1) {
            if (response != RESP_OK) {
                char error_msg[BUFFER_SIZE];
                find_error(response, error_msg);
                mvprintw(2, 0, "Last action failed. %s", error_msg);
                refresh();
                return;
            }
            player_score = new_score;

            // Update display
            move(0, 0);
            clrtoeol();
            mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", 
                     player_id, player_score);
            // Clear error line
            move(2, 0);
            clrtoeol();   
            refresh();
        }
    } else {
        perror("Failed to receive response from server");
    }
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
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    // Initialize ZeroMQ
    context = zmq_ctx_new();
    requester = zmq_socket(context, ZMQ_REQ);
    
    // Connect to server's REQ/REP socket
    if (zmq_connect(requester, CLIENT_CONNECT_REQ) != 0) {
        perror("Failed to connect to server");
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        exit(1);
    }

    // Send connect message and receive player ID
    send_connect_message();
    
    // Main game loop
    while(1) {
        handle_key_input();
        usleep(10000); // 10ms delay to prevent busy waiting
    }

    // Cleanup
    endwin();
    zmq_close(requester);
    zmq_ctx_destroy(context);
    return 0;
}