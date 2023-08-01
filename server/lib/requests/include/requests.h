#pragma once

#include <linux/limits.h>  // PATH_MAX
#include <stddef.h>
#include "ascii_str.h"

#define REQUEST_MAX_LENGTH PATH_MAX

enum requests_result {
  REQUEST_OK,
  REQUEST_ERROR,  // general error
  REQUEST_INVALID_SOCKFD,
  REQUEST_INVALID_ARGS,
  REQUEST_CONN_CLOSED,
  REQUEST_EAGAIN,
  REQUEST_TOO_LONG,
};

/**
 * @brief sends a request to a socket
 *
 * @param[in] sockfd - a socket file descriptor
 * @param[in] flags - flags to apply upon sending
 * @param[in] request - the request to send
 * @return `enum requests_result` - `REQUEST_OK` on success, REQUEST_* otherwise
 */
enum requests_result requests_send(int sockfd, int flags, struct ascii_str *restrict request);

/**
 * @brief recieves a request from a socket
 *
 * @param[in] sockfd - a socket file descriptor
 * @param[in] flags - flags to apply upon sending
 * @param[out] request - a pointer to a `ascii_str` struct. the recived request will be placed there
 * @return `enum requests_result` - `REQUEST_OK` on success. the recieved request will be placed in `request`. REQUEST_*
 * otherwise
 *
 * one should make sure `request` points to a valid `struct ascii_str`. one shouldn't pass a used `ascii_str` into
 * `requests_recieve` without destroying it first
 */
enum requests_result requests_recieve(int sockfd, int flags, struct ascii_str *restrict request);
