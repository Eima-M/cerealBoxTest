/*
 * CerealBox - Copyright 2015 Ali Motisi. All rights reserved.
*/

#include "../../../common/src/oogame.h"
#include "../../../extras/graphics/fggraphics.h"

#include <string.h>
#include <math.h>

/* 
left mouse - attach and detach string
W - shorten string
*/

#define MX_MEMORY_MAX 50000000

#define OO_GAME_MIN_WIDTH  640
#define OO_GAME_MIN_HEIGHT 360
#define OO_GAME_MAX_WIDTH  640
#define OO_GAME_MAX_HEIGHT 360

#define OO_GAME_TILESIZE 10

// ADJUST: physics constants
#define OO_GRAVITATIONAL_CONST 200
#define OO_FRICTION_CONST 300

#define OO_FROG_OFFSET 14.0f

#define OO_MAX_BLOCKS 10000
#define OO_MAX_GOAL_BLOCKS 10

#define OO_NUM_OF_SCREENS 3

#define OO_TEMP_STRING_MAX 2048

// stage design
const char STAGE[OO_NUM_OF_SCREENS][(OO_GAME_MAX_WIDTH / OO_GAME_TILESIZE + 1) * (OO_GAME_MAX_HEIGHT / OO_GAME_TILESIZE)] = {
"\
__________#####_____________####________#############___________*\
__________#####_____________####________#############___________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
__________________________________________________________######*\
________________________________________________________________*\
________________________________________________________________*\
_____________##########_________________________________________*\
_____________##########_________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
_________________________________##########_____________________*\
####################_____________##########_____________________*\
___________________#____________________________________________*\
___________________#____________________________________________*\
___________________#____________________________________________*\
___________________#____________________________________________*\
___________________#____________________________________________*\
___________________#____________________________________________*\
___________________################_____________________________*\
__________________________________#_____________________________*\
__________________________________#_____________________________*\
__________________________________#_____________________________*\
__________________________________#_____________________________*\
__________________________________#_____________________________*\
__________________________________#_____________________________*\
################################################################*\
",
"\
__________________________________________________#_____________*\
__________________________________________________#_____________*\
__________________________________________________#_____________*\
__________________________________________________#_____________*\
__________________________________________________#_____________*\
__________________________________________________#____________#*\
_____________________________##________________________________#*\
_______________________________________________________________#*\
_______________________________________________________________#*\
_______________________________________________________________#*\
_______________________________________________________________#*\
_________###____________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________###_____________________*\
________________________________________###_____________________*\
________________________________________###_____________________*\
_________________####_________________________________________##*\
___________________##_________________________________________##*\
___________________##_________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
####__________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
______________________________________________________________##*\
________________________________________#############___________*\
",
"\
________________________######________#####______##_____________*\
_____________#__________________________________________________*\
_____________#__________________________________________________*\
_____________#__________________________________________________*\
_____________#__________________________________________________*\
_____________#__________________________________________________*\
__________________________#_____________________________________*\
__________________________#_____________________________________*\
__________________________#_____________________________________*\
__________________________#_____________________________________*\
__________________________#_______________________@@@___________*\
________________#_________________________________@@@___________*\
________________#_________________________________@@@___________*\
________________#_______________________________________________*\
_______________________#________________________________________*\
_______________________#________________________________________*\
_______________________#________________________________________*\
_______________________#________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
___________#____________________________________________________*\
___________#______________________________#####_________________*\
___________#____________________________________________________*\
___________#____________________________________________________*\
___________#____________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
________________________________________________________________*\
____________________________________________________________#___*\
____________________________________________________________#___*\
____________________________________________________________#___*\
____________________________________________________________#___*\
____________________________________________________________#___*\
____________________________________________________________#___*\
________________________________________________________________*\
________________________________________________________________*\
"
};

//"\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//________________________________________________________________*\
//"

typedef struct {
	oofloat x;
	oofloat y;
	oofloat prevX;
	oofloat prevY;
	oofloat velX;
	oofloat velY;
	oobool grounded;
	oobool goalReached;
} Player;

typedef struct {
	oofloat x;
	oofloat y;
	oofloat prevX;
	oofloat prevY;
	oobool active;
} Pivot;

typedef struct {
	oofloat angle;
	oofloat angleV;
	oofloat angleA;
	oofloat length;
	oofloat prevLength;
	oobool attached;
	oobool swingAble;
} String;

typedef struct {
	oofloat x;
	oofloat y;
	oofloat xInput;
	oofloat yInput;
} Cursor;

typedef struct {
	oofloat x;
	oofloat y;
} Block;

typedef struct {
	oofloat x;
	oofloat y;
} GoalBlock;

typedef struct {
	oofloat y;
} Camera;

typedef struct {
	oobool initialised;
	oofloat dt;

	OOGameInput input;
	OOGameInput previousInput;
	OOGameOutput* output;

	oofloat time;
	oofloat tSine;

	FGGraphicBuffer gbuffer;

	ooint memoryUsed;
	oobyte memory[MX_MEMORY_MAX];

	FGSprite backgroundSprite;
	FGSprite ballSprite;
	FGSprite pivotSprite;
	FGSprite yippieSprite;

	Camera camera;

	Player player;

	Cursor cursor;

	Pivot pivot;

	String string;
	oochar tempString[OO_TEMP_STRING_MAX];

	Block blocks[OO_MAX_BLOCKS];
	ooint numOfBlocks;

	GoalBlock goalBlocks[OO_MAX_GOAL_BLOCKS];
	ooint numOfGoalBlocks;
} oog;

void clearScreen(oog* g) {
	memset(g->output->graphic.buffer, 0, g->output->graphic.width * g->output->graphic.height * sizeof(oouint));
}

OOGameSetupInfo getGameSetupInfo(ooint windowWidth, ooint windowHeight) {
	OOGameSetupInfo info = { 0 };
	info.memorySize = sizeof(oog);

#ifdef OO_LIVE_CODING
	info.memorySize += 10000; // room for extra data
#endif
	return info;
}

void computeGraphicsOutputSize(oog* g) {
	ooint w = OO_GAME_MIN_WIDTH;
	ooint h = OO_GAME_MIN_HEIGHT;
	ooint scale = 1;

	oo_computePixelPerfectSizeAndScale(g->output->graphic.originalWidth, g->output->graphic.originalHeight,
		OO_GAME_MIN_WIDTH, OO_GAME_MIN_HEIGHT,
		OO_GAME_MAX_WIDTH, OO_GAME_MAX_HEIGHT, &w, &h, &scale);

	g->output->graphic.width = w;
	g->output->graphic.height = h;

	g->output->graphic.renderWidth = (oofloat)g->output->graphic.width * scale;
	g->output->graphic.renderHeight = (oofloat)g->output->graphic.height * scale;

	g->output->graphic.leftPadding = floorf((g->output->graphic.originalWidth - g->output->graphic.renderWidth) / 2.f);
	g->output->graphic.topPadding = floorf((g->output->graphic.originalHeight - g->output->graphic.renderHeight) / 2.f);

}

void loadSprite(oog* g, FGSprite* sprite, oochar* spritePath) {

#ifdef OO_PLATFORM_WINDOWS
	oo_strcpy_s(g->tempString, OO_TEMP_STRING_MAX, "../../games/samplegame/res/");
#else
	oo_strcpy_s(g->tempString, OO_TEMP_STRING_MAX, "../../../games/samplegame/res/");
#endif

	oo_strcat_s(g->tempString, OO_TEMP_STRING_MAX, spritePath);

	OOFile file = ooOpenAndReadFile(g->tempString, ootrue);
	ooAssert(file.size > 0);

	g->memoryUsed += fg_loadSpriteFromMemory(file.data, sprite, g->memory + g->memoryUsed, MX_MEMORY_MAX - g->memoryUsed);

	if (file.data) {
		ooCloseFile(&file);
	}
}

void addPlayer(oog* g, oofloat x, oofloat y, oofloat velX, oofloat velY) {
	Player* player = &g->player;
	player->x = x;
	player->y = y;
	player->velX = velX;
	player->velY = velY;
	player->prevX = x;
	player->prevY = y;
	player->grounded = oofalse;
	player->goalReached = oofalse;
}

// highkey not being used since string controls pivot - UNTIL NOW
void addPivot(oog* g, oofloat x, oofloat y, oofloat prevX, oofloat prevY) {
	Pivot* pivot = &g->pivot;
	pivot->x = x;
	pivot->y = y;
	pivot->prevX = prevX;
	pivot->prevY = prevY;
	pivot->active = oofalse;
}

void addString(oog* g, oofloat angleV, oofloat angleA, oofloat length, oofloat angle) {
	String* string = &g->string;
	string->angleV = angleV;
	string->angleA = angleA;
	string->length = length;
	string->prevLength = length;
	string->angle = angle;
	string->attached = oofalse;
	string->swingAble = oofalse;
}

void addCursor(oog* g, oofloat x, oofloat y) {
	Cursor* cursor = &g->cursor;
	cursor->x = x;
	cursor->y = y;
}

void addBlock(oog* g, oofloat x, oofloat y) {
	if (g->numOfBlocks < OO_MAX_BLOCKS) {
		Block* block = &g->blocks[g->numOfBlocks];
		block->x = x;
		block->y = y;
		g->numOfBlocks++;
	}
}

void addGoalBlock(oog* g, oofloat x, oofloat y) {
	if (g->numOfGoalBlocks < OO_MAX_GOAL_BLOCKS) {
		GoalBlock* goalBlock = &g->goalBlocks[g->numOfGoalBlocks];
		goalBlock->x = x;
		goalBlock->y = y;
		g->numOfGoalBlocks++;
	}
}

void addAllBlocks(oog* g) {
	ooint numOfTiles = (OO_GAME_MAX_WIDTH / OO_GAME_TILESIZE + 1) * (OO_GAME_MAX_HEIGHT / OO_GAME_TILESIZE);
	for (int screen = 0; screen < OO_NUM_OF_SCREENS; screen++) {
		ooint screenOffset = screen * OO_GAME_MAX_HEIGHT;
		ooint rowCount = 0;
		for (int tile = 0; tile < numOfTiles; tile++) {
			oochar c = STAGE[screen][tile];
			if (c == '#') {
				oofloat x = (tile - (65 * rowCount)) * OO_GAME_TILESIZE;
				oofloat y = (rowCount * OO_GAME_TILESIZE) - screenOffset;
				addBlock(g, x, y);
			}
			else if (c == '@') {
				oofloat x = (tile - (65 * rowCount)) * OO_GAME_TILESIZE;
				oofloat y = (rowCount * OO_GAME_TILESIZE) - screenOffset;
				addGoalBlock(g, x, y);
			}
			else if (c == '*') {
				rowCount++;
			}
		}
	}
}

void addCamera(oog* g) {
	Camera* camera = &g->camera;
	camera->y = 0.0f;
}

void initGame(oog* g) {
	g->initialised = ootrue;

	addPlayer(g, 600.0f, OO_GAME_MAX_HEIGHT - 25.0f, 0.0f, 0.0f);
	addPivot(g, 0.0f, 0.0f, 0.0f, 0.0f);
	addString(g, 0.0f, 0.0f, 0.0f, 0.0f);
	addCursor(g, 0.0f, 0.0f);
	addAllBlocks(g);
	addCamera(g);

	loadSprite(g, &g->backgroundSprite, "NanukaLvl7Background.dpi");
	loadSprite(g, &g->ballSprite, "Frog.dpi");
	loadSprite(g, &g->pivotSprite, "pinksquare.dpi");
	loadSprite(g, &g->yippieSprite, "Yippie.dpi");
}

ooint iroundf(oofloat v) {
	return (ooint)roundf(v);
}

oofloat square(oofloat num) {
	return num * num;
}

oofloat getPlayerBottomY(Player* player) {
	return player->y + OO_FROG_OFFSET;
}

oofloat getPlayerTopY(Player* player) {
	return player->y - OO_FROG_OFFSET;
}

oofloat getPlayerRightX(Player* player) {
	return player->x + OO_FROG_OFFSET;
}

oofloat getPlayerLeftX(Player* player) {
	return player->x - OO_FROG_OFFSET;
}

oobool groundedCheck(oog* g, Player* player) {
	oofloat playerBottomY = getPlayerBottomY(player);
	oofloat playerRightX = getPlayerRightX(player);
	oofloat playerLeftX = getPlayerLeftX(player);
	for (int i = 0; i < g->numOfBlocks; i++) {
		Block* block = &g->blocks[i];
		if (playerBottomY >= (block->y - 2.0f) &&
			playerBottomY <= (block->y + 10.0f) &&
			playerRightX >= block->x &&
			playerLeftX <= (block->x + OO_GAME_TILESIZE) ) {
			return ootrue;
		}
	}
	return oofalse;
}

void gravity(oog* g, Player* player, String* string) {
	if (!player->grounded && !string->attached) {
		// ADJUST: gravity perchance
		player->velY += g->dt * OO_GRAVITATIONAL_CONST * 1.0f;
	}
}

void friction(oog* g, Player* player) {
	if (player->grounded) {
		if (player->velX > 0) {
			player->velX -= OO_FRICTION_CONST * g->dt;
			if (player->velX < 0) player->velX = 0;
		}
		else if (player->velX < 0) {
			player->velX += OO_FRICTION_CONST * g->dt;
			if (player->velX > 0) player->velX = 0;
		}
	}
}

oobool stringCollisionCheck(oog* g, Pivot* pivot) {
	for (int i = 0; i < g->numOfBlocks; i++) {
		Block* block = &g->blocks[i];
		if (pivot->x >= block->x &&
			pivot->x <= block->x + OO_GAME_TILESIZE &&
			pivot->y >= block->y &&
			pivot->y <= block->y + OO_GAME_TILESIZE) {
			return ootrue;
		}
	}
	return oofalse;
}

oobool playerCollisionCheck(oog* g, Player* player) {
	oofloat playerBottomY = getPlayerBottomY(player);
	oofloat playerTopY = getPlayerTopY(player);
	oofloat playerRightX = getPlayerRightX(player);
	oofloat playerLeftX = getPlayerLeftX(player);

	for (int i = 0; i < g->numOfBlocks; i++) {
		Block* block = &g->blocks[i];
		if (playerLeftX < block->x + OO_GAME_TILESIZE &&
			playerRightX > block->x &&
			playerTopY < block->y + OO_GAME_TILESIZE &&
			playerBottomY > block->y) {
			return ootrue;
		}
	}
	return oofalse;
}

oobool screenWidthCheck(oog* g, Player* player) {
	oofloat playerRightX = getPlayerRightX(player);
	oofloat playerLeftX = getPlayerLeftX(player);

	if (playerLeftX < 0 || playerRightX > OO_GAME_MAX_WIDTH) {
		return ootrue;
	}
	return oofalse;
}

void attachString(oog* g, Player* player, String* string, Cursor* cursor, Pivot* pivot) {
	pivot->active = ootrue;
	pivot->x = player->x;
	pivot->y = player->y;

	cursor->xInput = cursor->x;
	cursor->yInput = cursor->y;
}

void startString(oog* g, Player* player, String* string, Pivot* pivot) {
	string->swingAble = ootrue;

	string->length = sqrtf(square(pivot->x - player->x) + square(pivot->y - player->y));

	// recalculates opposite angle but honestly its okay
	oofloat dx = player->x - pivot->x;
	oofloat dy = player->y - pivot->y;
	string->angle = atan2(dx, dy);

	oofloat tangentX = cos(string->angle);
	oofloat tangentY = -sin(string->angle);
	oofloat tangentialVel = player->velX * tangentX + player->velY * tangentY;
	string->angleV = tangentialVel / string->length;
}

void controlString(oog* g, String* string, Player* player, Pivot* pivot) {
	oofloat lengthChangeRate = 200.0f;

	oosbyte extensionSign = 0;

	if (g->input.keyboard['W'] || g->input.mouse.wheelDelta > 0.5f) {
		extensionSign = -1;
	}
	else if (g->input.keyboard['S'] || g->input.mouse.wheelDelta < -0.5f) {
		extensionSign = 1;
	}

	if (extensionSign < 0) {
		string->length -= lengthChangeRate * g->dt;
		if (string->length < 10.0f) {
			string->length = 10.0f;
		}
		if (player->prevX == player->x && pivot->y + 10.0f < player->y && pivot->x != player->x) {
			player->y -= 100.0f * g->dt;
		}
	}
	/*else if (extensionSign>0) {
		string->length += lengthChangeRate * g->dt;
		if (string->length > 400.0f) {
			string->length = 400.0f;
		}
	}*/
}

void playerInputs(oog* g, Player* player) {
	if (player->grounded && g->input.keyboard['W']) {
		player->velY = -200.0f;
	}

	oofloat horizontalAccel = 2000.0f;
	oofloat maxHorizontalSpeed = 200.0f;

	if (g->input.keyboard['D']) {
		player->velX += horizontalAccel * g->dt;
		if (player->velX > maxHorizontalSpeed) {
			player->velX = maxHorizontalSpeed;
		}
	}
	else if (g->input.keyboard['A']) {
		player->velX -= horizontalAccel * g->dt;
		if (player->velX < -maxHorizontalSpeed) {
			player->velX = -maxHorizontalSpeed;
		}
	}
}

void playerInput(oog* g, Player* player, String* string, Cursor* cursor, Pivot* pivot) {

	OOGameInput* input = &g->input;
	OOGameInput* pinput = &g->previousInput;

	if (input->mouse.leftButtonDown && !pinput->mouse.leftButtonDown && !string->attached) {
		attachString(g, player, string, cursor, pivot);

	}
	else if (input->mouse.leftButtonDown && !pinput->mouse.leftButtonDown && string->attached) {
		string->attached = oofalse;
		pivot->active = oofalse;
		// ADJUST: velocity conservation doesn't always feel right
		player->velX *= 0.8f;
		player->velY *= 1.2f;
	}

	if (string->attached) {
		controlString(g, string, player, pivot);
	}
}

void uadPivot(oog* g, Pivot* pivot, Player* player, String* string, Cursor* cursor, Camera* camera) {
	if (pivot->active) {
		oofloat stringSpeed = 70.0f;

		oofloat angle;
		oofloat dx = cursor->xInput - player->x;
		oofloat dy = player->y - cursor->yInput;
		angle = atan2(dy, dx);

		for (int i = 0; i < 10; i++) {
			pivot->prevX = pivot->x;
			pivot->prevY = pivot->y;

			if (!string->attached) {
				pivot->x += stringSpeed * cosf(angle) * g->dt;
				pivot->y += -stringSpeed * sinf(angle) * g->dt;
			}

			if (stringCollisionCheck(g, pivot)) {
				// should fix below
				pivot->x = pivot->prevX;
				pivot->y = pivot->prevY;
				string->attached = ootrue;
				startString(g, player, string, pivot);
			}
		}

		// ADJUST: max length of string
		oofloat stringMaxLength = 200.0f;
		oofloat stringLengthTemp = sqrtf(square(pivot->x - player->x) + square(pivot->y - player->y));
		if (pivot->x < 0.0f || pivot->x >= 640.0f || stringLengthTemp > stringMaxLength) {
			pivot->active = oofalse;
			string->attached = oofalse;
		}

		ooint pivotPosX = iroundf(pivot->x) - 1;
		ooint pivotPosY = iroundf(pivot->y) - 1;

		fg_drawIndexedSpriteWithAlignment(&g->gbuffer, &g->pivotSprite, 0, pivotPosX, pivotPosY - camera->y, fgalign_left, fgvalign_top, NULL);
	}
}

void uadString(oog* g, String* string, Player* player, Pivot* pivot, Camera* camera) {
	if (string->attached) {
		player->prevX = player->x;
		player->prevY = player->y;

		oofloat currentFloorY = 400;
		if (player->grounded) {
			string->swingAble = oofalse;
			currentFloorY = player->y;
			player->velX = 0;
			player->velY = 0;
			
		}
		if (!string->swingAble && player->y + 10.0f <= currentFloorY) {
			startString(g, player, string, pivot);
		}

		if (string->swingAble) {
			// ADJUST: swing speed
			string->angleA = -(OO_GRAVITATIONAL_CONST / string->length) * sin(string->angle) * 1.5f;
			string->angleV += g->dt * string->angleA;
			string->angle += g->dt * string->angleV;
		}

		player->x = string->length * sin(string->angle) + pivot->x;
		if (playerCollisionCheck(g, player) || screenWidthCheck(g, player)) {
			player->x = player->prevX;
			player->velX = 0;
			player->velY = 0;
			startString(g, player, string, pivot);
		}

		player->y = string->length * cos(string->angle) + pivot->y;
		if (playerCollisionCheck(g, player)) {
			player->y = player->prevY;
			player->velY = 0;
		}

		player->velX = (player->x - player->prevX) / g->dt;
		player->velY = (player->y - player->prevY) / g->dt;
	}

	if (pivot->active) {
		fg_drawLine(&g->gbuffer, pivot->x, pivot->y - camera->y, player->x, player->y - camera->y, fg_colourWithBytes(255, 30, 100));
	}
}

void uadCursor(oog* g, Cursor* cursor, Camera* camera) {
	cursor->x = g->input.mouse.x;

	oofloat cursorBufferY = g->input.mouse.y;
	cursor->y = cursorBufferY + camera->y;

	ooint x = iroundf(cursor->x);
	ooint y = iroundf(cursorBufferY);

	if (x >= 0 && x < OO_GAME_MAX_WIDTH && y >= 0 && y < OO_GAME_MAX_HEIGHT) {
		fg_drawRectangle(&g->gbuffer, x - 1, y - 1, 3, 3, fg_colourWithBytes(0, 0, 255));
	}
}

void uadPlayer(oog* g, Player* player, String* string, Cursor* cursor, Pivot* pivot, Camera* camera) {
	player->grounded = groundedCheck(g, player);

	playerInput(g, player, string, cursor, pivot);

	if (!string->attached) {
		player->prevX = player->x;
		player->prevY = player->y;

		// comment below function out to remove wasd movement
		//playerInputs(g, player);

		gravity(g, player, string);

		player->x += g->dt * player->velX;
		if (playerCollisionCheck(g, player)) {
			player->x = player->prevX;
			player->velX = 0;
		}
		for (int i = 0; i < 10; i++) {
			player->y += g->dt * (player->velY * 0.1f);
			if (playerCollisionCheck(g, player)) {
				player->y = player->prevY;
				player->velY = 0;
			}
		}

		if (screenWidthCheck(g, player)) {
			player->x = player->prevX;
			player->velX = 0;
		}

		friction(g, player);
	}

	// dev privileges
	if (g->input.keyboard['M']) {
		player->goalReached = oofalse;
		player->x = 600.0f;
		player->y = OO_GAME_MAX_HEIGHT - 25.0f;
		player->velX = 0.0f;
		player->velY = 0.0f;
		string->attached = oofalse;
		pivot->active = oofalse;
	}
	if (g->input.keyboard['N']) {
		player->x = 500.0f;
		player->y = -25.0f;
		player->velX = 0.0f;
		player->velY = 0.0f;
		string->attached = oofalse;
		pivot->active = oofalse;
	}
	if (g->input.keyboard['B']) {
		player->x = 20.0f;
		player->y = -100.0f;
		player->velX = 0.0f;
		player->velY = 0.0f;
		string->attached = oofalse;
		pivot->active = oofalse;
	}
	if (g->input.keyboard['V']) {
		player->x = 415.0f;
		player->y = -200.0f;
		player->velX = 0.0f;
		player->velY = 0.0f;
		string->attached = oofalse;
		pivot->active = oofalse;
	}
	if (g->input.keyboard['C']) {
		player->x = 450.0f;
		player->y = -1000.0f;
		player->velX = 0.0f;
		player->velY = 0.0f;
		string->attached = oofalse;
		pivot->active = oofalse;
	}

	ooint x = iroundf(player->x);
	ooint y = iroundf(player->y);

	fg_drawIndexedSpriteWithAlignment(&g->gbuffer, &g->ballSprite, 0, x, y - camera->y, fgalign_centre, fgvalign_middle, NULL);
}

void uadBlock(oog* g, Block* block, Camera* camera) {
	if (block->y + OO_GAME_TILESIZE < camera->y || block->y > camera->y + OO_GAME_MAX_HEIGHT) {
		return;
	}
	fg_drawRectangle(&g->gbuffer, block->x, block->y - camera->y, OO_GAME_TILESIZE, OO_GAME_TILESIZE, fg_colourWithBytes(100, 20, 50));
}

void uadGoalBlock(oog* g, GoalBlock* goalBlock, Player* player, Camera* camera) {
	oofloat playerBottomY = getPlayerBottomY(player);
	oofloat playerTopY = getPlayerTopY(player);
	oofloat playerRightX = getPlayerRightX(player);
	oofloat playerLeftX = getPlayerLeftX(player);

	if (playerLeftX < goalBlock->x + OO_GAME_TILESIZE &&
		playerRightX > goalBlock->x &&
		playerTopY < goalBlock->y + OO_GAME_TILESIZE &&
		playerBottomY > goalBlock->y) {
		player->goalReached = ootrue;
	}
	if (goalBlock->y + OO_GAME_TILESIZE < camera->y || goalBlock->y > camera->y + OO_GAME_MAX_HEIGHT) {
		return;
	}
	fg_drawRectangle(&g->gbuffer, goalBlock->x, goalBlock->y - camera->y, OO_GAME_TILESIZE, OO_GAME_TILESIZE, fg_colourWithBytes(255, 215, 0));
}

void updateCamera(oog* g, Camera* camera, Player* player) {
	oofloat target_y = player->y - 336.0f;
	oofloat cameraSpeed = 8.0f;
	oofloat dead_zone = 150.0f;

	oofloat distance = target_y - camera->y;

	if (distance > 0) {
		camera->y += distance * cameraSpeed * g->dt;
	}
	else if (fabs(distance) > dead_zone) {
		oofloat adjusted_target = target_y - (distance > 0 ? dead_zone : -dead_zone);
		camera->y += (adjusted_target - camera->y) * cameraSpeed * g->dt;
	}
}

void updateGraphics(void* gameMemory, OOGameInput* input, OOGameOutput* output) {
	oog* g = (oog*)gameMemory;

	g->input = *input;
	g->output = output;

	g->dt = g->input.dt;

	computeGraphicsOutputSize(g);

	g->gbuffer.buffer = output->graphic.buffer;
	g->gbuffer.stride = output->graphic.width;
	g->gbuffer.width = output->graphic.width;
	g->gbuffer.height = output->graphic.height;

	if (!g->initialised) {
		initGame(g);
	}

	fg_drawIndexedSpriteWithAlignment(&g->gbuffer, &g->backgroundSprite, 0, 0, 0, fgalign_left, fgvalign_top, NULL);

	String* string = &g->string;
	Block* block;
	GoalBlock* goalBlock;
	Cursor* cursor = &g->cursor;
	Pivot* pivot = &g->pivot;
	Camera* camera = &g->camera;
	Player* player = &g->player;

	updateCamera(g, camera, player);
	uadString(g, string, player, pivot, camera);
	for (int i = 0; i < g->numOfBlocks; i++) {
		block = &g->blocks[i];
		uadBlock(g, block, camera);
	}
	for (int i = 0; i < g->numOfGoalBlocks; i++) {
		goalBlock = &g->goalBlocks[i];
		uadGoalBlock(g, goalBlock, player, camera);
	}
	uadPivot(g, pivot, player, string, cursor, camera);
	uadPlayer(g, player, string, cursor, pivot, camera);
	uadCursor(g, cursor, camera);

	if (player->goalReached) {
		fg_drawIndexedSpriteWithAlignment(&g->gbuffer, &g->yippieSprite, 0, 0, 0, fgalign_left, fgvalign_top, NULL);
	}

	g->previousInput = g->input;
}

void updateAudio(void* gameMemory, OOGameInput* input, OOGameOutput* output) {
	oog* g = (oog*)gameMemory;
	//outputSine(g, input, output);
}