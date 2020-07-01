#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

// Analog camera movement by Pathétique (github.com/vrmiguel), y0shin and Mors
// Contribute or communicate bugs at github.com/vrmiguel/sm64-analog-camera

#include <ultra64.h>

#include "controller_api.h"

#include "../configfile.h"

extern int16_t rightx;
extern int16_t righty;

#ifdef BETTERCAMERA
int mouse_x;
int mouse_y;

extern u8 newcam_mouse;
#endif

static bool init_ok;
static SDL_GameController *sdl_cntrl;

static SDL_Joystick *joy = NULL;	

enum{
		OGA_BUTTON_B=0,
		OGA_BUTTON_A,
		OGA_BUTTON_X,
		OGA_BUTTON_Y,
		OGA_BUTTON_L,
		OGA_BUTTON_R,
		OGA_BUTTON_UP,
		OGA_BUTTON_DOWN,
		OGA_BUTTON_LEFT,
		OGA_BUTTON_RIGHT,
		OGA_BUTTON_F1,
		OGA_BUTTON_F2,
		OGA_BUTTON_F3,
		OGA_BUTTON_F4,
		OGA_BUTTON_F5,
		OGA_BUTTON_F6
};

static void controller_sdl_init(void) {
    if (SDL_Init(/*SDL_INIT_GAMECONTROLLER | */ SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return;
    }
	
	// Initialize the joystick subsystem
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	if (SDL_NumJoysticks() > 0)
	{	
		SDL_JoystickEventState(SDL_ENABLE);
		joy = SDL_JoystickOpen(0);
		
		if (joy) {
			printf("Opened Joystick 0\n");
			printf("Name: %s\n", SDL_JoystickNameForIndex(0));
			printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
			printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
			printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
		} else {
			printf("Couldn't open Joystick 0\n");
		}
	}
		
	
#ifdef BETTERCAMERA
    if (newcam_mouse == 1)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
#endif

    init_ok = true;
}

static void controller_sdl_read(OSContPad *pad) {
    if (!init_ok) {
        return;
    }

#ifdef BETTERCAMERA
    if (newcam_mouse == 1)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);
    
    const u32 mbuttons = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    
    if (configMouseA && (mbuttons & SDL_BUTTON(configMouseA))) pad->button |= A_BUTTON;
    if (configMouseB && (mbuttons & SDL_BUTTON(configMouseB))) pad->button |= B_BUTTON;
    if (configMouseL && (mbuttons & SDL_BUTTON(configMouseL))) pad->button |= L_TRIG;
    if (configMouseR && (mbuttons & SDL_BUTTON(configMouseR))) pad->button |= R_TRIG;
    if (configMouseZ && (mbuttons & SDL_BUTTON(configMouseZ))) pad->button |= Z_TRIG;
#endif




#if 1
	SDL_JoystickUpdate(); 

	if (SDL_JoystickGetButton(joy, OGA_BUTTON_F4)) pad->button |= START_BUTTON;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_L))     pad->button |= Z_TRIG;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_F3))     pad->button |= L_TRIG;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_R))     pad->button |= R_TRIG;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_B))     pad->button |= A_BUTTON;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_Y))     pad->button |= B_BUTTON;

	if (SDL_JoystickGetButton(joy, OGA_BUTTON_X)) pad->button |= L_CBUTTONS;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_F6)) pad->button |= R_CBUTTONS;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_F5)) pad->button |= U_CBUTTONS;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_A)) pad->button |= D_CBUTTONS;


	if (SDL_JoystickGetButton(joy, OGA_BUTTON_LEFT)) pad->button |= L_JPAD;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_RIGHT)) pad->button |= R_JPAD;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_UP)) pad->button |= U_JPAD;
	if (SDL_JoystickGetButton(joy, OGA_BUTTON_DOWN)) pad->button |= D_JPAD;

	int16_t leftx = SDL_JoystickGetAxis(joy, 0);
	int16_t lefty = SDL_JoystickGetAxis(joy, 1);

	uint32_t magnitude_sq = (uint32_t)(leftx * leftx) + (uint32_t)(lefty * lefty);
	if (magnitude_sq > (uint32_t)(DEADZONE * DEADZONE)) {
		pad->stick_x = leftx / 0x100;
		int stick_y = -lefty / 0x100;
		pad->stick_y = stick_y == 128 ? 127 : stick_y;
	}
#else	
    SDL_GameControllerUpdate();

	

    if (sdl_cntrl != NULL && !SDL_GameControllerGetAttached(sdl_cntrl)) {
        SDL_GameControllerClose(sdl_cntrl);
        sdl_cntrl = NULL;
    }
    if (sdl_cntrl == NULL) {
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
			const char *name = SDL_GameControllerNameForIndex(i);
			if (name) {
				printf("Joystick %i has game controller name '%s'\n", i, name);
			} else {
				printf("Joystick %i has no game controller name.\n", i);
			}			
            if (SDL_IsGameController(i)) {
                sdl_cntrl = SDL_GameControllerOpen(i);
                if (sdl_cntrl != NULL) {
					printf("[trngaje] success\n");
                    break;
                }
            }
        }
        if (sdl_cntrl == NULL) {
			printf("[trngaje] sdl_cntrl is NULL\n");
            return;
        }
    }

    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyStart)) pad->button |= START_BUTTON;
    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyZ))     pad->button |= Z_TRIG;
    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyL))     pad->button |= L_TRIG;
    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyR))     pad->button |= R_TRIG;
    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyA))     pad->button |= A_BUTTON;
    if (SDL_GameControllerGetButton(sdl_cntrl, configJoyB))     pad->button |= B_BUTTON;

    int16_t leftx = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_LEFTX);
    int16_t lefty = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_LEFTY);
    rightx = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_RIGHTX);
    righty = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_RIGHTY);

    int16_t ltrig = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    int16_t rtrig = SDL_GameControllerGetAxis(sdl_cntrl, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

#ifdef TARGET_WEB
    // Firefox has a bug: https://bugzilla.mozilla.org/show_bug.cgi?id=1606562
    // It sets down y to 32768.0f / 32767.0f, which is greater than the allowed 1.0f,
    // which SDL then converts to a int16_t by multiplying by 32767.0f, which overflows into -32768.
    // Maximum up will hence never become -32768 with the current version of SDL2,
    // so this workaround should be safe in compliant browsers.
    if (lefty == -32768) {
        lefty = 32767;
    }
    if (righty == -32768) {
        righty = 32767;
    }
#endif

    if (rightx < -0x4000) pad->button |= L_CBUTTONS;
    if (rightx > 0x4000) pad->button |= R_CBUTTONS;
    if (righty < -0x4000) pad->button |= U_CBUTTONS;
    if (righty > 0x4000) pad->button |= D_CBUTTONS;

    if (ltrig > 30 * 256) pad->button |= Z_TRIG;
    if (rtrig > 30 * 256) pad->button |= R_TRIG;

    uint32_t magnitude_sq = (uint32_t)(leftx * leftx) + (uint32_t)(lefty * lefty);
    if (magnitude_sq > (uint32_t)(DEADZONE * DEADZONE)) {
        pad->stick_x = leftx / 0x100;
        int stick_y = -lefty / 0x100;
        pad->stick_y = stick_y == 128 ? 127 : stick_y;
    }
	
	}
#endif
}

struct ControllerAPI controller_sdl = {
    controller_sdl_init,
    controller_sdl_read
};