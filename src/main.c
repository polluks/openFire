#include "main.h"
#include <stdlib.h>
#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/timer.h>
#include <ace/managers/system.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/managers/game.h>

#include "config.h"
#include "input.h"
#include "gamestates/precalc/precalc.h"

int main(void) {
	systemCreate();
	memCreate();
	logOpen();
	timerCreate();

	blitManagerCreate();
	copCreate();

	inputOpen();
	gameCreate();
	gamePushState(precalcCreate, precalcLoop, precalcDestroy);
	while (gameIsRunning()) {
		timerProcess();
		inputProcess();
		gameProcess();
	}
	gameDestroy();
	inputClose();

	copDestroy();
	blitManagerDestroy();

	timerDestroy();
	logClose();
	memDestroy();
	systemDestroy();
	return EXIT_SUCCESS;
}
