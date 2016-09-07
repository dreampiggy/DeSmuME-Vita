/* main.c - this file is part of DeSmuME
*
* Copyright (C) 2006,2007 DeSmuME Team
* Copyright (C) 2007 Pascal Giard (evilynux)
* Copyright (C) 2009 Yoshihiro (DsonPSP)
* This file is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This file is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/display.h>
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>

#include "video.h"
#include "input.h"
#include "menu.h"
#include "sound.h"
#include "config.h"

#include <stdio.h>
#include <malloc.h>

#include "../MMU.h"
#include "../NDSSystem.h"
#include "../debug.h"
#include "../render3D.h"
#include "../rasterize.h"
#include "../saves.h"
#include "../mic.h"
#include "../SPU.h"


#define FRAMESKIP 1

volatile bool execute = FALSE;

GPU3DInterface *core3DList[] = {
	&gpu3DNull,
	&gpu3DRasterize,
	NULL
};

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
  &SNDDummy,
  &SNDVITA,
  NULL
};

const char * save_type_names[] = {
	"Autodetect",
	"EEPROM 4kbit",
	"EEPROM 64kbit",
	"EEPROM 512kbit",
	"FRAM 256kbit",
	"FLASH 2mbit",
	"FLASH 4mbit",
	NULL
};

static void desmume_cycle()
{
	input_UpdateKeypad();
	input_UpdateTouch();

    NDS_exec<false>();

    if(UserConfiguration.soundEnabled)
    	SPU_Emulate_user();
}

extern "C" {
	int scePowerSetArmClockFrequency(int freq);
	int scePowerSetBusClockFrequency(int freq);
	int scePowerSetGpuClockFrequency(int freq);
}

#define FPS_CALC_INTERVAL 1000000

static unsigned int frames = 0;

static inline void calc_fps(char fps_str[32])
{
	static SceKernelSysClock old = 0;
	SceKernelSysClock now;
	SceKernelSysClock diff;
	float fps;

	sceKernelGetProcessTime(&now);
	diff = now - old;

	if (diff >= FPS_CALC_INTERVAL) {
		fps = frames / ((diff/1000)/1000.0f);
		sprintf(fps_str, "FPS: %.2f", fps);
		frames = 0;
		sceKernelGetProcessTime(&old);
	}
}


int main()
{
	char fps_str[32] = {0};

	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

	video_Init();

	char *filename = menu_FileBrowser();

	if(!filename) {
		video_BeginDrawing();
		vita2d_pgf_draw_text(video_font,0,20,RGBA8(0,255,0,255),1.0f,"ROM list is empty. Please upload *.nds to ux0:/data/desmume.");
		vita2d_pgf_draw_text(video_font,0,40,RGBA8(0,255,0,255),1.0f,"Program will exit after 6s...");
		video_EndDrawing();

		sceKernelDelayThread(6000000);
		video_Exit();
		sceKernelExitProcess(0);
		return 0;
	}

	struct NDS_fw_config_data fw_config;
	NDS_FillDefaultFirmwareConfigData(&fw_config);
  	NDS_Init();
	NDS_3D_ChangeCore(1);
	backup_setManualBackupType(0);

	if(UserConfiguration.jitEnabled){
		CommonSettings.use_jit = true;
		CommonSettings.jit_max_block_size = 60; // Some games can be higher but let's not make it even more unstable
	}

	if (NDS_LoadROM(filename) < 0) {
		goto exit;
	}

	execute = true;

	int i;

	if(UserConfiguration.soundEnabled)
		SPU_ChangeSoundCore(SNDCORE_VITA, 735 * 4);

	while (execute) {

		for (i = 0; i < UserConfiguration.frameSkip; i++) {
			NDS_SkipNextFrame();
			desmume_cycle();
			if (UserConfiguration.fpsCounterEnabled)
				frames++;
		}

		desmume_cycle();
		video_BeginDrawing();
		video_DrawFrame();
		if (UserConfiguration.fpsCounterEnabled) {
			frames++;
			calc_fps(fps_str);
			vita2d_pgf_draw_text(video_font, 10, 30, RGBA8(255, 255, 255, 255),1.0f, fps_str);
		}
		video_EndDrawing();

	}

exit:
	video_Exit();

	sceKernelExitProcess(0);
	return 0;
}