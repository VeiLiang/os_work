#ifndef _REQUEST_H
#define _REQUEST_H

void usb_setup_in(BYTE bCount);
void usb_fetch_out(BYTE bCount);

void video_control_set_cur();
void video_control_get_cur();
void video_control_get_min();
void video_control_get_max();
void video_control_get_res();
void video_control_get_info();
void video_control_get_len();
void video_control_get_def();

void video_stream_set_cur();
void video_stream_get_cur();

void audio_control_set_cur();
void audio_control_get_cur();
void audio_control_get_min();
void audio_control_get_max();
void audio_control_get_res();

void audio_endpoint_control_set_cur();

void usb_request();
void uvc_request();

#endif
