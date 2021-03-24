#pragma once

#include "list.h"
#include "includes.h"

/* math */
float scale_value_to(float value, float from_min, float from_max, float to_min, float to_max);
float clamped_scale_value_to(float value, float from_min, float from_max, float to_min, float to_max);
float clamp(float val, float min, float max);
float clampr(float *val, float min, float max);
float get_distf(float x1, float y1, float x2, float y2);
float rotate_towards(v2f from, v2f to);
float return_lowest(float v1, float v2);
float return_highest(float v1, float v2);
char is_straight_line(v2f p1, v2f p2, v2f p3);
void const_write_float(const float *f, float val);
int rand_num_ranged(int start, int end);

/* strings */
char *concat_str(char *str1, char *str2);
void append_to_str(char **dest, char *append);
void append_to_str_free_append(char **dest, char *append);
list *tokenize_str(char *str, char *by);
list *split_str(char *str, char *by);
void print_strs_in_list(list *ptr, char wall);
list *split_str_char_lim(char *str, int char_limit);
char *int_to_str(int num);
char *float_to_str(float num);
char *init_empty_string();
int strs_are_equal(char *s1, char *s2);
char str_contains(char *to_check, char *check_for, char ignore_case);
char is_str_legal(char *to_check, char *legal_chars);
char *str_to_lower(char *str);
char *file_to_str(char *file_loc);
char *str_glue_fmt(char *flags, char *fmt, ...);

/* probably thread safe but hasn't been tested */
char *dupfmt(char *fmt, ...);

void str_insert_char(char **dest_str, int index, char c);
void str_remove_forbidden_chars(char **dest_str, char *forbidden, char reverse);
void str_replace_chars(char *str, char old, char new);
int count_char_occurances(char *str, char c);

/* this function is broken */
char *rand_str(size_t len, int r1, int r2);

char *dupe_str(char *str);
char *get_file_ext(char *file);
char *get_file_from_path(char *path);

/* num allocs */
int *new_int(int i);
float *new_float(float f);

/* file handling */
list *file_to_list_of_lines(char *file_loc);
char str_to_file(char *file_loc, char *str);
char file_exists(char *filename);
char dir_exists(char *path);
long file_get_size(char *filename);
int make_dir(char *dir);
int copy_file_to(const char *source, const char *destination);

/* perf timing */
void perf_timer_start();
void perf_timer_end(char *name);

/* hashing */
#define SHA256_STRING_LEN 65
char *sha256(char *data);
void sha256into(char *into, char *data);
char *sha256_free_data(char *data);

/* animation */
typedef enum easing_type
{
	easing_type_in,
	easing_type_out,
	easing_type_in_out,
	easing_type_in_cubic,
	easing_type_out_cubic,
	easing_type_in_out_cubic,
	easing_type_in_quart,
	easing_type_out_quart,
	easing_type_in_out_quart,
	easing_type_in_quint,
	easing_type_out_quint,
	easing_type_in_out_quint,
	easing_type_in_sine,
	easing_type_out_sine,
	easing_type_in_out_sine,
	easing_type_in_expo,
	easing_type_out_expo,
	easing_type_in_out_expo,
	easing_type_in_circ,
	easing_type_out_circ,
	easing_type_in_out_circ,
	easing_type_in_elastic,
	easing_type_out_elastic,
	easing_type_out_elastic_half,
	easing_type_out_elastic_quarter,
	easing_type_in_out_elastic,
	easing_type_in_back,
	easing_type_out_back,
	easing_type_in_out_back,
	easing_type_in_bounce,
	easing_type_out_bounce,
	easing_type_in_out_bounce
} easing_type;

double apply_easing(easing_type easing, double time, double initial, double change, double duration);

#ifndef SERV
void open_url(char *url);
#endif