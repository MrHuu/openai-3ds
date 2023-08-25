#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void UTIL_updateString(char** str, const char* newContent);
void UTIL_appendString(char** str, const char* appendContent);

char* UTIL_string_concat(char *str1,char *str2);

#endif /* _UTIL_H */
