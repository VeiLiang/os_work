/*

   itu656.h

*/

#ifndef _ITU_656_ARKN141_HEADFILE_
#define _ITU_656_ARKN141_HEADFILE_


typedef struct _itu656_window_s{

   unsigned int Left_cut;
   unsigned int right_cut;
   unsigned int up_cut;
   unsigned int down_cut;
   unsigned int width;
   unsigned int height;
   unsigned int real_w;
   unsigned int real_h;   
} itu656_window_s;

itu656_window_s *ITU656_Get_Window();

itudata_s *ITU656in_GetDispAddress();

#endif

