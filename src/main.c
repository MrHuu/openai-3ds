
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>

#include <3ds.h>

//#include <time.h>
//#include <inttypes.h>

#include "curl.h"
#include "gfx.h"
#include "main.h"
#include "menu.h"
#include "sfx.h"
#include "sys.h"
#include "json.h"

#include "mic.h"
#include "image.h"







char input[MAX_INPUT_LENGTH];
char user_input[MAX_INPUT_LENGTH];


int current_menu;

char api_key[128];


sys_state_t     sys_state     = { STATE_INIT, PROC_NONE, API_IDLE };
options_t       options       = { false, false, 0, 2 , false, true, true};
api_options_t   api_options   = { MODEL_GPT35, 512, 1.0f ,false}; // API defaults










api_info_t api_info[MAX_APIS];


void callbackFunction1() { // finish this properly.. callbacks for response / request
    printf("Callback Function 1\n");
}

void callbackFunction2() {
    printf("Callback Function 2\n");
}



void init_api_info() {

    strcpy(api_info[MODEL_GPT35].api_name, "gpt-3.5");
    strcpy(api_info[MODEL_GPT35].api_model, "gpt-3.5-turbo");
    strcpy(api_info[MODEL_GPT35].api_url, "https://api.openai.com/v1/chat/completions");
//    api_info[MODEL_GPT35].callback_functions[0] = callbackFunction1;

    strcpy(api_info[MODEL_GPT4].api_name, "gpt-4");
    strcpy(api_info[MODEL_GPT4].api_model, "gpt-4");
    strcpy(api_info[MODEL_GPT4].api_url, "https://api.openai.com/v1/chat/completions");
//    api_info[MODEL_GPT4].callback_functions[0] = callbackFunction2;

    strcpy(api_info[MODEL_DAVINCI3].api_name, "davinci-003");
    strcpy(api_info[MODEL_DAVINCI3].api_model, "text-davinci-003");
    strcpy(api_info[MODEL_DAVINCI3].api_url, "https://api.openai.com/v1/completions");
//    api_info[MODEL_DAVINCI3].callback_functions[0] = callbackFunction2;

    strcpy(api_info[MODEL_WHISPER].api_name, "whisper");
    strcpy(api_info[MODEL_WHISPER].api_model, "whisper-1");
    strcpy(api_info[MODEL_WHISPER].api_url, "https://api.openai.com/v1/audio/transcriptions");
//    api_info[MODEL_WHISPER].callback_functions[0] = callbackFunction2;

    strcpy(api_info[MODEL_DALLE].api_name, "dall-e");
    strcpy(api_info[MODEL_DALLE].api_model, "dall-e");
    strcpy(api_info[MODEL_DALLE].api_url, "https://api.openai.com/v1/images/generations");
//    api_info[MODEL_DALLE].callback_functions[0] = callbackFunction2;

}
















typedef struct {
    char sentences[MAX_SENTENCES][MAX_SENTENCE_LENGTH];
    int num_sentences;
} sentence_list;

sentence_list list = {
   .num_sentences = 14,
   .sentences = {
      "Continue",
      "Add a joke and continue.",
      "Add a random fact and continue.",
      "Haha, please continue.",
      "Please, tell me more.",
      "Are you sure about that?",
      "Add bunnies to the story.",
      "Another one!",
      "Make a plot-twist",
      "Proceed.",
      "Continue, making it sad.",
      "Continue, making it funny.",
      "Continue, making it scary.",
      "Continue, making it cute.",
   }
};






char* get_random_sentence(sentence_list* list) {
   srand(time(NULL));

   int index = rand() % list->num_sentences;

   return list->sentences[index];
}

void print_sentence_list(sentence_list* list) {
   printf("Sentence list (%d sentences):\n", list->num_sentences);
   for (int i = 0; i < list->num_sentences; i++) {
      printf("  %d. %s\n", i + 1, list->sentences[i]);
   }
}

void add_sentence(sentence_list* list, const char* sentence) {
   if (list->num_sentences < MAX_SENTENCES) {
      strcpy(list->sentences[list->num_sentences], sentence);
      list->num_sentences++;
   }
}

void delete_sentence(sentence_list* list, int index) {
   if (index >= 0 && index < list->num_sentences) {
      // Shift all sentences after the deleted sentence back by one index
      for (int i = index; i < list->num_sentences - 1; i++) {
         strcpy(list->sentences[i], list->sentences[i + 1]);
      }
      list->num_sentences--;
   }
}


void debug_menu(u32 kDown)
{
   if (kDown & KEY_START)
   {
//      flite_queue_clear(&flite_queue);
   }

   if (kDown & KEY_A)
   {
      int ret;
      ret = sys_swkbd("Enter system message","",user_input);
      // Add a new sentence to the list
      if (ret == 1)
         CURL_set_system_message(user_input);

   }

   if (kDown & KEY_L)
   {
      add_text_to_buff(0,"Huu",0);
   }

   if (kDown & KEY_R)
   {
      download_image_to_buff("https://raw.githubusercontent.com/MrHuu/openbor-3ds/3DS/engine/resources/ctr/OpenBOR_Icon_48x48.png");
   }
   if (kDown & KEY_ZR)
   {
      download_image_to_buff("https://raw.githubusercontent.com/MrHuu/openbor-3ds/3DS/engine/resources/ctr/OpenBOR_Logo_256x128.png");

   }
   if (kDown & KEY_ZL)
   {
      add_image_to_buff("sdmc:/3ds/OpenAI/image4.png");
   }
}

void options_menu(u32 kDown)
{


	  if (kDown & KEY_A)
      {
		  options.voice_enable=!options.voice_enable;
//refresh_options=true;

      }

	  if (kDown & KEY_X)
      {
		  	int ret;
			ret = sys_swkbd("Enter a automated response","",user_input);
			// Add a new sentence to the list
			if (ret == 1)
				add_sentence(&list, user_input);
      } 
	  
	  if (kDown & KEY_Y)
      {
         options.auto_response = !options.auto_response;
#ifdef CONSOLE_ENABLE
         printf("auto_response: %s\n", options.auto_response?"enabled":"disabled");
#endif
      } 

}

int chat_menu(u32 kDown)
{
//	Color clr = {255,255,255,255}; // black
   int ret = 0;
   if (options.auto_response)
   {
      if (kDown & KEY_B){
         options.auto_response=false;
         ret = -2;
         return ret;
      }
      if ((playBuf.status == NDSP_WBUF_DONE) ||  (playBuf.status == NDSP_WBUF_FREE))// TODO get proper playback status
      {
         char* sentence = get_random_sentence(&list);
         strcpy(user_input,sentence);
         ret = 1;
      }
      else
         if (ret != -2) ret = -2;

   }
   else
      ret = sys_swkbd("Ask a question","",user_input);

   if (ret == 0)
   {
      options.auto_response=true;
   }
   else if (ret == 1)
   {
      gfx_text_clear(gfx_text_menu, &gfx_text_menu_count, &next_gfx_text_menu_id);

//      if (options.auto_response)
//         gfx_text_add(gfx_text_menu, &gfx_text_menu_count, &next_gfx_text_menu_id, "auto-response", 115.0f, 20.0f, 0.5f,&clr);

      sys_state.Process=PROC_VOICE;

      SFX_enqueueText(user_input, options.voice_request);

#ifdef CONSOLE_ENABLE
      printf("\x1b[37;1m");
      printf("\nYou: ");
      printf("\x1b[0m");
      printf("\x1b[46m");
      printf("%s\n\n", user_input);
      printf("\x1b[0m\x1b[46;1m");
#else
//      char user_input_tmp[1024];
//      sprintf(user_input_tmp,"You: %s\n", user_input);


      add_text_to_buff(0,user_input, 1);


#endif
      CURL_message_add(msg_history, &message_count, &next_message_id, "user", user_input);

      sys_state.Process   = PROC_CURL_SENT;
   }
   else if (ret == -1)
      sys_state.MenuState=STATE_MAIN_CHAT;

   return ret;
}


void main_chat_menu(u32 kDown)
{
   if(kDown & KEY_X)
   {/*
      if(mic_recording) // MICU_IsSampling()
      {
         MIC_record_stop();

         sys_state.MenuState = STATE_RECORDING;
         sys_state.api_state = API_WHISPER_REQUEST;

         svcSignalEvent(threadRequest_curl);
      }
      else
      {*/
         api_options.model   = MODEL_WHISPER;
		 sys_state.MenuState = STATE_RECORDING;
		 sys_state.Process   = PROC_RECORD;
		 
         MIC_record_start();
		 
 //     }
   }

   if(kDown & KEY_START)
      sys_exit();

   if(kDown & KEY_SELECT)
      sys_state.MenuState=STATE_GET_API_KEY;

}

void debug_mic_menu(u32 kDown)
{


   if(kDown & KEY_A)
   {
      if(mic_recording) // MICU_IsSampling()
      {
         MIC_record_stop();


//         sys_state.MenuState = STATE_PROCESS;
//         sys_state.api_state = API_WHISPER_REQUEST;

//         svcSignalEvent(threadRequest_curl);
      }
      else
      {
//         api_options.model=MODEL_WHISPER;
         MIC_record_start();
      }
   }


   if(kDown & KEY_B)
   {
	   MIC_record_play();
   }


   if(kDown & KEY_X)
   {
	   wav_buf();
	   
   }

   if(kDown & KEY_X)
   {
//	   wav_buf();
   }

   if(kDown & KEY_L)
   {
	   play_wav_buf();
   }

   if(kDown & KEY_R)
   {
	   play_wav_file();
   }



}


int selected_chat = 0;

void chat_select_menu(u32 kDown)
{
   if(kDown & KEY_UP)
   {
      if (selected_chat >1)
         selected_chat--;
   }
   if(kDown & KEY_DOWN)
   {
      if (selected_chat < buffer->total_obj_block)
         selected_chat++;
   }
   if(kDown & KEY_B)
   {
      sys_state.MenuState=STATE_DEBUG_01;
	  selected_chat = 0;
   }
}



int main()
{
   sys_state.MenuState=STATE_INIT;

   init_api_info();
   sys_init();

   if ((JSON_read_api_key()) != 0)
      sys_error(0,"NO API-Key.\n");

   current_menu = MAIN;

#ifdef CONSOLE_ENABLE
   printf("\x1b[46m\x1b[2J");
   printf("\x1b[37;1m");
   printf("\n OpenAI-3DS\n\n");
   printf("\x1b[0m");
   printf("\x1b[46m");
#endif

   while (aptMainLoop()) {
      hidScanInput();
      u32 kDown = hidKeysDown();

      touchPosition touch;
      hidTouchRead(&touch);



      switch(sys_state.MenuState)
      {
         case STATE_IDLE:
            current_menu = MAIN;
            break;

         case STATE_MAIN_CHAT:
            current_menu = MAIN_CHAT;
            break;
	  
         case STATE_OPTIONS:
            current_menu = OPTIONS;
            break;
			
         case STATE_OPTIONS_API:
            current_menu = OPTIONS_API;
            break;
	  
         case STATE_DEBUG_01:
            current_menu = DEBUG_01;
            break;

         case STATE_DEBUG_02:
            current_menu = DEBUG_02;
            break;

         case STATE_DEBUG_MIC:
            current_menu = DEBUG_MIC;
            break;

         case STATE_CHAT_SELECT:
            current_menu = CHAT_SELECT;
            break;

         case STATE_GET_API_KEY:
            current_menu = GET_API_KEY;
            break;

         default:
            break;
      }

      for (size_t i = 0; i < menus[current_menu].num_functions; i++)
      {
         if (menus[current_menu].functions[i].callback != NULL)
            menus[current_menu].functions[i].callback(menus[current_menu].functions[i].data);
         
      }


   if (sys_state.Process == PROC_NONE)
   {
      for (size_t i = 0; i < menus[current_menu].num_buttons; i++)
      {
         if ((touch.px >  menus[current_menu].buttons[i].x) &&
               (touch.px < (menus[current_menu].buttons[i].x + menus[current_menu].buttons[i].width)) &&
               (touch.py >  menus[current_menu].buttons[i].y) &&
               (touch.py < (menus[current_menu].buttons[i].y + menus[current_menu].buttons[i].height)) &&
               (kDown & KEY_TOUCH))
         {
         if (menus[current_menu].buttons[i].callback != NULL)
            menus[current_menu].buttons[i].callback(menus[current_menu].buttons[i].data);
         }
      }
   }
      if ((sys_state.MenuState==STATE_IDLE)&&(options.auto_response))
         sys_state.MenuState=STATE_CHAT;



      if(sys_state.MenuState==STATE_DEBUG_01)
      {
        debug_menu(kDown);
      }


   if ((sys_state.api_state == API_WHISPER_RESPONSE) && (sys_state.Process==PROC_NONE))
   {
      sys_state.api_state = API_IDLE;
	  sys_state.MenuState = STATE_MAIN_CHAT;
   }

   
   
   if ((sys_state.api_state == API_WHISPER_REQUEST) && (sys_state.Process==PROC_NONE))
   {
      sys_state.MenuState = STATE_PROCESS;
      api_options.model   = MODEL_GPT35;
sys_state.Process = PROC_RECORD_DONE;
      svcSignalEvent(threadRequest_curl);
      sys_state.api_state = API_WHISPER_RESPONSE;
   }





      if(mic_recording) // MICU_IsSampling()
      {
		     if(kDown & KEY_X)
   {
         MIC_record_stop();

//         sys_state.MenuState = STATE_RECORDING;
      sys_state.Process = PROC_RECORD_DONE;
         sys_state.api_state = API_WHISPER_REQUEST;

         svcSignalEvent(threadRequest_curl);
      }
   }


      if(sys_state.MenuState==STATE_CHAT_SELECT)
      {
         chat_select_menu(kDown);
      }

      if(sys_state.MenuState==STATE_MAIN_CHAT)
      {
         main_chat_menu(kDown);
      }

      if(sys_state.MenuState==STATE_OPTIONS)
      {
         options_menu(kDown);
      }

      if(sys_state.MenuState==STATE_DEBUG_MIC)
      {
         debug_mic_menu(kDown);
      }



      if(sys_state.MenuState==STATE_CHAT)
      {
         if(*api_key == 0)
         {
            sys_error(0,"API-Key not found.\n");
            sys_state.MenuState=STATE_MAIN_CHAT;
         }
		 else
            chat_menu(kDown);
      }

      if(sys_state.MenuState==STATE_IMAGE_GEN)
      {

         int ret = sys_swkbd("Describe an image","",user_input);
         if (ret == 1)
         {
            api_options.model = MODEL_DALLE;
            add_text_to_buff(0,user_input, 1);

            CURL_message_add(msg_history, &message_count, &next_message_id, "user", user_input);

            sys_state.Process = PROC_CURL_SENT;
         }
         else if (ret == -1)
            sys_state.MenuState=STATE_MAIN_CHAT;
		  // TODO set model to dalle and open keyboard (Add generic keyboard call)
	  }

      if((sys_state.Process==PROC_CURL_SENT) && (sys_state.MenuState!=STATE_PROCESS))
      {
         sys_state.MenuState=STATE_PROCESS;
         svcSignalEvent(threadRequest_curl);
      }




      if ((sys_state.MenuState==STATE_PROCESS) && (kDown & KEY_Y))
	  {
         sys_state.MenuState=STATE_IDLE;
		 sys_state.Process=PROC_CANCEL;
	  }

      if ((options.auto_response) && (kDown & KEY_B))
	  {
         options.auto_response=false;
	  }

      if ((playBuf.status != NDSP_WBUF_DONE) && (kDown & KEY_A))
         ndspChnWaveBufClear(0);



      if(mic_recording)
         MIC_record_frame();

      if (sys_state.MenuState==STATE_INIT)
      {
#ifdef DEBUG_ENABLE
         sys_state.MenuState=STATE_IDLE;
#else
         if ((JSON_read_api_key()) != 0)
            sys_state.MenuState=STATE_GET_API_KEY;
         else
            sys_state.MenuState=STATE_MAIN_CHAT;
#endif
      }
      gfx_frame();
   }
   
   

   sys_exit();

   return 0;
}
