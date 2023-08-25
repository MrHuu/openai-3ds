#ifndef _MAIN_H
#define _MAIN_H

/* Compilation options, define to use */

//#define DEBUG_ENABLE
//#define CONSOLE_ENABLE
//#define CURL_NO_CERT
//#define CURL_VERBOSE
//#define SFX_ALL_VOICES





#define MAX_SENTENCES          20   // Maximum auto-reply sentences
#define MAX_SENTENCE_LENGTH   100   // Maximum length of auto-reply sentence

#define MAX_APIS 5
#define MAX_CALLBACKS 10


extern char api_key[128];
extern int selected_chat;
extern int current_menu;



typedef struct
{
   enum { STATE_INIT,
          STATE_IDLE,
          STATE_PROCESS,
		  STATE_RECORDING,
          STATE_MAIN,
          STATE_MAIN_CHAT,
          STATE_OPTIONS,
          STATE_OPTIONS_API,
          STATE_CHAT,
		  STATE_IMAGE_GEN,
		  STATE_DEBUG_01,
		  STATE_DEBUG_02,
		  STATE_DEBUG_MIC,
         STATE_CHAT_SELECT,
         STATE_GET_API_KEY,
   } MenuState;

   enum { PROC_NONE,
          PROC_RECORD,
		  PROC_RECORD_DONE,
          PROC_VOICE,
          PROC_JSON_REQUEST,
          PROC_JSON_RESPONSE,
          PROC_CURL_SENT,
		  PROC_CANCEL
   } Process;

   enum { API_IDLE,
          API_WHISPER_REQUEST,
          API_WHISPER_RESPONSE,
   } api_state;

} sys_state_t;

extern sys_state_t sys_state;


typedef struct
{
   bool auto_response;
   bool voice_enable;
   int  voice_request;
   int  voice_response;
   bool debug_enable;
   bool border_enable;
   bool whisp_to_gpt;
} options_t;
extern options_t options;

typedef struct
{
   enum { MODEL_GPT35,
          MODEL_GPT4,
		  MODEL_DAVINCI3,
		  MODEL_WHISPER,
		  MODEL_DALLE
   } model;
   int   max_tokens;
   float temperature;
   bool stream;
} api_options_t;
extern api_options_t api_options;



typedef struct {
    char api_name[50];
    char api_model[50];
    char api_url[100];
    void (*callback_functions[MAX_CALLBACKS])();
} api_info_t;

extern api_info_t api_info[MAX_APIS];





#endif /* _MAIN_H */




/*

/v1/chat/completions
	gpt-4, gpt-4-0613, gpt-4-32k, gpt-4-32k-0613, gpt-3.5-turbo, gpt-3.5-turbo-0613, gpt-3.5-turbo-16k, gpt-3.5-turbo-16k-0613

/v1/completions
	text-davinci-003, text-davinci-002, text-curie-001, text-babbage-001, text-ada-001

/v1/edits
	text-davinci-edit-001, code-davinci-edit-001

/v1/audio/transcriptions
	whisper-1

/v1/audio/translations
	whisper-1

/v1/fine-tunes
	davinci, curie, babbage, ada

/v1/embeddings
	text-embedding-ada-002, text-search-ada-doc-001

/v1/moderations
	text-moderation-stable, text-moderation-latest

*/

