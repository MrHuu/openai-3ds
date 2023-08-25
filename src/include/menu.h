#ifndef _MENU_H
#define _MENU_H

#include <3ds.h>
#include "gfx.h"

#define MAX_MENUS              10
#define MAX_MENU_FUNCTIONS      1
#define MAX_MENU_BUTTONS       10
#define MAX_MENU_SPRITES       10
#define MAX_MENU_TEXTS         10
#define MAX_MENU_TEXT_LENGTH   20

extern enum { MAIN,
              MAIN_CHAT,
              OPTIONS,
			  OPTIONS_API,
			  DEBUG_01,
			  DEBUG_02,
			  DEBUG_MIC,
			  CHAT_SELECT,
			  GET_API_KEY,
} menu_enum;

typedef enum {
   TYPE_NULL,
   TYPE_INT,
   TYPE_FLOAT,
   TYPE_STRING, // why tho?
   TYPE_BOOL
} DataType;

typedef struct {
   float x;
   float y;
   float size;
   Color color;
   char* text;
   DataType format;
   void* data;
   char* (*get_value_string)(void*);
} MenuText;

typedef struct {
   float x;
   float y;
   float width;
   float height;
   float scale;
   char* text;
   void (*callback)(void* data);
   void* data;
} MenuButton;

typedef struct {
   float x;
   float y;
   float width;
   float height;
   float scale;
   void* data;
} MenuSprite;

typedef struct {
   bool run_once;
   void (*callback)(void* data);
   void* data;
} MenuFunction;

typedef struct {
   char* name; // description ?
   int menu_id;
   MenuText* texts;
   int num_texts;
   MenuButton* buttons;
   int num_buttons;
   MenuSprite* sprites;
   int num_sprites;
   MenuFunction* functions;
   int num_functions;
} Menu;

extern Menu menus[MAX_MENUS];
extern int num_menus;


void menu_init(void);
void menu_exit(void);
Menu* add_menu(char* name);
void delete_menu(char* name);
Menu* get_menu(char* name);

void add_function(Menu* menu, MenuFunction function);
void add_button(Menu* menu, MenuButton button);
void add_sprite(Menu* menu, MenuSprite sprite);
void add_text(Menu* menu, MenuText text);
char* update_menu_texts(char* menu_name, int id);


#endif /* _MENU_H */
