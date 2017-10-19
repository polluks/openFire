#include "gamestates/game/scoretable.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>
#include "gamestates/game/player.h"

#define SCORE_TABLE_BPP 4

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tFont *s_pFont;

void scoreTableCreate(tVPort *pHudVPort, tFont *pFont) {
	logBlockBegin("scoreTableCreate(pHudVPort: %p, pFont: %p)", pHudVPort, pFont);
	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, 8*2 + (6+2*SCORE_TABLE_BPP) + 3,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		// TAG_VPORT_HEIGHT, 192,
		TAG_VPORT_BPP, SCORE_TABLE_BPP,
		TAG_DONE
	);
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, 8*2,
		TAG_DONE
	);
	s_pFont = pFont;
	paletteLoad("data/game.plt", s_pVPort->pPalette, 16);
	paletteLoad("data/openfire_sprites.plt", &s_pVPort->pPalette[16], 16);

	tSimpleBufferManager *pHudBfr = (tSimpleBufferManager*)vPortGetManager(pHudVPort, VPM_SCROLL);

	copRawDisableSprites(s_pView->pCopList, 0xFF, 0);

	// Jump to HUD - back buffer to back buffer
	ULONG ulHudListAddr = (ULONG)((void*)
		&pHudBfr->sCommon.pVPort->pView->pCopList->pBackBfr->pList[pHudBfr->uwCopperOffset]
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 0].sMove,
		&pCopLc[1].uwHi, ulHudListAddr >> 16
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 1].sMove,
		&pCopLc[1].uwLo, ulHudListAddr & 0xFFFFF
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 2].sMove,
		&custom.copjmp2, 1
	);

	// Jump to HUD - front buffer to front buffer
	ulHudListAddr = (ULONG)((void*)
		&pHudBfr->sCommon.pVPort->pView->pCopList->pFrontBfr->pList[pHudBfr->uwCopperOffset]
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 0].sMove,
		&pCopLc[1].uwHi, ulHudListAddr >> 16
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 1].sMove,
		&pCopLc[1].uwLo, ulHudListAddr & 0xFFFFF
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 2].sMove,
		&custom.copjmp2, 1
	);
	logBlockEnd("scoreTableCreate()");
}

void scoreTableDestroy(void) {
	viewDestroy(s_pView);
}

void scoreTableUpdate(void) {
	const FUBYTE fubColorBlue = 12;
	const FUBYTE fubColorRed  = 10;
	for(FUBYTE i = 0; i != g_ubPlayerCount; ++i) {
		fontDrawStr(
			s_pBfr->pBuffer, s_pFont,	16, 16 + 6*i,	g_pPlayers[i].szName,
			(g_pPlayers[i].ubTeam == TEAM_RED ? fubColorRed : fubColorBlue),
			FONT_TOP | FONT_LEFT
		);
	}
}

void scoreTableShow(void) {
	scoreTableUpdate();
	viewLoad(s_pView);
}

void scoreTableProcessView(void) {
	viewProcessManagers(s_pView);
}