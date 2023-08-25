
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include <jansson.h>

#include <curl.h>


#include "sys.h"
#include "main.h"
#include "json.h"

// Used with for dalle model with prompt
char* JSON_curl_request_dalle(const char* prompt, const messages_t *messages)
{
   json_t* request_obj = json_object();

   json_object_set_new(request_obj, "n",      json_integer(1));
   json_object_set_new(request_obj, "size",   json_string("256x256"));
   json_object_set_new(request_obj, "prompt", json_string(messages[0].content));

   char* request_str = json_dumps(request_obj, 0);

   json_decref(request_obj);
   return request_str;
}

// Used with the whisper-1 model
char* JSON_curl_request_whisper(const char* prompt, const messages_t *messages)
{
   json_t* request_obj = json_object();
   char* api_model = api_info[api_options.model].api_model;

   json_object_set_new(request_obj, "model", json_string(api_model));

   char* request_str = json_dumps(request_obj, 0);

   json_decref(request_obj);
   return request_str;
}

char* concatenated_string = NULL;
// Used with for text-davinci-003 model with prompt
char* JSON_curl_request(const char* prompt, const messages_t *messages)
{
   char* api_model = api_info[api_options.model].api_model;

   json_t* request_obj = json_object();

   json_object_set_new(request_obj, "model",       json_string(api_model));
   json_object_set_new(request_obj, "max_tokens",  json_integer(api_options.max_tokens));
   json_object_set_new(request_obj, "temperature", json_real(api_options.temperature));




   if (concatenated_string != NULL)
      free(concatenated_string);



   if (system_msg != NULL)
      concatenated_string = strdup(system_msg);

//    concatenated_string = NULL;
//    for (int i = 0; i < sizeof(messages) / sizeof(messages[0].role); i++) {
	for (int i = message_count-1; i > -1; i--) {
        if (concatenated_string == NULL) {
            concatenated_string = strdup(messages[i].role);

            char* temp = concatenated_string;
            concatenated_string = malloc(strlen(temp) + strlen(messages[i].content) + 3);
            strcpy(concatenated_string, temp);
			strcat(concatenated_string, ": ");
            strcat(concatenated_string, messages[i].content);
            free(temp);
        } else {
            char* temp = concatenated_string;
            concatenated_string = malloc(strlen(temp) + strlen(messages[i].role) + strlen(messages[i].content) + 3);
            strcpy(concatenated_string, temp);
            strcat(concatenated_string, messages[i].role);
			strcat(concatenated_string, ": ");
            strcat(concatenated_string, messages[i].content);
            free(temp);
        }
    }

//sys_error(0,concatenated_string);

json_object_set_new(request_obj, "prompt",      json_string(concatenated_string));









//   json_object_set_new(request_obj, "prompt",      json_string(messages[0].content));

   char* request_str = json_dumps(request_obj, 0);

   json_decref(request_obj);
   return request_str;
}


// Used with the gpt models with role / message
char* JSON_curl_request_chat(const char* prompt, const messages_t *messages)
{
   json_t* request_obj = json_object();
   char* api_model     = api_info[api_options.model].api_model;

   json_object_set_new(request_obj, "model", json_string(api_model));
   json_object_set_new(request_obj, "max_tokens", json_integer(api_options.max_tokens));
   json_object_set_new(request_obj, "temperature", json_real(api_options.temperature));

   if (api_options.stream)
      json_object_set_new(request_obj, "stream", json_boolean(true));

   json_t* history_arr = json_array();

   for (int i = message_count-1; i > -1; i--) {
      json_t* message_obj = json_object();
      json_object_set_new(message_obj, "role", json_string(messages[i].role));
      json_object_set_new(message_obj, "content", json_string(messages[i].content));
      json_array_append_new(history_arr, message_obj);
   }

   json_object_set_new(request_obj, "messages", history_arr);

   char* request_str = json_dumps(request_obj, 0);

   json_decref(request_obj);
   return request_str;
}







int JSON_curl_response_dalle(char *jsonObj,char *text)
{
   size_t i;
   json_t *root;
   json_error_t error;

   if(!text)
      return 1;

   root = json_loads(text, 0, &error);

   if ( !root ) {
      printf("JSON error %d: %s\n", error.line, error.text );
      printf("JSON: %s\n", text );
//	  gfx_chat_text_add(gfx_chat_text, &gfx_chat_text_count, &next_gfx_chat_text_id, text, 25.0f, 0.0f, 0.5f,&clr);
//svcSleepThread(10000000);
//	  sys_error(0,error.text);
//	  sys_error(0,text);
      return 1;
   }

   json_t *data_obj, *data;

   data_obj = json_object_get(root, "data");
   if(!json_is_array(data_obj))
   {
      printf("\x1b[37;1merror:\x1b[0m choices is not an array\n");
      printf("\x1b[37;1mRequest:\x1b[0m\n%s\n\n",jsonObj);
      printf("\x1b[37;1mResponse:\x1b[0m\n%s\n\n",text);
   }


   for(i = 0; i < json_array_size(data_obj); i++)
   {
      json_t *url;

      data = json_array_get(data_obj, i);
      if(!json_is_object(data))
      {
         sys_error(0,"error1: data is not an object\n");
         return 1;
      }


      url = json_object_get(data, "url");
      if(!json_is_string(url))
      {
         sys_error(0,"error5: url is not a string\n");
         return 1;
      }

#ifdef CONSOLE_ENABLE
      printf("\x1b[37;1m");
      printf("DALL-E: ");
      printf("\x1b[0m");
      printf("\x1b[46m");
      printf("%s\n", json_string_value(url));
      printf("\x1b[0m\x1b[46;1m");
#endif

      add_text_to_buff(0,json_string_value(url),2); // TODO Add proper object block id !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to all add_text_to_buff calls

      download_image_to_buff(json_string_value(url));

   }

    json_decref(root);
    return 0;
}







int JSON_curl_response_whisper(char *jsonObj,char *text)
{
//   size_t i;
   json_t *root;
   json_error_t error;
//Color clr = {255,255,255,255}; // black
   if(!text)
      return 1;

   root = json_loads(text, 0, &error);

   if ( !root )
   {
      printf("JSON error %d: %s\n", error.line, error.text );
printf("JSON: %s\n", text );
//	  gfx_chat_text_add(gfx_chat_text, &gfx_chat_text_count, &next_gfx_chat_text_id, text, 25.0f, 0.0f, 0.5f,&clr);
//svcSleepThread(10000000);
//	  sys_error(0,error.text);
//	  sys_error(0,text);
      return 1;
   }

   json_t *content;


      content = json_object_get(root, "text");
      if(!json_is_string(content))
      {
         sys_error(0,"whisper error0: textbuf is not a string\n");
      printf("JSON error %d: %s\n", error.line, error.text );
printf("JSON: %s\n", text );
         return 1;
      }
	  

#ifdef CONSOLE_ENABLE
      printf("\x1b[37;1m");
      printf("You: ");
      printf("\x1b[0m");
      printf("\x1b[46m");
      printf("%s\n", json_string_value(content));
      printf("\x1b[0m\x1b[46;1m");
#endif


char response_tmp[json_string_length(content)+7];
sprintf(response_tmp,"You: %s\n", json_string_value(content));

CURL_message_add(NULL, NULL, NULL, "user", response_tmp);

// DEPRECATED
//	  gfx_chat_text_add(gfx_chat_text, &gfx_chat_text_count, &next_gfx_chat_text_id, response_tmp, 25.0f, 0.0f, 0.5f,&clr);

add_text_to_buff(0,json_string_value(content),1);
   

    json_decref(root);

    return 0;
}


int JSON_curl_response(char *jsonObj,char *text)
{
   size_t i;
   json_t *root;
   json_error_t error;
//Color clr = {255,255,255,255}; // black
   if(!text)
      return 1;

   root = json_loads(text, 0, &error);

   if ( !root )
   {
      printf("JSON error %d: %s\n", error.line, error.text );
printf("JSON: %s\n", text );
//	  gfx_chat_text_add(gfx_chat_text, &gfx_chat_text_count, &next_gfx_chat_text_id, text, 25.0f, 0.0f, 0.5f,&clr);
//svcSleepThread(10000000);
	  sys_error(0,error.text);
	  sys_error(0,text);
      return 1;
   }

   json_t *choices, *choice;

   choices = json_object_get(root, "choices");
   if(!json_is_array(choices))
   {
      printf("\x1b[37;1merror:\x1b[0m choices is not an array\n");
      printf("\x1b[37;1mRequest:\x1b[0m\n%s\n\n",jsonObj);
      printf("\x1b[37;1mResponse:\x1b[0m\n%s\n\n",text);

	  sys_error(0,text);
   }


   for(i = 0; i < json_array_size(choices); i++)
   {
      json_t *message, *role, *content;

      choice = json_array_get(choices, i);
      if(!json_is_object(choice))
      {
         sys_error(0,"error1: choice is not an object\n");
         return 1;
      }

      message = json_object_get(choice, "message");
      if(!json_is_object(message))
      {
//         sys_error(0,"error2: message is not an object\n");
//         return 1;
      }

      role = json_object_get(message, "role");
      if(!json_is_string(role))
      {
//         sys_error(0,"error3: role is not a string\n");
//         return 1;
      }

      content = json_object_get(message, "content");
      if(!json_is_string(content))
      {
//         sys_error(0,"error4: content is not a string\n");
//         return 1;
      

      content = json_object_get(choice, "text");
      if(!json_is_string(content))
      {
         sys_error(0,"error5: textbuf is not a string\n");
         return 1;
      }
	  }

#ifdef CONSOLE_ENABLE
      printf("\x1b[37;1m");
      printf("AI: ");
      printf("\x1b[0m");
      printf("\x1b[46m");
      printf("%s\n", json_string_value(content));
      printf("\x1b[0m\x1b[46;1m");
#endif

      CURL_message_add(NULL, NULL, NULL, "assistant", json_string_value(content));


//char response_tmp[json_string_length(content)+7];
//sprintf(response_tmp,"AI: %s\n", json_string_value(content));


// DEPRECATED
//	  gfx_chat_text_add(gfx_chat_text, &gfx_chat_text_count, &next_gfx_chat_text_id, response_tmp, 25.0f, 0.0f, 0.5f,&clr);



add_text_to_buff(0,json_string_value(content),2);



   }

    json_decref(root);
    return 0;
}


int JSON_curl_response_stream(char *jsonObj,char *text)
{
   size_t i;
   json_t *root;
   json_error_t error;

   json_t *choices, *choice;

   if(!text)
   {
//	   sys_error(0,"JSON_curl_response_stream with no text"); // Do not use errors during curl_perform...
      return 1;
   }

   root = json_loads(text, 0, &error);

   if ( !root )
   {
      if (error.line == 3)
      {
		  printf("JSON_curl_response_stream error: %i",error.line);
/*
         char *ptr = strstr(text, "data:");
         if (ptr != NULL) {
            *ptr = '\0';
            return JSON_curl_response_stream(NULL, ptr + strlen("data:"));
			*/
  //       }
	//	 else
	//	 {
//            sys_error(0,error.text);
//			sys_error(0,text);
//		 }
      }
      else
      {
//         printf("JSON error %d: %s\n", error.line, error.text );
//         sys_error(0,error.text);
//		 sys_error(0,text);
      }
      return 1;
   }


   choices = json_object_get(root, "choices");
   if(!json_is_array(choices))
   {
//      printf("\x1b[37;1merror:\x1b[0m choices is not an array\n");
//      printf("\x1b[37;1mRequest:\x1b[0m\n%s\n\n",jsonObj);
//      printf("\x1b[37;1mResponse:\x1b[0m\n%s\n\n",text);
   }


   for(i = 0; i < json_array_size(choices); i++)
   {
      json_t *message, *role, *content, *delta;

      choice = json_array_get(choices, i);
      if(!json_is_object(choice))
      {
 //        printf("error1: choice %d is not an object\n", i + 1);
 //        return 1;
      }

      message = json_object_get(choice, "message");
      if(!json_is_object(message))
      {
//         printf("error2: choice %d is not an object\n", i + 1);
//         return 1;
      }
      else
      {
         role = json_object_get(message, "role");
         if(!json_is_string(role))
         {
//            printf("error3: choice %d: role is not a string\n", i + 1);
//            return 1;
         }
      }

      delta = json_object_get(choice, "delta");
      if(!json_is_object(delta))
      {
//         printf("error4: choice %d: message is not a string\n", i + 1);
//         return 1;
      }

      role = json_object_get(delta, "role");
      if(!json_is_string(role))
      {
//         printf("role=%s\n", json_string_value(role));
//         return 1;
      }

      content = json_object_get(delta, "content");
      if(json_is_string(content))
      {
//		  sys_error(0,json_string_value(content));
         CURL_addWordToStruct(&bufferStruct, json_string_value(content));
      }
	        else
      {
         json_decref(root);
//         sys_error(0,text);
		 return 1;
	  }
/*
#ifdef CONSOLE_ENABLE
      printf("\x1b[37;1m");
      printf("AI: ");
      printf("\x1b[0m");
      printf("\x1b[46m");
      printf("%s", json_string_value(content));
      printf("\x1b[0m\x1b[46;1m");
#endif
*/
   }
    json_decref(root);
    return 0;
}

int JSON_write_json_to_file(const char* filename, json_t* value)
{
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error opening file for writing.\n");
        return 1;
    }

    json_dumpf(value, fp, JSON_INDENT(4));
    fclose(fp);

    return 0;
}

json_t* JSON_read_json_from_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Error opening file for reading.\n");
        return NULL;
    }

    json_error_t error;
    json_t* data = json_loadf(fp, 0, &error);
    fclose(fp);

    if (!data) {
        printf("Error parsing JSON: %s\n", error.text);
        return NULL;
    }

    return data;
}




int json_write_test(void)
{
    // Creating a JSON object
    json_t* root = json_object();
    json_object_set_new(root, "api_key", json_string(api_key));
//    json_object_set_new(root, "age", json_integer(25));

    // Writing JSON object to a file
    if (JSON_write_json_to_file("data.json", root) != 0) {
        printf("Error writing JSON to file.\n");
        json_decref(root);
        return 1;
    }

    json_decref(root);
   return 0;
}

int json_read_test(void)
{

    // Reading JSON from file into memory
    json_t* data = JSON_read_json_from_file("data.json");
    if (!data) {
        printf("Error reading JSON from file.\n");
        return 1;
    }

    // Accessing values from the JSON object
    json_t* key = json_object_get(data, "api_key");
//    json_t* age = json_object_get(data, "age");

    if (key && json_is_string(key)) {
//        printf("api_key: %s\n", json_string_value(key));
		sprintf(api_key, json_string_value(key));
    }

//    if (age && json_is_integer(age)) {
//        printf("Age: %d\n", (int)json_integer_value(age));
//    }

    json_decref(data);

    return 0;
}

int JSON_read_api_key(void)
{
   json_t* data = JSON_read_json_from_file("sdmc:/3ds/OpenAI/api_key.json");

   if (!data) 
      data = JSON_read_json_from_file("romfs:/api_key.json");

   if (!data)
      return 1;

   json_t* key = json_object_get(data, "api_key");

   if (key && json_is_string(key))
      sprintf(api_key, json_string_value(key));

   json_decref(data);
   return 0;
}

int JSON_write_api_key(void)
{
    // Creating a JSON object
    json_t* root = json_object();
    json_object_set_new(root, "api_key", json_string(api_key));
//    json_object_set_new(root, "age", json_integer(25));

    // Writing JSON object to a file
    if (JSON_write_json_to_file("sdmc:/3ds/OpenAI/api_key.json", root) != 0) {
        printf("Error writing JSON to file.\n");
        json_decref(root);
        return 1;
    }

    json_decref(root);
   return 0;
}


void json_exit()
{
   if (concatenated_string != NULL)
      free(concatenated_string);
}






