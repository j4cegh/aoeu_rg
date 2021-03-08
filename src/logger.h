#pragma once

#define LOGGER_FOLDER "./logs/"

typedef enum log_level
{
    loglev_normal,
    loglev_warning,
    loglev_error
} log_level;

void log_normal(char *msg);
void log_warning(char *msg);
void log_error(char *msg);
void log_msg(log_level level, char *msg);
void log_to_file(char *type, char *msg);
