#ifndef GUARD_OF_GAMESTATES_MENU_MENU_H
#define GUARD_OF_GAMESTATES_MENU_MENU_H

#include <ace/types.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/font.h>

void menuCreate(void);

void menuLoop(void);

void menuDestroy(void);

void menuMainCreate(void);

void menuMainDestroy(void);

void menuProcess(void);

void menuDrawButton(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN char *szText,
	IN UBYTE isSelected
);

extern tSimpleBufferManager *g_pMenuBuffer;
extern tFont *g_pMenuFont;

#endif // GUARD_OF_GAMESTATES_MENU_MENU_H
