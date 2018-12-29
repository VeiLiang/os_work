#ifndef _XM_SPEECH_H_
#define _XM_SPEECH_H_

#if defined (__cplusplus)
	extern "C"{
#endif

void speech_init	(char *stream_buff, unsigned int stream_buff_size);
int	speech_decode	(char **bit_stream, short *pcm_stream, unsigned long compose_type);

int	XM_LoadSPHModule (void);
int	XM_ExitSPHModule (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif
