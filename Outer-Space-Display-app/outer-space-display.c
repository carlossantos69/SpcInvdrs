/*
 * PSIS 2024/2025 - Project Part 1
 *
 * Filename: outer-space-display.c
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Description:
 * Code that handles gameplay display. Calls space-display.c
 */

#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include "../src/config.h" 
#include "../src/space-display.h" 

/**
 * @brief Main function to initialize and run the Outer Space Display application.
 *
 * This function sets up the ZeroMQ context and subscriber socket, connects to the game server,
 * subscribes to all incoming messages, and starts the display. It also handles cleanup of
 * ZeroMQ resources before exiting.
 *
 * @return int Returns 0 on successful execution, or 1 if there is a failure in connecting to the server.
 */
int main() {
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
}