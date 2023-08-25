#ifndef _GFX_H
#define _GFX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <citro2d.h>
#include <assert.h>

#include "images.h"


#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define MAX_SPRITES    20 // TODO chack sane limits
#define MAX_GFX_TEXT   10 // TODO chack sane limits
#define MAX_BUTTONS    10 // TODO chack sane limits

#define MAX_BUFFER_SIZE 200 // TODO chack sane limits chack wtf?

extern Thread threadHandle_gfx;

typedef struct {
   u8 r;
   u8 g;
   u8 b;
   u8 a;
} Color;

typedef struct {
	C2D_Sprite spr;
	float dx, dy;
} Sprite;

extern Sprite sprites[MAX_SPRITES];

typedef struct {
   int   id;
   char *text;
   float x;
   float y;
   float scale;
   float width;
   float height;
   Color color;
} gfx_text_t;

extern gfx_text_t gfx_text_button[MAX_GFX_TEXT];
extern int gfx_text_button_count;
extern int next_gfx_text_button_id;

extern gfx_text_t gfx_text_menu[MAX_GFX_TEXT];
extern int gfx_text_menu_count;
extern int next_gfx_text_menu_id;

extern gfx_text_t gfx_chat_text[MAX_GFX_TEXT];
extern int gfx_chat_text_count;
extern int next_gfx_chat_text_id;


typedef struct {
    int id;
    char *text;
	float x;
	float y;
	float width;
	float height;
	float scale;
} buttons_t;

extern buttons_t buttons[MAX_BUTTONS];


void gfx_text_add(gfx_text_t *gfx_text, int *gfx_text_count, int *next_gfx_text_id,
      const char *text, float x, float y, float size, Color* color);

void gfx_text_clear(gfx_text_t *gfx_text, int *gfx_text_count, int *next_gfx_text_id);
void gfx_chat_text_clear(gfx_text_t *gfx_chat_text, int *gfx_chat_text_count, int *next_gfx_chat_text_id);

void gfx_frame(void);
void gfx_init(void);
void gfx_exit(void);


typedef struct {
    char **buffers;     // Array of buffers
    int numBuffers;     // Number of buffers in the struct
    int totalWords;     // Total number of words
    int totalChars;     // Total number of characters
	int currBuffer;     // 
	int spltBuffer;     // 
    int shouldSplt;
} BufferStruct;

extern BufferStruct bufferStruct;



typedef enum {
   OBJ_TYPE_TEXT,
   OBJ_TYPE_IMAGE,
    // Add other object types here
} obj_type_t;

typedef struct {
   obj_type_t type;
   void* object;
   float obj_width;
   float obj_height;
   float obj_pos_y;
   float obj_pos_x;
   int obj_buffers;
   int obj_user;
   int obj_blck_id;
   void (*free_object)(void*); // Pointer to a function to free the object
} buffer_object_t;

typedef struct {
   buffer_object_t** objects;
//   int start;
//   int end;
//    int size;

   int total_objects;

   float offset;
   float total_height;
   int total_obj_block;
   int current_obj; // not required?
} buffer_t;
//buffer_t* buffer;

extern buffer_t* buffer; // should this be extern? rename to gfx_obj_buffer or smimmilar







void download_image_to_buff(const char* url);
void add_text_to_buff(int blck_id, const char* text, int user);
void update_object_in_buffer(const char* text);
void add_image_to_buff();

#endif /* _GFX_H */










