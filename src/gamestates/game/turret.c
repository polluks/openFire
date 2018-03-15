#include "gamestates/game/turret.h"

#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/managers/rand.h>
#include <ace/utils/custom.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/player.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/team.h"

#define TURRET_SPRITE_OFFS    (MAP_FULL_TILE - TURRET_SPRITE_SIZE)

UWORD g_uwTurretCount;
tTurret *g_pTurrets; // 20x25: 1100*7 ~ 8KiB
tBitMap *g_pTurretFrames;

static UWORD s_uwMaxTurrets;
UWORD **g_pTurretTiles; // 20x25: +2KiB

static tAvg *s_pAvg;
static FUBYTE s_fubMapWidth, s_fubMapHeight;

void turretListCreate(FUBYTE fubMapWidth, FUBYTE fubMapHeight) {
	logBlockBegin("turretListCreate()");
	s_fubMapWidth = fubMapWidth;
	s_fubMapHeight = fubMapHeight;

	g_uwTurretCount = 0;
	s_uwMaxTurrets = (fubMapWidth/2 + 1) * fubMapHeight;
	g_pTurrets = memAllocFastClear(s_uwMaxTurrets * sizeof(tTurret));

	// Tile-based turret list
	g_pTurretTiles = memAllocFast(sizeof(UWORD*) * fubMapWidth);
	for(FUBYTE i = 0; i < fubMapWidth; ++i) {
		g_pTurretTiles[i] = memAllocFast(sizeof(UWORD) * fubMapHeight);
		memset(g_pTurretTiles[i], 0xFF, sizeof(UWORD) * fubMapHeight);
	}

	// Attach sprites
	// TODO vehicle/turret jitter could be fixed by setting lsbit in ctl
	// accordingly.
	tCopCmd *pCopInit = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_INIT_POS];
	copSetMove(&pCopInit[0].sMove, &g_pCustom->spr[1].ctl, 1);
	copSetMove(&pCopInit[1].sMove, &g_pCustom->spr[0].ctl, 1);
	// Same in front bfr
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_INIT_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_INIT_POS],
		2*sizeof(tCopCmd)
	);

	// Cleanup block for sprites trimmed from bottom
	tCopCmd *pCopCleanup = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CLEANUP_POS];
	copSetMove(&pCopCleanup[0].sMove, &g_pCustom->spr[1].ctl, 1);
	copSetMove(&pCopCleanup[1].sMove, &g_pCustom->spr[0].ctl, 0);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CLEANUP_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_CLEANUP_POS],
		3*sizeof(tCopCmd)
	);

	s_pAvg = logAvgCreate("turretUpdateSprites()", 50*5);

	// Force blank last line of turret gfx
	for(UWORD uwFrame = 0; uwFrame != VEHICLE_BODY_ANGLE_COUNT; ++uwFrame) {
		blitRect(
			g_pTurretFrames,
			0, TURRET_SPRITE_SIZE*uwFrame + TURRET_SPRITE_SIZE-1,
			16, 1, 0
		);
	}

	logBlockEnd("turretListCreate()");
}

void turretListDestroy(void) {
	logBlockBegin("turretListDestroy()");

	memFree(g_pTurrets, s_uwMaxTurrets * sizeof(tTurret));

	// Tile-based turret list
	for(int i = 0; i != s_fubMapWidth; ++i)
		memFree(g_pTurretTiles[i], sizeof(UWORD) * s_fubMapHeight);
	memFree(g_pTurretTiles, sizeof(UWORD*) * s_fubMapWidth);

	logAvgDestroy(s_pAvg);

	logBlockEnd("turretListDestroy()");
}

UWORD turretAdd(UWORD uwTileX, UWORD uwTileY, UBYTE ubTeam) {
	logBlockBegin(
		"turretAdd(uwTileX: %hu, uwTileY: %hu, ubTeam: %hhu)",
		uwTileX, uwTileY, ubTeam
	);

	// Initial values
	tTurret *pTurret = &g_pTurrets[g_uwTurretCount];
	pTurret->uwX = (uwTileX << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pTurret->uwY = (uwTileY << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pTurret->ubTeam = ubTeam;
	pTurret->ubAngle = ANGLE_90;
	pTurret->ubDestAngle = ANGLE_90;
	pTurret->isTargeting = 0;
	pTurret->ubCooldown = 0;
	pTurret->fubSeq = (uwTileX & 3) |	((uwTileY & 3) << 2);

	// Add to tile-based list
	g_pTurretTiles[uwTileX][uwTileY] = g_uwTurretCount;

	logBlockEnd("turretAdd()");
	return g_uwTurretCount++;
}

void turretDestroy(UWORD uwIdx) {
	// Find turret
	if(uwIdx >= s_uwMaxTurrets) {
		logWrite("ERR: turretDestroy() - Index out of range %u\n", uwIdx);
		return;
	}
	tTurret *pTurret = &g_pTurrets[uwIdx];

	// Already destroyed?
	if(!pTurret->uwX)
		return;

	// Remove from tile-based list
	UWORD uwTileX = pTurret->uwX >> MAP_TILE_SIZE;
	UWORD uwTileY = pTurret->uwY >> MAP_TILE_SIZE;
	g_pTurretTiles[uwTileX][uwTileY] = TURRET_INVALID;

	// Add explosion
	explosionsAdd(
		uwTileX << MAP_TILE_SIZE, uwTileY << MAP_TILE_SIZE
	);

	// Mark turret as destroyed
	pTurret->uwX = 0;
}

static void turretUpdateTarget(tTurret *pTurret) {
	pTurret->isTargeting = 0;
	// Scan nearby enemies
	UBYTE ubEnemyTeam = pTurret->ubTeam == TEAM_BLUE ? TEAM_RED : TEAM_BLUE;
	tPlayer *pClosestPlayer = playerGetClosestInRange(
		pTurret->uwX, pTurret->uwY, TURRET_MIN_DISTANCE, ubEnemyTeam
	);

	// Anything in range?
	if(pClosestPlayer) {
		pTurret->isTargeting = 1;
		// Determine destination angle
		pTurret->ubDestAngle = getAngleBetweenPoints(
			pTurret->uwX, pTurret->uwY,
			pClosestPlayer->sVehicle.uwX,
			pClosestPlayer->sVehicle.uwY
		);
	}
}

void turretSim(void) {
	FUBYTE fubSeq = g_ulGameFrame & 15;

	for(UWORD uwTurretIdx = 0; uwTurretIdx != s_uwMaxTurrets; ++uwTurretIdx) {
		tTurret *pTurret = &g_pTurrets[uwTurretIdx];
		if(!pTurret->uwX || pTurret->ubTeam == TEAM_NONE)
			continue;

		if(pTurret->fubSeq == fubSeq) {
			turretUpdateTarget(pTurret);
		}

		// Process cooldown
		if(pTurret->ubCooldown)
			--pTurret->ubCooldown;

		if(pTurret->ubAngle != pTurret->ubDestAngle) {
			pTurret->ubAngle += ANGLE_360 + getDeltaAngleDirection(
				pTurret->ubAngle, pTurret->ubDestAngle, 2
			);
			if(pTurret->ubAngle >= ANGLE_360)
				pTurret->ubAngle -= ANGLE_360;
		}
		else if(pTurret->isTargeting && !pTurret->ubCooldown) {
			tProjectileOwner uOwner;
			uOwner.pTurret = pTurret;
			projectileCreate(PROJECTILE_OWNER_TYPE_TURRET, uOwner, PROJECTILE_TYPE_BULLET);
			pTurret->ubCooldown = TURRET_COOLDOWN;
		}
	}
}

/**
 *  Copies data using blitter using A->D channels.
 *  Word count is not flat but rectangular because of blitter internal workings.
 *  It could be converted to flat model but why bother abstraction if copy size
 *  is already almost always result of some kind of multiplication, so this time
 *  you get one for (almost) free!
 *  OpenFire rationale:
 *  One line takes 6*7=42 copper cmds, so 84 words.
 *  4*84/7.16 = 46,9us per line, so filling 15 lines should take 704us.
 *  There are gonna be up to rows on screen, so 4 turet rows will take 2,8ms.
 *  @param pSrc       Source data.
 *  @param pDst       Destination data.
 *  @param uwRowWidth Number of words to copy per 'row', max 64.
 *  @param uwRowCnt   Number of 'rows' to copy. Max 1024
 */
 static void copyWithBlitter(UBYTE *pSrc, UBYTE *pDst, UWORD uwRowWidth, UWORD uwRowCnt) {
	// Modulo: 0, 'cuz 2nd line will go from 1st, 3rd from 2nd etc.
	WaitBlit();
	g_pCustom->bltcon0 = USEA|USED|MINTERM_A;
	g_pCustom->bltcon1 = 0;
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltamod = 0;
	g_pCustom->bltdmod = 0;
	g_pCustom->bltapt = pSrc;
	g_pCustom->bltdpt = pDst;
	g_pCustom->bltsize = (uwRowCnt << 6) | uwRowWidth;
}

// Screen is 320x256, tiles are 32x32, hud 64px, so 10x6 tiles on screen
// Assuming that at most half of row tiles may have turret, 5x6 turret tiles
// But also, because of scroll, there'll be 7 turret rows and 6 cols.
void turretUpdateSprites(void) {
	logAvgBegin(s_pAvg);
	g_pCustom->dmacon = BITSET | DMAF_BLITHOG;
	tTurret *pTurret;
	UWORD uwSpriteLine;
	const UWORD uwCopperInsCount = 6;
	WORD wCopVPos, wCopHPos;

	// Tiles range to process
	UWORD uwFirstTileX, uwFirstTileY, uwLastTileX, uwLastTileY;
	// Offsets of sprite' beginning from top-left corner
	WORD wSpriteOffsX, wSpriteOffsY;
	// Sprite's pos on screen
	WORD wSpriteBeginOnScreenY, wSpriteEndOnScreenY;
	WORD wSpriteBeginOnScreenX;
	// Sprite's visible lines
	UWORD uwFirstVisibleSpriteLine;
	// Iterators, counters, flags
	FUWORD uwTileX, uwTileY;
	UWORD uwTurretsInRow;

	UWORD uwCameraX = g_pWorldCamera->uPos.sUwCoord.uwX;
	UWORD uwCameraY = g_pWorldCamera->uPos.sUwCoord.uwY;
	// This is equivalent to number of cols/rows trimmed by view, if negative
	// OffsX needs to be even 'cuz sprite pos lsbit is always 0
	wSpriteOffsX = ((TURRET_SPRITE_OFFS>>1) - (uwCameraX & (MAP_FULL_TILE -1))) & 0xfffe;
	wSpriteOffsY = (TURRET_SPRITE_OFFS>>1) - (uwCameraY & (MAP_FULL_TILE -1));

	// Tile range to be checked
	// If last sprite begins just at bottom-right end of screen, it garbles
	// HUD's copperlist. So uwLastTileX is trimmed so that it won't try to display
	// last sprite if there is only a bit of it.
	uwFirstTileX = uwCameraX >> MAP_TILE_SIZE;
	uwFirstTileY = uwCameraY >> MAP_TILE_SIZE;
	uwLastTileX  = (uwCameraX + WORLD_VPORT_WIDTH -1-8) >> MAP_TILE_SIZE;
	uwLastTileY  = (uwCameraY + WORLD_VPORT_HEIGHT -1-8) >> MAP_TILE_SIZE;
	UWORD uwCopOffs = WORLD_COP_TURRET_START_POS;
	UWORD uwRowStartCopOffs = 0;
	tCopCmd *pCmdList = g_pWorldView->pCopList->pBackBfr->pList;

	// Iterate thru visible tile rows
	UWORD *pRowSpriteBpls[6];
	for(uwTileY = uwFirstTileY; uwTileY <= uwLastTileY; ++uwTileY) {
		// Determine copperlist & sprite display lines
		wSpriteBeginOnScreenY = wSpriteOffsY + ((uwTileY-uwFirstTileY) << MAP_TILE_SIZE);
		if(wSpriteBeginOnScreenY < 0) {
			// Sprite is trimmed from top
			uwFirstVisibleSpriteLine = -wSpriteOffsY;
			if(uwFirstVisibleSpriteLine >= TURRET_SPRITE_SIZE) {
				// Row has only visible lines below screen
				continue;
			}
			wSpriteBeginOnScreenY = 0;
			wSpriteEndOnScreenY = wSpriteOffsY + TURRET_SPRITE_SIZE-1;
		}
		else {
			// Sprite is not trimmed on top - may be on bottom
			uwFirstVisibleSpriteLine = 0;
			wSpriteEndOnScreenY = wSpriteBeginOnScreenY + TURRET_SPRITE_SIZE-1;
			if(wSpriteEndOnScreenY >= WORLD_VPORT_HEIGHT) {
				// Sprite is trimmed on bottom
				wSpriteEndOnScreenY = WORLD_VPORT_HEIGHT-1;
			}
		}

		// Iterate thru row's visible columns
		uwTurretsInRow = 0;
		uwRowStartCopOffs = uwCopOffs;
		wCopVPos = WORLD_VPORT_BEGIN_Y + wSpriteBeginOnScreenY;
		const UWORD uwWordsPerRow = (g_pTurretFrames->BytesPerRow >> 1);
		for(uwTileX = uwFirstTileX; uwTileX <= uwLastTileX; ++uwTileX) {
			// Get turret from tile, skip if there is none
			if(g_pTurretTiles[uwTileX][uwTileY] == 0xFFFF)
			continue;
			pTurret = &g_pTurrets[g_pTurretTiles[uwTileX][uwTileY]];

			// Get proper turret sprite data
			// 0 < wCopHPos < 0xE2 always 'cuz only 8px after 0xE2
			wSpriteBeginOnScreenX = ((uwTileX-uwFirstTileX) << MAP_TILE_SIZE) + wSpriteOffsX;
			wCopHPos = (0x48 + wSpriteBeginOnScreenX/2 - uwCopperInsCount*4);

			// Get turret gfx
			uwSpriteLine = (angleToFrame(pTurret->ubAngle)*TURRET_SPRITE_SIZE + uwFirstVisibleSpriteLine)* uwWordsPerRow;
			UWORD *pSpriteBpls = &((UWORD**)g_pTurretFrames->Planes)[0][uwSpriteLine];
			pRowSpriteBpls[uwTurretsInRow] = pSpriteBpls;
			const UWORD pTeamColors[TEAM_COUNT+1] = {0x69C, 0xB21, 0xAAA};

			// Do a WAIT
			// TODO: copWait VPOS could be ignored here and only set on 1st line later
			copSetWait(&pCmdList[uwCopOffs+0].sWait, wCopHPos, wCopVPos);
			// pCmdList[uwCopOffs+0].sWait.bfVE = 0;

			// Add MOVEs - no need setting sprite VPos 'cuz WAIT ensures same line
			UWORD uwSpritePos = 63 + (wSpriteBeginOnScreenX >> 1);
			copSetMove(&pCmdList[uwCopOffs+1].sMove, &g_pCustom->color[16+3], pTeamColors[pTurret->ubTeam]);
			copSetMove(&pCmdList[uwCopOffs+2].sMove, &g_pCustom->spr[0].pos, uwSpritePos);
			copSetMove(&pCmdList[uwCopOffs+3].sMove, &g_pCustom->spr[0].datab, pSpriteBpls[1]);
			copSetMove(&pCmdList[uwCopOffs+4].sMove, &g_pCustom->spr[0].dataa, pSpriteBpls[0]);
			++uwTurretsInRow;
			// Force 1 empty tile
			++uwTileX;
			uwCopOffs += 5;
		}
		if(!uwTurretsInRow)
			continue;
		WORD wRowsToCopy = wSpriteEndOnScreenY - wSpriteBeginOnScreenY;
		UWORD uwCmdsPerRow = (uwCopOffs-uwRowStartCopOffs);
		if(wRowsToCopy <= 0)
			continue;
		// Copy rows using blitter
		copyWithBlitter(
			(UBYTE*)&pCmdList[uwRowStartCopOffs],	(UBYTE*)&pCmdList[uwCopOffs],
			uwCmdsPerRow, wRowsToCopy*(sizeof(tCopCmd)/sizeof(UWORD))
		);
		WaitBlit();
		// TODO: WAIT could be optimized by copying one with VPOS compare disabled
		// and then re-enabled only in topmost line.
		// NOTE NOTE NOTE This should wait for blitter to finish or at least
		// give it a bit of headstart
		for(FUWORD fuwRow = 0; fuwRow != wRowsToCopy; ++fuwRow) {
			++wCopVPos;
			for(FUWORD i = 0; i != uwTurretsInRow; ++i) {
				// Get turret gfx
				pRowSpriteBpls[i] += uwWordsPerRow;
				UWORD *pRow = pRowSpriteBpls[i];

				pCmdList[uwCopOffs+0].sWait.bfWaitY = wCopVPos; // WAIT Y
				pCmdList[uwCopOffs+3].sMove.bfValue = pRow[1];  // spr0datb
				pCmdList[uwCopOffs+4].sMove.bfValue = pRow[0];  // spr0data
				uwCopOffs += 5;
			}
		}
	}

	// Jump to cleanup if not completely filled
	if(uwCopOffs < WORLD_COP_VPHUD_POS) {
		ULONG ulEndPos = (ULONG)((void*)&pCmdList[WORLD_COP_VPHUD_DMAOFF_POS]);
		copSetMove(&pCmdList[uwCopOffs+0].sMove, &g_pCopLc[1].uwHi, ulEndPos>>16);
		copSetMove(&pCmdList[uwCopOffs+1].sMove, &g_pCopLc[1].uwLo, ulEndPos & 0xFFFF);
		copSetMove(&pCmdList[uwCopOffs+2].sMove, &g_pCustom->copjmp2, 1);
	}
	g_pCustom->dmacon = BITCLR | DMAF_BLITHOG;
	logAvgEnd(s_pAvg);
	// DMA-exact HOG on turretsFull map
	// Avg turretUpdateSprites():  30.311 ms, min:  30.677 ms, max:  41.228 ms

}

void turretCapture(UWORD uwIdx, FUBYTE fubTeam) {
	g_pTurrets[uwIdx].ubTeam = fubTeam;
	g_pTurrets[uwIdx].isTargeting = 0;
}
