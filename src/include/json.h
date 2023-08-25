#ifndef _JSON_H
#define _JSON_H

#include <jansson.h>
#include "curl.h"

char* JSON_curl_request(const char* prompt, const messages_t *messages);
char* JSON_curl_request_chat(const char* prompt, const messages_t *messages);
char* JSON_curl_request_whisper(const char* prompt, const messages_t *messages);
char* JSON_curl_request_dalle(const char* prompt, const messages_t *messages);


int   JSON_curl_response(char *jsonObj,char *text);
int   JSON_curl_response_stream(char *jsonObj,char *text);
int   JSON_curl_response_whisper(char *jsonObj,char *text);
int   JSON_curl_response_dalle(char *jsonObj,char *text);


int     JSON_write_json_to_file(const char* filename, json_t* value);
json_t* JSON_read_json_from_file(const char* filename);

int json_write_test(void);
int json_read_test(void);

int JSON_read_api_key(void);
int JSON_write_api_key(void);

void json_exit();

#endif /* _JSON_H */