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
#include "../src/config.h" 
#include "../src/space-display.h" 


void *thread_display_routine(void *arg) {
    // Avoid unused argument warning
    (void)arg;

    // Initialize ZeroMQ sockets
    void* cont = zmq_ctx_new();
    void* sub = zmq_socket(cont, ZMQ_SUB);

    // Connect to server's PUB socket
    if (zmq_connect(sub, CLIENT_CONNECT_SUB) != 0) {
        perror("Failed to connect to game server");
        return NULL;
    }

    // Subscribe to all messages
    zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);

    // Start the display
    display_main(sub);

    // Close ZeroMQ sockets
    zmq_close(sub);
    zmq_ctx_destroy(cont);
    zmq_ctx_term(cont);

    return NULL;
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
                endwin();
                exit(0);
            }
        }
    }
}



int main() {
    pthread_t thread_display;
    pthread_t thread_input;
    int ret;

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

    pthread_join(thread_display, NULL);
    pthread_join(thread_input, NULL);
    return 0;
}