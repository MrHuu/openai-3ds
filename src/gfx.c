#include "gfx.h"
#include "main.h"

#include "menu.h"
#include "sys.h"
#include "util.h"
#include "mic.h"
#include "image.h"

#define STACKSIZE (4 * 1024)

#define INITIAL_BUFFER_SIZE 512


#define COLOR_WHITE       C2D_Color32(255,255,255,255)
#define COLOR_RED         C2D_Color32(255,  0,  0,255)
#define COLOR_GREEN       C2D_Color32(  0,255,  0,255)
#define COLOR_BLUE        C2D_Color32(  0,  0,255,255)
#define COLOR_BLACK       C2D_Color32(  0,  0,  0,255)

#define COLOR_GRAY        C2D_Color32( 64, 64, 64,255)
#define COLOR_LIGHT_GRAY  C2D_Color32(128,128,128,255)

#define COLOR_GPT_GREEN   C2D_Color32( 18,163,130,255)

Thread threadHandle_gfx;
Handle threadRequest_gfx;

volatile bool runGfxThread = true;

static C3D_RenderTarget* top;
static C3D_RenderTarget* bottom;


static C2D_SpriteSheet spriteSheet;

Sprite sprites[MAX_SPRITES];

C2D_TextBuf g_chatBuf, g_textBuf, g_objBuf;

gfx_text_t gfx_text_button[MAX_GFX_TEXT];
int gfx_text_button_count;
int next_gfx_text_button_id;

gfx_text_t gfx_text_menu[MAX_GFX_TEXT];
int gfx_text_menu_count;
int next_gfx_text_menu_id;

gfx_text_t gfx_chat_text[MAX_GFX_TEXT];
int gfx_chat_text_count;
int next_gfx_chat_text_id;

float chat_text_offset = 35.0f; // for scrolling :)

buttons_t buttons[MAX_BUTTONS];
bool bufferlock = false;
bool auto_scroll = true;






//int max_obj_block = 0; // defined in curl.h









char *users[] = {"System", "User", "AI"};



buffer_t* buffer;

buffer_t* create_buffer()
{
   buffer_t* buffer = (buffer_t*)malloc(sizeof(buffer_t));
   buffer->objects = (buffer_object_t**)malloc(MAX_BUFFER_SIZE * sizeof(buffer_object_t*));
//   buffer->start = 0;
//   buffer->end = -1;
//    buffer->size = 0;
   buffer->total_objects = 0;
   buffer->offset = 0.0f; // doet niks, zaadje
   buffer->total_height = 0.0f;
   buffer->total_obj_block = 0;
   return buffer;
}


void free_text_object(void* object)
{
   gfx_text_t* text_object = (gfx_text_t*)object;
    // Free any dynamically allocated memory in the text object
   free(text_object->text);
//	C2D_TextDelete(text_object->txt.buf);

//C2D_TextBufDelete(text_object->g_objBuf);
//    free(text_object);
}

void free_image_object(void* object)
{
   gfx_image_t* image_object = (gfx_image_t*)object;
    // Free any dynamically allocated memory in the image object
//    free(image_object->url);
//    free(image_object->image_data); FIX THIS!!
//    free(image_object->request_text);
   free(image_object);
}


void destroy_buffer(buffer_t* buffer)
{
   if (buffer == NULL)
      return;
    
   for (int i = 0; i < buffer->total_objects; i++) {
      if (buffer->objects[i] != NULL) {
         buffer->objects[i]->free_object(buffer->objects[i]->object);
         free(buffer->objects[i]);
      }
   }
    
   free(buffer->objects);
   free(buffer);
}


void clear_buffer(buffer_t* buffer) // obsolete? or implement properly..
{
   while (buffer->total_objects > 0) {
      buffer_object_t* object = buffer->objects[0];
      buffer->total_height -= object->obj_height;

      object->free_object(object->object);
      free(object);
   }
   buffer->total_objects = -1;
}


void clear_buffer_object(buffer_t* buffer)
{
   buffer_object_t* object = buffer->objects[(buffer->total_objects)-1];
   buffer->total_height -= object->obj_height;

   object->free_object(object->object);
   free(object);

   buffer->total_objects--;
}





void add_object_to_buffer(buffer_t* buffer, obj_type_t type, void* object, float obj_width, float obj_height)
{


	
   buffer_object_t* buffer_object = (buffer_object_t*)malloc(sizeof(buffer_object_t));
   buffer_object->type            = type;
   buffer_object->object          = object;
   buffer_object->obj_width       = obj_width;



// ---------------------------------------------------------- WIP !!!!!
//max_obj_block++;
//buffer_object->obj_block_id = max_obj_block;
// -----------------------------------------------------

   switch (type)
   {
      case OBJ_TYPE_TEXT:
         buffer_object->free_object = free_text_object;
         break;
      case OBJ_TYPE_IMAGE:
         buffer_object->obj_height  = obj_height;
		 buffer_object->obj_buffers = 0;
         buffer_object->free_object = free_image_object;
         break;
   }

   if (buffer->total_objects == MAX_BUFFER_SIZE) {

      buffer->total_objects--;
      buffer_object_t* oldest_object = buffer->objects[0];
      buffer->total_height -= oldest_object->obj_height;
        
      oldest_object->free_object(oldest_object->object);
      free(oldest_object);
        

      for (int i = 0; i < buffer->total_objects; i++) {
         buffer->objects[i] = buffer->objects[i+1];
      }
   }


   buffer->objects[buffer->total_objects] = buffer_object;
   buffer->total_height += obj_height;

   buffer->total_objects++;


   float obj_pos_y = 0.0f;
   for (int i = 0; i < buffer->total_objects; i++) {
      buffer->objects[i]->obj_pos_y = obj_pos_y;
      obj_pos_y += buffer->objects[i]->obj_height;
   }



}





gfx_text_t* create_text_object(int id, const char* text, float x, float y, float scale, float height)
{
   gfx_text_t* text_object = (gfx_text_t*)malloc(sizeof(gfx_text_t));

   text_object->text = (char*)malloc(strlen(text) + 1);

   if (text_object->text == NULL)
      sys_error(1,"malloc failed!\n");

   strcpy(text_object->text, text);

   text_object->id = id;
   text_object->x = x;
   text_object->y = y;
   text_object->scale = scale;

   return text_object;
}


gfx_image_t* create_image_object(int id, char* url, C2D_Image image_data, char* request_text) {
    gfx_image_t* image_object = (gfx_image_t*)malloc(sizeof(gfx_image_t));
    image_object->id = id;
    image_object->url = url;
//    image_object->image_data = image_data;
    image_object->request_text = request_text;

    return image_object;
}






void add_text_to_buff(int blck_id, const char* text, int user)
{
   gfx_text_t* new_text_object = create_text_object(1, text, 10.0f, 20.0f, 0.5f, 15.0f);

   float scaleX   = 0.5f;
   float scaleY   = 0.5f;
   float outWidth = 0.0f;
   float outHeight= 0.0f;
   
   C2D_Text txt;
   C2D_TextBufClear(g_objBuf);

   C2D_TextParse(&txt, g_objBuf, new_text_object->text);
   C2D_LayoutText(&txt, C2D_AlignLeft | C2D_WordWrap, scaleX, scaleY, 350.0f);
   C2D_TextGetDimensions(&txt, scaleX, scaleY,&outWidth,&outHeight);

   new_text_object->height = outHeight;
   new_text_object->width  = outWidth;

   add_object_to_buffer(buffer, OBJ_TYPE_TEXT, new_text_object, new_text_object->width, new_text_object->height);

   buffer_object_t* buffer_object = buffer->objects[(buffer->total_objects)-1];
   buffer_object->obj_width   = outWidth;
   buffer_object->obj_height  = outHeight;
   buffer_object->obj_buffers = 0;
   buffer_object->obj_user    = user;

   if (blck_id==0)
   {
      buffer->total_obj_block++;
      buffer_object->obj_blck_id = buffer->total_obj_block;
   }
   else
      buffer_object->obj_blck_id = blck_id;
}










void update_object_in_buffer(const char* text)
{
   buffer_object_t* buffer_object = buffer->objects[buffer->total_objects-1];
   gfx_text_t* text_object        = (gfx_text_t*)buffer_object->object;

   if (bufferStruct.numBuffers > 1)
   {
      UTIL_updateString(&text_object->text, bufferStruct.buffers[bufferStruct.spltBuffer]);

      for (int i = ((bufferStruct.spltBuffer)+1); i < (bufferStruct.numBuffers)-1; i++)
         UTIL_appendString(&text_object->text, bufferStruct.buffers[i]);
   }
   else
      UTIL_updateString(&text_object->text, text);

   float scaleX   = 0.5f;
   float scaleY   = 0.5f;
   float outWidth = 0.0f;
   float outHeight= 0.0f;

   C2D_Text txt;
   C2D_TextBufClear(g_objBuf);

   C2D_TextParse(&txt, g_objBuf, text_object->text);
   C2D_LayoutText(&txt, C2D_AlignLeft | C2D_WordWrap, scaleX, scaleY, 350.0f);
   C2D_TextGetDimensions(&txt, scaleX, scaleY, &outWidth, &outHeight);
   buffer_object->obj_width  = outWidth;
   buffer_object->obj_height = outHeight;

   if (bufferStruct.spltBuffer > 0)
      buffer_object->obj_buffers = bufferStruct.currBuffer;
}



void add_image_to_buff(const char* filename)
{
   gfx_image_t *image = (gfx_image_t *)malloc(sizeof(gfx_image_t));
   image->request_text = "Image Request";
   image->url = "https://notrequired.com/image.png";
   image->id = 1; //remove?

   IMAGE_loadImageFromFile(filename, image);

   add_object_to_buffer(buffer, OBJ_TYPE_IMAGE, image, (float)image->width, (float)image->height);

buffer_object_t* buffer_object = buffer->objects[(buffer->total_objects)-1];
      buffer->total_obj_block++;
      buffer_object->obj_blck_id = buffer->total_obj_block;
	  

}


void download_image_to_buff(const char* url)
{
   gfx_image_t *image = (gfx_image_t *)malloc(sizeof(gfx_image_t));
   image->request_text = "Image Request";
//   image->url = "https://raw.githubusercontent.com/MrHuu/openbor-3ds/3DS/engine/resources/ctr/OpenBOR_Icon_48x48.png";
   image->url = (char*)url;
   image->id = 1; //remove?

   IMAGE_load_image_from_url(image->url, image);

   add_object_to_buffer(buffer, OBJ_TYPE_IMAGE, image, (float)image->width, (float)image->height);


buffer_object_t* buffer_object = buffer->objects[(buffer->total_objects)-1];
      buffer->total_obj_block++;
      buffer_object->obj_blck_id = buffer->total_obj_block;

}





























void sprite_init()
{
   size_t numImages = C2D_SpriteSheetCount(spriteSheet);

   for (size_t i = 0; i < numImages; i++)
   {
      Sprite* sprite = &sprites[i];

      C2D_SpriteFromSheet(&sprite->spr, spriteSheet, i);
	  
	  if (i == images_border_small_idx )
	  {
		  C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
		  C2D_SpriteSetScale(&sprite->spr, 2.0f, 2.0f);
		  C2D_SpriteSetPos(&sprite->spr, 200.0f, 120.0f);
		  C2D_SpriteSetDepth(&sprite->spr, 1.0f);
	  }
	  else if (i == images_logo_idx )
	  {
         C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
         C2D_SpriteSetPos(&sprite->spr, 160.0f, 120.0f);
	  }
   }
   





}

void sprite_move(size_t spriteId)
{
/*
	for (size_t i = 0; i < numSprites; i++)
	{
		Sprite* sprite = &sprites[i];
		C2D_SpriteMove(&sprite->spr, sprite->dx, sprite->dy);
		C2D_SpriteRotateDegrees(&sprite->spr, 1.0f);

		// Check for collision with the screen boundaries
		if ((sprite->spr.params.pos.x < sprite->spr.params.pos.w / 2.0f && sprite->dx < 0.0f) ||
			(sprite->spr.params.pos.x > (SCREEN_WIDTH-(sprite->spr.params.pos.w / 2.0f)) && sprite->dx > 0.0f))
			sprite->dx = -sprite->dx;

		if ((sprite->spr.params.pos.y < sprite->spr.params.pos.h / 2.0f && sprite->dy < 0.0f) ||
			(sprite->spr.params.pos.y > (SCREEN_HEIGHT-(sprite->spr.params.pos.h / 2.0f)) && sprite->dy > 0.0f))
			sprite->dy = -sprite->dy;
	}
*/
}

void sprite_rotate(size_t spriteId)
{
   Sprite* sprite = &sprites[spriteId];
   C2D_SpriteRotateDegrees(&sprite->spr, 1.0f);
}


void gfx_text_add(gfx_text_t *gfx_text, int *gfx_text_count, int *next_gfx_text_id, 
      const char *text, float x, float y, float scale_ptr, Color* color)
{
   if ((*gfx_text_count + 1) > MAX_GFX_TEXT)
   {
      sys_error(false, "MAX_TEXT_REACHED");
      return;
   }
   *gfx_text_count = *gfx_text_count + 1;

   for (int i = ((*gfx_text_count < MAX_GFX_TEXT)? *gfx_text_count:MAX_GFX_TEXT); i > 0; i--)
   {
      gfx_text[i] = gfx_text[i-1];
   }

   gfx_text[0].text = (char*) malloc(strlen(text) + 1);

   if (gfx_text[0].text == NULL)
   {
      sys_error(1,"malloc failed!\n");
   }

   strcpy(gfx_text[0].text, text);

   gfx_text[0].x = x;
   gfx_text[0].y = y;
   gfx_text[0].scale = scale_ptr;

   Color clr = (Color)*color;

   gfx_text[0].color = clr;

   gfx_text[0].id = *next_gfx_text_id;
   *next_gfx_text_id = *next_gfx_text_id + 1;
}



void gfx_text_clear(gfx_text_t *gfx_text, int *gfx_text_count, int *next_gfx_text_id)
{
   if(!gfx_text)
      return;

   for (int i = 0; i < *gfx_text_count; i++)
      free(gfx_text[i].text);

   *gfx_text_count   = 0;
   *next_gfx_text_id = 1;
}

void text_sceneRender_button(gfx_text_t *gfx_text, int *gfx_text_count)
{
   C2D_TextBufClear(g_textBuf);

   for (int i = ((*gfx_text_count < MAX_GFX_TEXT)? *gfx_text_count:MAX_GFX_TEXT); i > 0; i--)
   {
      C2D_Text dynText;

      C2D_TextParse(&dynText, g_textBuf, gfx_text[i-1].text);
      C2D_TextOptimize(&dynText);
	  C2D_DrawText(&dynText, C2D_AtBaseline | C2D_AlignCenter | C2D_WithColor, gfx_text[i-1].x+1, gfx_text[i-1].y+1, 0.5f, 0.75f, gfx_text[i-1].scale, COLOR_GRAY); // shadow
      C2D_DrawText(&dynText, C2D_AtBaseline | C2D_AlignCenter | C2D_WithColor, gfx_text[i-1].x, gfx_text[i-1].y, 0.5f, 0.75f, gfx_text[i-1].scale, COLOR_WHITE);
   }
}

void text_sceneRender_menu(gfx_text_t *gfx_menu_text, int *gfx_menu_text_count)
{
   C2D_TextBufClear(g_textBuf);

   for (int i = ((*gfx_menu_text_count < MAX_GFX_TEXT)? *gfx_menu_text_count:MAX_GFX_TEXT); i > 0; i--)
   {
      C2D_Text dynText;

      C2D_TextParse(&dynText, g_textBuf, gfx_menu_text[i-1].text);
      C2D_TextOptimize(&dynText);
      C2D_DrawText(&dynText, C2D_AlignLeft | C2D_WithColor, gfx_menu_text[i-1].x+1, gfx_menu_text[i-1].y+1, 0.0f, gfx_menu_text[i-1].scale, gfx_menu_text[i-1].scale, COLOR_GRAY); // shadow
      C2D_DrawText(&dynText, C2D_AlignLeft | C2D_WithColor, gfx_menu_text[i-1].x, gfx_menu_text[i-1].y, 0.0f, gfx_menu_text[i-1].scale, gfx_menu_text[i-1].scale, gfx_menu_text[i-1].color);
   } 
}




void inline text_sceneRender_chat_offset(void)
{
	if ( sys_state.MenuState != STATE_CHAT_SELECT )
	{
   u32 kDown = hidKeysHeld();

   if (buffer->total_objects>0)
   {
      buffer_object_t* buffer_object = buffer->objects[buffer->total_objects-1];

      if ((kDown & KEY_UP) && ((buffer->objects[0]->obj_pos_y + chat_text_offset < 35.0f)))
	  {
         chat_text_offset = chat_text_offset + 5.0f;
		 auto_scroll = false;
	  }
      if ((kDown & KEY_DOWN) && (((buffer_object->obj_pos_y + buffer_object->obj_height + chat_text_offset+ ( (buffer->total_objects-1) * 25 )) > 210.0f)))
	  {
         chat_text_offset = chat_text_offset - 5.0f;
		 auto_scroll = false;
	  }

      if ((auto_scroll) && (((buffer_object->obj_pos_y + buffer_object->obj_height + chat_text_offset+ ( (buffer->total_objects-1) * 25 )) > 210.0f)))
         chat_text_offset = chat_text_offset - 5.0f;

      if ((!auto_scroll) && (((buffer_object->obj_pos_y + buffer_object->obj_height + chat_text_offset+ ( (buffer->total_objects-1) * 25 )) <= 210.0f)))
         auto_scroll=true;
   }
	}
	  
}


/*
void text_sceneRender_chat(gfx_text_t *gfx_chat_text, int *gfx_chat_text_count)
{
   text_sceneRender_chat_offset();

   C2D_TextBufClear(g_chatBuf);

   for (int i = ((*gfx_chat_text_count < MAX_GFX_TEXT)? *gfx_chat_text_count:MAX_GFX_TEXT); i > 0; i--)
   {

      gfx_chat_text[i-1].y = gfx_chat_text[i].y+gfx_chat_text[i].height + 15;

      if (((gfx_chat_text[i-1].y+gfx_chat_text[i-1].height+ chat_text_offset) > 15.0f) && ((gfx_chat_text[i-1].y+ chat_text_offset) < 230.0f))
      {
         C2D_Text dynText4;

         C2D_TextParse(&dynText4, g_chatBuf, gfx_chat_text[i-1].text);
         C2D_LayoutText(&dynText4, C2D_AlignLeft | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
         C2D_TextOptimize(&dynText4);
         C2D_DrawText(&dynText4, C2D_WithColor, gfx_chat_text[i-1].x, (gfx_chat_text[i-1].y+ chat_text_offset), 0.8f, 0.5f, gfx_chat_text[i-1].scale, COLOR_WHITE);
      }
   } 
}
*/


void text_sceneInit(void)
{
   g_chatBuf = C2D_TextBufNew(4096);
   g_textBuf = C2D_TextBufNew(512);
   g_objBuf  = C2D_TextBufNew(512);
}

void text_sceneExit(void)
{
   C2D_TextBufDelete(g_chatBuf);
   C2D_TextBufDelete(g_textBuf);
   C2D_TextBufDelete(g_objBuf);

   for (int i = 0; i < gfx_text_button_count; i++)
      free(gfx_text_button[i].text);

   for (int i = 0; i < gfx_text_menu_count; i++)
      free(gfx_text_menu[i].text);

   for (int i = 0; i < gfx_chat_text_count; i++)
      free(gfx_chat_text[i].text);
}

void gfx_text_render(char* text, float x, float y, float scale, Color* color)
{
   gfx_text_add(gfx_text_menu, &gfx_text_menu_count, &next_gfx_text_menu_id,
         text,
         x,
         y,
         scale,
         color
    );
}



void gfx_border_render(float x, float y, float width, float height, float scale)
{
   if (!options.border_enable)
      return;

   float corner_size = 22.0f;
   float scale_x     = scale;
   float scale_y     = scale;

   size_t numImages = C2D_SpriteSheetCount(spriteSheet);

   for (size_t i = 0; i < numImages; i++)
   {
      Sprite* sprite = &sprites[i];

//      C2D_SpriteFromSheet(&sprite->spr, spriteSheet, i);
	  
      C2D_SpriteSetDepth(&sprite->spr, 0.5f);
	  
	  if (i == images_border_bottom_left_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, scale_y);
         C2D_SpriteSetPos(&sprite->spr, x, y+height-(corner_size*scale_y));
	  }
	  else if (i == images_border_bottom_right_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, scale_y);
         C2D_SpriteSetPos(&sprite->spr, x+width-(corner_size*scale_x), y+height-(corner_size*scale_y));
	  }
  	  else if (i == images_border_top_left_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, scale_y);
         C2D_SpriteSetPos(&sprite->spr, x, y);
	  }
  	  else if (i == images_border_top_right_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, scale_y);
         C2D_SpriteSetPos(&sprite->spr, x+width-(corner_size*scale_x), y);
	  }


  	  else if (i == images_border_bottom_idx )
	  {
		  C2D_SpriteSetScale(&sprite->spr, width-((corner_size*scale_x)*2), scale_y);
		  C2D_SpriteSetPos(&sprite->spr, x+(corner_size*scale_x), y+height-(10*scale_y));
	  }
	  else if (i == images_border_top_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, width-((corner_size*scale_x)*2), scale_y);
         C2D_SpriteSetPos(&sprite->spr, x+(corner_size*scale_x), y);
	  }
	  else if (i == images_border_left_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, height-((corner_size*scale_y)*2));
         C2D_SpriteSetPos(&sprite->spr, x, y+(corner_size*scale_y));
	  }
	  else if (i == images_border_right_idx )
	  {
         C2D_SpriteSetScale(&sprite->spr, scale_x, height-((corner_size*scale_y)*2));
         C2D_SpriteSetPos(&sprite->spr, x+width-(10*scale_x), y+(corner_size*scale_y));
	  }
   }

   C2D_DrawSprite(&sprites[images_border_bottom_left_idx].spr);
   C2D_DrawSprite(&sprites[images_border_bottom_right_idx].spr);
   C2D_DrawSprite(&sprites[images_border_top_left_idx].spr);
   C2D_DrawSprite(&sprites[images_border_top_right_idx].spr);
   C2D_DrawSprite(&sprites[images_border_bottom_idx].spr);
   C2D_DrawSprite(&sprites[images_border_top_idx].spr);
   C2D_DrawSprite(&sprites[images_border_left_idx].spr);
   C2D_DrawSprite(&sprites[images_border_right_idx].spr);

}
void gfx_button_render(char* text, float x, float y, float width, float height, float scale)
{
   Color clr = {255,0,0,255};     // red This is not working atm..
   gfx_border_render(x,y,width,height,scale);
   gfx_text_add(gfx_text_button, &gfx_text_button_count, &next_gfx_text_button_id, text, x+(width/2), y+(height/2)+9, 1.0f, &clr);
}


void draw_obj_text(char* text, float x, float y, float scale, u32 color)
{
   char user_tmp[64];
   sprintf(user_tmp,"%s:", text);

   C2D_TextBufClear(g_chatBuf);
   C2D_Text dynText;
   C2D_TextParse(&dynText, g_chatBuf, user_tmp);
   C2D_TextOptimize(&dynText);
   C2D_DrawText(&dynText, C2D_WithColor, x, y, 0.8f, scale, scale, color);
}


void render_gfx_obj()
{
   if (buffer->total_objects == 0)
      return;

   text_sceneRender_chat_offset();

   int tmp_width         = 0;
   int tmp_height        = 0;
   int count             = 0;
   int block_count       = 0;
   int part_of_block     = 0;
   int block_frame_drawn = 0;
   int border_color;

   while (count < buffer->total_objects)
   {
      buffer_object_t* buffer_object = buffer->objects[count];
      block_frame_drawn = 0;
      if ((((buffer_object->obj_pos_y+buffer_object->obj_height+ chat_text_offset+ ( count * 25 )) > 15.0f) &&
            ((buffer_object->obj_pos_y+ chat_text_offset+ ( count * 25 )) < 230.0f)) || (( block_frame_drawn == 0 ) && ( part_of_block > 0 )))
      {

if (sys_state.MenuState == STATE_CHAT_SELECT)
{
if (selected_chat == 0) selected_chat = buffer_object->obj_blck_id;
   border_color = (buffer_object->obj_blck_id==selected_chat) ? COLOR_GRAY:COLOR_WHITE;
}
else
{
border_color = COLOR_WHITE;
}

         switch (buffer_object->type)
         {
            case OBJ_TYPE_TEXT:
               gfx_text_t* text_object = (gfx_text_t*)buffer_object->object;

               C2D_TextBufClear(g_chatBuf);
               C2D_Text dynText4;
               C2D_TextParse(&dynText4, g_chatBuf, text_object->text);
               C2D_LayoutText(&dynText4, C2D_AlignLeft | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
               C2D_TextOptimize(&dynText4);
//               C2D_DrawText(&dynText4, C2D_WithColor, 25+1, buffer_object->obj_pos_y + chat_text_offset + ( count * 25 )+1, 0.8f, 0.5f, 0.5, COLOR_GRAY);
               C2D_DrawText(&dynText4, C2D_WithColor, 25, buffer_object->obj_pos_y + chat_text_offset + ( count * 25 ), 0.8f, 0.5f, 0.5, border_color);

               if (buffer_object->obj_buffers > 0) {
                  tmp_height    = 0;
                  part_of_block = 1;

                  for (int i = 0; i < (buffer_object->obj_buffers+1); i++) {
                     if (buffer->objects[count-i]->obj_width > tmp_width)
                        tmp_width = buffer->objects[count-i]->obj_width;

                     tmp_height = tmp_height + buffer->objects[count-i]->obj_height;
                  }

                  if (!(buffer->objects[count+1] == NULL)) {
                     block_frame_drawn = 0;
                     if (buffer->objects[count+1]->obj_buffers == 0) {
                        gfx_border_render(20.0f, (buffer->objects[count-buffer_object->obj_buffers]->obj_pos_y + chat_text_offset) + ((count) * 25 )-((buffer_object->obj_buffers)*25)-6, tmp_width + 10, tmp_height +( (buffer_object->obj_buffers ) * 25 )+10, 0.3f);
                        draw_obj_text(users[buffer_object->obj_user], 20.0f, (buffer->objects[count-buffer_object->obj_buffers]->obj_pos_y + chat_text_offset) + ( count * 25 )-((buffer_object->obj_buffers)*25) - 16, 0.4f, border_color);

                        block_frame_drawn = 1;
                     }
                  }
                  else {
                     gfx_border_render(20.0f, (buffer->objects[count-buffer_object->obj_buffers]->obj_pos_y + chat_text_offset) + ((count) * 25 )-((buffer_object->obj_buffers)*25)-6, tmp_width + 10, tmp_height +( (buffer_object->obj_buffers ) * 25 )+10, 0.3f);
                     draw_obj_text(users[buffer_object->obj_user], 20.0f, (buffer->objects[count-buffer_object->obj_buffers]->obj_pos_y + chat_text_offset) + ( count * 25 )-((buffer_object->obj_buffers)*25) - 16, 0.4f, border_color);

                     block_frame_drawn = 2;
                  }
               }
               else {
                  if (buffer->objects[count+1] != NULL) {
                     if ((buffer->objects[count+1]->obj_buffers == 0) || (buffer->total_objects == 1)) {
                        gfx_border_render(20.0f, (buffer_object->obj_pos_y + chat_text_offset) - 5 + ( count * 25 ), buffer_object->obj_width + 10, buffer_object->obj_height + 10, 0.3f);
                        draw_obj_text(users[buffer_object->obj_user], 20.0f, buffer_object->obj_pos_y + chat_text_offset - 16 + ( count * 25 ), 0.4f, border_color);

                        tmp_width         = 0;
                        tmp_height        = 0;
                        part_of_block     = 0;
block_count++; // still using this??
                        block_frame_drawn = 3;
					 }
					 else
                        part_of_block     = 1;
                  }
                  else {
                     gfx_border_render(20.0f, buffer_object->obj_pos_y + chat_text_offset - 5 + ( count * 25 ), buffer_object->obj_width + 10, buffer_object->obj_height + 10, 0.3f);
                     draw_obj_text(users[buffer_object->obj_user], 20.0f, buffer_object->obj_pos_y + chat_text_offset - 16 + ( count * 25 ), 0.4f, border_color);

                     block_frame_drawn = 4;
                     part_of_block     = 0;
block_count++;
				  }
               }
               break;

            case OBJ_TYPE_IMAGE:
               gfx_image_t* image_object = (gfx_image_t*)buffer_object->object;
//                printf("URL: %s\n", image_object->url);
               C2D_DrawImageAt((C2D_Image)image_object->image_data, 75.0f, buffer_object->obj_pos_y + chat_text_offset + ( count * 25 ), 0.6f, NULL, 1.0f, 1.0f);
               gfx_border_render(70.0f, buffer_object->obj_pos_y + chat_text_offset - 5 + ( count * 25 ), buffer_object->obj_width + 10, buffer_object->obj_height + 10, 0.3f);

               block_frame_drawn = 5;
               break;
         }
      }
      count++;

      if (options.debug_enable)
      {
         if (((buffer_object->obj_pos_y+buffer_object->obj_height+ chat_text_offset+ ( count * 25 )) > 15.0f) &&
               ((buffer_object->obj_pos_y+ chat_text_offset+ ( count * 25 )) < 230.0f))
         {
            char response_tmp[128];
            sprintf(response_tmp,"#%i, y=%.1f, w=%.1f, h=%.1f, buf: %i, blck=%i, drw=%i, tw=%i, th=%i\n", 
                  count, buffer_object->obj_pos_y, buffer_object->obj_width, buffer_object->obj_height, buffer_object->obj_buffers,part_of_block,block_frame_drawn,tmp_width,tmp_height);

            C2D_TextBufClear(g_chatBuf);
            C2D_Text dynText5;
            C2D_TextParse(&dynText5, g_chatBuf, response_tmp);
            C2D_LayoutText(&dynText5, C2D_AlignLeft | C2D_WordWrap, 0.4f, 0.4f, 350.0f);
            C2D_TextOptimize(&dynText5);
            C2D_DrawText(&dynText5, C2D_WithColor, 20, buffer_object->obj_pos_y + chat_text_offset + ( (count-1) * 25 )-5, 0.8f, 0.5f, 1.0, COLOR_BLACK);
         }
      }
   }

   if (options.debug_enable)
   {
      char response_tmp[64];
      sprintf(response_tmp,"Drawn = %i\n block = %i\n tmp_width = %i\n selected = %i\n", block_frame_drawn, part_of_block,tmp_width, selected_chat);

      C2D_TextBufClear(g_chatBuf);
      C2D_Text dynText5;
      C2D_TextParse(&dynText5, g_chatBuf, response_tmp);
      C2D_LayoutText(&dynText5, C2D_AlignRight | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
      C2D_TextOptimize(&dynText5);
      C2D_DrawText(&dynText5, C2D_WithColor, 380, 15, 1.0f, 0.5f, 1.0, C2D_Color32(0,0,0,255));
   }

}


void render_gfx_debug()
{
   if (options.debug_enable)
   {
      char response_tmp[64];
      sprintf(response_tmp,"offset = %.1f\n total objects = %i\n", chat_text_offset, buffer->total_objects);

      C2D_TextBufClear(g_chatBuf);
      C2D_Text dynText5;
      C2D_TextParse(&dynText5, g_chatBuf, response_tmp);
      C2D_LayoutText(&dynText5, C2D_AlignRight | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
      C2D_TextOptimize(&dynText5);
      C2D_DrawText(&dynText5, C2D_WithColor, 380, 200, 1.0f, 0.5f, 1.0, COLOR_BLACK);
   }
}



void gfx_frame_top(C3D_RenderTarget* top)
{
   C2D_TargetClear(top, COLOR_BLACK);

   C2D_SceneBegin(top);

   C2D_DrawRectSolid( 10.0f, 10.0f, 0.1f, 381.0f, 220.0f, COLOR_GPT_GREEN);




//C2D_DrawImageAt(img, 50.0f, buffer_pos, 0.7f, NULL, 0.5f, 0.5f);

//text_sceneRender_chat(gfx_chat_text, &gfx_chat_text_count);
      render_gfx_obj();

   C2D_DrawRectangle( 10.0f, 10.0f, 0.9f, 381.0f, 10.0f,
         C2D_Color32(0x12, 0xA3, 0x82, 0xFF), // top left
         C2D_Color32(0x12, 0xA3, 0x82, 0xFF), // top right
         C2D_Color32(0x12, 0xA3, 0x82, 0x00), // bottom left
         C2D_Color32(0x12, 0xA3, 0x82, 0x00)  // bottom right
         );

   C2D_DrawRectangle( 10.0f, 220.0f, 0.9f, 381.0f, 10.0f,
         C2D_Color32(0x12, 0xA3, 0x82, 0x00), // top left
         C2D_Color32(0x12, 0xA3, 0x82, 0x00), // top right
         C2D_Color32(0x12, 0xA3, 0x82, 0xFF), // bottom left
         C2D_Color32(0x12, 0xA3, 0x82, 0xFF)  // bottom right
         );

   C2D_SpriteSetCenter(&sprites[images_border_small_idx].spr, 0.5f, 0.5f);
   C2D_SpriteSetScale(&sprites[images_border_small_idx].spr, 2.0f, 2.0f);
   C2D_SpriteSetPos(&sprites[images_border_small_idx].spr, 200.0f, 120.0f);
   C2D_SpriteSetDepth(&sprites[images_border_small_idx].spr, 1.0f);


   C2D_DrawSprite(&sprites[images_border_small_idx].spr);



render_gfx_debug();

}















#include <stdio.h>
#include <math.h>

#define PI 3.14159265359f

// Function to draw a circle using a line
void drawCircle(float centerX, float centerY, float radius, u32 clr, float thickness, float depth) {
    int numSegments = 100; // Adjust this to change the smoothness of the circle

    for (int i = 0; i < numSegments; ++i) {
        float angle1 = (2 * PI * i) / numSegments;
        float angle2 = (2 * PI * (i + 1)) / numSegments;
        float x0 = centerX + radius * cos(angle1);
        float y0 = centerY + radius * sin(angle1);
        float x1 = centerX + radius * cos(angle2);
        float y1 = centerY + radius * sin(angle2);

        C2D_DrawLine(x0, y0, clr, x1, y1, clr, thickness, depth);
    }
}


// Function to draw a progress-filled circle
void drawProgressCircle(float centerX, float centerY, float radius, u32 clr, float depth, int progress, float startAngle) {
    int numSegments = 100;
    int filledSegments = numSegments * progress / 100;

    for (int i = 0; i < filledSegments; ++i) {
        float angle1 = (2 * PI * (i + startAngle)) / numSegments;
        float angle2 = (2 * PI * (i + 1 + startAngle)) / numSegments;
        float x0 = centerX + radius * cos(angle1);
        float y0 = centerY + radius * sin(angle1);
        float x1 = centerX + radius * cos(angle2);
        float y1 = centerY + radius * sin(angle2);

        C2D_DrawTriangle(centerX, centerY, clr, x0, y0, clr, x1, y1, clr, depth);
    }
}

// Function to draw a solid circle
void drawSolidCircle(float centerX, float centerY, float radius, u32 clr, float thickness, float depth) { // obsolete and inefficient
    int numSegments = 100; // Adjust this to change the smoothness of the circle

    // Calculate the angle increment based on the number of segments
    float angleIncrement = (2 * PI) / numSegments;

    // Draw consecutive lines very close to each other to fill the circle
    for (int i = 0; i < numSegments; ++i) {
        float angle = angleIncrement * i;
        float x0 = centerX + radius * cos(angle);
        float y0 = centerY + radius * sin(angle);
        float x1 = centerX + radius * cos(angle + angleIncrement);
        float y1 = centerY + radius * sin(angle + angleIncrement);

        C2D_DrawLine(x0, y0, clr, x1, y1, clr, thickness, depth);
    }
}





float startAngle;

void draw_rec_progress(void)
{
	 int rec_prog = roundf(((float)audiobuf_pos/(float)audiobuf_size)*100);



if (!mic_recording) {
	
	
	startAngle += (PI / 9);
	}


C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 45.0f, COLOR_WHITE);

drawProgressCircle(160.0f, 120.0f, 40.0f, COLOR_GPT_GREEN, 1.0f, rec_prog, startAngle);

C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 35.0f, COLOR_WHITE);

C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 30.0f, COLOR_GPT_GREEN);

      C2D_SpriteSetCenter(&sprites[images_mic_idx].spr, 0.5f, 0.5f);
      C2D_SpriteSetPos(&sprites[images_mic_idx].spr, 160.0f, 120.0f);
	  C2D_SpriteSetDepth(&sprites[images_mic_idx].spr, 1.0f);
      C2D_DrawSprite(&sprites[images_mic_idx].spr);

}




void gfx_frame_bottom(C3D_RenderTarget* bottom)
{
//   C2D_TargetClear(bottom, COLOR_BLACK);
   C2D_SceneBegin(bottom);
   C2D_TargetClear(bottom, COLOR_GPT_GREEN);





   if (sys_state.Process != PROC_NONE)
      sprite_rotate(images_logo_idx);
/*
   C2D_SpriteSetCenter(&sprites[images_logo_idx].spr, 0.5f, 0.5f);
   C2D_SpriteSetPos(&sprites[images_logo_idx].spr, 313.0f, 233.0f);
   C2D_DrawSprite(&sprites[images_logo_idx].spr);
*/
   C2D_SpriteSetScale(&sprites[images_logo_idx].spr, 1.0f, 1.0f);
   C2D_SpriteSetCenter(&sprites[images_logo_idx].spr, 0.5f, 0.5f);
   C2D_SpriteSetPos(&sprites[images_logo_idx].spr, 7.0f, 233.0f);
   C2D_DrawSprite(&sprites[images_logo_idx].spr);


   if (options.voice_enable) {
      C2D_SpriteSetCenter(&sprites[images_speak_idx].spr, 0.5f, 0.5f);
      C2D_SpriteSetPos(&sprites[images_speak_idx].spr, 295.0f, 20.0f);
      C2D_DrawSprite(&sprites[images_speak_idx].spr);
   }
   
   if((sys_state.MenuState==STATE_RECORDING) && (sys_state.Process==PROC_RECORD)) {
//      C2D_SpriteSetCenter(&sprites[images_mic_idx].spr, 0.5f, 0.5f);
//      C2D_SpriteSetPos(&sprites[images_mic_idx].spr, 295.0f, 60.0f);
//      C2D_DrawSprite(&sprites[images_mic_idx].spr);




draw_rec_progress();
/*
char mic_info[128];
sprintf(mic_info,"audiobuf_size: %li\naudiobuf_pos: %li",audiobuf_size,audiobuf_pos);

      C2D_TextBufClear(g_textBuf);
      C2D_Text dynText5;
      C2D_TextParse(&dynText5, g_textBuf, mic_info);
      C2D_LayoutText(&dynText5, C2D_AlignLeft | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
      C2D_TextOptimize(&dynText5);
      C2D_DrawText(&dynText5, C2D_WithColor, 50, 15, 1.0f, 0.5f, 1.0, C2D_Color32(0,0,0,255));
	  */
startAngle = 0;
   }

if((sys_state.MenuState==STATE_RECORDING) && (sys_state.Process==PROC_RECORD_DONE)) 
{
	startAngle += (PI / 9); // twice? it's already being called in draw_rec_progress()
	draw_rec_progress();
}







      if(sys_state.MenuState==STATE_DEBUG_MIC)
      {
		  /*
         char mic_info[128];
sprintf(mic_info,"audiobuf_size: %li\naudiobuf_pos: %li",audiobuf_size,audiobuf_pos);

      C2D_TextBufClear(g_textBuf);
      C2D_Text dynText5;
      C2D_TextParse(&dynText5, g_textBuf, mic_info);
      C2D_LayoutText(&dynText5, C2D_AlignLeft | C2D_WordWrap, 0.5f, 0.5f, 350.0f);
      C2D_TextOptimize(&dynText5);
      C2D_DrawText(&dynText5, C2D_WithColor, 50, 15, 1.0f, 0.5f, 1.0, C2D_Color32(0,0,0,255));
*/




 int rec_prog = roundf(((float)audiobuf_pos/(float)audiobuf_size)*100);



if (!mic_recording) {
    // Loop through start angles from 0 to 360 degrees (in radians)
	startAngle += (PI / 9);
	}


C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 45.0f, COLOR_WHITE);

drawProgressCircle(160.0f, 120.0f, 40.0f, COLOR_GPT_GREEN, 1.0f, rec_prog, startAngle);

C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 35.0f, COLOR_WHITE);

C2D_DrawCircleSolid (160.0f, 120.0f, 1.0f, 30.0f, COLOR_GPT_GREEN);

      C2D_SpriteSetCenter(&sprites[images_mic_idx].spr, 0.5f, 0.5f);
      C2D_SpriteSetPos(&sprites[images_mic_idx].spr, 160.0f, 120.0f);
	  C2D_SpriteSetDepth(&sprites[images_mic_idx].spr, 1.0f);
      C2D_DrawSprite(&sprites[images_mic_idx].spr);





      }

// d-pad none    
// d-pad up      
// d-pad down    
// d-pad both    

if (current_menu==MAIN_CHAT)
{
// TODO add button
//      C2D_SpriteSetCenter(&sprites[images_mic_idx].spr, 0.5f, 0.5f);
//      C2D_SpriteSetPos(&sprites[images_mic_idx].spr, 295.0f, 60.0f);
//      C2D_DrawSprite(&sprites[images_mic_idx].spr);




   char scroll_icon[4];

   if (buffer->total_objects>0)
   {
      buffer_object_t* buffer_object = buffer->objects[buffer->total_objects-1];

      if (((buffer->objects[0]->obj_pos_y + chat_text_offset < 35.0f)) && (((buffer_object->obj_pos_y + buffer_object->obj_height + chat_text_offset+ ( (buffer->total_objects-1) * 25 )) > 210.0f)))
         sprintf(scroll_icon,"");

      else if ((buffer->objects[0]->obj_pos_y + chat_text_offset < 35.0f))
         sprintf(scroll_icon,"");

      else if (((buffer_object->obj_pos_y + buffer_object->obj_height + chat_text_offset+ ( (buffer->total_objects-1) * 25 )) > 210.0f))
         sprintf(scroll_icon,"");

      else
         sprintf(scroll_icon,"");

   }
   else
      sprintf(scroll_icon,"");

   {
      C2D_TextBufClear(g_textBuf);
      C2D_Text dynText5;
      C2D_TextParse(&dynText5, g_textBuf, scroll_icon);
      C2D_TextOptimize(&dynText5);
      C2D_DrawText(&dynText5, C2D_AlignLeft | C2D_WithColor, 10.0f+1, 10.0f+1, 1.0f, 1.0f, 1.0f, COLOR_GRAY); // shadow
      C2D_DrawText(&dynText5, C2D_AlignLeft | C2D_WithColor, 10.0f, 10.0f, 1.0f, 1.0f, 1.0f, COLOR_WHITE);
   }
}

   if (sys_state.Process == PROC_NONE)
   {

   gfx_text_clear(gfx_text_menu, &gfx_text_menu_count, &next_gfx_text_menu_id);
   for (size_t i = 0; i < menus[current_menu].num_texts; i++)
   {
      gfx_text_render(
            update_menu_texts(menus[current_menu].name, i),
            menus[current_menu].texts[i].x,
            menus[current_menu].texts[i].y,
            menus[current_menu].texts[i].size,
            &menus[current_menu].texts[i].color
      );
   }

   gfx_text_clear(gfx_text_button, &gfx_text_button_count, &next_gfx_text_button_id);
   for (size_t i = 0; i < menus[current_menu].num_buttons; i++)
   {
      gfx_button_render(
            menus[current_menu].buttons[i].text,
            menus[current_menu].buttons[i].x,
            menus[current_menu].buttons[i].y,
            menus[current_menu].buttons[i].width, 
            menus[current_menu].buttons[i].height,
            menus[current_menu].buttons[i].scale
      );
   }
   
   for (size_t i = 0; i < menus[current_menu].num_sprites; i++)
   {
      int sprite_id = (int)menus[current_menu].sprites[i].data;
      Sprite* sprite = &sprites[sprite_id];

      C2D_SpriteFromSheet(&sprite->spr, spriteSheet, sprite_id);
	  
      C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
      C2D_SpriteSetScale(&sprite->spr, menus[current_menu].sprites[i].scale, menus[current_menu].sprites[i].scale);
      C2D_SpriteSetPos(&sprite->spr, menus[current_menu].sprites[i].x, menus[current_menu].sprites[i].y);
      C2D_SpriteSetDepth(&sprite->spr, 1.0f);

      C2D_DrawSprite(&sprite->spr);
   }
   

   text_sceneRender_button(gfx_text_button, &gfx_text_button_count);
   }
   text_sceneRender_menu(gfx_text_menu, &gfx_text_menu_count);
}

void gfx_frame(void)
{
   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

#ifndef CONSOLE_ENABLE
   if (!bufferlock)
      gfx_frame_top(top);
#endif
   gfx_frame_bottom(bottom);

   C3D_FrameEnd(0);
}

/*
void gfx_thread(void *arg)
{
   while(runGfxThread) {
      gfx_frame();
      gspWaitForVBlank();
   }
}
*/

void gfx_init(void)
{
   C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
   C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
   C2D_Prepare();

   spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/images.t3x");
   if (!spriteSheet) svcBreak(USERBREAK_PANIC);

   sprite_init();
   text_sceneInit();

   top    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
   bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);


   buffer = create_buffer();
}

void gfx_exit(void)
{
   runGfxThread = false;

   text_sceneExit();

 // Clear the buffer
//clear_buffer(buffer);
   destroy_buffer(buffer);


   C2D_SpriteSheetFree(spriteSheet);

   C2D_Fini();
   C3D_Fini();
}
