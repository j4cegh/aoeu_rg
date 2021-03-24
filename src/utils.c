#include "utils.h"
#include "rg.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stddef.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <sodium.h>
#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

/* math start */
float scale_value_to(float value, float from_min, float from_max, float to_min, float to_max)
{
    return ((to_max - to_min) * (value - from_min) / (from_max - from_min)) + to_min;
}

float clamped_scale_value_to(float value, float from_min, float from_max, float to_min, float to_max)
{
	return clamp(scale_value_to(value, from_min, from_max, to_min, to_max), to_min, to_max);
}

#define CLAMPIMP return val < min ? min : (val > max ? max : val)
float clamp(float val, float min, float max)
{
    CLAMPIMP;
}
#undef CLAMPIMP

float clampr(float *val, float min, float max)
{
	*val = clamp(*val, min, max);
	return *val;
}

float get_distf(float x1, float y1, float x2, float y2)
{
	float nmx = (x1 - x2);
	float nmy = (y1 - y2);
	return sqrtf((nmx * nmx) + (nmy * nmy));
}

float rotate_towards(v2f from, v2f to)
{
	return ((atan2(from.y - to.y, from.x - to.x)) * 180.0f / PI_NUM) - 90.0f;
}

float return_lowest(float v1, float v2)
{
	return (v1 < v2 ? v1 : (v2 < v1 ? v2 : v1));
}

float return_highest(float v1, float v2)
{
	return (v1 > v2 ? v1 : (v2 > v1 ? v2 : v1));
}

char is_straight_line(v2f p1, v2f p2, v2f p3)
{
	return ((p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y) == 0.0f);
}

void const_write_float(const float *f, float val)
{
	*((float*) f) = val;
}

int lr = 25;
int rand_num_ranged(int start, int end)
{
	if(start == end || end < start) return 0;
	srand(time(NULL) + lr++);
	int ret = clamp(start + (rand() % (end - start)), start, end);
	lr = ret;
	return ret;
}
/* math end */

/* strings start */
char *concat_str(char *str1, char *str2)
{
    char *result = malloc(1 + strlen(str1) + strlen(str2));
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

void append_to_str(char **dest, char *append)
{
	char *dest_str = *dest;
	*dest = concat_str(*dest, append);
	free(dest_str);
}

void append_to_str_free_append(char **dest, char *append)
{
	append_to_str(dest, append);
	free(append);
}

list *tokenize_str(char *str, char *by)
{
	size_t len = strlen(str);
	size_t bylen = strlen(by);
	if(len < 1 || bylen < 1 || (bylen > len)) return NULL;
	list *ret = list_init();
	char *tmp = (malloc((len + 1) * sizeof(char)));
	size_t i, tt = 0;
	for(i = 0; i <= len; ++i)
	{
		char ch = str[i];
		size_t gf = 0;
		if(ch != '\0')
		{
			size_t chi = 0;
			for(chi = 0; chi < bylen; ++chi) 
			{
				if(str[i + chi] == by[chi])
				{
					++gf;
				}
			}
		}
		if(gf == bylen || ch == '\0')
		{
			tmp[tt] = '\0';
			list_push_back(ret, dupe_str(tmp));
			tt = 0;
			i += bylen - 1;
		}
		else tmp[tt++] = ch;
	}
	free(tmp);
	return ret;
}

list *split_str(char *str, char *by)
{
	int len = strlen(str);
	if(len < 1) return NULL;
	list *ret = list_init();
	char rby = by[0];
	char *tmp = malloc((len + 1) * sizeof(char));
	int chi;
	int tmpi = 0;
	for(chi = 0; chi <= len; ++chi)
	{
		char ch = str[chi];
		if(ch == rby || ch == '\0')
		{
			tmp[tmpi] = '\0';
			char *std = dupe_str(tmp);
			list_push_back(ret, std);
			tmpi = 0;
		}
		else
		{
			tmp[tmpi] = ch;
			tmpi++;
		}
	}
	free(tmp);
	return ret;
}

void print_strs_in_list(list *ptr, char wall)
{
	if(!ptr) return;
	list_node *n;
	for(n = ptr->start; n != NULL; n = n->next)
	{
		char *str = (char*) n->val;
		if(wall) printf("|%s|\n", str);
		else printf("%s\n", str);
	}
}

list *split_str_char_lim(char *str, int char_limit)
{
	int len = strlen(str);
	if(len < 1) return NULL;
	list *ret = list_init();
	char *tmp = init_empty_string();
	int chi;
	for(chi = 0; chi < len; ++chi)
	{
		char ch = str[chi];
		if(ch == '\n') continue;
		if(chi % char_limit == 0 && chi != 0)
		{
			list_push_back(ret, dupe_str(tmp));
			free(tmp);
			tmp = init_empty_string();
		}
		char rbys[2];
		rbys[0] = ch;
		rbys[1] = '\0';
		append_to_str(&tmp, &rbys[0]);
	}
	list_push_back(ret, dupe_str(tmp));
	free(tmp);
	return ret;
}

char *int_to_str(int num)
{
	char str[128];
	snprintf(str, sizeof(str), "%d", num);
	return dupe_str(str);
}

char *float_to_str(float num)
{
	char str[128];
	snprintf(str, sizeof(str), "%.2f", num);
	return dupe_str(str);
}

char *init_empty_string()
{
	char *ret = malloc(sizeof(char));
	ret[0] = '\0';
	return ret;
}

int strs_are_equal(char *s1,  char *s2)
{
	return (strcmp(s1, s2) == 0);
}

char str_contains(char *to_check, char *check_for, char ignore_case)
{
	char does_contain = 0;

	int tclen = strlen(to_check);
	int cflen = strlen(check_for);

	if(cflen > tclen) return 0;

	int tci;
	for(tci = 0; tci < tclen && tci <= (tclen - cflen); tci++)
	{
		int cfound = 0;

		int cfi;
		for(cfi = 0; cfi < cflen; cfi++)
		{
			if(ignore_case)
			{
				if(tolower(to_check[tci + cfi]) == tolower(check_for[cfi])) cfound++;
			}
			else
			{
				if(to_check[tci + cfi] == check_for[cfi]) cfound++;
			}
		}

		if(cfound == cflen)
		{
			does_contain = 1;
			break;
		}
	}

	return does_contain;
}

char is_str_legal(char *to_check, char *legal_chars)
{
	char ret = 1;
	size_t tcl = strlen(to_check), lcl = strlen(legal_chars);
	int i;
	for(i = 0; i < tcl; ++i)
	{
		char cc1 = to_check[i];
		char good = 0;
		int i2;
		for(i2 = 0; i2 < lcl && !good; ++i2)
		{
			char cc2 = legal_chars[i2];
			if(cc1 == cc2) good = 1;
		}
		if(!good)
		{
			ret = 0;
			break;
		}
	}
	return ret;
}

char *str_to_lower(char *str)
{
	int slen = strlen(str);
	char *nstr = dupe_str(str);

	int in;
	for(in = 0; in < slen; in++)
	{
		nstr[in] = tolower(str[in]);
	}

	return nstr;
}

char *file_to_str(char *file_loc)
{
	char *ret = init_empty_string();

	FILE *fp = fopen(file_loc, "r");

	if(!fp)
	{
		free(ret);
		return NULL;
	}

	fseek(fp, 0L, SEEK_END);
	int sz = ftell(fp);
	rewind(fp);

	char *buf = malloc(sizeof(char) * sz);

	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		buf[strlen(buf)] = '\0';
		append_to_str_free_append(&ret, dupe_str(&buf[0]));
	}

	free(buf);

	fclose(fp);

	return ret;
}

char *str_glue_fmt(char *flags, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char fstr[1024];
	vsnprintf(fstr, sizeof(fstr), fmt, args);
	int i;
	int count = strlen(flags);
	for(i = 0; i < count; ++i) if(flags[i] == '1') free((char*) va_arg(args, char*));
	va_end(args);
	return dupe_str(fstr);
}

/*
#define fstrl 16384
char fstr[fstrl];
char dupfmt_lock = 0;

char *dupfmt(char *fmt, ...)
{
	while(dupfmt_lock) {};
	dupfmt_lock = 1;
	va_list args;
	va_start(args, fmt);
	int w = vsnprintf(&fstr[0], fstrl, fmt, args);
	assert(w <= fstrl);
	va_end(args);
	dupfmt_lock = 0;
	return dupe_str(&fstr[0]);
}

#undef fstrl
*/

char *dupfmt(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char fstr[16384];
	size_t fstrl = sizeof(fstr);
	vsnprintf(fstr, fstrl, fmt, args);
	va_end(args);
	return dupe_str(fstr);
}

void str_insert_char(char **dest_str, int index, char c)
{
	char *str = *dest_str;
	char *new1 = malloc(strlen(str) + 2);
	char *aft = dupe_str(&str[index]);
	strcpy(new1, str);
	new1[index] = c;
	strcpy(&new1[index + 1], aft);
	free(aft);
	free(*dest_str);
	*dest_str = new1;
}

void str_remove_forbidden_chars(char **dest_str, char *forbidden, char reverse)
{
	char *allowed = init_empty_string();
	char *str = *dest_str;
	int len = strlen(str);
	int i;
	for(i = 0; i < len; i++)
	{
		char cur_char = str[i];

		char cstr[2];
		cstr[0] = cur_char;
		cstr[1] = '\0';
		char good = !(str_contains(forbidden, &cstr[0], 1));
		if(reverse) good = !good;

		if(good) append_to_str(&allowed, &cstr[0]);
	}
	free(*dest_str);
	*dest_str = allowed;
}

void str_replace_chars(char *str, char old, char new)
{
	int i = 0;
	char c = str[i];
	while(c != '\0')
	{
		c = str[i];
		if(c == old) str[i] = new;
		++i;
	}
}

int count_char_occurances(char *str, char c)
{
	int occ = 0;
	size_t i;
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] == c) occ++;
	}
	return occ;
}

char *rand_str(size_t len, int r1, int r2)
{
	srand(rg.time_open->microseconds  % INT_MAX);
	char *ret = malloc((len + 1) * sizeof(char));
	int chi;
	for(chi = 0; chi < len; ++chi)
	{
		char rc = rand_num_ranged(r1, r2);
		ret[chi] = rc;
	}
	ret[len] = '\0';
	return ret;
}

char *dupe_str(char *str)
{
	if(str == NULL) return NULL;
	size_t len = strlen(str);
	char *ret = malloc((len + 1) * sizeof(char));
	strncpy(ret, str, len);
	ret[len] = '\0';
	return ret;
}

char *get_file_ext(char *file)
{
	size_t fl = strlen(file);
	if(fl < 1) return NULL;
	int i = fl;
	char p = file[i];
	while(i-- > 0 && p != '.') p = file[i];
	if(p == '.')
	{
	    char *f = &file[i + 2];
		if(f[0] != '\0') return f;
	}
	return NULL;
}

char *get_file_from_path(char *path)
{
	size_t fl = strlen(path);
#ifdef USING_WINDOWS
	char sep = '\\';
#else
	char sep = '/';
#endif
	if(fl < 1) return NULL;
	int i = fl;
	char p = path[i];
	while(i-- > 0 && p != sep) p = path[i];
	if(p == sep)
	{
	    char *f = &path[i + 2];
		if(f[0] != '\0') return f;
	}
	return NULL;
}
/* strings end */

/* num allocs start */
int *new_int(int i)
{
	int *ret = malloc(sizeof *ret);
	*ret = i;
	return ret;
}

float *new_float(float f)
{
	float *ret = malloc(sizeof *ret);
	*ret = f;
	return ret;
}
/* num allocs end */

/* file handling start */
list *file_to_list_of_lines(char *file_loc)
{
	list *ret = list_init();

	FILE *fp = fopen(file_loc, "r");

	if(!fp)
	{
		list_free(ret, list_dummy);
		return NULL;
	}

	char buf[2048];

	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		buf[strlen(buf) - 1] = '\0';
		list_push_back(ret, dupe_str(&buf[0]));
	}

	fclose(fp);

	return ret;
}

char str_to_file(char *file_loc, char *str)
{
	FILE *fp = fopen(file_loc, "wb");
	if(fp)
	{
		fputs(str, fp);
		fclose(fp);
		return 0;
	}
	return -1;
}

char file_exists(char *filename)
{
	FILE *file = fopen(filename, "r");
    if(file != NULL)
    {
        fclose(file);
        return 1;
    }
    return 0;
}

char dir_exists(char *path)
{
	struct stat stats;
	stat(path, &stats);
	if(S_ISDIR(stats.st_mode)) return 1;
	return 0;
}

long file_get_size(char *filename)
{
	FILE *f = fopen(filename, "r");
	if(f != NULL)
	{
		fseek(f, 0L, SEEK_END);
		long sz = ftell(f);
		fclose(f);
		return sz;
	}
	else return 0;
}

int make_dir(char *dir)
{
#ifdef USING_WINDOWS
	return _mkdir(dir);
#else
	return mkdir(dir, 0700);
#endif
}

#include <fcntl.h>
#ifndef USING_WINDOWS
#include <unistd.h>
#endif
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <copyfile.h>
#elif defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#else
#include <sys/sendfile.h>
#endif

int copy_file_to(const char *source, const char *destination)
{
#if defined(__WIN32__) || defined(WIN32) || defined(WIN64)
	return CopyFileA(source, destination, 0);
#else
    int input, output;    
    if((input = open(source, 0)) == -1) return -1;
    if((output = creat(destination, 0660)) == -1)
    {
        close(input);
        return -1;
    }

#if defined(__APPLE__) || defined(__FreeBSD__)
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    off_t bytesCopied = 0;
    struct stat fileinfo = { 0 };
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
#endif
}
/* file handling end */

/* perf timing start */
#ifndef RELEASE
timer *perf_timer = NULL;
#endif

void perf_timer_start()
{
#ifndef RELEASE
	if(perf_timer == NULL) perf_timer = timer_init();
#endif
}

void perf_timer_end(char *name)
{
#ifndef RELEASE
	if(perf_timer != NULL)
	{
		unsigned long ms = timer_microseconds(perf_timer);
		printf("(%s) took %lu micro seconds\n", name, ms);
		timer_free(perf_timer);
		perf_timer = NULL;
	}
#endif
}
/* perf timing end */

/* hashing start */
char *sha256(char *data)
{
	if(data == NULL) return NULL;
    unsigned char out[crypto_hash_sha256_BYTES];
	size_t len = strlen(data);
    crypto_hash_sha256(out, (const unsigned char*) data, len);
	size_t sha_size = sizeof(out) * 2 + 1;
    char *sha = malloc(sha_size);
    sodium_bin2hex(sha, sha_size, out, sizeof(out));
	return sha;
}

void sha256into(char *into, char *data)
{
	if(data == NULL) return;
    unsigned char out[crypto_hash_sha256_BYTES];
	size_t len = strlen(data);
    crypto_hash_sha256(out, (const unsigned char*) data, len);
    sodium_bin2hex(into, SHA256_STRING_LEN, out, sizeof(out));
}

char *sha256_free_data(char *data)
{
	if(data == NULL) return NULL;
	char *ret = sha256(data);
	free(data);
	return ret;
}
/* hashing end */

/* animation start */
double apply_easing(easing_type easing, double time, double initial, double change, double duration)
{
	if(change == 0 || time == 0 || duration == 0) return initial;
	if(time == duration) return initial + change;

	const float pi = 3.14159274f;
	switch (easing)
	{
		default:
			return change * (time / duration) + initial;
		case easing_type_in:
			return change * (time /= duration) * time + initial;
		case easing_type_out:
			return -change * (time /= duration) * (time - 2) + initial;
		case easing_type_in_out:
			if((time /= duration / 2) < 1) return change / 2 * time * time + initial;
			return -change / 2 * ((--time) * (time - 2) - 1) + initial;
		case easing_type_in_cubic:
			return change * (time /= duration) * time * time + initial;
		case easing_type_out_cubic:
			return change * ((time = time / duration - 1) * time * time + 1) + initial;
		case easing_type_in_out_cubic:
			if((time /= duration / 2) < 1) return change / 2 * time * time * time + initial;
			return change / 2 * ((time -= 2) * time * time + 2) + initial;
		case easing_type_in_quart:
			return change * (time /= duration) * time * time * time + initial;
		case easing_type_out_quart:
			return -change * ((time = time / duration - 1) * time * time * time - 1) + initial;
		case easing_type_in_out_quart:
			if((time /= duration / 2) < 1) return change / 2 * time * time * time * time + initial;
			return -change / 2 * ((time -= 2) * time * time * time - 2) + initial;
		case easing_type_in_quint:
			return change * (time /= duration) * time * time * time * time + initial;
		case easing_type_out_quint:
			return change * ((time = time / duration - 1) * time * time * time * time + 1) + initial;
		case easing_type_in_out_quint:
			if((time /= duration / 2) < 1) return change / 2 * time * time * time * time * time + initial;
			return change / 2 * ((time -= 2) * time * time * time * time + 2) + initial;
		case easing_type_in_sine:
			return -change * cos(time / duration * (pi / 2)) + change + initial;
		case easing_type_out_sine:
			return change * sin(time / duration * (pi / 2)) + initial;
		case easing_type_in_out_sine:
			return -change / 2 * (cos(pi * time / duration) - 1) + initial;
		case easing_type_in_expo:
			return change * pow(2, 10 * (time / duration - 1)) + initial;
		case easing_type_out_expo:
			return (time == duration) ? initial + change : change * (-pow(2, -10 * time / duration) + 1) + initial;
		case easing_type_in_out_expo:
			if((time /= duration / 2) < 1) return change / 2 * pow(2, 10 * (time - 1)) + initial;
			return change / 2 * (-pow(2, -10 * --time) + 2) + initial;
		case easing_type_in_circ:
			return -change * (sqrt(1 - (time /= duration) * time) - 1) + initial;
		case easing_type_out_circ:
			return change * sqrt(1 - (time = time / duration - 1) * time) + initial;
		case easing_type_in_out_circ:
			if((time /= duration / 2) < 1) return -change / 2 * (sqrt(1 - time * time) - 1) + initial;
			return change / 2 * (sqrt(1 - (time -= 2) * time) + 1) + initial;
		case easing_type_in_elastic:
			{
				if((time /= duration) == 1) return initial + change;

				double p = duration * .3;
				double a = change;
				double s = 1.70158;
				if(a < fabs(change)) { a = change; s = p / 4; }
				else s = p / (2 * pi) * asin(change / a);
				return -(a * pow(2, 10 * (time -= 1)) * sin((time * duration - s) * (2 * pi) / p)) + initial;
			}
		case easing_type_out_elastic:
			{
				if((time /= duration) == 1) return initial + change;

				double p = duration * .3;
				double a = change;
				double s = 1.70158;
				if(a < fabs(change)) { a = change; s = p / 4; }
				else s = p / (2 * pi) * asin(change / a);
				return a * pow(2, -10 * time) * sin((time * duration - s) * (2 * pi) / p) + change + initial;
			}
		case easing_type_out_elastic_half:
			{
				if((time /= duration) == 1) return initial + change;

				double p = duration * .3;
				double a = change;
				double s = 1.70158;
				if(a < fabs(change)) { a = change; s = p / 4; }
				else s = p / (2 * pi) * asin(change / a);
				return a * pow(2, -10 * time) * sin((0.5f * time * duration - s) * (2 * pi) / p) + change + initial;
			}
		case easing_type_out_elastic_quarter:
			{
				if((time /= duration) == 1) return initial + change;

				double p = duration * .3;
				double a = change;
				double s = 1.70158;
				if(a < fabs(change)) { a = change; s = p / 4; }
				else s = p / (2 * pi) * asin(change / a);
				return a * pow(2, -10 * time) * sin((0.25f * time * duration - s) * (2 * pi) / p) + change + initial;
			}
		case easing_type_in_out_elastic:
			{
				if((time /= duration / 2) == 2) return initial + change;

				double p = duration * (.3 * 1.5);
				double a = change;
				double s = 1.70158;
				if(a < fabs(change)) { a = change; s = p / 4; }
				else s = p / (2 * pi) * asin(change / a);
				if(time < 1) return -.5 * (a * pow(2, 10 * (time -= 1)) * sin((time * duration - s) * (2 * pi) / p)) + initial;
				return a * pow(2, -10 * (time -= 1)) * sin((time * duration - s) * (2 * pi) / p) * .5 + change + initial;
			}
		case easing_type_in_back:
			{
				double s = 1.70158;
				return change * (time /= duration) * time * ((s + 1) * time - s) + initial;
			}
		case easing_type_out_back:
			{
				double s = 1.70158;
				return change * ((time = time / duration - 1) * time * ((s + 1) * time + s) + 1) + initial;
			}
		case easing_type_in_out_back:
			{
				double s = 1.70158;
				if((time /= duration / 2) < 1) return change / 2 * (time * time * (((s *= (1.525)) + 1) * time - s)) + initial;
				return change / 2 * ((time -= 2) * time * (((s *= (1.525)) + 1) * time + s) + 2) + initial;
			}
		case easing_type_in_bounce:
			return change - apply_easing(easing_type_out_bounce, duration - time, 0, change, duration) + initial;
		case easing_type_out_bounce:
			if((time /= duration) < (1 / 2.75))
			{
				return change * (7.5625 * time * time) + initial;
			}
			else if(time < (2 / 2.75))
			{
				return change * (7.5625 * (time -= (1.5 / 2.75)) * time + .75) + initial;
			}
			else if(time < (2.5 / 2.75))
			{
				return change * (7.5625 * (time -= (2.25 / 2.75)) * time + .9375) + initial;
			}
			else
			{
				return change * (7.5625 * (time -= (2.625 / 2.75)) * time + .984375) + initial;
			}
		case easing_type_in_out_bounce:
			if(time < duration / 2) return apply_easing(easing_type_in_bounce, time * 2, 0, change, duration) * .5 + initial;
			return apply_easing(easing_type_out_bounce, time * 2 - duration, 0, change, duration) * .5 + change * .5 + initial;
	}
}
/* animation end */

#ifndef SERV
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

void open_url(char *url)
{
#ifdef _WIN32
	ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
	char buf[1024];
	snprintf(buf, sizeof(buf), "xdg-open %s", url);
	system(buf);
#endif
}
#endif
