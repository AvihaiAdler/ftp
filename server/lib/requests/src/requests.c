#include "requests.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

#include "algorithm.h"

#define CRLF "\r\n"

static enum requests_result get_last_error(int err) {
  switch (err) {
    case EAGAIN:
      return REQUEST_EAGAIN;
    case EBADF:
    case EISCONN:   // fallthrough
    case ENOTSOCK:  // fallthrough
      return REQUEST_INVALID_SOCKFD;
    case EINVAL:
    case EOPNOTSUPP:  // fallthrough
      return REQUEST_INVALID_ARGS;
    case EMSGSIZE:
      return REQUEST_TOO_LONG;
    default:
      return REQUEST_ERROR;
  }
}

enum requests_result requests_send(int sockfd, int flags, struct ascii_str *restrict request) {
  if (sockfd < 0) return REQUEST_INVALID_SOCKFD;
  if (!request || !ascii_str_len(request)) return REQUEST_INVALID_ARGS;

  char const *buf = ascii_str_c_str(request);
  size_t len = ascii_str_len(request);

  size_t sent = 0;
  do {
    ssize_t ret = send(sockfd, buf + sent, len - sent, flags);
    if (ret == -1) { return get_last_error(errno); }

    sent += ret;

  } while (sent < len);

  return REQUEST_OK;
}

enum requests_result requests_recieve(int sockfd, int flags, struct ascii_str *restrict request) {
  if (sockfd < 0) return REQUEST_INVALID_SOCKFD;
  if (!request) return REQUEST_INVALID_ARGS;

  size_t crlf_len = strlen(CRLF);

  char buf[REQUEST_MAX_LENGTH];
  size_t recieved = 0;
  do {
    ssize_t ret = recv(sockfd, buf, sizeof buf, flags);
    if (ret == -1) return get_last_error(errno);

    recieved += ret;

    // potential bottleneck. maybe just check the last 2 chars?
    if (match(buf, recieved, CRLF, crlf_len)) break;

  } while (recieved < (size_t)REQUEST_MAX_LENGTH);

  char *end = match(buf, recieved, CRLF, crlf_len);
  if (!end) { return REQUEST_TOO_LONG; }

  *request = ascii_str_from_arr(buf, recieved);
  return REQUEST_OK;
}
