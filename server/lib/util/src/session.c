#include "session.h"
#include <unistd.h>

// TODO: default data_sockfd shouldn't be -1
struct session session_create(struct ascii_str *restrict ip,
                              struct ascii_str *restrict port,
                              struct ascii_str *restrict username,
                              struct ascii_str *restrict password,
                              struct ascii_str *restrict working_dir,
                              int control_sockfd) {
  if (!working_dir || ascii_str_empty(working_dir)) goto session_create_invalid;
  if (!username || ascii_str_empty(username)) goto session_create_invalid;
  if (!password) *password = ascii_str_create(NULL, 0);

  if (!ip || ascii_str_empty(ip)) goto session_create_invalid;
  if (!port || ascii_str_empty(port)) goto session_create_invalid;

  struct ascii_str wd = ascii_str_create(ascii_str_c_str(working_dir), ascii_str_len(working_dir));
  if (!ascii_str_contains(working_dir, ascii_str_c_str(&wd))) goto session_create_invalid;

  return (struct session){
    .ip = *ip,
    .port = *port,
    .sockets = {.control_sockfd = control_sockfd, .data_sockfd = -1, .mode = SOCKET_ACTIVE},
    .state = SESSION_LOGIN_REQUIRED,
    .username = *username,
    .password = *password,
    .working_dir = wd,
    .current_dir = ascii_str_create(NULL, 0),
    .last_seen = time(NULL),
  };

session_create_invalid:
  return (struct session){.sockets = {.control_sockfd = -1, .data_sockfd = -1, .mode = SOCKET_ACTIVE},
                          .state = SESSION_INVALID};
}

void session_destroy(struct session *session) {
  if (!session) return;

  close(session->sockets.control_sockfd);
  close(session->sockets.data_sockfd);

  ascii_str_destroy(&session->ip);
  ascii_str_destroy(&session->port);

  ascii_str_destroy(&session->username);
  ascii_str_destroy(&session->current_dir);
}
