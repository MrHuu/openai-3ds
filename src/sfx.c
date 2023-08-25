#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include <flite/flite.h>

#include "main.h"
#include "sfx.h"
#include "sys.h"

#define MAX_TEXT_QUEUE_SIZE       64
#define MAX_WAVE_QUEUE_SIZE       16
#define MAX_PROCESSING_THREADS     2

volatile bool runSFXThread = true;


cst_voice *register_cmu_us_kal();     // Male
cst_voice *register_cmu_us_slt();     // Female, slow on old3ds
#ifdef SFX_ALL_VOICES
cst_voice *register_cmu_us_kal16();   // 
cst_voice *register_cmu_us_rms();     // 
cst_voice *register_cmu_us_awb();     // Irish accent?
#endif

cst_voice *voice;

ndspWaveBuf playBuf;
u16 *samples = NULL;

Thread playbackThreadHandle;
Thread textQueueThreadHandle;
Thread processingThreads[MAX_PROCESSING_THREADS];

LightEvent addSentenceEvent;
LightEvent startPlaybackEvent;
static LightEvent waveReadyEvents[MAX_WAVE_QUEUE_SIZE];

static int textQueueFront = 0;
static int textQueueRear = -1;
static int textQueueSize = 0;

static int waveQueueFront = 0;
static int waveQueueRear = -1;
static int waveQueueSize = 0;


typedef struct {
   char* text;
   int voice;
} textData_t;
textData_t textQueue[MAX_TEXT_QUEUE_SIZE];


typedef struct {
   cst_wave* wave;
} waveData_t;
waveData_t waveQueue[MAX_WAVE_QUEUE_SIZE];




void SFX_enqueueText(const char* text, int voice)
{
   if (!options.voice_enable)
      return;

   if (textQueueSize < MAX_TEXT_QUEUE_SIZE) {
      textQueueRear = (textQueueRear + 1) % MAX_TEXT_QUEUE_SIZE;
      textQueue[textQueueRear].text = strdup(text);
	  textQueue[textQueueRear].voice = voice;
      textQueueSize++;
   } else {
	   sys_error(0,"textQueue is full. Cannot enqueue more text.\n");
   }
   LightEvent_Signal(&addSentenceEvent);
}

int enqueueWave(void)
{
   if (waveQueueSize < MAX_WAVE_QUEUE_SIZE) {
      waveQueueRear = (waveQueueRear + 1) % MAX_WAVE_QUEUE_SIZE;
      waveQueueSize++;
	  return 0;
   } else {
      sys_error(0,"waveQueue is full. Cannot enqueue more waves.\n");
      return -1;
   }
}




void generateWave(void* arg)
{
   int currentWave  = (int)arg;
   int currentText  = textQueueFront;

   textQueueFront = (textQueueFront + 1) % MAX_TEXT_QUEUE_SIZE;
   textQueueSize--;

   const char* text = textQueue[currentText].text;
   int active_voice = textQueue[currentText].voice;

   if (active_voice == 0)
   {
      waveQueue[currentWave].wave = NULL;
   }
   else
   {
      if (active_voice==1)
         voice = register_cmu_us_kal(NULL);
      else if (active_voice==2)
         voice = register_cmu_us_slt(NULL);
#ifdef SFX_ALL_VOICES
      else if (active_voice==3)
         voice = register_cmu_us_kal16(NULL);
      else if (active_voice==4)
         voice = register_cmu_us_rms(NULL);
      else if (active_voice==5)
         voice = register_cmu_us_awb(NULL);
#endif
      waveQueue[currentWave].wave = flite_text_to_wave(text, voice);
   }

   free(textQueue[currentText].text);

   LightEvent_Signal(&waveReadyEvents[currentWave]);
   LightEvent_Signal(&startPlaybackEvent);

   threadExit(0);
}






void processTextQueue(void* arg)
{
   while (runSFXThread) {
      LightEvent_Wait(&addSentenceEvent);

      if (!runSFXThread)
         break;

      if (textQueueSize == 0)
         continue;

      while (enqueueWave() == -1)
         svcSleepThread(1000);

//	s32 prio = 0;
//	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

      for (int i = 0; i < MAX_PROCESSING_THREADS; i++)
      {
         int ret = threadGetExitCode(processingThreads[i]);
         if (ret == 0)
         {
            threadJoin(processingThreads[i], U64_MAX);
            threadFree(processingThreads[i]);
            processingThreads[i] = NULL;
         }

         if ((processingThreads[i] == NULL) && (textQueueSize != 0))
         {
//            if (i==1)
               processingThreads[i] = threadCreate(generateWave, (void*)waveQueueRear, (2 * 1024), 0x3d, -1, false);
               if (playbackThreadHandle == NULL)
                  sys_error(0,"playbackThread failed to start.\n");
//            else
//               processingThreads[i] = threadCreate(generateWave, (void*)waveQueueRear, (2 * 1024), 0x3d, 2, false);
            break;
         }
      }

      if (textQueueSize != 0)
      {
         svcSleepThread(100);
         LightEvent_Signal(&addSentenceEvent);
      }
   }
}



void playAudio(int currentWave)
{
   LightEvent_Wait(&waveReadyEvents[currentWave]);

   cst_wave* waveData = waveQueue[currentWave].wave;

   if (waveData == NULL)
      return;

   static int channel = 0;
   int dataSize;

   dataSize = waveData->num_samples * waveData->num_channels * 2;

   linearFree(samples);
   samples = linearAlloc(dataSize);
   if (samples == NULL)
   {
//	  printf("playAudio error 2\n");
      return;
   }
   memcpy(samples, waveData->samples, dataSize);

   memset(&playBuf, 0, sizeof(ndspWaveBuf));
   playBuf.data_vaddr = samples;
   playBuf.nsamples = waveData->num_samples / waveData->num_channels;
   playBuf.looping = false;
   playBuf.status = NDSP_WBUF_FREE;

   ndspChnReset(channel);
   ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
   ndspChnSetRate(channel, waveData->sample_rate);
   ndspChnSetFormat(channel, NDSP_FORMAT_MONO_PCM16);

   DSP_FlushDataCache((u8*)samples, dataSize);
   ndspChnWaveBufAdd(channel, &playBuf);

   while ((playBuf.status != NDSP_WBUF_DONE) && (runSFXThread))
   {
      svcSleepThread(10000);
   }
}


void playbackThread(void* arg)
{
   while (runSFXThread) {
      LightEvent_Wait(&startPlaybackEvent);

      if (!runSFXThread)
         break;

      playAudio(waveQueueFront);

      waveQueueFront = (waveQueueFront + 1) % MAX_WAVE_QUEUE_SIZE;
      waveQueueSize--;

      if (waveQueueSize == 0)
         LightEvent_Clear(&startPlaybackEvent);
   }
}


void sfx_init(void)
{
   ndspInit();

   flite_init();

   LightEvent_Init(&addSentenceEvent, RESET_ONESHOT);
   LightEvent_Init(&startPlaybackEvent, RESET_STICKY);

   for (int i = 0; i < MAX_WAVE_QUEUE_SIZE; i++) {
      LightEvent_Init(&waveReadyEvents[i], RESET_ONESHOT);
   }

   bool isN3DS;
   APT_CheckNew3DS(&isN3DS);

   if ((!isN3DS) || (envIsHomebrew()))
      APT_SetAppCpuTimeLimit(70); // working for .3dsx  / new3ds

//   s32 prio = 0;
//   svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);


   playbackThreadHandle  = threadCreate(playbackThread, NULL, (1 * 1024), 0x3d, (((isN3DS) && (!envIsHomebrew())) ? 2:1), false);

   textQueueThreadHandle = threadCreate(processTextQueue, NULL, (1 * 1024), 0x3e, -1, false);


   if (playbackThreadHandle == NULL)
      sys_error(0,"playbackThread failed to start.\n");

   if (textQueueThreadHandle == NULL)
      sys_error(0,"textQueueThread failed to start.\n");

}


void sfx_exit(void)
{
   runSFXThread = false;

   for (int i = 0; i < MAX_PROCESSING_THREADS; i++)
   {
      threadJoin(processingThreads[i], U64_MAX);
      threadFree(processingThreads[i]);
   }


   LightEvent_Signal(&addSentenceEvent);
   threadJoin(textQueueThreadHandle, U64_MAX);
   threadFree(textQueueThreadHandle);

   LightEvent_Signal(&startPlaybackEvent);
   threadJoin(playbackThreadHandle, U64_MAX);
   threadFree(playbackThreadHandle);

   LightEvent_Clear(&addSentenceEvent);
   LightEvent_Clear(&startPlaybackEvent);
   for (int i = 0; i < MAX_WAVE_QUEUE_SIZE; i++) {
      LightEvent_Clear(&waveReadyEvents[i]);
   }

   linearFree(samples);

   ndspExit();
}
