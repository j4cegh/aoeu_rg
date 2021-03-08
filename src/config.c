#include "config.h"
#include "rg.h"
#include "utils.h"
#include "logger.h"

list *config_groups_list;

#define CONFIG_DEFAULT_GROUP "options"

void config_init()
{
	config_groups_list = list_init();
	config_load();
}

void config_deinit()
{
	config_write();

	list_free(config_groups_list, config_key_group_deinit);
}

void config_load()
{
	list *l = file_to_list_of_lines(CONFIG_FILENAME);

	if(!l) return;
	if(l->count < 1)
	{
		list_free(l, free);
		return;
	}

	char read_mode = 0;
	int cur_line;
	int groups_detected = 0;
	int lcount = l->count;
	
	config_key_group *cur_group = NULL;
	
	for(cur_line = 0; cur_line < lcount; cur_line++)
	{
		char *clstr = list_get_val_at(l, cur_line);
		int cllen = strlen(clstr);
		if(cllen < 2) continue;
		
		if(read_mode == 0)
		{
			if(clstr[0] == '[' && clstr[cllen - 1] == ']')
			{
				read_mode = 1;
				if(cur_group) list_push_back(config_groups_list, cur_group);
				char *end_cut = dupe_str(clstr);
				end_cut[cllen - 1] = '\0';
				char *cg_name = dupe_str(&end_cut[1]);
				free(end_cut);
				cur_group = config_key_group_init(cg_name);
				free(cg_name);
				groups_detected++;
			}
		}
		else if(read_mode == 1 && cur_group != NULL)
		{
			if(clstr[0] == '[')
			{
				cur_line--;
				read_mode = 0;
				continue;
			}
			
			int cur_char = 0;
			char key_read_mode = 0;
			char key_inside_value_quotes = 0;
			
			char *key_name = init_empty_string();
			char *key_value = init_empty_string();
			
			while(cur_char < cllen)
			{
				char ch = clstr[cur_char];
				if(ch == ':') key_read_mode = 1;
				if(ch == '\"') 
				{
					key_inside_value_quotes = !key_inside_value_quotes;
					cur_char++;
					continue;
				}
				
				char append[2];
				append[0] = ch;
				append[1] = '\0';
				
				if(key_read_mode == 0)
					append_to_str(&key_name, &append[0]);
				else if(key_read_mode == 1 && key_inside_value_quotes)
					append_to_str(&key_value, &append[0]);
				
				cur_char++;
			}
			
			config_key *new_key = config_key_init(key_name, key_value);
			list_push_back(cur_group->config_keys, new_key);
			
			free(key_value);
			free(key_name);
		}
	}
	
	if(groups_detected >= 1) list_push_back(config_groups_list, cur_group);

	list_free(l, free);
}

void config_write()
{	
	FILE *fp = fopen(CONFIG_FILENAME, "wb");
	
	fputs("# aoeu's rhythm game config file\n\n", fp);
	
	list_node *gn;
	for(gn = config_groups_list->start; gn != NULL; gn = gn->next)
	{
		config_key_group *group = gn->val;

		char *group_name_locator = dupfmt("[%s]\n", group->name);
		
		fputs(group_name_locator, fp);
		
		free(group_name_locator);
		
		list_node *kn;
		for(kn = group->config_keys->start; kn != NULL; kn = kn->next)
		{
			config_key *key = kn->val;
			
			char *key_string = dupfmt("%s: \"%s\"\n", key->name, key->value);
			
			fputs(key_string, fp);
			
			free(key_string);
		}
		
		if(gn != config_groups_list->end) fputs("\n", fp);
	}
	
	fclose(fp);

	log_normal("wrote configuration to disk.");
}

void config_print()
{
	list_node *gn;
	for(gn = config_groups_list->start; gn != NULL; gn = gn->next)
	{
		config_key_group *group = gn->val;
		
		printf(group->name);
		
		list_node *kn;
		for(kn = group->config_keys->start; kn != NULL; kn = kn->next)
		{
			config_key *key = kn->val;
			printf(key->name);
			printf(key->value);
		}
	}
}

void config_set(char *key_name, char *value)
{
	config_set_in_group(CONFIG_DEFAULT_GROUP, key_name, value);
}

void config_setf(char *key_name, char *value)
{
	config_set(key_name, value);
	free(value);
}

void config_set_in_group(char *group_name, char *key_name, char *value)
{
	list_node *gn;
	for(gn = config_groups_list->start; gn != NULL; gn = gn->next)
	{
		config_key_group *group = gn->val;
		if(strs_are_equal(group->name, group_name))
		{
			list_node *kn;
			for(kn = group->config_keys->start; kn != NULL; kn = kn->next)
			{
				config_key *key = kn->val;
				if(strs_are_equal(key->name, key_name))
				{
					config_key_set_value(key, value);
					return;
				}
			}

			config_key *nkey = config_key_init(key_name, value);
			list_push_back(group->config_keys, nkey);

			return;
		}
	}

	config_key_group *ngroup = config_key_group_init(group_name);
	config_key *nkey = config_key_init(key_name, value);
	list_push_back(ngroup->config_keys, nkey);
	list_push_back(config_groups_list, ngroup);
}

void config_set_if_missing(char *key_name, char *value)
{
	config_set_if_missing_in_group(CONFIG_DEFAULT_GROUP, key_name, value);
}

void config_set_if_missing_in_group(char *group_name, char *key_name, char *value)
{
	if(!config_get_in_group(group_name, key_name)) config_set_in_group(group_name, key_name, value);
}

char *config_get(char *key_name)
{
	return config_get_in_group(CONFIG_DEFAULT_GROUP, key_name);
}

char *config_get_in_group(char *group_name, char *key_name)
{
	list_node *gn;
	for(gn = config_groups_list->start; gn != NULL; gn = gn->next)
	{
		config_key_group *group = gn->val;
		if(strs_are_equal(group->name, group_name))
		{
			list_node *kn;
			for(kn = group->config_keys->start; kn != NULL; kn = kn->next)
			{
				config_key *key = kn->val;
				if(strs_are_equal(key->name, key_name)) return key->value;
			}
		}
	}
	return NULL;
}

config_key *config_key_init(char *name, char *value)
{
	config_key *ret = malloc(sizeof *ret);
	ret->name = dupe_str(name);
	ret->value = NULL;
	config_key_set_value(ret, value);
	return ret;
}

void config_key_set_value(config_key *ptr, char *value)
{
	if(!(ptr->value == NULL)) free(ptr->value);
	ptr->value = dupe_str(value);
}

void config_key_deinit(config_key *ptr)
{
	free(ptr->value);
	free(ptr->name);
	free(ptr);
}

config_key_group *config_key_group_init(char *name)
{
	config_key_group *ret = malloc(sizeof *ret);
	ret->name = dupe_str(name);
	ret->config_keys = list_init();
	return ret;
}

void config_key_group_add_key(config_key_group *ptr, config_key *key)
{
	list_push_back(ptr->config_keys, key);
}

void config_key_group_deinit(config_key_group *ptr)
{
	list_free(ptr->config_keys, config_key_deinit);

	free(ptr->name);
	free(ptr);
}
