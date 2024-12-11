/*
 * File: game-server.c
 * Author: Carlos Santos e Tomás Corral
 * Description: This file implements the game sever functionality.
 * Date: 11/12/2024
 */

#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <zmq.h>
#include "../common-files/config.h" 
#include "../common-files/game-logic.h" 
#include "../common-files/space-display.h" 


int main() {
    pid_t pid;

    // Create a new process
    pid = fork();

    if (pid < 0) {
        perror("Failed to fork");
        return 1;
    } else if (pid == 0) {
        // Child process, game display

        // Initialize ZeroMQ sockets
        void* cont = zmq_ctx_new();
        void* disp_sub = zmq_socket(cont, ZMQ_SUB);

        // Connect to server's PUB socket
        if (zmq_connect(disp_sub, CLIENT_CONNECT_SUB) != 0) {
            perror("Failed to connect to game server");
            return 1;
        }

        // Subscribe to all messages
        zmq_setsockopt(disp_sub, ZMQ_SUBSCRIBE, "", 0);

        // Start the display
        display_main(disp_sub);

        // Close ZeroMQ sockets
        zmq_close(disp_sub);
        zmq_ctx_destroy(cont);
        zmq_ctx_term(cont);
        exit(0);

    } else if (pid > 0) {
        // Parent process, game logic

        // Initialize ZeroMQ context
        void* cont = zmq_ctx_new();

        // Set up REQ/REP socket for astronaut clients
        void* resp = zmq_socket(cont, ZMQ_REP);
        zmq_bind(resp, SERVER_ENDPOINT_REQ);

        // Set up PUB socket for display client
        void* pub = zmq_socket(cont, ZMQ_PUB);
        zmq_bind(pub, SERVER_ENDPOINT_PUB);

        // Start the game logic
        game_logic(resp, pub);
        

        // Close ZeroMQ sockets
        zmq_close(resp);
        zmq_close(pub);
        zmq_ctx_destroy(cont);
        zmq_ctx_term(cont);

        // Wait for child process to finish
        wait(NULL);
    } else {
        // Fork failed
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}