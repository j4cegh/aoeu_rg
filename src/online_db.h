#pragma once

#include "list.h"
#include "timer.h"

typedef struct online_db
{
    list *user_accounts;
    char *data_folder;
    timer *time_since_last_save;
} online_db;

typedef struct online_account
{
    char *name;
    char *pass_hash;
    unsigned long score;
	unsigned long play_count;
	char is_admin;
	char is_mod;
	unsigned long rank;
    int creation_year;
    int creation_month;
    int creation_day;
    int creation_hour;
    int creation_min;
    int creation_sec;
    online_db *db;
} online_account;

online_account *online_account_init(char *name, char *password_hash, online_db *db);
online_account *online_account_load(char *file_loc, online_db *db);
void online_account_free(online_account *ptr);
void online_account_save(online_account *ptr);

online_db *online_db_init(char *data_folder);
void online_db_load(online_db *ptr);
void online_db_free(online_db *ptr);
void online_db_save_all(online_db *ptr);
void online_db_sort_rank(online_db *ptr);
online_account *online_db_new(online_db *ptr, char *username, char *pass_hash);
online_account *online_db_get_acc(online_db *ptr, char *username);
