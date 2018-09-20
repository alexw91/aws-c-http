/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/http/connection.h>
#include <aws/http/http_decode.h>

struct aws_http_connection {
    struct aws_channel_handler handler;
    struct aws_channel_slot *slot;
    struct aws_http_decoder *decoder;
    aws_http_on_response_fn *on_response;
    aws_http_on_header_fn *on_header;
    aws_http_on_body_fn *on_body;
    struct aws_socket_endpoint *endpoint;
    void *user_data;
};

static int s_handler_process_read_message(
        struct aws_channel_handler *handler,
        struct aws_channel_slot *slot,
        struct aws_io_message *message) {
    (void)handler;
    (void)slot;
    (void)message;
    return AWS_OP_SUCCESS;
}

int s_handler_process_write_message(
        struct aws_channel_handler *handler,
        struct aws_channel_slot *slot,
        struct aws_io_message *message) {
    (void)handler;
    (void)slot;
    (void)message;
    return AWS_OP_SUCCESS;
}

int s_handler_increment_read_window(struct aws_channel_handler *handler, struct aws_channel_slot *slot, size_t size) {
    (void)handler;
    (void)slot;
    (void)size;
    return AWS_OP_SUCCESS;
}


size_t s_handler_initial_window_size(struct aws_channel_handler *handler) {
    (void)handler;
    return 0;
}

void s_handler_destroy(struct aws_channel_handler *handler) {
    (void)handler;
}

int s_handler_shutdown(
        struct aws_channel_handler *handler,
        struct aws_channel_slot *slot,
        enum aws_channel_direction dir,
        int error_code,
        bool free_scarce_resources_immediately) {
    (void)handler;
    (void)slot;
    (void)dir;
    (void)error_code;
    (void)free_scarce_resources_immediately;
    return AWS_OP_SUCCESS;
}

struct aws_channel_handler_vtable s_channel_handler = {
    s_handler_process_read_message,
    s_handler_process_write_message,
    s_handler_increment_read_window,
    s_handler_shutdown,
    s_handler_initial_window_size,
    s_handler_destroy
};

int s_client_channel_setup(
    struct aws_client_bootstrap *bootstrap,
    int error_code,
    struct aws_channel *channel,
    void *user_data) {
    (void)bootstrap;
    (void)error_code;
    (void)channel;
    (void)user_data;

    struct aws_http_connection *connection = (struct aws_http_connection *)user_data;

    struct aws_channel_slot *slot = aws_channel_slot_new(channel);
    if (!slot) {
        /* TODO (randgaul: Report error somehow. */
        return AWS_OP_ERR;
    }
    connection->slot = slot;

    aws_channel_slot_insert_end(channel, connection->slot);
    aws_channel_slot_set_handler(connection->slot, &connection->handler);

    return AWS_OP_SUCCESS;
}

int s_client_channel_shutdown(
    struct aws_client_bootstrap *bootstrap,
    int error_code,
    struct aws_channel *channel,
    void *user_data) {
    (void)bootstrap;
    (void)error_code;
    (void)channel;
    (void)user_data;
    return 0;
}

int s_server_channel_setup(
    struct aws_server_bootstrap *bootstrap,
    int error_code,
    struct aws_channel *channel,
    void *user_data) {
    (void)bootstrap;
    (void)error_code;
    (void)channel;
    (void)user_data;
    return AWS_OP_SUCCESS;
}

int s_server_channel_shutdown(
    struct aws_server_bootstrap *bootstrap,
    int error_code,
    struct aws_channel *channel,
    void *user_data) {
    (void)bootstrap;
    (void)error_code;
    (void)channel;
    (void)user_data;
    return AWS_OP_SUCCESS;
}

struct aws_http_connection *aws_http_client_connection_new(
    struct aws_allocator *alloc,
    struct aws_socket_endpoint *endpoint,
    struct aws_socket_options *socket_options,
    struct aws_tls_connection_options *tls_options,
    struct aws_client_bootstrap *bootstrap,
    aws_http_on_header_fn *on_header,
    aws_http_on_body_fn *on_body,
    void *user_data) {
    (void)on_header;
    (void)on_body;
    (void)user_data;

    struct aws_http_connection *connection = (struct aws_http_connection *)aws_mem_acquire(alloc, sizeof(struct aws_http_connection));
    if (!connection) {
        return NULL;
    }

    struct aws_channel_handler handler;
    handler.vtable = s_channel_handler;
    handler.alloc = alloc;
    handler.impl = (void*)connection;
    connection->handler = handler;

    if (tls_options) {
        if (aws_client_bootstrap_new_tls_socket_channel(
            bootstrap,
            endpoint,
            socket_options,
            tls_options,
            s_client_channel_setup,
            s_client_channel_shutdown,
            (void*)connection)) {
            goto cleanup;
        }
    } else {
        if (aws_client_bootstrap_new_socket_channel(
            bootstrap,
            endpoint,
            socket_options,
            s_client_channel_setup,
            s_client_channel_shutdown,
            (void*)connection)) {
            goto cleanup;
        }
    }

    return connection;

cleanup:

    return NULL;
}

struct aws_http_connection *aws_http_server_connection_new(
    struct aws_allocator *alloc,
    struct aws_socket_endpoint *endpoint,
    struct aws_socket_options *socket_options,
    struct aws_tls_connection_options *tls_options,
    struct aws_server_bootstrap *bootstrap,
    aws_http_on_header_fn *on_header,
    aws_http_on_body_fn *on_body,
    void *user_data) {
    (void)on_header;
    (void)on_body;
    (void)user_data;

    struct aws_http_connection *connection = (struct aws_http_connection *)aws_mem_acquire(alloc, sizeof(struct aws_http_connection));

    if (!connection) {
        return NULL;
    }

    if (tls_options) {
        if (aws_server_bootstrap_add_tls_socket_listener(
            bootstrap,
            endpoint,
            socket_options,
            tls_options,
            s_server_channel_setup,
            s_server_channel_shutdown,
            NULL)) {
            goto cleanup;
        }
    } else {
        if (aws_server_bootstrap_add_socket_listener(
            bootstrap,
            endpoint,
            socket_options,
            s_server_channel_setup,
            s_server_channel_shutdown,
            NULL)) {
            goto cleanup;
        }
    }

    return connection;

cleanup:

    return NULL;
}

void aws_http_connection_destroy(struct aws_http_connection *connection) {
    (void)connection;
}
