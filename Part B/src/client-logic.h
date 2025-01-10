/*
 * PSIS 2024/2025 - Project Part 2
 *
 * Filename: client-logic.h
 *
 * Authors:
 * - Carlos Santos - 102985 - carlos.r.santos@tecnico.ulisboa.pt
 * - Tomas Corral  - 102446 - tomas.corral@tecnico.ulisboa.pt
 *
 * Group ID: 20
 *
 * Description:
 * Header file for client-logic.h
 */


#ifndef CLIENT_LOGIC_H
#define CLIENT_LOGIC_H

#include "config.h"
#include "pthread.h"

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
void find_error(int code, char *msg);

/**
 * @brief Sends a connect message to the server and processes the response.
 *
 * This function sends a connect message using ZeroMQ, waits for the server's response,
 * and processes it. 
 * It then sends a message to the ncurses thread with the appropriate text to display.
 * 
 * @note This function is not thread-safe.
 * 
 * Returns 0 if the connection was successful, or -1 if an error occurred.
 */
int send_connect_message();

/**
 * @brief Handles user key input and communicates with the server.
 *
 * Waits for text output to be displayed, processes user input, and sends commands to the server.
 * Updates player's score based on server's response.
 * 
 * @note This function is not thread-safe.
 * 
 * @return 1 if client exits, 0 if client continues, or -1 if an error occurs.
 */
int handle_key_input();

/**
 * @brief Handles input key events.
 *
 * This function locks the client mutex, sets the input character and marks it as ready,
 * signals the condition variable to notify other threads, and then unlocks the mutex.
 *
 * It is used by main programs to send characters to the client logic
 * 
 * @param ch The input character to be processed.
 */
void input_key(int ch);

/**
 * @brief Main function for the client logic.
 *
 * This function initializes the necessary synchronization primitives, sends a connect message to the server,
 * and enters the main client loop to handle key inputs. It uses a mutex and condition variable to synchronize
 * input handling. The function will exit if there is a failure in initialization, connection, or if the key
 * input handling indicates to stop.
 *
 * @param requester A pointer to the requester object.
 * @param ncurses An integer flag indicating whether ncurses mode is enabled.
 */
void client_main(void* requester, int ncurses);

#endif