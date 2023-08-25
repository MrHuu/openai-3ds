#ifndef _MIC_H
#define _MIC_H

void MIC_record_start(void);
void MIC_record_play(void);
void MIC_record_stop(void);
void MIC_record_frame(void);


extern bool mic_recording;

	extern u32 audiobuf_size;
	extern u32 audiobuf_pos;
	extern u8* audiobuf;

void mic_init(void);
void mic_exit(void);
void mic_clean(void);



int main_sndfile (void);

#include <curl/curl.h>

void curl_libsndfile(CURL* curl);
void curl_libsndfile_clean(void);




void wav_buf(void);
void play_wav_buf(void);
void play_wav_file(void);
#endif /* _MIC_H */

