#pragma once

#include <stdint.h>
#include <sys/types.h>  // ssize_t

#define REQUEST_MAX_LEN 512
#define REPLY_MAX_LEN 4096
#define DATA_BLOCK_MAX_LEN 2048

enum err_codes {
  ERR_SUCCESS = 0,
  ERR_INVALID_SOCKET_FD,
  ERR_INVALID_RPLY_CODE,
  ERR_INVALID_LEN,
  ERR_SOCKET_TRANSMISSION_ERR,
  ERR_INVALID_ARGS
};

enum reply_codes {
  RPLY_DATA_CONN_OPEN_STARTING_TRANSFER = 125,
  RPLY_FILE_OK_OPEN_DATA_CONN = 150,
  RPLY_CMD_OK = 200,
  RPLY_SERVICE_READY = 220,
  RPLY_CLOSING_CTRL_CONN = 221,
  RPLY_DATA_CONN_OPEN_NO_TRANSFER = 225,
  RPLY_PASSIVE = 227,
  RPLY_FILE_ACTION_COMPLETE = 250,
  RPLY_PATHNAME_CREATED = 257,
  RPLY_CANNOT_OPEN_DATA_CONN = 425,
  RPLY_DATA_CONN_CLOSED = 426,
  RPLY_FILE_ACTION_NOT_TAKEN_FILE_BUSY = 450,
  RPLY_FILE_ACTION_NOT_TAKEN_PROCESS_ERROR = 451,
  RPLY_FILE_ACTION_NOT_TAKEN_NOT_ENOUGH_SPACE = 452,
  RPLY_CMD_SYNTAX_ERR = 500,
  RPLY_CMD_ARGS_SYNTAX_ERR = 501,
  RPLY_FILE_ACTION_NOT_TAKEN_FILE_UNAVAILABLE = 550,
  RPLY_FILE_ACTION_NOT_TAKEN_INVALID_FILENAME = 553,
};

enum descriptor_codes {
  DESCPTR_EOF = 0x40,  // 64. specifies EOF for the last block of a file
};

enum request_type {
  REQ_UNKNOWN,
  REQ_PWD,
  REQ_CWD,
  REQ_MKD,
  REQ_RMD,
  REQ_PORT,
  REQ_PASV,
  REQ_DELE,
  REQ_LIST,
  REQ_RETR,
  REQ_STOR,
  REQ_QUIT,
};

struct reply {
  uint16_t code;
  uint16_t length;
  uint8_t reply[REPLY_MAX_LEN];
};

struct request {
  uint16_t length;
  uint8_t request[REQUEST_MAX_LEN];
};

struct data_block {
  uint8_t descriptor;
  uint16_t length;
  uint8_t data[DATA_BLOCK_MAX_LEN];
};

/* sends a reply. returns ERR_SUCCESS on success */
int send_reply(struct reply *reply, int sockfd, int flags);

/* recieve a reply. returns ERR_SUCCESS on success. returns reply:reply as a null terminated string */
int recieve_reply(struct reply *reply, int sockfd, int flags);

/* sends a request. returns ERR_SUCCESS on success */
int send_request(struct request *request, int sockfd, int flags);

/* recieve a request. returns ERR_SUCCESS on success. returns request::request as a null terminated string */
int recieve_request(struct request *request, int sockfd, int flags);

/* sends a data block 'as is'. returns ERR_SUCCESS on success */
int send_data(struct data_block *data, int sockfd, int flags);

/* recieves a data block. returns ERR_SUCCESS on success. data_block::data is not necessarily a null terminated string
 */
int receive_data(struct data_block *data, int sockfd, int flags);

/* converts an enum err_codes to its string representation */
const char *str_err_code(enum err_codes err_code);

/* converts an enum reply_codes to its string representation */
const char *str_reply_code(enum reply_codes reply_code);

/* converts an enum request_type to its string representation */
const char *str_request_type(enum request_type request_type);