/*
 * CerealBox - Copyright 2015 Ali Motisi. All rights reserved.
*/

#include "../../common/src/oogame.h"
#include "../../extras/graphics/fggraphics.h"

#include <string.h>
#include <math.h>


#define MX_MEMORY_MAX 50000000

#define OO_GAME_MIN_WIDTH  320
#define OO_GAME_MIN_HEIGHT 180
#define OO_GAME_MAX_WIDTH  320
#define OO_GAME_MAX_HEIGHT 180

#define GRAVITY_CONSTANT 10000


typedef struct {
	oofloat x;
	oofloat y;
	oofloat velx;
	oofloat vely;
    ooint mass;
} oodot;

#define OO_MAX_DOTS 10

typedef struct {
	oobool initialised;
	oofloat dt;

	OOGameInput  input;
	OOGameOutput* output;

	oofloat tSine;

	FGGraphicBuffer gbuffer;

	ooint memoryUsed;
	oobyte memory[MX_MEMORY_MAX];

	oodot dots[OO_MAX_DOTS];
	ooint numOfDots;

	// oodot playerDot;
} oog;


void addDot(oog* g, oofloat x, oofloat y, oofloat velx, oofloat vely, ooint mass) {

	if( g->numOfDots<OO_MAX_DOTS ) {
		oodot* dot = &g->dots[g->numOfDots];
		dot->x = x;
		dot->y = y;
		dot->velx = velx;
		dot->vely = vely;
        dot->mass = mass;

		g->numOfDots++;
	}


}

void initGame(oog* g) {
	g->initialised = ootrue;
	
	//addDot(g, OO_GAME_MIN_WIDTH/4.0f, OO_GAME_MIN_HEIGHT/4.0f, 40.0f, -50.0f, 50);
	addDot(g, 0.0f, OO_GAME_MIN_HEIGHT/8.0f, 40.0f, 0.0f, 5);
	addDot(g, OO_GAME_MIN_WIDTH/2.0f, OO_GAME_MIN_HEIGHT/2.0f - 25.0f, -20.0f, 0.0f, 10);
	addDot(g, OO_GAME_MIN_WIDTH/2.0f - 50.0f, OO_GAME_MIN_HEIGHT/6.0f, -30.0f, -80.0f, 8);
	

	// g->playerDot.x = OO_GAME_MIN_WIDTH/2.0f;
	// g->playerDot.y = OO_GAME_MIN_HEIGHT/2.0f;

	//g->dot.x = OO_GAME_MIN_WIDTH/2.0f;
	//g->dot.y = OO_GAME_MIN_HEIGHT/2.0f;
	//g->dot.velx = 100.0f;
	//g->dot.vely =  50.0f;
}



void outputSine(oog* g, OOGameInput* input, OOGameOutput* output) {
	ooshort toneVolume = 4000;
	//oofloat freq = 440 + (game->p1.x-game->p1.y) * 5;
	oofloat freq = 440.f+(fabsf(input->mouse.x));


	ooint period = (ooint) (44100.f/freq);
	ooshort value;
	oouint k = 0;

	for(oouint i=0; i<output->audio.framesToWrite; i++) {
		value = (ooshort)(sinf(g->tSine)*toneVolume);

		output->audio.buffer[k++] = value;
		output->audio.buffer[k++] = value;

		g->tSine += (2.f*M_PIf)/period;
		if( g->tSine>2.f*M_PIf ) {
			g->tSine -= 2.f*M_PIf;
		}
	}

}

void clearScreen(oog* g) {
	memset(g->output->graphic.buffer, 0, g->output->graphic.width*g->output->graphic.height*sizeof(oouint));
}


OOGameSetupInfo getGameSetupInfo(ooint windowWidth, ooint windowHeight) {
	OOGameSetupInfo info = {0};
	info.memorySize = sizeof(oog);

#ifdef OO_LIVE_CODING
	info.memorySize += 10000; // room for extra data
#endif
	return info;
}


void computeGraphicsOutputSize(oog* g) {
    
    //g->output->graphic.width = g->output->graphic.originalWidth;
    //g->output->graphic.height = g->output->graphic.originalHeight;
    //g->output->graphic.renderWidth = g->output->graphic.width;
    //g->output->graphic.renderHeight = g->output->graphic.height;
    
	ooint w = OO_GAME_MIN_WIDTH;
	ooint h = OO_GAME_MIN_HEIGHT;
	ooint scale = 1;
	
	oo_computePixelPerfectSizeAndScale(g->output->graphic.originalWidth, g->output->graphic.originalHeight,
		OO_GAME_MIN_WIDTH, OO_GAME_MIN_HEIGHT,
		OO_GAME_MAX_WIDTH, OO_GAME_MAX_HEIGHT, &w, &h, &scale);
	
	g->output->graphic.width = w;
	g->output->graphic.height = h;
	
	g->output->graphic.renderWidth = (oofloat)g->output->graphic.width*scale;
	g->output->graphic.renderHeight = (oofloat)g->output->graphic.height*scale;
	
	g->output->graphic.leftPadding = floorf((g->output->graphic.originalWidth - g->output->graphic.renderWidth) / 2.f);
	g->output->graphic.topPadding = floorf((g->output->graphic.originalHeight - g->output->graphic.renderHeight) / 2.f);
     
}

ooint iroundf(oofloat v) {
	return (ooint) roundf(v);
}


void loadSprite(oog* g, FGSprite* sprite, oochar* spritePath) {
	OOFile file = ooOpenAndReadFile(spritePath, ootrue);
	ooAssert(file.size>0);

	g->memoryUsed += fg_loadSpriteFromMemory(file.data, sprite, g->memory+g->memoryUsed, MX_MEMORY_MAX-g->memoryUsed);

	if(file.data) {
		ooCloseFile(&file);
	}
}


void uadDot(oog* g, oodot* dot) {
    oofloat forceX = 0.0f;
    oofloat forceY = 0.0f;

	oodot* otherDot;
    for (int i = 0; i < g->numOfDots; i++) {
        otherDot = &g->dots[i];
		if (otherDot != dot) {
			oofloat dx = otherDot->x - dot->x;
			oofloat dy = otherDot->y - dot->y;
			oofloat distanceSquared = dx * dx + dy * dy;

			if (distanceSquared < 50.0f){
				distanceSquared = 50.0f;
			}

			oofloat distance = sqrtf(distanceSquared);

			oofloat forceMagnitude = GRAVITY_CONSTANT * dot->mass * otherDot->mass / distanceSquared;

			forceX += (dx / distance) * (forceMagnitude);
			forceY += (dy / distance) * (forceMagnitude);

			dot->velx += (forceX / dot->mass) * g->dt * 0.7f;
			dot->vely += (forceY / dot->mass) * g->dt * 0.7f;
			
        }
    }

	// update x
	oofloat prevx = dot->x;
	dot->x += g->dt * dot->velx;
	if( dot->x < 0.0f || dot->x >= (g->gbuffer.width - 1.0f) ) {
		dot->x = prevx;
		dot->velx *= -1.0f;
	}

	// update y
	oofloat prevy = dot-> y;
	dot -> y += g->dt * dot-> vely;
	if( dot->y < 0.0f || dot->y >= (g->gbuffer.height - 1.0f) ) {
		dot->y = prevy;
		dot -> vely *= -1.0f;
	}

	ooint x = iroundf(dot->x);
	ooint y = iroundf(dot->y);

	g->gbuffer.buffer[y*g->gbuffer.stride+x] = fg_colourWithBytes(255, 255, 0);
}

void updateGraphics(void* gameMemory, OOGameInput* input, OOGameOutput* output) {
	oog * g = (oog*) gameMemory;

	g->input = *input;
	g->output = output;

	g->dt = g->input.dt;

	computeGraphicsOutputSize(g);

	g->gbuffer.buffer = output->graphic.buffer;
	g->gbuffer.stride = output->graphic.width;
	g->gbuffer.width  = output->graphic.width;
	g->gbuffer.height = output->graphic.height;

	if( !g->initialised ) {
		initGame(g);
	}

	clearScreen(g);

	oodot* dot;
	for(int d=0; d<g->numOfDots; d++){
		dot = &g->dots[d];
		uadDot(g, dot);
	}


	// oodot* dot = &g->playerDot;

	// dot->velx = 0.0f;
	// dot->vely = 0.0f;

	// oofloat speed = 120.0f;

	// if( g->input.keyboard['D'] ) {
	// 	dot->velx = speed;
	// } else if( g->input.keyboard['A'] ) {
	// 	dot->velx = -speed;
	// }

	// if( g->input.keyboard['S'] ) {
	// 	dot->vely = speed;
	// } else if( g->input.keyboard['W'] ) {
	// 	dot->vely = -speed;
	// }

	// uadDot(g, dot);


	// ooint x = iroundf(g->input.mouse.x);
	// ooint y = iroundf(g->input.mouse.y);

	// g->gbuffer.buffer[y*g->gbuffer.stride+x] = fg_colourWithBytes(255, 255, 255);

}

void updateAudio(void * gameMemory, OOGameInput * input, OOGameOutput * output) {
	oog * g = (oog*) gameMemory;
	//outputSine(g, input, output);
}
