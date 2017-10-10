#ifndef GUARD_OF_CURSOR_H
#define GUARD_OF_CURSOR_H

#include <ace/types.h>
#include <ace/utils/extview.h>

/**
 * Creates cursor on supplied view.
 * @param pView        Parent view.
 * @param fubSpriteIdx Index of sprite to be used. Must not be disabled
 *                     by cop*DisableSprites().
 * @param szPath       Path to 2bpp interleaved bitmap used as cursor.
 *                     Bitmap has to have two unused lines on top & bottom
 *                     of cursor image.
 * @param uwRawCopPos  If copperlist is in raw mode, specify position
 *                     for 2 MOVEs. Should be as close to 0,0 as possible.
 * @todo Passing hotspot coord as params.
 * @todo Support for 4bpp sprites.
 */
void cursorCreate(
	IN tView *pView,
	IN FUBYTE fubSpriteIdx,
	IN char *szPath,
	IN UWORD uwRawCopPos
);

/**
 * Frees stuff allocated by copCreate().
 * This function doesn't remove created copBlock as it will be destroyed along
 * with copperlist.
 */
void cursorDestroy(void);

/**
 * Set on-screen constraints for cursor.
 * @param uwXLo Minimum cursor X position.
 * @param uwYLo Minimum cursor Y position.
 * @param uwXHi Maximum cursor X position.
 * @param uwYHi Maximum cursor Y position.
 */
void cursorSetConstraints(
	IN UWORD uwXLo,
	IN UWORD uwYLo,
	IN UWORD uwXHi,
	IN UWORD uwYHi
);

/**
 * Updates cursor position.
 */
void cursorUpdate(void);

extern UWORD g_uwMouseX, g_uwMouseY; ///< Cursor position, read only

#endif // GUARD_OF_GAMESTATES_GAME_CURSOR_H