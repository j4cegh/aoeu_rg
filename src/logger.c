#include "logger.h"
#include "utils.h"
#include <time.h>

void log_normal(char *msg)
{
	log_to_file("norm", msg);
}

void log_warning(char *msg)
{
	log_to_file("warn", msg);
}

void log_error(char *msg)
{
	log_to_file("err", msg);
}

void log_msg(log_level level, char *msg)
{
	switch(level)
	{
		case loglev_normal: { log_normal(msg); break; };
		case loglev_warning: { log_warning(msg); break; };
		case loglev_error: { log_error(msg); break; };
		default: log_to_file("unknown", msg);
	}
}

void log_to_file(char *type, char *msg)
{
	make_dir(LOGGER_FOLDER);

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	char *date = dupfmt("%d-%d-%d", tm.tm_mon + 1, tm.tm_mday, 1900 + tm.tm_year);

	char *formatted = dupfmt("[%s %02d:%02d:%02d] (%s): %s\n", date, tm.tm_hour, tm.tm_min, tm.tm_sec, type, msg);

	char *filename = dupfmt(LOGGER_FOLDER "%s.txt", date);

	FILE *f = fopen(filename, "a");
	if(f)
	{
		fputs(formatted, f);
		fclose(f);
	}

	free(filename);
	free(formatted);
	free(date);
}
