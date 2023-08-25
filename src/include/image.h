#ifndef _IMAGE_H
#define _IMAGE_H

typedef struct {
    int id;
    char* url;
    C2D_Image image_data;
	C3D_Tex   tex;
	Tex3DS_SubTexture subt3x;
    char* request_text;
	int width;
  int height;
} gfx_image_t;

bool IMAGE_loadImageFromFile(const char *filename, gfx_image_t *image);


bool IMAGE_load_image_from_url(const char* url, gfx_image_t* image);


#endif /* _IMAGE_H */
