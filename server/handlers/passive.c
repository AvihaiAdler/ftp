#include "passive.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // close() write()
#include "misc/util.h"
#include "util.h"

int passive(void *arg) {
  if (!arg) return 1;
  struct args *args = arg;

  // find the session
  struct session *tmp_session = vector_s_find(args->sessions, &(struct session){.fds.control_fd = args->remote_fd});
  if (!tmp_session) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] failed to find the session for fd [%d]",
               thrd_current(),
               __func__,
               tmp_session->context.ip,
               tmp_session->context.port,
               args->remote_fd);
    send_reply_wrapper(args->remote_fd,
                       args->logger,
                       RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                       "[%d] %s",
                       RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                       str_reply_code(RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR));
    return 1;
  }

  struct session session = {0};
  memcpy(&session, tmp_session, sizeof session);
  free(tmp_session);

  struct list *ips = get_local_ip();
  int pasv_fd = -1;
  for (size_t i = 0; i < list_size(ips); i++) {
    pasv_fd = get_passive_socket(args->logger, (const char *)list_at(ips, i), NULL, 1, AI_PASSIVE);

    if (pasv_fd != -1) break;  // successfuly got a passive socket
  }

  list_destroy(ips, NULL);
  if (pasv_fd == -1) {  // failed to get a passive socket
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] failed to open a passive data socket for [%s:%s]",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port);

    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_CANNOT_OPEN_DATA_CONN,
                                                 "[%d] %s",
                                                 RPLY_CANNOT_OPEN_DATA_CONN,
                                                 str_reply_code(RPLY_CANNOT_OPEN_DATA_CONN));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);
    return 1;
  }

  // close the old sockfd
  switch (session.data_sock_type) {
    case ACTIVE:
      close(session.fds.data_fd);
      break;
    case PASSIVE:
      close(session.fds.listen_sockfd);
      break;
  }

  // update the session
  session.data_sock_type = PASSIVE;
  session.fds.listen_sockfd = pasv_fd;

  bool update = update_session(args->sessions, args->logger, &session);
  if (!update) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] failed to update the session for [%s:%s]",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port);

    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 "[%d] %s",
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 str_reply_code(RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);
    return 1;
  }

  // signify the main thread to update its pollfds
  uint64_t discard = 1;
  ssize_t ret = write(args->event_fd, &discard, sizeof discard);  // write the value into event_fd

  if (ret == -1) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] failed to write to event_fd [%d]",
               thrd_current(),
               __func__,
               args->event_fd);

    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 "[%d] %s",
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 str_reply_code(RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);
    return 1;
  }

  // get the ip and port to send to the client
  char local_ip[INET6_ADDRSTRLEN];
  char pasv_port[NI_MAXSERV];
  get_ip_and_port(session.fds.listen_sockfd, local_ip, sizeof local_ip, pasv_port, sizeof pasv_port);

  // send a feedback
  enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                               args->logger,
                                               RPLY_PASSIVE,
                                               "[%d] %s. (%s,%s)",
                                               RPLY_PASSIVE,
                                               str_reply_code(RPLY_PASSIVE),
                                               local_ip,
                                               pasv_port);
  handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

  logger_log(args->logger,
             ERROR,
             "[%lu] [%s] created a passive socket for client [%s:%s]. socket ip & port [%s:%s]",
             thrd_current(),
             __func__,
             session.context.ip,
             session.context.port,
             local_ip,
             pasv_port);

  return 0;
}
