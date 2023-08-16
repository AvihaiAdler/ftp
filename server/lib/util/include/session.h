#pragma once

#include "ascii_str.h"

/**
 * @brief represent the state of the session
 *
 * the mode state represented by bits.
 * the first (LSB) bit represent the state of the data socket
 * the next bit represent the state of the session itself (logged in or not)
 * i.e.
 * state = LOGGED_IN | DATA_SOCKET_MODE
 */
enum session_state {
  SESSION_PASSIVE,
  SESSION_ACTIVE = 1,
  SESSION_LOGGED_IN = 1 << 1,
};

struct session {
  int control_sockfd;
  int data_sockfd;
  enum session_state mode;
  struct ascii_str ip;
  struct ascii_str port;
};
