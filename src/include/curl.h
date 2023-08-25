#ifndef _CURL_H
#define _CURL_H

#include "gfx.h"

#define BUFFER_SIZE           512   // curl return process max buffer size
#define MAX_MESSAGES            8   // Maximum number of messages to store as history

extern Handle threadRequest_curl;

extern int message_count;
extern int next_message_id;

typedef struct {
    int id;
    char *role;
	char *content;
} messages_t;
extern messages_t msg_history[MAX_MESSAGES];

extern int message_count;


extern BufferStruct bufferStruct;

//extern int max_obj_block;


void CURL_addWordToStruct(BufferStruct *bufferStruct, const char *word);

void CURL_message_add(messages_t *messages, int *message_count, int *next_message_id, const char *role, const char *input);


void CURL_curl_sent_request(char* jsonObj);

void curl_init(void);
void curl_exit(void);






void CURL_set_system_message(const char *system_msg_input);

extern char *system_msg;

#endif /* _CURL_H */