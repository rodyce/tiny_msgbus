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

#define MAX_MSG_HANDLERS 100
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
  const char *remote_ip;
  int remote_port;
} tiny_msgbus_peer_info_t;

typedef struct {
  bool valid;
  void *client_socket;
} tiny_msgbus_peer_t;

typedef struct {
  size_t num_peers;
  tiny_msgbus_peer_t peers[MAX_PEERS];

  size_t num_recv_threads;
  void *recv_threads_sockets[MAX_RECV_THREADS];

  void *server_socket;
  void *internal_ctrl_socket;

  event_handle_t event_handlers[MAX_MSG_HANDLERS];

  pthread_t thread_id;

  /**
   * ZeroMQ context
   */
  void *zmq_context;

} tiny_msgbus_instance_t;

static void *tiny_msgbus_thread_body(void *args) {
  tiny_msgbus_instance_t *msgbus_instance = (tiny_msgbus_instance_t *)args;

  zmq_pollitem_t items[] = {
      {.socket = msgbus_instance->server_socket, .events = ZMQ_POLLIN},
      {.socket = msgbus_instance->internal_ctrl_socket, .events = ZMQ_POLLIN}};

  // Poll for internal control sockets and server socket. No poll timeout
  // defined for now.
  int zmq_ret = zmq_poll(items, 2, -1);
  if (zmq_ret < 0) {
    switch (zmq_errno()) {
    case ETERM:
      // Thread going down

      break;
    }
  }
}

int tiny_msgbus_init(tiny_msgbus_peer_info_t peer_info[], size_t num_peers,
                     size_t num_recv_threads,
                     void **new_tiny_msgbus_instance) {

  // Check input parameters ranges
  if (new_tiny_msgbus_instance == NULL) {
    return TINY_MSGBUS_RET_FAILURE;
  }
  if (size < 1 || size > MAX_PEERS) {
    return TINY_MSGBUS_RET_FAILURE;
  }
  if (num_recv_threads < 1 || num_recv_threads > MAX_RECV_THREADS) {
    return TINY_MSGBUS_RET_FAILURE;
  }

  // Declare useful local dummy variables
  char dummy_zmq_dest[ZMQ_DEST_MAX_LEN] = {0};
  void *s;
  int ret = TINY_MSGBUS_RET_SUCCESS;
  tiny_msgbus_instance_t *tiny_msgbus_instance = NULL;

  tiny_msgbus_instance = malloc(sizeof(tiny_msgbus_instance_t));

  // Create new ZeroMQ context
  tiny_msgbus_instance.zmq_context = zmq_ctx_new();

  // Create and bind inproc internal control socket
  s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
  zmq_bind(s, INTERNAL_CTRL_SOCKET_BIND_ADDR);

  // Create and bind inproc sockets to communicate with recv threads
  for (size_t i = 0; i < num_recv_threads; i++) {
    s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);

    create_zmq_inproc_dest(i, dummy_zmq_dest);
    zmq_bind(s, dummy_zmq_dest);
    tiny_msgbus_instance.recv_threads_sockets[i] = s;
  }

  // Create and bind a single server socket to receive messages from
  // external peers (TCP IPv4 or IPv6, if desired). Port will be bound to
  // BASE_PORT
  tiny_msgbus_instance.server_socket =
      tiny_msgbus_socket(bidicache_singleton.zmq_context, ZMQ_SERVER);
  create_zmq_tcp_dest(dummy_zmq_dest, BASE_PORT);
  zmq_bind(tiny_msgbus_instance.server_socket, dummy_zmq_dest);

  // Create client sockets to receive messages to be forwarded to the recv
  // threads.
  for (size_t i = 0; i < num_peers; i++) {
    tiny_msgbus_instance.peers[i].client_socket =
        tiny_msgbus_socket(bidicache_singleton.zmq_context, ZMQ_CLIENT);
    tiny_msgbus_instance.peers[i].valid = true;
  }

  // If everything went right up to this point, proceed to create the
  // tiny_msgbus thread. All sockets created in this thread are migrated
  // to that thread.
  pthread_create(&tiny_msgbus_instance.thread_id, NULL, tiny_msgbus_thread_body,
                 tiny_msgbus_instance);
__exit:
  if (ret == TINY_MSGBUS_RET_SUCCESS) {
    *new_tiny_msgbus_instance = tiny_msgbus_instance;
  } else {
    if (tiny_msgbus_instance != NULL) {
      free(tiny_msgbus_instance);
    }
    *new_tiny_msgbus_instance = NULL;
  }
  return ret;
}

int tiny_msgbus_deinit(void* a_tiny_msgbus_instance) {
    if (a_tiny_msgbus_instance == NULL) {
        return TINY_MSGBUS_RET_FAILURE;
    }

    tiny_msgbus_instance_t* tiny_msgbus_instance = (tiny_msgbus_instance_t*)a_tiny_msgbus_instance;

    // Terminate ZeroMQ context. Proceed once all its associated are closed.
    zmq_ctx_term(tiny_msgbus_instance->zmq_context);

    pthread_join(tiny_msgbus_instance->thread_id, NULL);

    memset(tiny_msgbus_instance, 0, sizeof(tiny_msgbus_instance_t));
}

int send_msg(void* a_tiny_msgbus_instance, int destination, void *buffer, size_t size) {
  char s_name[ZMQ_DEST_MAX_LEN];
  void *s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
  
  create_zmq_inproc_dest(recv_thread_id, s_name);

  zmq_connect(s, s_name);
  zmq_send(s, buffer, size, 0);

  zmq_close(s);
}

int recv_msg(void* a_tiny_msgbus_instance, unit16_t recv_thread_id, void *buffer, size_t buffer_size,
             int32_t *msg_size) {
  if (a_tiny_msgbus_instance == NULL) {
      return TINY_MSGBUS_RET_FAILURE;
  }
    tiny_msgbus_instance_t* tiny_msgbus_instance = instance_cast(a_tiny_msgbus_instance);
  chat s_name[ZMQ_DEST_MAX_LEN];
  create_zmq_inproc_dest(recv_thread_id, s_name);
  void *s = zmq_recv(tiny_msgbus_instance->recv_threads_sockets[recv_thread_id], buffer,
    buffer_size, 0);
}

int register_handler(tiny_msgbus_instance_t* a_tiny_msgbus_instance,
msg_type_t msg_type, event_handle_t event_handle) {
    if (a_tiny_msgbus_instance == NULL) {
        return TINY_MSGBUS_RET_FAILURE;
    }
  char s_name[ZMQ_DEST_MAX_LEN];
  void *s = zmq_socket(tiny_msgbus_instance.zmq_context, ZMQ_PAIR);
  
  create_zmq_inproc_dest(recv_thread_id, s_name);

  zmq_connect(s, s_name);
  zmq_send(s, &event_handle, sizeof(event_handle_t), 0);

  zmq_close(s);
}

int handle_message(void* a_tiny_msgbus_instance, msg_t *msg) {
    if (a_tiny_msgbus_instance == NULL) {
        return TINY_MSGBUS_RET_FAILURE;
    }
    if (msg == NULL) {
        return TINY_MSGBUS_RET_FAILURE;
    }
    tiny_msgbus_instance_t* tiny_msgbus_instance = instance_cast(a_tiny_msgbus_instance);

    msg_type_t msg_type = msg->msg_type;

    if (tiny_msgbus_instance->event_handlers[msg_type] != NULL) {
        tiny_msgbus_instance->event_handlers[msg_type](msg);
    }

    return TINY_MSGBUS_RET_SUCCESS;
}

static void *tiny_msgbus_socket(void *zmq_ctx, int type) {
  // Create a new ZeroMQ socket.
  void *s = zmq_socket(zmq_ctx, type);
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

static void create_zmq_tcp_dest(const chat *ip, uint16_t port, char *result) {
  snprintf(result, ZMQ_DEST_MAX_LEN, "tcp://[%s]:%u", ip, port);
}

static void create_zmq_inproc_dest(uint16_t id, char *result) {
  snprintf(result, ZMQ_DEST_MAX_LEN, "inproc://recv_thread_%u", ip, port);
}

static tiny_msgbus_instance_t* instance_cast(void* a_tiny_msgbus_instance) {
    return (tiny_msgbus_instance_t*)a_tiny_msgbus_instance;
}

static int close_all_sockets(void* a_tiny_msgbus_instance) {
    if (a_tiny_msgbus_instance == NULL) {
        return TINY_MSGBUS_RET_FAILURE;
    }

    tiny_msgbus_instance_t* tiny_msgbus_instance = instance_cast(a_tiny_msgbus_instance);
    zmq_close(tiny_msgbus_instance->server_socket);
    zmq_close(tiny_msgbus_instance->internal_ctrl_socket);



    return TINY_MSGBUS_RET_SUCCESS;
}
