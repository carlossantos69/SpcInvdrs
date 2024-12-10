// This code needs to include space-display.h to have the display logic
// Display logic removed from game-server.c

#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <zmq.h>
#include "config.h"
#include "game-logic.h"
#include "space-display.h"


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
        return 0;

    } else {
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
    }
}