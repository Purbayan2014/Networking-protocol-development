/*
 * =====================================================================================
 *
 *       Filename:  string_util.h
 *
 *    Description:  iString util
 *
 *        Version:  1.0
 *        
 *       Revision:  1.0
 *       Compiler:  gcc
 *
 *         Author:  purbayan, purbayan2014@gmail.com
 *        
 *
 * =====================================================================================
 */

#ifndef __STRING_UTIL__
#define __STRING_UTIL__
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


char** tokenizer(char* a_str, 
                 const char a_delim, 
                 size_t *token_cnt);

void
string_space_trim(char *string);

void
print_tokens(unsigned int index);

void
init_token_array();

void
re_init_tokens(int token_cnt);

void
tokenize(char *token, unsigned int size, unsigned int index);

void
untokenize(unsigned int index);

char *
get_token(unsigned int index);

void replaceSubstring(char string[], char sub[], char new_str[]);

bool
pattern_match(char string[], int string_size, char pattern[]);

int
grep (char string[], int string_size, char pattern[]);

uint64_t
string_fetch_integer(char *string, int string_size, int index);

#endif
