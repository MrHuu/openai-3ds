#ifndef _SYS_H
#define _SYS_H

#include <3ds.h>
#define MAX_INPUT_LENGTH      1024   // Maximum length of user input

extern bool running;

void sys_error(bool fatal, const char* error);
int sys_swkbd(const char *hintText, const char *inText, char *outText);

void sys_init();
void sys_exit();

#endif /* _SYS_H */
