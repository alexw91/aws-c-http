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

struct aws_http_connection {
    struct aws_channel_handler handler;
    struct aws_channel_slot *slot;
    struct aws_http_decoder *decoder;
    aws_http_on_response_fn *on_response;
    aws_http_on_header_fn *on_header;
    aws_http_on_body_fn *on_body;
    void *user_data;
};
