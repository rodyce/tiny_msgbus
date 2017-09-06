/*
 * Copyright (C) 2017 Hewlett Packard Enterprise Development LP
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential
 * to the Hewlett Packard Enterprise Development LP. No part of
 * this program  may be photocopied, reproduced, or translated
 * into another programming language without prior written consent
 * of the Hewlett Packard Enterprise Development LP.
 *
 *************************************************************************/

#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

/*******************************************************************************
 * INCLUDED FILES
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <zmq.h>
#include <unity.h>

/*******************************************************************************
 * DEFINITIONS
 ******************************************************************************/

#define NUM_PEERS 10

/*******************************************************************************
 * PRIVATE TYPES
 ******************************************************************************/


/*******************************************************************************
 * PRIVATE DATA
 ******************************************************************************/

static volatile bool g_signal_received;
static bool          g_init_ok;
static pid_t         g_child_pids[NUM_PEERS] = {0};
uint32_t             current_instance_id_in_test       = 0;

/*******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/


static void sig_handler(int signo);

static bool aggressive_poll_for_secs(time_t secs, bool (*action)());

static void
sig_handler(__attribute__((unused)) int signo) {
    g_signal_received = true;
}

static bool
aggressive_poll_for_secs(time_t secs, bool (*action)()) {
    time_t start = time(NULL);
    do {
        if (action()) {
            return true;
        }
    } while (time(NULL) - start < secs);
    return false;
}

/*******************************************************************************
 * SETUP, TEARDOWN
 ******************************************************************************/

void
setUp() {
    // Handle the SIGUSR1 signal.
    signal(SIGUSR1, sig_handler);

    // Set initial state for globals.
    g_signal_received = false;
    g_init_ok         = false;
    memset(g_child_pids, 0, sizeof(g_child_pids));
}

void
tearDown() {
    int status;
    for (int i = 0; i < NUM_PEERS; i++) {
        if (g_child_pids[i] == 0) {
            continue;
        }
        // Send the SIGUSR1 signal to the child process so it may resume
        // from pause().
        kill(g_child_pids[i], SIGUSR1);
        // Wait for the child process to exit.
        waitpid(g_child_pids[i], &status, 0);
    }
    if (g_init_ok) {
        // Destroy the msgbus instance if it had been successfully
        // initialized within the test.
    }
}

/*******************************************************************************
 * TESTS
 ******************************************************************************/


void
test_inproc_pair() {
    void* context = zmq_ctx_new();
    int rc;
    bool running = true;

    int t_args[][2] = {
        {1, 10}, {2, 50}
    };

    void* thread_body(void* args) {
        void* s = zmq_socket(context, ZMQ_PAIR);
        int rc;
        int tno = ((int*)args)[0];
        int data = ((int*)args)[1];
        char destination[50];
        snprintf(destination, 50, "inproc://internal%d", tno);
        rc = zmq_connect(s, destination);
        if (rc != 0) {
            VLOG_ERR("Error connecting from thread %d\n", tno);
        }
        while (running) {
            if (tno == 2 && data >= 60) {
                break;
            }
            rc = zmq_send(s, &data, sizeof(data), 0);
            if (rc != 0 && errno != EAGAIN) {
                VLOG_ERR("Error sending from thread %d\n", tno);
            }
            // 500 milliseconds
            usleep(500000);
            data++;
        }
        zmq_close(s);
        return NULL;
    }

    void* internal_socket1 = zmq_socket(context, ZMQ_PAIR);
    rc = zmq_bind(internal_socket1, "inproc://internal1");
    assert (rc == 0);

    pthread_t thread[2];
    for (int t = 0; t < 2; t++) {
        running = true;
        pthread_create(&thread[t], NULL, &thread_body, t_args[t]);
    }

    int recv_data = 0;
    for (int t = 0; t < 2; t++) {
    for (int m = 0; m < 15; m++) {
        zmq_recv(internal_socket1, &recv_data, sizeof(recv_data), 0);
        printf("Received from socket %d: [%d]\n", m, recv_data);
        recv_data = 0;
    }
    }

    for (int t = 0; t < 2; t++) {
        running = false;
        pthread_join(thread[t], NULL);
    }

    zmq_close(internal_socket1);
}

int
main() {
    UNITY_BEGIN();

    //RUN_TEST(test_inproc_pair);

    return UNITY_END();
}
