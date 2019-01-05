#include "app_imageloader.h"

#define XMSYS_IMAGELOADER_TASK_STACK_SIZE   0x2000

#define IMAGELOADER_EVENT_START     0x01
#define IMAGELOADER_EVENT_STOP      0x02

__no_init static OS_STACKPTR int StackImageloaderTask[XMSYS_APP_TASK_STACK_SIZE/4];          /* Task stacks */

static OS_TASK ImageLoader;
static char *dir_path = NULL;

static XMBOOL   image_loader_flag = FALSE;  //

static queue_s  

static void load_image()
{
    image_loader_flag = TRUE;
    char image_event;
    while(TRUE)
    {
        image_event = OS_WaitEvent(IMAGELOADER_EVENT_START | IMAGELOADER_EVENT_STOP);
        if(image_event & IMAGELOADER_EVENT_START)
        {
            
        }
    }
}

void start_imageloader(const char * dir_path, u8_t file_type)
{
    memset(&ImageLoader,0,sizeof(OS_TASK));
    OS_CREATETASK(&ImageLoader, "ImageLoader", load_image, XMSYS_APP_TASK_PRIORITY, StackImageloaderTask);
}

void stop_imageloader()
{
    OS_SignalEvent(MOTION_EVENT_SENSITIVITY, &ImageLoader);
}



