#ifndef __AUDIO__H_
#define __AUDIO__H_

#include<RtAudio.h>

int init_audio(unsigned int channels, float sense);
void close_audio();
float get_amp();
float get_sense();
void set_output(bool o);

#endif
