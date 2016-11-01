#ifndef GUARD_OF_GAMESTATES_GAME_BUILDING_H
#define GUARD_OF_GAMESTATES_GAME_BUILDING_H

#include <ace/config.h>

#define BUILDING_INVALID 0
#define BUILDING_TYPE_WALL 0
#define BUILDING_TYPE_AMMO 1
#define BUILDING_TYPE_FUEL 2
#define BUILDING_TYPE_FLAG 3

#define BUILDING_DAMAGED   1
#define BUILDING_DESTROYED 0

#define BUILDING_MAX_COUNT 256

typedef struct {
	UBYTE ubHp;
} tBuilding;

typedef struct {
	tBuilding pBuildings[BUILDING_MAX_COUNT];
	UBYTE ubLastIdx;
	
} tBuildingManager;

void buildingManagerReset(void);

UBYTE buildingAdd(
	IN UBYTE ubType
);

UBYTE buildingDamage(
	IN UBYTE ubIdx,
	IN UBYTE ubDamage
);

#endif // GUARD_OF_GAMESTATES_GAME_BUILDING_H
