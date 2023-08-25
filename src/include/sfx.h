#ifndef _SFX_H
#define _SFX_H

#include <3ds.h>

extern LightEvent addSentenceEvent;
extern LightEvent startPlaybackEvent;

extern ndspWaveBuf playBuf;

void SFX_enqueueText(const char* text, int voice);

void sfx_init();
void sfx_exit();

#endif /* _SFX_H */