#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

//#include "sfx.h"
#include "menu.h"
#include "main.h"
#include "sys.h"
#include "json.h"

#include "images.h"

#include <citro2d.h>

Menu menus[MAX_MENUS];
int num_menus;


void set_menu_state(void* data)
{
   sys_state.MenuState = (int)data;
}

void set_menu_toggle(void* data)
{
   bool* value = (bool*)data;
   *value = !(*value);
}

void add_int_value(void* data)
{
   int* value = (int*)data;
   *value = (*value+1);
}

void set_chat_select_return(void* data)
{
   sys_state.MenuState = (int)data;
   selected_chat = 0;
}




void set_menu_voice(void* data)
{
   int* value = (int*)data;
   *value = (*value+1);
#ifdef SFX_ALL_VOICES
   if (*value>5) *value=0;
#else
   if (*value>2) *value=0;
#endif
}




void set_api_tokens_add(void* data)
{
   int* value = (int*)data;
   *value = (*value+16);
   if (*value>4096) *value=16;
}

void set_api_tokens_sub(void* data)
{
   int* value = (int*)data;
   *value = (*value-16);
   if (*value<=15) *value=4096;
}


void set_api_temp_add(void* data)
{
   float* value = (float*)data;
   *value = (*value+0.1f);
   if (*value>2.0f) *value=0.1f;
}

void set_api_temp_sub(void* data)
{
   float* value = (float*)data;
   *value = (*value-0.1f);
   if (*value<=0.0f) *value=2.0f;
}


void queue_clear(void* data)
{
//flite_queue_clear(&flite_queue);
}



void sentence_event(void* data)
{
//LightEvent_Signal(&addSentenceEvent);
}

void playback_event(void* data)
{
//LightEvent_Signal(&startPlaybackEvent);
}


static char name_buf[16];

void set_api_model(void* data)
{
   int* value = (int*)data;
   *value = (*value+1);
   if (*value>4) *value=0;
   
   snprintf(name_buf, 16, "%s", api_info[api_options.model].api_name);
}


void get_api_key(void* data)
{
int ret;
ret = sys_swkbd("Enter API-Key","",api_key);

   if (ret == 1)
   {
	   // TODO test for valid key before continue..
      JSON_write_api_key(); // write api-key to file
	  sys_state.MenuState=STATE_MAIN_CHAT;
   }
}



char selected_obj_type[16];
int selected_obj_id;
int selected_obj;

// TODO finish this please...
void func_chat_select(void* data)
{
 
selected_obj      = selected_chat;
selected_obj_id   = buffer->objects[selected_chat]->obj_blck_id;


switch (buffer->objects[selected_chat]->type)
{
   case OBJ_TYPE_TEXT:

	  snprintf(selected_obj_type, 16, "Text");
      break;
   case OBJ_TYPE_IMAGE:

	  snprintf(selected_obj_type, 16, "Image");
      break;
   default:

	  snprintf(selected_obj_type, 16, "None");
}
}



void menu_init(void)
{
   memset(menus, 0, sizeof(menus));
   num_menus = 0;

   Menu* menu;

   snprintf(name_buf, 16, "%s", api_info[api_options.model].api_name); // init this somewhere or somehow else...


   menu = add_menu("main");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   { 100.0f,  10.0f,  0.75f, {255, 255, 255, 255}, "Main", TYPE_NULL, NULL});

      add_button(menu, (MenuButton) {  70.0f,  50.0f, 180.0f,  50.0f, 0.7f, "Playground", &set_menu_state, (void*)STATE_MAIN_CHAT});
      add_button(menu, (MenuButton) {  70.0f, 110.0f, 180.0f,  50.0f, 0.7f, "Debug",      &set_menu_state, (void*)STATE_DEBUG_01});
      add_button(menu, (MenuButton) {  70.0f, 170.0f, 180.0f,  50.0f, 0.7f, "Quit",       &sys_exit, NULL});
   }

   menu = add_menu("playground");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   { 108.0f,  10.0f,  0.75f, {255, 255, 255, 255}, "OpenAI-3DS", TYPE_NULL, NULL});

      add_button(menu, (MenuButton) {  70.0f,  50.0f, 180.0f,  50.0f, 0.7f, "Keyboard",  &set_menu_state, (void*)STATE_CHAT});
	  add_button(menu, (MenuButton) {  70.0f, 110.0f, 180.0f,  50.0f, 0.7f, "Image",     &set_menu_state, (void*)STATE_IMAGE_GEN});
      add_button(menu, (MenuButton) {  70.0f, 170.0f, 180.0f,  50.0f, 0.7f, "Options",   &set_menu_state, (void*)STATE_OPTIONS});
#ifdef DEBUG_ENABLE
	  add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "",         &set_menu_state, (void*)STATE_IDLE});
#endif
      add_text  (menu, (MenuText)   {  15.0f,  40.0f,  0.60f, {255, 255, 255, 255}, " rec", TYPE_NULL, NULL});
   }

   menu = add_menu("options");
   if (!(menu == NULL))
   {
      add_button(menu, (MenuButton) {  70.0f,  40.0f, 180.0f,  30.0f, 0.3f, "Voice toggle", &set_menu_toggle, (void*)&options.voice_enable});

      add_text  (menu, (MenuText)   {  95.0f,  85.0f,  0.65f, {255, 255, 255, 255}, "Voice request: %s", TYPE_INT, &options.voice_request});
      add_button(menu, (MenuButton) {  70.0f,  80.0f, 180.0f,  30.0f, 0.3f, " ", &set_menu_voice, (void*)&options.voice_request});

      add_text  (menu, (MenuText)   {  90.0f, 125.0f,  0.65f, {255, 255, 255, 255},"Voice response: %s", TYPE_INT, &options.voice_response});
      add_button(menu, (MenuButton) {  70.0f, 120.0f, 180.0f,  30.0f, 0.3f, " ", &set_menu_voice, (void*)&options.voice_response});

      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "", &set_menu_state,  (void*)STATE_MAIN_CHAT});
      add_button(menu, (MenuButton) { 250.0f, 200.0f,  30.0f,  30.0f, 0.3f, "<", &set_menu_state,  (void*)STATE_OPTIONS_API});
   }

   menu = add_menu("options_api");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   {  90.0f,  45.0f,  0.65f, {255, 255, 255, 255},"Model: %s", TYPE_STRING, &name_buf});
      add_button(menu, (MenuButton) {  70.0f,  40.0f, 180.0f,  30.0f, 0.3f, " ",&set_api_model, (void*)&api_options.model});

      add_text  (menu, (MenuText)   {  90.0f,  85.0f,  0.65f, {255, 255, 255, 255},"Stream: %s", TYPE_BOOL, &api_options.stream});
      add_button(menu, (MenuButton) {  70.0f,  80.0f, 180.0f,  30.0f, 0.3f, " ",&set_menu_toggle, (void*)&api_options.stream});

      add_text  (menu, (MenuText)   { 105.0f, 125.0f,  0.65f, {255, 255, 255, 255},"Tokens = %s", TYPE_INT, &api_options.max_tokens});
      add_button(menu, (MenuButton) {  70.0f, 120.0f,  30.0f,  30.0f, 0.3f, "-",&set_api_tokens_sub, (void*)&api_options.max_tokens});
      add_button(menu, (MenuButton) { 220.0f, 120.0f,  30.0f,  30.0f, 0.3f, "+",&set_api_tokens_add, (void*)&api_options.max_tokens});

      add_text  (menu, (MenuText)   { 115.0f, 165.0f,  0.65f, {255, 255, 255, 255},"Temp = %s", TYPE_FLOAT,  &api_options.temperature});
      add_button(menu, (MenuButton) {  70.0f, 160.0f,  30.0f,  30.0f, 0.3f, "-",&set_api_temp_sub, (void*)&api_options.temperature});
      add_button(menu, (MenuButton) { 220.0f, 160.0f,  30.0f,  30.0f, 0.3f, "+",&set_api_temp_add, (void*)&api_options.temperature});

      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "",&set_menu_state, (void*)STATE_MAIN_CHAT});
      add_button(menu, (MenuButton) { 250.0f, 200.0f,  30.0f,  30.0f, 0.3f, ">",&set_menu_state, (void*)STATE_OPTIONS});
   }

   menu = add_menu("debug_01");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   {  90.0f,  45.0f,  0.65f, {255, 255, 255, 255}, "Debug: %s", TYPE_BOOL, &options.debug_enable});
      add_button(menu, (MenuButton) {  70.0f,  40.0f, 180.0f,  30.0f, 0.3f, " ", &set_menu_toggle, (void*)&options.debug_enable});

      add_text  (menu, (MenuText)   {  90.0f,  85.0f,  0.65f, {255, 255, 255, 255}, "Border: %s", TYPE_BOOL, &options.border_enable});
      add_button(menu, (MenuButton) {  70.0f,  80.0f, 180.0f,  30.0f, 0.3f, " ", &set_menu_toggle, (void*)&options.border_enable});

      add_button(menu, (MenuButton) {  70.0f, 120.0f, 180.0f,  30.0f, 0.2f, "Dump API-Key", (void*)&json_write_test, NULL});
//      add_button(menu, (MenuButton) {  70.0f, 160.0f, 180.0f,  30.0f, 0.2f, "Read API-Key", (void*)&json_read_test,  NULL});

add_button(menu, (MenuButton) {  70.0f, 160.0f, 180.0f,  30.0f, 0.2f, "chat select", &set_menu_state, (void*)STATE_CHAT_SELECT});

      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "",&set_menu_state, (void*)STATE_IDLE});
      add_button(menu, (MenuButton) { 250.0f, 200.0f,  30.0f,  30.0f, 0.3f, "<",&set_menu_state, (void*)STATE_DEBUG_02});
   }


   menu = add_menu("debug_02");
   if (!(menu == NULL))
   {
      add_text(menu, (MenuText) { 10.0f,   10.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,   25.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,   40.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,   55.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,   70.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,   85.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,  100.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});
      add_text(menu, (MenuText) { 10.0f,  115.0f,  0.75f, {255, 255, 255, 255}, "\n", TYPE_NULL, NULL});

// Create a seperate sprite struct or something.. this messes with the menu.. idiot
//      add_sprite(menu, (MenuSprite) {  160.0f, 160.0f, 180.0f,  30.0f, 0.5f, (void*)images_logo_idx});

      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, ">",&set_menu_state, (void*)STATE_DEBUG_01});
      add_button(menu, (MenuButton) { 250.0f, 200.0f,  30.0f,  30.0f, 0.3f, "<",&set_menu_state, (void*)STATE_DEBUG_MIC});
   }


   menu = add_menu("debug_mic");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   { 108.0f,  10.0f,  0.75f, {255, 255, 255, 255}, "Debug mic", TYPE_NULL, NULL});


      add_text  (menu, (MenuText)   {  15.0f,  40.0f,  0.60f, {255, 255, 255, 255}, " rec",           TYPE_NULL, NULL});
      add_text  (menu, (MenuText)   {  15.0f,  60.0f,  0.60f, {255, 255, 255, 255}, " play mic",      TYPE_NULL, NULL});
      add_text  (menu, (MenuText)   {  15.0f,  80.0f,  0.60f, {255, 255, 255, 255}, " process",       TYPE_NULL, NULL});
      add_text  (menu, (MenuText)   {  15.0f, 100.0f,  0.60f, {255, 255, 255, 255}, "",               TYPE_NULL, NULL});
      add_text  (menu, (MenuText)   {  15.0f, 120.0f,  0.60f, {255, 255, 255, 255}, " play wav buf",  TYPE_NULL, NULL});
      add_text  (menu, (MenuText)   {  15.0f, 140.0f,  0.60f, {255, 255, 255, 255}, " play wav file", TYPE_NULL, NULL});



      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "", &set_menu_state,  (void*)STATE_DEBUG_02});
   }

   menu = add_menu("chat_select");
   if (!(menu == NULL))
   {
      add_function (menu, (MenuFunction)   { false, &func_chat_select, NULL});

      add_text  (menu, (MenuText)   { 108.0f,  10.0f,  0.75f, {255, 255, 255, 255}, "Select ", TYPE_NULL, NULL});

      add_text  (menu, (MenuText)   {  95.0f,  85.0f,  0.65f, {255, 255, 255, 255}, "obj_blck_id: %s", TYPE_INT, &selected_obj_id});
      add_text  (menu, (MenuText)   {  90.0f, 125.0f,  0.65f, {255, 255, 255, 255}, "Obj type: %s", TYPE_STRING, &selected_obj_type});
      add_text  (menu, (MenuText)   {  90.0f, 165.0f,  0.65f, {255, 255, 255, 255}, "Obj slct: %s", TYPE_INT, &selected_obj});

      add_button(menu, (MenuButton) { 285.0f, 200.0f,  30.0f,  30.0f, 0.3f, "", &set_chat_select_return, (void*)STATE_DEBUG_01});
      add_text  (menu, (MenuText)   {  15.0f,  60.0f,  0.60f, {255, 255, 255, 255}, " Back", TYPE_NULL, NULL});
   }


   menu = add_menu("api_key");
   if (!(menu == NULL))
   {
      add_text  (menu, (MenuText)   {  95.0f,  10.0f,  0.75f, {255, 255, 255, 255}, "Setup API-Key", TYPE_NULL, NULL});

      add_button(menu, (MenuButton) {  70.0f, 110.0f, 180.0f,  50.0f, 0.7f, "Enter key", &get_api_key, NULL});
      add_button(menu, (MenuButton) {  70.0f, 170.0f, 180.0f,  50.0f, 0.7f, "Quit",      &sys_exit, NULL});

   }


}

void menu_exit(void)
{
   for (int i = 0; i < num_menus; i++) {
      free(menus[i].name);
      free(menus[i].buttons);
      free(menus[i].sprites);
      free(menus[i].texts);
free(menus[i].functions);
   }
   num_menus = 0;
}

Menu* add_menu(char* name)
{
   if (num_menus >= MAX_MENUS) {
	   sys_error(false, "Error: Maximum number of menus reached\n");
//      printf("Error: Maximum number of menus reached\n"); // USE SYS_ERROR HERE!!
      return NULL;
   }

   for (int i = 0; i < num_menus; i++) {
      if (strcmp(menus[i].name, name) == 0) {
         printf("Error: Menu with name %s already exists\n", name); // USE SYS_ERROR HERE!!
         return NULL;
      }
   }

   Menu* menu = &menus[num_menus++];
   menu->name = strdup(name);
   menu->buttons = calloc(MAX_MENU_BUTTONS, sizeof(MenuButton));
   menu->texts = calloc(MAX_MENU_TEXTS, sizeof(MenuText));
   menu->sprites = calloc(MAX_MENU_SPRITES, sizeof(MenuSprite));
menu->functions = calloc(MAX_MENU_FUNCTIONS, sizeof(MenuFunction));
   return menu;
}

Menu* get_menu(char* name)
{
    for (int i = 0; i < num_menus; i++) {
        if (strcmp(menus[i].name, name) == 0) {
            return &menus[i];
        }
    }

    return NULL;
}

void delete_menu(char* name)
{
   int index = -1;
   for (int i = 0; i < num_menus; i++) {
      if (strcmp(menus[i].name, name) == 0) {
         index = i;
         break;
      }
   }

   if (index == -1) {
      printf("Error: Menu with name %s not found\n", name); // USE SYS_ERROR HERE!!
      return;
   }

   free(menus[index].name);
   free(menus[index].buttons);
   free(menus[index].texts);

   num_menus--;
   for (int i = index; i < num_menus; i++)
      menus[i] = menus[i+1];
}

void add_button(Menu* menu, MenuButton button)
{
//   Menu* menu = get_menu(menu_name);
    if (menu == NULL) {
//        printf("Error: Menu with name %s not found\n", menu_name); // USE SYS_ERROR HERE!!
        return;
    }

    if (menu->num_buttons >= MAX_MENU_BUTTONS) {
//        printf("Error: Maximum number of buttons reached for menu %s\n", menu_name);  // USE SYS_ERROR HERE!!
        return;
    }

    menu->buttons[menu->num_buttons++] = button;
}

void add_sprite(Menu* menu, MenuSprite sprite)
{
//   Menu* menu = get_menu(menu_name);
    if (menu == NULL) {
//        printf("Error: Menu with name %s not found\n", menu_name); // USE SYS_ERROR HERE!!
        return;
    }

    if (menu->num_sprites >= MAX_MENU_SPRITES) {
//        printf("Error: Maximum number of buttons reached for menu %s\n", menu_name);  // USE SYS_ERROR HERE!!
        return;
    }

    menu->sprites[menu->num_sprites++] = sprite;
}

void add_function(Menu* menu, MenuFunction function)
{
//   Menu* menu = get_menu(menu_name);
    if (menu == NULL) {
//        printf("Error: Menu with name %s not found\n", menu_name); // USE SYS_ERROR HERE!!
        return;
    }

    if (menu->num_sprites >= MAX_MENU_FUNCTIONS) {
//        printf("Error: Maximum number of buttons reached for menu %s\n", menu_name);  // USE SYS_ERROR HERE!!
        return;
    }

    menu->functions[menu->num_functions++] = function;
}






char* int_to_string(void* data)
{
   static char buf[12];
   snprintf(buf, 12, "%d", *(int*)data);
   return buf;
}

char* bool_to_string(void* data)
{
   return *(bool*)data ? "Enabled" : "Disabled";
}

char* float_to_string(void* data)
{
   static char buf[12];
   snprintf(buf, 12, "%.1f", *(float*)data);
   return buf;
}

char* string_to_string(void* data)
{
   static char buf[16];
   snprintf(buf, 16, "%s", (char*)data);

   return buf;
}


void add_text(Menu* menu, MenuText text)
{
//   Menu* menu = get_menu(menu_name);
   if (menu == NULL) {
      printf("Error: Menu not found\n"); // USE SYS_ERROR HERE!!
      return;
   }
   if (menu->num_texts >= MAX_MENU_TEXTS) {
      printf("Error: Maximum number of texts reached for menu %s\n", menu->name); // USE SYS_ERROR HERE!!
      return;
   }

   text.get_value_string = NULL;

   switch(text.format)
   {
      case TYPE_INT:
         text.get_value_string = &int_to_string;
         break;

      case TYPE_BOOL:
         text.get_value_string = &bool_to_string;
         break;

      case TYPE_FLOAT:
         text.get_value_string = &float_to_string;
         break;

      case TYPE_STRING:
         text.get_value_string = &string_to_string;
         break;

      default:
         break;
   }

   menu->texts = realloc(menu->texts, (menu->num_texts + 1) * sizeof(MenuText));
   menu->texts[menu->num_texts++] = text;
}



char* update_menu_texts(char* menu_name, int id)
{
   Menu* menu = get_menu(menu_name);
   if (menu == NULL) {
      printf("Error: Menu with name %s not found\n", menu_name); // USE SYS_ERROR HERE!!
      return "error";
   }

   MenuText* text = &menu->texts[id];
	
   if (text->get_value_string == NULL)
      return text->text;
	
   static char buffer[256];
   sprintf(buffer, text->text, text->get_value_string(text->data));

   return buffer;

}






