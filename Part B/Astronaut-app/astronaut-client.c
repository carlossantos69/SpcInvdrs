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
 * Code that handles astronaut client application.
 */


#include <zmq.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/config.h"
#include "../src/client-logic.h"

// ZeroMQ socket
void* context;
void* requester;

void cleanup() {
    zmq_close(requester);
    zmq_ctx_destroy(context);
    zmq_ctx_term(context);
    endwin();
}

void *thread_client_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Start the client
    client_main(requester);

    // Kill the program
    cleanup();
    exit(0);
}

void *thread_ncurses_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);


    while(1) {
        pthread_mutex_lock(&ncurses_lock);

        if (input_ready) {
            // Read char from user
            int ch = getch();
            if (ch == ERR) { // No key pressed
                ch = 'q'; // Tell the client to quit
            };

            input_buffer = ch;
            input_ready = 0;
        }
        if (output_ready) {
            // Update display

            // Update line 1
            move(0, 0);
            clrtoeol();
            mvprintw(0, 0, "%s", output_buffer_line1);

            // Line 2
            move(2, 0);
            clrtoeol();
            mvprintw(2, 0, "%s", output_buffer_line2);
            refresh();

            output_ready = 0;
        }

        pthread_mutex_unlock(&ncurses_lock);
    }


    // Exit thread
    // Note: program should not reach this point, as other threads will manage program exit
    pthread_exit(NULL);
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
    // Initialize ZeroMQ
    context = zmq_ctx_new();
    requester = zmq_socket(context, ZMQ_REQ);
    
    // Connect to server's REQ/REP socket
    if (zmq_connect(requester, CLIENT_CONNECT_REQ) != 0) {
        cleanup();
        exit(1);
    }

    // Create threads
    pthread_t thread_client;
    pthread_t thread_ncurses;
    int ret;

    ret = pthread_create(&thread_client, NULL, thread_client_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_client");
        return 1;
    }
    ret = pthread_create(&thread_ncurses, NULL, thread_ncurses_routine, NULL);
    if (ret != 0) {
        perror("Failed to create thread_ncurses");
        return 1;
    }

    // Wait for threads to finish
    // Note: program should not reach this point, as threads will manage program exit
    pthread_join(thread_client, NULL);
    pthread_join(thread_ncurses, NULL);

    cleanup();
    exit(0);
}