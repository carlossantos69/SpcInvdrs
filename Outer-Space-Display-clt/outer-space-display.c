/*
 * File: game-logic.c
 * Author: Carlos Santos e Tomás Corral
 * Description: This file implements the outer space display program.
 * Date: 11/12/2024
 */

#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include "../common-files/config.h" 
#include "../common-files/space-display.h" 

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