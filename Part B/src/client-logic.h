/*
 * PSIS 2024/2025 - Project Part 1
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