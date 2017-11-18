#include "gamestates/menu/maplist.h"
#include <string.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/mouse.h>
#include "cursor.h"
#include "gamestates/menu/menu.h"
#include "gamestates/menu/button.h"
#include "gamestates/menu/listctl.h"

#define MAPLIST_FILENAME_MAX 108
#define MAP_NAME_MAX 30

typedef struct _tMapListEntry {
	char szFileName[MAPLIST_FILENAME_MAX];
	char szMapName[MAP_NAME_MAX];
} tMapListEntry;

typedef struct _tMapList {
	UWORD uwMapCount;
	tMapListEntry *pMaps;
} tMapList;

tMapList s_sMapList;


void mapListPrepareList(void) {
	// Get map count
	s_sMapList.uwMapCount = 0;
	BPTR pLock;
	struct FileInfoBlock sFileBlock;
	pLock = Lock("data/maps", ACCESS_READ);
	LONG lResult;
	lResult = Examine(pLock, &sFileBlock);
	if(lResult != DOSFALSE) {
		lResult = ExNext(pLock, &sFileBlock); // Skip dir name
		while(lResult != DOSFALSE) {
			if(!memcmp(
				&sFileBlock.fib_FileName[strlen(sFileBlock.fib_FileName)-strlen(".json")],
				".json", strlen(".json")
			)) {
				++s_sMapList.uwMapCount;
				lResult = ExNext(pLock, &sFileBlock);
			}
		}
	}
	UnLock(pLock);

	// Alloc map list
	if(!s_sMapList.uwMapCount)
		return;
	s_sMapList.pMaps = memAllocFast(s_sMapList.uwMapCount * sizeof(tMapListEntry));
	pLock = Lock("data/maps", ACCESS_READ);
	UWORD i = 0;
	lResult = Examine(pLock, &sFileBlock);
	if(lResult != DOSFALSE) {
		lResult = ExNext(pLock, &sFileBlock); // Skip dir name
		while(lResult != DOSFALSE) {
			if(!memcmp(
				&sFileBlock.fib_FileName[strlen(sFileBlock.fib_FileName)-strlen(".json")],
				".json", strlen(".json")
			)) {
				memcpy(
					s_sMapList.pMaps[i].szFileName, sFileBlock.fib_FileName,
					MAPLIST_FILENAME_MAX
				);
				++i;
				lResult = ExNext(pLock, &sFileBlock);
			}
		}
	}
	UnLock(pLock);
}

tListCtl *s_pListCtl;

void mapListCreate(void) {
	// Clear bg
	blitRect(
		g_pMenuBuffer->pBuffer, 0, 0,
		bitmapGetByteWidth(g_pMenuBuffer->pBuffer) << 3, g_pMenuBuffer->pBuffer->Rows,
		0
	);
	WaitBlit();

	buttonListCreate(10, g_pMenuBuffer->pBuffer, g_pMenuFont);

	mapListPrepareList();

	s_pListCtl = listCtlCreate(
		g_pMenuBuffer->pBuffer, 10, 10, 100, 200,
		g_pMenuFont, s_sMapList.uwMapCount
	);
	for(UWORD i = 0; i != s_sMapList.uwMapCount; ++i) {
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)-5] = 0;
		listCtlAddEntry(s_pListCtl, s_sMapList.pMaps[i].szFileName);
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)-5] = '.';
	}

	listCtlDraw(s_pListCtl);
	buttonDrawAll();
}

void mapListLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuMainCreate, menuLoop, menuMainDestroy);
		return;
	}

	if(mouseUse(MOUSE_LMB))
		if(!buttonProcessClick(g_uwMouseX, g_uwMouseY))
			listCtlProcessClick(s_pListCtl, g_uwMouseX, g_uwMouseY);

	menuProcess();
}

void mapListDestroy(void) {
	memFree(s_sMapList.pMaps, s_sMapList.uwMapCount * sizeof(tMapListEntry));
	listCtlDestroy(s_pListCtl);
	buttonListDestroy();
}
