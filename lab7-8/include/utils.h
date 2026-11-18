#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <cctype>

void to_lowercase(char* str);

bool is_alpha(char c);

void clean_string(char* str);

char** split_string(const char* str, const char* delimiters, int* count);

void free_string_array(char** arr, int count);

char* strdup(const char* src);

#endif