#pragma once

#include <time.h>
#include "ascii_str.h"

enum data_socket_mode {
  SOCKET_PASSIVE,
  SOCKET_ACTIVE,
};

enum session_state {
  SESSION_LOGIN_REQUIRED, /* PASS reuqired */
  SESSION_ACTIVE,         /* logged in. accepts commands */
  SESSION_INVALID,
};

struct session_sockets {
  int control_sockfd;
  int data_sockfd;

  enum data_socket_mode mode;
};

struct session {
  time_t last_seen;

  enum session_state state;

  struct session_sockets sockets;

  struct ascii_str ip;
  struct ascii_str port;

  struct ascii_str working_dir; /**< the root directory. 'user space' is considered to be <working_dir>/<user_name>.
                                   working_dir better be an absolute path*/
  struct ascii_str username;
  struct ascii_str password;
  struct ascii_str current_dir;
};

/**
 * @brief takes ownership over `ip`, `port`, `username` and `password`
 *
 * @param ip
 * @param port
 * @param username
 * @param working_dir
 * @param state
 * @param control_sockfd
 * @return struct session
 */
struct session session_create(struct ascii_str *restrict ip,
                              struct ascii_str *restrict port,
                              struct ascii_str *restrict username,
                              struct ascii_str *restrict password,
                              struct ascii_str *restrict working_dir,
                              int control_sockfd);

void session_destroy(struct session *session);
