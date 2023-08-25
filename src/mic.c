
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <3ds.h>

#include <sndfile.h>
#include <curl/curl.h>


#include "mic.h"
#include "sys.h"


bool mic_recording;

u32 micbuf_size;
u32 micbuf_pos;
u8* micbuf;

u32 audiobuf_size;
u32 audiobuf_pos;
u8* audiobuf;
	
u32 micbuf_datasize;

SNDFILE* wavBuffer;
curl_mime* mime;



typedef struct
{
   sf_count_t offset, length;
   unsigned char data[0x100000];
} VIO_DATA ;

static VIO_DATA vio_data ;



void dump_data_to_file (const char *filename, const void *data, unsigned int datalen)
{
   FILE *file;

   if ((file = fopen (filename, "wb")) == NULL)
   {
      printf ("\n\nLine %d : could not open file : %s\n\n", __LINE__, filename);
      sys_error(0,"dump_data_to_file();\n\nError creating the file.\n");
	  return;
   };

   if (fwrite (data, 1, datalen, file) != datalen)
   {
      printf ("\n\nLine %d : fwrite failed.\n\n", __LINE__);
      sys_error(0,"Error creating the virtual file.\n");
   };

   fclose (file) ;
}


static sf_count_t vfget_filelen (void *user_data)
{
   VIO_DATA *vf = (VIO_DATA *) user_data;

   return vf->length;
}


static sf_count_t vfseek (sf_count_t offset, int whence, void *user_data)
{
   VIO_DATA *vf = (VIO_DATA *) user_data;

   switch (whence)
   {
      case SEEK_SET:
         vf->offset = offset;
         break;

      case SEEK_CUR:
         vf->offset = vf->offset + offset;
         break;

      case SEEK_END:
         vf->offset = vf->length + offset;
         break;
      default:
         break;
   };
   return vf->offset;
}


static sf_count_t vfread (void *ptr, sf_count_t count, void *user_data)
{
   VIO_DATA *vf = (VIO_DATA *) user_data;
   if (vf->offset + count > vf->length)
      count = vf->length - vf->offset;

   memcpy (ptr, vf->data + vf->offset, count);
   vf->offset += count;

   return count;
}


static sf_count_t vfwrite (const void *ptr, sf_count_t count, void *user_data)
{
   VIO_DATA *vf = (VIO_DATA *) user_data;

//   if (vf->offset >= SIGNED_SIZEOF (vf->data))
   if (vf->offset >= sizeof(vf->data))
      return 0;

//   if (vf->offset + count > SIGNED_SIZEOF (vf->data))
   if (vf->offset + count > sizeof(vf->data))
      count = sizeof (vf->data) - vf->offset;

   memcpy (vf->data + vf->offset, ptr, (size_t) count);
   vf->offset += count;

   if (vf->offset > vf->length)
      vf->length = vf->offset;

   return count;
}


static sf_count_t vftell (void *user_data)
{
   VIO_DATA *vf = (VIO_DATA *) user_data;

   return vf->offset;
}











// Function to create a .wav buffer object
SNDFILE* createWavBuffer(const short* buffer, int numSamples, int sampleRate)
{
    SF_VIRTUAL_IO virtualIO;
    memset(&virtualIO, 0, sizeof(SF_VIRTUAL_IO));

	virtualIO.get_filelen = vfget_filelen;
	virtualIO.seek  = vfseek;
	virtualIO.read  = vfread;
	virtualIO.write = vfwrite;
	virtualIO.tell  = vftell;


    memset(&vio_data, 0, sizeof(VIO_DATA));

	vio_data.length = 0;
	vio_data.offset = 0;
//	vio_data.length = audiobuf_pos ;

    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(SF_INFO));

    sfInfo.frames = numSamples;
    sfInfo.samplerate = sampleRate;
    sfInfo.channels = 1;  // Mono
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndFile = sf_open_virtual(&virtualIO, SFM_WRITE, &sfInfo, &vio_data);
    if (!sndFile) {
        printf("Error creating the virtual file.\n");
		sys_error(0,"Error creating the virtual file.\n");
        return NULL;
    }



    int writeCount = sf_write_short(sndFile, buffer, numSamples);

    if (writeCount != numSamples) {
        printf("Error writing the audio data to the virtual file.\n");
		sys_error(0,"Error writing the audio data to the virtual file.\n");
        sf_close(sndFile);
        return NULL;
    }
#if CONSOLE_ENABLE
printf("written %i samples\n",writeCount);
#endif

    sf_write_sync(sndFile);

/*sf_close(sndFile);


sndFile = sf_open_virtual(&virtualIO, SFM_READ, &sfInfo, &vio_data);
    if (!sndFile) {
        printf("Error reading the virtual file.\n");
		sys_error(0,"Error reading the virtual file.\n");
        return NULL;
    }
*/
    return sndFile;
}

void wav_buf(void)
{
	
int numSamples = audiobuf_pos;
	
   const short* buffer = (const short*)audiobuf;





//   int numSamples = audiobuf_pos;
   int sampleRate = 16360;

//   if (wavBuffer != NULL)
//      sf_close(wavBuffer);

   wavBuffer = createWavBuffer(buffer, numSamples, sampleRate);


vio_data.length = (vio_data.length-44)/2; // THIS NEEDS TO BE FIXED.. THIS CURRENTLY COVERS UP FOR MY FUCKUP..

   if (!wavBuffer)
      sys_error(0,"wavBuffer failed!\n");

//   dump_data_to_file ("/3ds/openai/tmp.wav", vio_data.data, (unsigned int) vio_data.length);
}



void curl_libsndfile(CURL *curl)
{
	/*
   const short *buffer = (const short*)audiobuf;

   int numSamples = audiobuf_pos;
   int sampleRate = 16360;

   wavBuffer = createWavBuffer(buffer, numSamples, sampleRate);

   if (!wavBuffer)
      sys_error(0,"wavBuffer failed!\n");
*/
wav_buf();

   if (curl) {
      mime = curl_mime_init(curl);

      curl_mimepart* part = curl_mime_addpart(mime);
      curl_mime_name(part, "model");
      curl_mime_data(part, "whisper-1",CURL_ZERO_TERMINATED);
/*
      dump_data_to_file ("tmp.wav", vio_data.data, (unsigned int) vio_data.length);
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "file");
      curl_mime_filedata(part, "tmp.wav");
*/
      part = curl_mime_addpart(mime);
      curl_mime_name(part, "file");
      curl_mime_data(part, (const char*)vio_data.data, (unsigned int) vio_data.length);
      curl_mime_filename(part, "huu.wav");

      part = curl_mime_addpart(mime);
      curl_mime_name(part, "response_format");
      curl_mime_data(part, "json",CURL_ZERO_TERMINATED);

      curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

   }
   return;
}


void curl_libsndfile_clean(void)
{
   curl_mime_free(mime);
   sf_close(wavBuffer);
   
//   free(&vio_data.data);
   
   mic_clean();
}



















/*

// Function declarations
void playWavFromFile(const char* filename);
void playWavFromMemory(const uint8_t* buffer, size_t bufferSize);

int main()
{
    // Initialize 3DS services
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    // Call the functions to play .wav files
    playWavFromFile("sdmc:/path/to/your/file.wav");

    // Replace this example buffer with your own .wav data if you want to play from memory
    uint8_t exampleWavData[] = {
        // Your PCM_16 formatted .wav data without header goes here
        // For example: { 0x00, 0x01, 0x02, ... }
    };
    size_t exampleBufferSize = sizeof(exampleWavData);
    playWavFromMemory(exampleWavData, exampleBufferSize);

    // Main loop
    while (aptMainLoop())
    {
        gspWaitForVBlank();
        hidScanInput();
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_START)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    // Cleanup
    gfxExit();
    return 0;
}

*/











void playWavFromMemory(const uint8_t* buffer, size_t bufferSize)
{
    // Initialize audio system
    ndspInit();

    // Configure and start playing the audio
    ndspWaveBuf waveBuf;
    memset(&waveBuf, 0, sizeof(ndspWaveBuf));
    waveBuf.data_vaddr = buffer;
    waveBuf.nsamples = bufferSize / sizeof(int16_t); // Assuming PCM_16 format

    ndspChnWaveBufAdd(0, &waveBuf);

    // Wait for audio playback to finish
    while (waveBuf.status != NDSP_WBUF_DONE)
    {
        svcSleepThread(1000000); // Sleep for 1 second
    }

    // Exit audio system
    ndspExit();
}

void playWavFromFileInMem(const char* filename) // THIS WORKS.. i think
{
/*
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error opening file: %s\n", filename);
        return;
    }
*/

 //   fseek(vio_data, 0, SEEK_END);
    size_t fileSize = vio_data.length;
//    fseek(vio_data, 0, SEEK_SET);

    // Assuming the wave header is 44 bytes, skip it if it's present
    const size_t headerSize = 44;
//    fseek(vio_data.data, headerSize, SEEK_SET);
	
#define FILE_IS_MONO 1 // enable or disable

#if FILE_IS_MONO
    // Allocate a buffer in linear memory to hold the data from the file
    uint8_t* buffer = (uint8_t*)linearAlloc(fileSize - headerSize);
    if (!buffer)
    {
        printf("Linear memory allocation error.\n");
//        fclose(file);
        return;
    }

    // Allocate a buffer in linear memory to hold the stereo data from the file
    uint8_t* stereoBuffer = (uint8_t*)linearAlloc(2 * (fileSize - headerSize));
    if (!stereoBuffer)
    {
        printf("Linear memory allocation error.\n");
//        fclose(file);
        return;
    }

    // Read the wave data into the mono buffer
//    fread(buffer, 1, fileSize - headerSize, &vio_data.data);

    // Convert mono data to stereo by duplicating the mono channel into both left and right channels
    for (size_t i = 0; i < (fileSize - headerSize) / sizeof(int16_t); ++i)
    {
        int16_t monoSample = ((int16_t*)vio_data.data)[i];
        ((int16_t*)stereoBuffer)[2 * i] = monoSample;
        ((int16_t*)stereoBuffer)[2 * i + 1] = monoSample;
    }

    // Close the file after reading
//    fclose(file);

    // Free the allocated linear memory
    linearFree(buffer);

    // Play the wave from the stereo buffer
    playWavFromMemory(stereoBuffer, 2 * (fileSize - headerSize));

    // Free the allocated linear memory
    linearFree(stereoBuffer);

#else
    // Allocate a buffer in linear memory to hold the data from the file
    uint8_t* buffer = (uint8_t*)linearAlloc(fileSize - headerSize);
    if (!buffer)
    {
        printf("Linear memory allocation error.\n");
//        fclose(file);
        return;
    }

    // Read the wave data into the buffer
//    fread(buffer, 1, fileSize - headerSize, file);

    // Close the file after reading
//    fclose(file);

    // Play the wave from the buffer
    playWavFromMemory(vio_data.data, vio_data.length);

    // Free the allocated linear memory
    linearFree(buffer);
#endif
}

void playWavFromFile(const char* filename) // THIS WORKS.. i think
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error opening file: %s\n", filename);
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Assuming the wave header is 44 bytes, skip it if it's present
    const size_t headerSize = 44;
    fseek(file, headerSize, SEEK_SET);
	
#define FILE_IS_MONO 1 // enable or disable

#if FILE_IS_MONO
    // Allocate a buffer in linear memory to hold the data from the file
    uint8_t* buffer = (uint8_t*)linearAlloc(fileSize - headerSize);
    if (!buffer)
    {
        printf("Linear memory allocation error.\n");
        fclose(file);
        return;
    }

    // Allocate a buffer in linear memory to hold the stereo data from the file
    uint8_t* stereoBuffer = (uint8_t*)linearAlloc(2 * (fileSize - headerSize));
    if (!stereoBuffer)
    {
        printf("Linear memory allocation error.\n");
        fclose(file);
        return;
    }

    // Read the wave data into the mono buffer
    fread(buffer, 1, fileSize - headerSize, file);

    // Convert mono data to stereo by duplicating the mono channel into both left and right channels
    for (size_t i = 0; i < (fileSize - headerSize) / sizeof(int16_t); ++i)
    {
        int16_t monoSample = ((int16_t*)buffer)[i];
        ((int16_t*)stereoBuffer)[2 * i] = monoSample;
        ((int16_t*)stereoBuffer)[2 * i + 1] = monoSample;
    }

    // Close the file after reading
    fclose(file);

    // Free the allocated linear memory
    linearFree(buffer);

    // Play the wave from the stereo buffer
    playWavFromMemory(stereoBuffer, 2 * (fileSize - headerSize));

    // Free the allocated linear memory
    linearFree(stereoBuffer);

#else
    // Allocate a buffer in linear memory to hold the data from the file
    uint8_t* buffer = (uint8_t*)linearAlloc(fileSize - headerSize);
    if (!buffer)
    {
        printf("Linear memory allocation error.\n");
        fclose(file);
        return;
    }

    // Read the wave data into the buffer
    fread(buffer, 1, fileSize - headerSize, file);

    // Close the file after reading
    fclose(file);

    // Play the wave from the buffer
    playWavFromMemory(buffer, fileSize - headerSize);

    // Free the allocated linear memory
    linearFree(buffer);
#endif
}

void play_wav_buf(void)
{


#if CONSOLE_ENABLE
printf("play_wav_buf\n");
printf("vio_data.length = %lli\naudiobuf_pos = %li\n",vio_data.length,audiobuf_pos);
#endif
//playWavFromMemory(vio_data.data, vio_data.length);
playWavFromFileInMem("huu.wav");


}


void play_wav_file(void)
{
#if CONSOLE_ENABLE
printf("play_wav_file\n");
printf("vio_data.length = %lli\naudiobuf_pos = %li\n",vio_data.length,audiobuf_pos);
#endif


playWavFromFile("/3ds/openai/tmp.wav");



}







			void MIC_record_play(void)
			{
				if(R_SUCCEEDED(GSPGPU_FlushDataCache(audiobuf, audiobuf_pos)) &&
				      R_SUCCEEDED(csndPlaySound(0x8, SOUND_ONE_SHOT | SOUND_FORMAT_16BIT, 16360, 1.0, 0.0, (u32*)audiobuf, NULL, audiobuf_pos)))
				   printf("Now playing.\n");

			}


			void MIC_record_stop(void)
			{
				if(R_FAILED(MICU_StopSampling()))
				{
					printf("Failed to stop sampling.\n");
					sys_error(1,"Failed to stop sampling.\n");
				}
				MIC_record_frame();
				
				printf("recorded %li samples.\n",audiobuf_pos);
				
				mic_recording = false;
			}


			void MIC_record_frame(void)
			{
			if(audiobuf_pos < audiobuf_size)
			{
				u32 micbuf_readpos = micbuf_pos;
				micbuf_pos = micGetLastSampleOffset();
				while(audiobuf_pos < audiobuf_size && micbuf_readpos != micbuf_pos)
				{
					audiobuf[audiobuf_pos] = micbuf[micbuf_readpos];
					audiobuf_pos++;
					micbuf_readpos = (micbuf_readpos + 1) % micbuf_datasize;
				}
			}
			}
			
			void MIC_record_start(void)
			{
				audiobuf_pos = 0;
				micbuf_pos = 0;

//				printf("Stopping audio playback...\n");
//				CSND_SetPlayState(0x8, 0);
//				if(R_FAILED(CSND_UpdateInfo(0))) printf("Failed to stop audio playback.\n");

//				printf("Starting sampling...\n");
				if(R_SUCCEEDED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micbuf_datasize, true)))
				{
				// printf("Now recording.\n");
				mic_recording = true;
				}
//				else printf("Failed to start sampling.\n");



			}



void mic_clean(void)
{
//   audiobuf[0] = '\0';
//   micbuf[0] = '\0';
//   audiobuf_pos = 0;
//   memset(&audiobuf, '\0', sizeof(audiobuf)); // crash..

//   micbuf_pos = 0;
//   memset(&micbuf, '\0', sizeof(micbuf_size)); // crash..
}

void mic_init(void)
{
//	gfxInitDefault();
//	consoleInit(GFX_BOTTOM, NULL);

//	bool initialized = true;

	micbuf_size = 0x30000;
	micbuf_pos = 0;
	micbuf = memalign(0x1000, micbuf_size);

//	printf("Initializing CSND...\n");
//	if(R_FAILED(csndInit()))
//	{
//		initialized = false;
//		printf("Could not initialize CSND.\n");
//	} else printf("CSND initialized.\n");

//	printf("Initializing MIC...\n");
	if(R_FAILED(micInit(micbuf, micbuf_size)))
	{
//		initialized = false;
		printf("Could not initialize MIC.\n");
//	} else printf("MIC initialized.\n");
}
	micbuf_datasize = micGetSampleDataSize();

	audiobuf_size = 0x80000; // 0x100000 = ~30sec, 0x80000 = ~15sec
	audiobuf_pos = 0;
	audiobuf = linearAlloc(audiobuf_size);

mic_recording = false;

csndInit();
}





/*
Gets an event handle triggered when the shared memory buffer is full. 

MICU_GetEventHandle()
Result MICU_GetEventHandle 	( 	Handle *  	handle	) 	


*/



void mic_exit(void)
{

	linearFree(audiobuf);

	micExit();
	free(micbuf);

	csndExit();
//	gfxExit();
}

