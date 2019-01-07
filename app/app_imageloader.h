#ifndef _APP_IMAGELOADER_H_
#define _APP_IMAGELOADER_H_

#define THUMB_WIDTH         304
#define THUMB_HEIGHT        208
#define THUMB_HOR_INTERVAL  28
#define THUMB_VERT_INTERVAL 28

typedef void (*LOADER_CALLBACK)(void *);

typedef struct video_thumb_data
{
    unsigned int item;
    char *image_addr;
};

void start_imageloader(const char* path,u8_t file_type);
void release_imageloader();
void set_loadercallback(LOADER_CALLBACK callback);

#endif
