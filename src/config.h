#pragma once

#include "includes.h"
#include "list.h"

#define CONFIG_FILENAME "options.cfg"

void config_init();
void config_deinit();

void config_load();
void config_write();

void config_print();

void config_set(char *key_name, char *value);
void config_setf(char *key_name, char *value);

void config_set_in_group(char *group_name, char *key_name, char *value);
void config_set_if_missing(char *key_name, char *value);
void config_set_if_missing_in_group(char *group_name, char *key_name, char *value);

char *config_get(char *key_name);

char *config_get_in_group(char *group_name, char *key_name);

typedef struct config_key
{
	char *name;
	char *value;
} config_key;

config_key *config_key_init(char *name, char *value);
void config_key_set_value(config_key *ptr, char *value);
void config_key_deinit(config_key *ptr);

typedef struct config_key_group
{
	char *name;
	list *config_keys;
} config_key_group;

config_key_group *config_key_group_init(char *name);
void config_key_group_add_key(config_key_group *ptr, config_key *key);
void config_key_group_deinit(config_key_group *ptr);

extern list *config_groups_list;
