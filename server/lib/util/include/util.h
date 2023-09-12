#pragma once

#include <stdbool.h>

bool sig_handler_install(int signum, void (*sig_handler)(int));

void session_destroy_wrapper(void *session);
