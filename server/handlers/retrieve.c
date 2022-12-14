#include "retrieve.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // stat
#include "misc/util.h"
#include "util.h"

int retrieve_file(void *arg) {
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

  // check the session has a valid data connection
  if (session.fds.data_fd == -1) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] invalid data_sockfd",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port);
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_DATA_CONN_CLOSED,
                                                 "[%d] %s",
                                                 RPLY_DATA_CONN_CLOSED,
                                                 str_reply_code(RPLY_DATA_CONN_CLOSED));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);
    return 1;
  }

  // validate the file path
  if (!validate_path(args->req_args.request_args, args->logger)) {
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 "[%d] %s",
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 str_reply_code(RPLY_CMD_ARGS_SYNTAX_ERR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

    return 1;
  }

  // get file path
  struct string *path = get_path(&session);
  if (!path) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] get_path() failure",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port);
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 "[%d] %s",
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 str_reply_code(RPLY_CMD_ARGS_SYNTAX_ERR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

    return 1;
  }

  size_t args_len = strlen(args->req_args.request_args);

  // path too long
  if (string_length(path) + 1 + args_len + 1 > MAX_PATH_LEN - 1) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] path too long",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port);
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_CMD_SYNTAX_ERR,
                                                 "[%d] %s",
                                                 RPLY_CMD_SYNTAX_ERR,
                                                 str_reply_code(RPLY_CMD_SYNTAX_ERR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

    string_destroy(path);
    return 1;
  }

  string_concat(path, "/");
  string_concat(path, args->req_args.request_args);

  // open the file
  FILE *fp = fopen(string_c_str(path), "r");
  int fp_fd = fileno(fp);
  if (!fp || fp_fd == -1) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] invalid path or file doesn't exists [%s]",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port,
               string_c_str(path));
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 "[%d] %s",
                                                 RPLY_CMD_ARGS_SYNTAX_ERR,
                                                 str_reply_code(RPLY_CMD_ARGS_SYNTAX_ERR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

    string_destroy(path);
    return 1;
  }

  struct stat statbuf = {0};
  int ret = fstat(fp_fd, &statbuf);
  if (ret == -1) {
    logger_log(args->logger,
               WARN,
               "[%lu] [%s] [%s:%s] invalid file descriptor [%s]",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port,
               string_c_str(path));
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 "[%d] %s",
                                                 RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR,
                                                 str_reply_code(RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

    string_destroy(path);
    return 1;
  }

  struct file_size file_size = get_file_size(statbuf.st_size);
  enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                               args->logger,
                                               RPLY_DATA_CONN_OPEN_STARTING_TRANSFER,
                                               "[%d] %s. %Lf%s",
                                               RPLY_DATA_CONN_OPEN_STARTING_TRANSFER,
                                               str_reply_code(RPLY_DATA_CONN_OPEN_STARTING_TRANSFER),
                                               file_size.size,
                                               file_size.units);
  handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);

  // read the file and send it
  struct data_block data = {0};
  bool successful_transfer = true;
  bool done = false;
  do {
    size_t bytes_read = fread(data.data, sizeof *data.data, DATA_BLOCK_MAX_LEN, fp);
    data.length = (uint16_t)bytes_read;

    if (bytes_read < DATA_BLOCK_MAX_LEN) {
      if (ferror(fp)) {  // encountered an error
        successful_transfer = false;
        break;
      }
      if (feof(fp)) {  // reached the end of file
        data.descriptor = DESCPTR_EOF;
        done = true;
      }
    }

    // failed to send a data block
    if (send_data(&data, session.fds.data_fd, 0) != ERR_SUCCESS) {
      successful_transfer = false;
      break;
    }

  } while (!done);
  fclose(fp);

  // send feedback
  if (successful_transfer) {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] the file [%s] successfully transfered",
               thrd_current(),
               __func__,
               session.context.ip,
               session.context.port,
               string_c_str(path) + string_length(session.context.root_dir) + 1);
    enum err_codes err_code = send_reply_wrapper(session.fds.control_fd,
                                                 args->logger,
                                                 RPLY_FILE_ACTION_COMPLETE,
                                                 "[%d] %s",
                                                 RPLY_FILE_ACTION_COMPLETE,
                                                 str_reply_code(RPLY_FILE_ACTION_COMPLETE));
    handle_reply_err(args->logger, args->sessions, &session, args->epollfd, err_code);
  } else {
    logger_log(args->logger,
               ERROR,
               "[%lu] [%s] [%s:%s] process error",
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
  }
  string_destroy(path);
  return 0;
}
