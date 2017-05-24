#include "gamestates/game/world.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/copper.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/palette.h>
#include "gamestates/game/game.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/hud.h"
#include "gamestates/game/turret.h"

// Viewport stuff
tView *g_pWorldView;
tVPort *s_pWorldMainVPort;
tSimpleBufferManager *g_pWorldMainBfr;
tCameraManager *s_pWorldCamera;
tBitMap *s_pTiles;

// Silo highlight
// TODO: struct?
tBob *s_pSiloHighlight;
UBYTE s_ubWasSiloHighlighted;
UBYTE g_ubDoSiloHighlight;
UWORD g_uwSiloHighlightTileY;
UWORD g_uwSiloHighlightTileX;

UBYTE worldCreate(void) {
	// Prepare view & viewport
	g_pWorldView = viewCreate(V_GLOBAL_CLUT);
	s_pWorldMainVPort = vPortCreate(g_pWorldView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT-64-1, GAME_BPP, 0);
	g_pWorldMainBfr = simpleBufferCreate(s_pWorldMainVPort, 20<<MAP_TILE_SIZE, 20<<MAP_TILE_SIZE, 0);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return 0;
	}
	paletteLoad("data/amidb32.plt", s_pWorldMainVPort->pPalette, 32);
	s_pWorldCamera = g_pWorldMainBfr->pCameraManager;

	hudCreate();

	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	s_pSiloHighlight = bobUniqueCreate("data/silohighlight.bm", "data/silohighlight.msk", 0, 0);

	// Draw map
	mapSetSrcDst(s_pTiles, g_pWorldMainBfr->pBuffer);
	mapRedraw();

	// Initial values
	s_ubWasSiloHighlighted = 0;
	g_ubDoSiloHighlight = 0;

	return 1;
}

void worldDestroy(void) {
	viewDestroy(g_pWorldView);
	bobUniqueDestroy(s_pSiloHighlight);
	bitmapDestroy(s_pTiles);
}

void worldShow(void) {
	viewLoad(g_pWorldView);
}

void worldHide(void) {
	bunkerShow();
}

void worldDraw(void) {
	UBYTE ubPlayer, ubTurret;

	// Silo highlight
	if(g_ubDoSiloHighlight)
		bobDraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer,
			g_uwSiloHighlightTileX << MAP_TILE_SIZE,
			g_uwSiloHighlightTileY << MAP_TILE_SIZE
		);

	// Vehicles
	for(ubPlayer = 0; ubPlayer != g_ubPlayerLimit; ++ubPlayer)
		vehicleDraw(&g_pPlayers[ubPlayer].sVehicle);

	// Turrets
	turretDrawAll();

	// Projectiles
	projectileDraw();

	s_ubWasSiloHighlighted = g_ubDoSiloHighlight;
}

void worldUndraw(void) {
	UBYTE ubPlayer;

	// Projectiles
	projectileUndraw();

	// Turrets
	turretUndrawAll();

	// Vehicles
	for(ubPlayer = g_ubPlayerLimit; ubPlayer--;)
		vehicleUndraw(&g_pPlayers[ubPlayer].sVehicle);

	// Silo highlight
	if(s_ubWasSiloHighlighted)
		bobUndraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer
		);
}

void worldProcess(void) {
	
	// World-specific & steering-irrelevant player input
	worldProcessInput();
	
	// TODO: Update HUD vport

	// Update main vport
	vPortWaitForEnd(s_pWorldMainVPort);
	worldUndraw();
	if(g_pLocalPlayer->sVehicle.ubLife) {
		UWORD uwLocalX, uwLocalY;
		uwLocalX = g_pLocalPlayer->sVehicle.fX;
		uwLocalY = g_pLocalPlayer->sVehicle.fY;
		cameraCenterAt(s_pWorldCamera, uwLocalX, uwLocalY);
	}
	mapUpdateTiles();
	worldDraw();
	
	viewProcessManagers(g_pWorldView);
	copProcessBlocks();
}

void worldProcessInput(void) {
	if(keyUse(KEY_C))
		bitmapSaveBmp(g_pWorldMainBfr->pBuffer, s_pWorldMainVPort->pPalette, "bufDump.bmp");
}
