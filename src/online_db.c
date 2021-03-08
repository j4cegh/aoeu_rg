#ifndef RGPUB
#include "online_db.h"
#include "online_server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include "utils.h"

online_account *online_account_init(char *name, char *password_hash, online_db *db)
{
    online_account *ret = malloc(sizeof *ret);
    ret->name = dupe_str(name);
    ret->pass_hash = dupe_str(password_hash);
    ret->db = db;
	ret->score = 0;
	ret->play_count = 0;
	ret->is_admin = 0;
	ret->is_mod = 0;
    ret->rank = 0;
    time_t t = time(NULL);
	struct tm tm = *localtime(&t);
    ret->creation_year = 1900 + tm.tm_year;
    ret->creation_month = 1 + tm.tm_mon;
    ret->creation_day = tm.tm_mday;
    ret->creation_hour = tm.tm_hour;
    ret->creation_min = tm.tm_min;
    ret->creation_sec = tm.tm_sec;
    return ret;
}

online_account *online_account_load(char *file_loc, online_db *db)
{
    online_account *ret = malloc(sizeof *ret);
	ret->score = 0;
	ret->play_count = 0;
    ret->creation_year = 0;
    ret->creation_month = 0;
    ret->creation_day = 0;
    ret->creation_hour = 0;
    ret->creation_min = 0;
    ret->creation_sec = 0;

    FILE *fp = fopen(file_loc, "r");

    if(!fp)
    {
        rgspf(loglev_error, "account load: unable to open \"%s\" for loading", file_loc);
        return NULL;
    }

    char buf[1024];

    int i = 0;

    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        size_t len = strlen(buf);
        if(len > 0)
        {
            buf[len - 1] = '\0';
            if(i == 0) ret->name = dupe_str(buf);
            else if(i == 1) ret->pass_hash = dupe_str(buf);
            else if(i == 2) ret->score = strtoul(buf, NULL, 10);
            else if(i == 3) ret->play_count = strtoul(buf, NULL, 10);
            else if(i == 4) ret->is_admin = atoi(buf);
            else if(i == 5) ret->is_mod = atoi(buf);
            else if(i == 6) ret->rank = strtoul(buf, NULL, 10);
            else if(i == 7) ret->creation_year = atoi(buf);
            else if(i == 8) ret->creation_month = atoi(buf);
            else if(i == 9) ret->creation_day = atoi(buf);
            else if(i == 10) ret->creation_hour = atoi(buf);
            else if(i == 11) ret->creation_min = atoi(buf);
            else if(i == 12) ret->creation_sec = atoi(buf);
            else break;
        }
        i++;
    }

    fclose(fp);

    ret->db = db;

    rgspf(loglev_normal, "account load: loaded %s from %s", ret->name, file_loc);

    return ret;
}

void online_account_free(online_account *ptr)
{
    online_account_save(ptr);
    free(ptr->pass_hash);
    free(ptr->name);
    free(ptr);
}

void online_account_save(online_account *ptr)
{
    online_db *db = ptr->db;

    char* nl = str_to_lower(ptr->name);
    
    char save_loc[PATH_MAX];
    snprintf(save_loc, sizeof(save_loc), "%s%s", db->data_folder, nl);

    free(nl);

    FILE *f = fopen(save_loc, "w");
	if(f)
	{
        char data[4096];
        snprintf(data, sizeof(data), "%s\n" "%s\n" "%lu\n" "%lu\n" "%d\n" "%d\n" "%lu\n" "%d\n" "%d\n" "%d\n" "%d\n" "%d\n" "%d\n", 
        ptr->name, ptr->pass_hash, ptr->score, ptr->play_count, ptr->is_admin, ptr->is_mod, ptr->rank,
        ptr->creation_year, ptr->creation_month, ptr->creation_day, ptr->creation_hour, ptr->creation_min, ptr->creation_sec);
		fputs(data, f);
		fclose(f);
        /* printf("account db: saved data for %s successfully at %s\n", ptr->name, save_loc); */
	}
    else rgspf(loglev_error, "account db: unable to save account file at \"%s\"", save_loc);
}

online_db *online_db_init(char *data_folder)
{
    online_db *ret = malloc(sizeof *ret);
    ret->data_folder = dupe_str(data_folder);
    ret->time_since_last_save = timer_init();
    ret->user_accounts = list_init();
    online_db_load(ret);
	online_db_sort_rank(ret);
    return ret;
}

void online_db_load(online_db *ptr)
{
    DIR *dir;
    struct dirent *de;
    char *path = ptr->data_folder;
    dir = opendir(path);
    char fullpath[PATH_MAX];
    if(dir != NULL)
    {
        while((de = readdir(dir)) != NULL)
        {
            if(de->d_type == DT_REG)
            {
                snprintf(fullpath, sizeof(fullpath), "%s%s", path, de->d_name);
                online_account *acc = online_account_load(fullpath, ptr);
                if(acc) list_push_back(ptr->user_accounts, acc);
                else rgsp(loglev_error, "db load: acc was NULL");
            }
        }
        closedir(dir);
    }
    else rgsp(loglev_error, "db load: unable to open account dir");

    timer_restart(ptr->time_since_last_save);
}

void online_db_free(online_db *ptr)
{
    list_free(ptr->user_accounts, online_account_free);
    timer_free(ptr->time_since_last_save);
    free(ptr->data_folder);
    free(ptr);
}

void online_db_save_all(online_db *ptr)
{
    list_node *n;
    for(n = ptr->user_accounts->start; n != NULL; n = n->next)
    {
        online_account *acc = n->val;
        online_account_save(acc);
    }
    timer_restart(ptr->time_since_last_save);

    rgspf(loglev_normal, "saved acc db to %s", ptr->data_folder);
}

void online_db_sort_rank(online_db *ptr)
{
	list_node *sn;

	char swapped;

	do
	{
		swapped = 0;
		sn = ptr->user_accounts->start;
		if (sn == NULL) break;
		list_node *ln = NULL;
		while (sn->next != ln)
		{
			online_account *o = sn->val;
			online_account *on = sn->next->val;
			if (o->score < on->score)
			{
				list_swap(sn, sn->next);
				swapped = 1;
			}
			sn = sn->next;
		}
		ln = sn;
	} while (swapped);

	unsigned long rank = 0;
	for (sn = ptr->user_accounts->start; sn != NULL; sn = sn->next)
	{
		online_account *acc = sn->val;
		acc->rank = ++rank;
	}
}

online_account *online_db_new(online_db *ptr, char *username, char *pass_hash)
{
    online_account *exists = online_db_get_acc(ptr, username);
    if(exists != NULL)
    {
        if(strs_are_equal(exists->pass_hash, pass_hash)) return exists;
        else return NULL;
    }
    online_account *ret = online_account_init(username, pass_hash, ptr);
    online_account_save(ret);
    list_push_back(ptr->user_accounts, ret);
    return ret;
}

online_account *online_db_get_acc(online_db *ptr, char *rusername)
{
    char *username = str_to_lower(rusername);
    online_account *ret = NULL;
    list_node *n;
    for(n = ptr->user_accounts->start; n != NULL; n = n->next)
    {
        online_account *acc = n->val;
        char* antl = str_to_lower(acc->name);
        if(strcmp(antl, username) == 0)
        {
            ret = acc;
            free(antl);
            break;
        }
        free(antl);
    }
    if(ret == NULL)
    {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s%s", ptr->data_folder, username);
        online_account *acc = online_account_load(path, ptr);
        if(acc != NULL) list_push_back(ptr->user_accounts, acc);
        ret = acc;
    }
    free(username);
    return ret;
}
#endif
