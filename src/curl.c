
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include <curl/curl.h>


#include "sfx.h"
#include "main.h"
#include "sys.h"
#include "util.h"
#include "curl.h"
#include "json.h"
#include "mic.h"

#define STACKSIZE (8 * 1024)

volatile bool runThread = true;


Thread threadHandle_curl;
Handle threadRequest_curl;


messages_t msg_history[MAX_MESSAGES];
int message_count = 0;
int next_message_id = 1;


char *system_msg;

void CURL_set_system_message(const char *system_msg_input)
{

   if (system_msg != NULL)
      free(system_msg);

   system_msg = (char*) malloc(strlen(system_msg_input) + 1);

   if (system_msg == NULL)
      sys_error(1,"set_system_message()\nmalloc failed!\n");

   strcpy(system_msg, system_msg_input);

}



void CURL_message_add(messages_t *messages_ptr, int *message_count_ptr, int *next_message_id_ptr, const char *role, const char *input)
{



   if ((role == NULL) || ((input == NULL) &&( msg_history[0].content ==NULL)))
   {
      sys_error(1,"message_add()\nerror!\n");
      printf("message_add() error\n");
      return;
   }

   message_count = message_count + 1;

   if (message_count >= MAX_MESSAGES) {
      free(msg_history[MAX_MESSAGES-1].role);
      free(msg_history[MAX_MESSAGES-1].content);

      message_count -= 1;
   }

   for (int i = ((message_count < MAX_MESSAGES)? message_count:MAX_MESSAGES); i >= 0; i--)
      msg_history[i] = msg_history[i-1];

   msg_history[0].role = (char*) malloc(strlen(role) + 1);

   if (msg_history[0].role == NULL)
     sys_error(1,"message_add()\nmalloc failed!\n");


   strcpy(msg_history[0].role, role);

   if (input != NULL)
   {
      msg_history[0].content = (char*) malloc(strlen(input) + 1);

      if (msg_history[0].content == NULL)
         sys_error(1,"message_add()\nmalloc failed!\n");

      strcpy(msg_history[0].content, input);
   }
   else
   {
      msg_history[0].content = (char*) malloc(sizeof(char));

      if (msg_history[0].content == NULL)
         sys_error(1,"message_add()\nmalloc failed!\n");

      UTIL_updateString(&msg_history[0].content, bufferStruct.buffers[0]);
      for (int i = 1; i < bufferStruct.numBuffers; i++)
         UTIL_appendString(&msg_history[0].content, bufferStruct.buffers[i]);
   }
/*
   printf("message_count       = %i\n",message_count);
   printf("next_message_id     = %i\n",next_message_id);
   printf("input               = %s\n",input);
   printf("msg_history[0].role    = %s\n",msg_history[0].role);
   printf("msg_history[0].content = %s\n",msg_history[0].content);

   printf("msg_history[1].role    = %s\n",msg_history[1].role);
   printf("msg_history[1].content = %s\n",msg_history[1].content);
*/
   msg_history[0].id = next_message_id;
   next_message_id = next_message_id + 1;
}














BufferStruct createBufferStruct() {
   BufferStruct bufferStruct;
   bufferStruct.buffers    = NULL;
   bufferStruct.numBuffers = 0;
   bufferStruct.totalWords = 0;
   bufferStruct.totalChars = 0;
   bufferStruct.currBuffer = 0;
   bufferStruct.spltBuffer = 0;
   bufferStruct.shouldSplt = 1;
   return bufferStruct;
}
BufferStruct bufferStruct;


void addWordToBuffer(char **buffer, int bufferSize, int *totalChars, const char *word)
{
   size_t wordLength = strlen(word);

    // Check if the word can fit in the current buffer
//    if (*totalChars + wordLength + 1 > bufferSize) {
        // Create a new buffer with increased size
//        bufferSize = (*totalChars + wordLength + 1) * 2;
   if (*totalChars + wordLength > bufferSize) {
        // Create a new buffer with increased size
      bufferSize = (*totalChars + wordLength + 1);
      *buffer = (char *)realloc(*buffer, bufferSize * sizeof(char));

      if (*buffer == NULL)
         sys_error(1,"Failed to allocate memory for the buffer.\n");

   }

    // Append the word to the buffer
   strcat(*buffer, word);
//    strcat(*buffer, " ");

//    *totalChars += wordLength + 1;
   *totalChars += wordLength;
}



void CURL_addWordToStruct(BufferStruct *bufferStruct, const char *word)
{
   int currentIndex = bufferStruct->numBuffers;


printf("%s", word);

    // Check if a buffer exists in the struct
   if ((bufferStruct->shouldSplt) == 0) 
   {
		bufferStruct->shouldSplt = 1;
   }
   
   if ((bufferStruct->shouldSplt) == 1)
   {
        // Create a new buffer in the struct
      char *newBuffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
      if (newBuffer == NULL)
         sys_error(1,"Failed to allocate memory for the buffer.\n");


      bufferStruct->buffers = (char **)realloc(bufferStruct->buffers, (bufferStruct->numBuffers + 1) * sizeof(char *));
      if (bufferStruct->buffers == NULL)
         sys_error(1,"Failed to allocate memory for the buffer array.\n");


        // Initialize the new buffer with null characters
      memset(newBuffer, '\0', BUFFER_SIZE * sizeof(char));

      bufferStruct->buffers[bufferStruct->numBuffers] = newBuffer;

 //     if (currentIndex != 0)
 //     {
//         bufferStruct->shouldSplt = 1;
 //        bufferStruct->spltBuffer = bufferStruct->numBuffers;
 //     }

      bufferStruct->numBuffers++;
   }
   else if (/*currentIndex == 0 ||*/
         strlen(bufferStruct->buffers[currentIndex - 1]) >= BUFFER_SIZE - 1 ||
//         (strstr(word, ".") != NULL) )
         (strstr(word, "\n") != NULL))
   {
     bufferStruct->shouldSplt = 0;
   }

//sprintf(prevWord, word);

   currentIndex = bufferStruct->numBuffers - 1;
   addWordToBuffer(&(bufferStruct->buffers[currentIndex]), BUFFER_SIZE, &(bufferStruct->totalChars), word);
   bufferStruct->totalWords++;
}



void freeBufferStruct(BufferStruct *bufferStruct) {
    for (int i = 0; i < (bufferStruct->numBuffers)-1; i++) {
        free(bufferStruct->buffers[i]);
    }
    free(bufferStruct->buffers);
}


bool sfx_buffer_done=true;






#define MAX_DATA_OBJECTS 16         // Maximum number of "data: " objects


int splitDataObjects(const char *inputString, char ***objects) {
    int objectCount = 0; // Number of "data: " objects found
    char dataMarker[] = "data: ";

    char *token = strdup(inputString); // Make a copy of the input string to avoid modifying the original
    char *start = token;
    char *end;

    while ((end = strstr(start, dataMarker)) != NULL) {
        if (end != start) {
            // Extract the object between the data markers
            int length = end - start;
            char *object = (char *)malloc(length + 1);
            strncpy(object, start, length);
            object[length] = '\0';

            // Add the object to the objects array
            (*objects)[objectCount++] = object;
        }
        start = end + strlen(dataMarker); // Move to the next potential object start
    }

    // Handle the last object (if any) after the last "data: " marker
    if (*start != '\0') {
        char *object = strdup(start);
        (*objects)[objectCount++] = object;
    }

    free(token); // Free the memory allocated by strdup
    return objectCount;
}








static size_t curl_curlopt_write_callback_stream(char *ptr, size_t size, size_t nmemb, void *str)
{
   char **objects = (char **)malloc(MAX_DATA_OBJECTS * sizeof(char *));

   int objectCount = splitDataObjects(ptr, &objects);
/*
   if (objectCount==0) {
      char *object = strdup(ptr);
      objects[0]=object;
      objectCount=1;
   }
*/
   for (int i = 0; i < objectCount; i++) {

      if (!strstr(objects[i], "[DONE]")) {

         if ((JSON_curl_response_stream(NULL, objects[i])) != 0) {
//            sys_state.Process=PROC_CANCEL;
//            sfx_buffer_done=true;
//            sys_error(0,"curl_curlopt_write_callback_stream error.\n");
//            printf("ERROR");
//            return CURLE_WRITE_ERROR;
         }

         sfx_buffer_done=false;

         if ((bufferStruct.currBuffer == 0) && (bufferStruct.totalWords == 1)) {
            add_text_to_buff(buffer->total_obj_block,bufferStruct.buffers[bufferStruct.currBuffer],2);

            bufferStruct.shouldSplt = -1;
         }
         else {
            if (bufferStruct.currBuffer != (bufferStruct.numBuffers)-1) {
               sfx_buffer_done=true;
               SFX_enqueueText(bufferStruct.buffers[bufferStruct.currBuffer], options.voice_response);
               bufferStruct.currBuffer = bufferStruct.numBuffers-1;

               if ((bufferStruct.shouldSplt) == 1) {
                  bufferStruct.spltBuffer = bufferStruct.currBuffer;
                  add_text_to_buff(buffer->total_obj_block,bufferStruct.buffers[bufferStruct.currBuffer],2);

                  bufferStruct.shouldSplt = -1;
               }
               else
                  update_object_in_buffer(bufferStruct.buffers[bufferStruct.currBuffer]);
            }
            else
               update_object_in_buffer(bufferStruct.buffers[bufferStruct.currBuffer]);
         }
      }
      free(objects[i]);
   }
   free(objects);


   if (sys_state.Process==PROC_CANCEL)
   {
//	   sys_error(0,"Cancelled.\n");
      return CURLE_WRITE_ERROR;
   }

   return size*nmemb;
}



struct string_t {
	char *ptr;
	size_t len;
};
struct string_t string;
void string_init(struct string_t *string) {
   string->len = 0;
   string->ptr = malloc(string->len+1);
   if (string->ptr == NULL) {
      sys_error(1,"malloc() failed");
   }
   string->ptr[0] = '\0';
}
static size_t curl_curlopt_write_callback(char *ptr, size_t size, size_t nmemb, void *str)
{
	struct string_t *string=str;
	size_t new_len = string->len + size*nmemb;
   string->ptr = realloc(string->ptr, new_len+1);
	if (string->ptr == NULL) {
		sys_error(1,"realloc() failed");
	}

	memcpy(string->ptr+string->len, ptr, size*nmemb);
   string->ptr[new_len] = '\0';
	string->len = new_len;

   return size*nmemb;
}

//#endif

void CURL_curl_sent_request(char* jsonObj)
{

   if (api_options.stream)
      bufferStruct = createBufferStruct();
   else
      string_init(&string);

buffer->total_obj_block++; // defined in curl.h keep track of split text objects which should form a single gfx object.

   CURL *curl = curl_easy_init();
   CURLcode res;

   if(curl != NULL) {
      struct curl_slist *headers = NULL;

      headers = curl_slist_append(headers, UTIL_string_concat("Authorization: Bearer ",api_key)); // fix this.. not being freed after strdup.

      if (api_options.model==MODEL_WHISPER)
      {
         headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
      }
      else
      {
         headers = curl_slist_append(headers, "Content-Type: application/json");
      }
//      headers = curl_slist_append(headers, "Accept:text/event-stream"); // not required



char* api_url = api_info[api_options.model].api_url;
curl_easy_setopt(curl, CURLOPT_URL, api_url);

      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

 
if (api_options.model==MODEL_WHISPER)
{
//   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, encodedData);


curl_libsndfile(curl);

}
else
{
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
}



if (api_options.stream)
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_curlopt_write_callback_stream);

else
{
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_curlopt_write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
}
#ifdef CURL_NO_CERT
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
#else
      curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/cacert.pem");
#endif
#ifdef CURL_VERBOSE
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

      res = curl_easy_perform(curl);

      if(res != CURLE_OK)
      {
//      printf("curl_easy_perform() failed: %s\n",
//            curl_easy_strerror(res));
			sys_error(0,curl_easy_strerror(res));
      }

      if (sys_state.Process != PROC_CANCEL)
      {
         if (api_options.stream)
         {
            if (!sfx_buffer_done)
               SFX_enqueueText(bufferStruct.buffers[bufferStruct.currBuffer], options.voice_response);

            if (bufferStruct.totalWords != 0)
               CURL_message_add(NULL, NULL, NULL, "assistant", NULL);

            freeBufferStruct(&bufferStruct);
         }

         else
         {
            if (api_options.model==MODEL_WHISPER)
               JSON_curl_response_whisper(jsonObj, string.ptr);
            else if (api_options.model==MODEL_DALLE)
               JSON_curl_response_dalle(jsonObj, string.ptr);
            else
            {
               SFX_enqueueText(string.ptr, options.voice_response);
               JSON_curl_response(jsonObj, string.ptr);
            }
         }
      }

      if (api_options.model==MODEL_WHISPER)
         curl_libsndfile_clean();

      curl_slist_free_all(headers);
      headers = NULL;
      curl_easy_cleanup(curl);
      curl = NULL;
      curl_global_cleanup();
   }
}


void curl_thread(void *arg)
{
   while(runThread) {
      svcWaitSynchronization(threadRequest_curl, U64_MAX);
      svcClearEvent(threadRequest_curl);

      if (!runThread)
         break;

      switch (api_options.model) {
         case MODEL_GPT4:
         case MODEL_GPT35:
		    api_options.stream = true;
            CURL_curl_sent_request(JSON_curl_request_chat("", msg_history));
            break;

         case MODEL_DAVINCI3:
		    api_options.stream = false;
            CURL_curl_sent_request(JSON_curl_request("", msg_history));
            break;

         case MODEL_WHISPER:
		    api_options.stream = false;
            CURL_curl_sent_request(JSON_curl_request_whisper("", msg_history));
            break;

         case MODEL_DALLE:
		    api_options.stream = false;
            CURL_curl_sent_request(JSON_curl_request_dalle("", msg_history));
			api_options.model = MODEL_GPT35; // TODO do this somewhere else..
            break;

         default:
            break;
      }
      sys_state.Process   = PROC_NONE;
      sys_state.MenuState = STATE_MAIN_CHAT;
   }
}



void curl_init(void)
{
   svcCreateEvent(&threadRequest_curl,0);
   threadHandle_curl = threadCreate(curl_thread, 0, STACKSIZE, 0x3f, 0, true);

   if (threadHandle_curl == NULL)
      sys_error(0,"threadHandle_curl failed to start.\n");
}

void curl_exit(void)
{
   runThread = false;

   svcSignalEvent(threadRequest_curl);
   threadJoin(threadHandle_curl, U64_MAX);
   svcCloseHandle(threadRequest_curl);

}
