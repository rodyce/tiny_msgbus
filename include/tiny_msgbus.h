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

#ifndef TINY_MSGBUS_H
#define TINY_MSGBUS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define TINY_MSGBUS_RET_SUCCESS 0
#define TINY_MSGBUS_RET_FAILURE -1

int send_msg(int destination, void* buffer, size_t size);

int recv_msg(void* buffer, size_t buffer_size, int32_t* msg_size);

int register_handler(msg_type_t msg_type, event_handle_t event_handle);

int handle_message(msg_t *msg);

#endif /* TINY_MSGBUS_H */
