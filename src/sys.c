#include <3ds.h>

#include "sfx.h"
#include "gfx.h"
#include "main.h"
#include "menu.h"
#include "sys.h"
#include "util.h"
#include "json.h"
#include "curl.h"
#include "mic.h"

aptHookCookie cookie;

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;


void aptHookFunc(APT_HookType hookType, void *param)
{
	switch (hookType) {
		case APTHOOK_ONEXIT:
//			running=false;
			break;
		case APTHOOK_ONSUSPEND:
		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			break;
		default:
			break;
	}
}



void sys_error(bool fatal, const char* error)
{
   if (!gspHasGpuRight())
      gfxInitDefault();

   errorConf msg;
   errorInit(&msg, ERROR_TEXT, CFG_LANGUAGE_EN);
   errorText(&msg, error);
   errorDisp(&msg);

   if (fatal)
      exit(0);

   return;
}

int sys_swkbd(const char *hintText, const char *inText, char *outText)
{
   SwkbdState swkbd;
   static SwkbdStatusData swkbdStatus;
   static SwkbdLearningData swkbdLearning;
   char mybuf[128];

//   swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 3, -1);
   swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
   swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
   swkbdSetInitialText(&swkbd, inText);
   swkbdSetHintText(&swkbd, hintText);
   swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Close", false);
//   swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, "Auto", false); // disabled for now..
   swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Send", true);
   swkbdSetFeatures(&swkbd,
         SWKBD_PREDICTIVE_INPUT | SWKBD_ALLOW_HOME | SWKBD_ALLOW_RESET | SWKBD_ALLOW_POWER);

   static bool reload = false;
   swkbdSetStatusData(&swkbd, &swkbdStatus, reload, true);
   swkbdSetLearningData(&swkbd, &swkbdLearning, reload, true);
   reload = true;

   SwkbdButton button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));

   if (button == SWKBD_BUTTON_LEFT)
      return -1;

   else if (button == SWKBD_BUTTON_MIDDLE)
      return 0;

   else if (button == SWKBD_BUTTON_RIGHT)
   {
      strcpy(outText, mybuf);
      return 1;
   }

   printf("swkbd event: %d\n", swkbdGetResult(&swkbd));
   return -2;
}










void sys_init()
{
osSetSpeedupEnable(true);

   int ret;
   aptHook(&cookie, aptHookFunc, NULL);
   gfxInitDefault();
#ifdef CONSOLE_ENABLE
   consoleInit(GFX_TOP, NULL);
#endif
   romfsInit();



   SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
   if(SOC_buffer == NULL) {
      sys_error(1,"SOC_buffer:\nmemalign() failed");
   }

   if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
      char error[64];
      sprintf(error,"socInit failed: 0x%08X\n", (unsigned int)ret);
      sys_error(1,error);
   }

   curl_init();

   mic_init();
   sfx_init();
   gfx_init();
   menu_init();
}

void sys_exit()
{
   mic_exit();
   sfx_exit();
   gfx_exit();
   menu_exit();
   curl_exit();

json_exit(); //cleanup

   for (int i = 0; i < message_count; i++) {
      free(msg_history[i].role);
      free(msg_history[i].content);
   }

   romfsExit();

   socExit();
   free(SOC_buffer);

   gfxExit();
   exit(0); // todo fix this..
}
