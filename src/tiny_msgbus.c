/*
 * Copyright 2017 Rodimiro Cerrato Espinal.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MAX_PEERS 20
#define MAX_RECV_THREADS 10
#define BASE_PORT 60066
#define ZMQ_DEST_MAX_LEN 50
#define INTERNAL_CTRL_SOCKET_BIND_ADDR "inproc://tiny_msglib_ctrl"

#define LOG_INFO(f_, ...) printf((f_), ##__VA_ARGS__)
#define LOG_WARN(f_, ...) printf((f_), ##__VA_ARGS__)
#define LOG_ERR(f_, ...) fprintf(stderr, (f_), ##__VA_ARGS__)

static const int socket_timeout_in_millis = 1000;

typedef struct {
    const char* remote_ip;
    int remote_port;
} tiny_msgbus_peer_info_t;

typedef struct {
    bool valid;
    void* server_socket;
    void* client_socket;
} tiny_msgbus_peer_t;

typedef struct {
    tiny_msgbus_peer_t peers[MAX_PEERS];

    void* recv_threads_sockets[MAX_RECV_THREADS];

    void* internal_ctrl_socket;

    pthread_t thread_id;

    /**
     * ZeroMQ context
     */
    void* zmq_context;

} tiny_msgbus_instance_t;

static tiny_msgbus_instance_t tiny_msgbus_instance = {0};

static void* tiny_msgbus_thread_body(void* args) {
    tiny_msgbus_instance_t* msgbus_instance = (tiny_msgbus_instance_t*)args;

}

int tiny_msgbus_init(tiny_msgbus_peer_info_t peer_info[], size_t num_peers,
        size_t num_recv_threads) {
    if (size < 1 || size > MAX_PEERS) {
        return TINY_MSGBUS_RET_FAILURE;
    }
    if (num_recv_threads < 1 || num_recv_threads > MAX_RECV_THREADS) {
        return TINY_MSGBUS_RET_FAILURE;
    }

    void* s;
    tiny_msgbus_instance.zmq_context = zmq_ctx_new();
    s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
    zmq_bind(s, INTERNAL_CTRL_SOCKET_BIND_ADDR);
    for (size_t i = 0; i < num_recv_threads; i++) {
        s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
        
        zmq_bind(s, bidicache_singleton.bind_addr);
        tiny_msgbus_instance.recv_threads_sockets[i] = s;
    }

    for (size_t i = 0; i < num_peers; i++) {
        tiny_msgbus_instance.peers[i].server_socket = tiny_msgbus_socket(
            bidicache_singleton.zmq_context, ZMQ_SERVER);
        tiny_msgbus_instance.peers[i].client_socket = tiny_msgbus_socket(
            bidicache_singleton.zmq_context, ZMQ_CLIENT);
        tiny_msgbus_instance.peers[i].valid = true;
    }
    pthread_create(&tiny_msgbus_instance.thread_id, NULL,
        &tiny_msgbus_instance, tiny_msgbus_instance.peers);

    return TINY_MSGBUS_RET_SUCCESS;
}

int send_msg(int destination, void* buffer, size_t size) {
    chat s_name[ZMQ_DEST_MAX_LEN];
    create_zmq_inproc_dest(recv_thread_id, s_name);
    void *s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
    zmq_close(s);
}

int recv_msg(unit16_t recv_thread_id, void* buffer, size_t buffer_size, int32_t* msg_size) {
    chat s_name[ZMQ_DEST_MAX_LEN];
    create_zmq_inproc_dest(recv_thread_id, s_name);
    void *s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
    zmq_close(s);
}

int register_handler(msg_type_t msg_type, event_handle_t event_handle) {

}

int handle_message(msg_t *msg) {

}

static void*
tiny_msgbus_socket(void* zmq_ctx, int type) {
    // Create a new ZeroMQ socket.
    void* s = zmq_socket(zmq_ctx, type);
    if (s != NULL) {
        // Set the default timeout for receiving and sending data.
        zmq_setsockopt(s, ZMQ_RCVTIMEO, &socket_timeout_in_millis,
                       sizeof(socket_timeout_in_millis));
        zmq_setsockopt(s, ZMQ_SNDTIMEO, &socket_timeout_in_millis,
                       sizeof(socket_timeout_in_millis));
        // Set the linger period for socket shutdown. After this period, the
        // socket may discard any pending message sent to a peer. This is in
        // contrast to lingering infinitely.
        zmq_setsockopt(s, ZMQ_LINGER, &socket_timeout_in_millis,
                       sizeof(socket_timeout_in_millis));
        // Enable IPv6 on socket.
        int ipv6_enabled = 1;
        zmq_setsockopt(socket, ZMQ_IPV6, &ipv6_enabled, sizeof(ipv6_enabled));
    }

    return s;
}

static void create_zmq_tcp_dest(const chat* ip, uint16_t port, char* result) {
    snprintf(result, ZMQ_DEST_MAX_LEN, "tcp://[%s]:%u", ip, port);
}

static void create_zmq_inproc_dest(uint16_t id, char* result) {
    snprintf(result, ZMQ_DEST_MAX_LEN, "inproc://recv_thread_%u", ip, port);
}
