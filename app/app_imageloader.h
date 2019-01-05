#ifndef _APP_IMAGELOADER_H_
#define _APP_IMAGELOADER_H_

typedef void (*LOADER_CALLBACK)(void *);

void start_imageloader(const char* path,u8_t file_type);
void release_imageloader();
void set_loadercallback(LOADER_CALLBACK callback);

#endif
