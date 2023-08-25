#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <png.h>

#include <curl/curl.h>

#include "image.h"
#include "sys.h"


#define TEX_MIN_SIZE 64
#define TEX_MAX_SIZE 1024


unsigned int mynext_pow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE;
}



bool IMAGE_loadImageFromFile(const char *filename, gfx_image_t *image)
{

C2D_Image* image_data = &image->image_data;
    image_data->tex = &image->tex;


    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, NULL);
        return false;
    }

    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);

image->width = width;
image->height = height;

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, ABGR format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    //output ABGR
    // png_set_bgr(png); // Doesn't seem to do anything??
//    png_set_swap_alpha(png); Huu?

    png_read_update_info(png, info);

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);







    unsigned int texWidth = mynext_pow2(width);
    unsigned int texHeight = mynext_pow2(height);

    if(texWidth > TEX_MAX_SIZE || texHeight > TEX_MAX_SIZE)
    {
        printf("3DS only supports up to 1024x1024 textures. Reduce the image size.\n");
        return false;
    }
    
	Tex3DS_SubTexture* subt3x = &image->subt3x;

    subt3x->width = width;
    subt3x->height = height;
    subt3x->left = 0.0f;
    subt3x->top = 1.0f;
    subt3x->right = width / (float)texWidth;
    subt3x->bottom = 1.0 - (height / (float)texHeight);
    image_data->subtex = subt3x;

    C3D_TexInit(image_data->tex, texWidth, texHeight, GPU_RGBA8);
    C3D_TexSetFilter(image_data->tex, GPU_LINEAR, GPU_NEAREST);

    memset(image_data->tex->data, 0, image_data->tex->size);

    for (int j = 0; j < height; j++)
    {
        png_bytep row = row_pointers[j];
        for (int i = 0; i < width; i++)
        {
            png_bytep px = &(row[i * 4]);

            // Swap the colours since png_set_bgr doesn't seem to work as expected
            // Note: 0 - B, 1 - G, 2 - R, 3 - A
            unsigned argb = (px[3]) + (px[2] << 8) + (px[1] << 16) + (px[0] << 24);

            u32 dst = ((((j >> 3) * (texWidth >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy((u8*)image_data->tex->data + dst, &argb, sizeof(u32));
        }
    }

    return true;
}






struct MemoryStruct {
  unsigned char* memory;
  size_t size;
};

// Callback function for libcurl to write the data into the memory buffer
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct* mem = (struct MemoryStruct*)userp;

  mem->memory = (unsigned char*)realloc(mem->memory, mem->size + realsize);
  if (mem->memory == NULL) {
    printf("Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;

  return realsize;
}


  struct MemoryStruct chunk;

void curl_download_file(const char* url)
{
	  CURL* curl_handle;
  CURLcode res;


  chunk.memory = (unsigned char*)malloc(1);
  chunk.size = 0;

//  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();

  if (!curl_handle) {
    printf("Curl initialization failed.\n");
    return;
  }

  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);

#ifdef CURL_NO_CERT
      curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, FALSE);
#else
      curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "romfs:/cacert.pem");
#endif
#ifdef CURL_VERBOSE
      curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
#endif

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      sys_error(0,"curl_easy_perform() failed\n");

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    return;
  }

  // Cleanup libcurl
  curl_easy_cleanup(curl_handle);
}


// This function reads data from the memory buffer
static void read_data_from_memory(png_structp png_ptr, png_bytep data, png_size_t length) {
  struct MemoryStruct* mem = (struct MemoryStruct*)png_get_io_ptr(png_ptr);

  if (length > 0 && mem->size > 0) {
    png_size_t read_length = (length <= mem->size) ? length : mem->size;
    memcpy(data, mem->memory, read_length);
    mem->memory += read_length;
    mem->size -= read_length;
  } else {
    png_error(png_ptr, "Read error from memory buffer.");
  }
}

bool IMAGE_load_image_from_url(const char* url, gfx_image_t* image) {

C2D_Image* image_data = &image->image_data;
image_data->tex = &image->tex;

curl_download_file(url);


// Initialize libpng structures
  png_image png_image_data;
  memset(&png_image_data, 0, sizeof(png_image_data));
  png_image_data.version = PNG_IMAGE_VERSION;
  
// Read the image from memory using libpng
if (!png_image_begin_read_from_memory(&png_image_data, chunk.memory, chunk.size)) {
    fprintf(stderr, "png_image_begin_read_from_memory failed!\n");
    free(chunk.memory);
    return false;
}

// Initialize libpng structures
png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
if (!png_ptr) {
    fprintf(stderr, "png_create_read_struct failed!\n");
    free(chunk.memory);
    return false;
}

png_infop info_ptr = png_create_info_struct(png_ptr);
if (!info_ptr) {
    fprintf(stderr, "png_create_info_struct failed!\n");
    free(chunk.memory);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
}

// Set up error handling
if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "Error during png_read_image!\n");
    free(chunk.memory);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return false;
}

// Set up custom read function
png_set_read_fn(png_ptr, &chunk, read_data_from_memory);

// Read the image information
png_read_info(png_ptr, info_ptr);

// Get image width and height
int width = png_get_image_width(png_ptr, info_ptr);
int height = png_get_image_height(png_ptr, info_ptr);

image->width = width;
image->height = height;

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    // Read any color_type into 8bit depth, ABGR format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    //output ABGR
    // png_set_bgr(png); // Doesn't seem to do anything??
//    png_set_swap_alpha(png); Huu?

    png_read_update_info(png_ptr, info_ptr);


// Set up row_pointers
png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
for (int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
}

// Read the image data into row_pointers
png_read_image(png_ptr, row_pointers);



  // Set up texture properties as per your existing code
  unsigned int texWidth = mynext_pow2(width);
  unsigned int texHeight = mynext_pow2(height);

    if(texWidth > TEX_MAX_SIZE || texHeight > TEX_MAX_SIZE)
    {
        printf("3DS only supports up to 1024x1024 textures. Reduce the image size.\n");
        return false;
    }
    
	Tex3DS_SubTexture* subt3x = &image->subt3x;

    subt3x->width = width;
    subt3x->height = height;
    subt3x->left = 0.0f;
    subt3x->top = 1.0f;
    subt3x->right = width / (float)texWidth;
    subt3x->bottom = 1.0 - (height / (float)texHeight);
    image_data->subtex = subt3x;

    C3D_TexInit(image_data->tex, texWidth, texHeight, GPU_RGBA8);
    C3D_TexSetFilter(image_data->tex, GPU_LINEAR, GPU_NEAREST);

    memset(image_data->tex->data, 0, image_data->tex->size);

    for (int j = 0; j < height; j++)
    {
        png_bytep row = row_pointers[j];
        for (int i = 0; i < width; i++)
        {
            png_bytep px = &(row[i * 4]);

            // Swap the colours since png_set_bgr doesn't seem to work as expected
            // Note: 0 - B, 1 - G, 2 - R, 3 - A
            unsigned argb = (px[3]) + (px[2] << 8) + (px[1] << 16) + (px[0] << 24);

            u32 dst = ((((j >> 3) * (texWidth >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy((u8*)image_data->tex->data + dst, &argb, sizeof(u32));
        }
    }



// Free resources
//for (int y = 0; y < height; y++) {
//    free(row_pointers[y]);
//}
//free(row_pointers);
//free(chunk.memory);

png_destroy_read_struct(&png_ptr, &info_ptr, NULL);



    return true;

}






