// astronaut-client.c

#include <ncurses.h>
#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"  // Contains SERVER_ENDPOINT definition

// ZeroMQ context and socket
void* context;
void* requester;

// Player state
char player_id = '\0';
int score = 0;
char session_token[33]; // To store the session token received from the server

void send_connect_message() {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "CONNECT");
    zmq_send(requester, buffer, strlen(buffer), 0);

    // Receive response from server
    int recv_size = zmq_recv(requester, buffer, sizeof(buffer) - 1, 0);
    if (recv_size != -1) {
        buffer[recv_size] = '\0';
        
        // Check if the response indicates server is full
        if (strncmp(buffer, "FULL", 4) == 0) {
            // Clear screen and display error
            clear();
            mvprintw(LINES/2, (COLS-strlen(buffer))/2, "%s", buffer);
            mvprintw(LINES/2 + 1, (COLS-35)/2, "Press any key to exit");
            refresh();
            
            // Wait for keypress before exiting
            nodelay(stdscr, FALSE);  // Make getch() blocking
            getch();
            
            // Clean up and exit gracefully
            endwin();
            zmq_close(requester);
            zmq_ctx_destroy(context);
            exit(0);
        }else{
            // Parse player ID and session token
            if (sscanf(buffer, "%c %32s", &player_id, session_token) != 2) {
                // Handle parsing error
                strcpy(session_token, "");
            }
            // Initialize display with player ID and session token
            move(0, 0);
            clrtoeol();
            mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", 
                     player_id, score);
            refresh();
        }
        
        // Normal connection success handling
        sscanf(buffer, "%c", &player_id);
        mvprintw(0, 0, "Playing as Astronaut %c", player_id);
        refresh();
    } else {
        perror("Failed to receive player ID");
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        exit(1);
    }
}

void update_score(int new_score) {
    score = new_score;
    // Update score display
    mvprintw(0, 0, "Score: %d", score);
    clrtoeol();
    refresh();
}

void handle_key_input() {
    int ch = getch();
    if (ch == ERR) return; // No key pressed

    char buffer[BUFFER_SIZE];
    switch (ch) {
        case KEY_UP:
            snprintf(buffer, sizeof(buffer), "MOVE %c %s UP", player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_DOWN:
            snprintf(buffer, sizeof(buffer), "MOVE %c %s DOWN", player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_LEFT:
            snprintf(buffer, sizeof(buffer), "MOVE %c %s LEFT", player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case KEY_RIGHT:
            snprintf(buffer, sizeof(buffer), "MOVE %c %s RIGHT", player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case ' ':
            snprintf(buffer, sizeof(buffer), "ZAP %c %s", player_id, session_token);
            zmq_send(requester, buffer, strlen(buffer), 0);
            break;
        case 'q':
        case 'Q':
            snprintf(buffer, sizeof(buffer), "DISCONNECT %c %s", player_id, session_token);
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
        char status[32];
        int new_score;
        if (sscanf(buffer, "%31s %d", status, &new_score) >= 1) {
            // Update score if provided
            if (sscanf(buffer, "%*s %d", &new_score) == 1) {
                score = new_score;
            }

            // Update display
            move(0, 0);
            clrtoeol();
            mvprintw(0, 0, "Astronaut %c | Score: %d | Use arrow keys to move, space to fire laser, 'q' to quit", 
                     player_id, score);
            if (strcmp(status, "ERROR") == 0) {
                mvprintw(1, 0, "Last action failed: %s", buffer);
            }
            refresh();
        }
    } else {
        perror("Failed to receive response from server");
    }
}

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