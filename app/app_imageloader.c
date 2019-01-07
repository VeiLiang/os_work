#include "app_imageloader.h"

#define XMSYS_IMAGELOADER_TASK_STACK_SIZE   0x2000

#define IMAGELOADER_EVENT_START     0x01
#define IMAGELOADER_EVENT_STOP      0x02

__no_init static OS_STACKPTR int StackImageloaderTask[XMSYS_APP_TASK_STACK_SIZE/4];          /* Task stacks */

static OS_TASK ImageLoader;
static char *load_thum_file_path = NULL;
static u8_t load_thum_file_type = 0xff;
static unsigned int load_thum_file_channel;

static queue_s thum_queue_list;

static XMBOOL   image_loader_flag = FALSE;      //
LOADER_CALLBACK image_loader_callback = NULL;

void set_loadercallback(LOADER_CALLBACK callback)
{
    if(image_loader_callback == NULL)
    {
        image_loader_callback = callback;
    }
}

static void load_image(const char* file_path,u8_t file_type,unsigned int file_channel)
{
    if(file_path != NULL)
    {
        
    }
}

static void loader_task()
{
    image_loader_flag = TRUE;
    char image_event;
    while(TRUE)
    {
        image_event = OS_WaitEvent(IMAGELOADER_EVENT_START | IMAGELOADER_EVENT_STOP);
        if(image_event & IMAGELOADER_EVENT_START)
        {
            load_image();
        }
        else if(image_event & IMAGELOADER_EVENT_STOP)
        {
            
        }
    }
}

void init_imageloader()
{
    memset(&ImageLoader,0,sizeof(OS_TASK));
    OS_CREATETASK(&ImageLoader, "ImageLoader", loader_task, XMSYS_APP_TASK_PRIORITY, StackImageloaderTask);
}

void load_video_thumb(const char * dir_path, u8_t file_type,UINT video_channel)
{
    image_loader_flag = TRUE;
    load_thum_file_path = dir_path;
    load_thum_file_channel = video_channel;
    OS_SignalEvent(IMAGELOADER_EVENT_START, &ImageLoader);
}

void stop_imageloader()
{
    //OS_SignalEvent(IMAGELOADER_EVENT_STOP, &ImageLoader);
}



