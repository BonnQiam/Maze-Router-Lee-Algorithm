#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* This file contains subroutines that is not native map routine
 * but is useful for the routing routine
 * Consider map.c as a ganaral map libeary, and this is an router extension
*/

//Clean a map, remove all waves
void cleanMap(Map map) {
	for (mapaddr_t i = 0; i < map.height; i++) {
		for (mapaddr_t j = 0; j < map.width; j++) {
			if (getMapSlotType(map,j,i) == mapslot_wave)
				setMapSlotWave(map,j,i,0); //Wave 0 is free
		}
	}
}

//For all neberhood, do function. Using *functionData to pass value to the function by reference
void applyNeighbor(Map map, mapaddr_t selfX, mapaddr_t selfY, void (*function)(), void *functionData) {
	mapaddr_t surroundingXY[4][2] = {{selfX,selfY-1},{selfX,selfY+1},{selfX-1,selfY},{selfX+1,selfY}}; //Up, down, right, left
	for (uint8_t i = 0; i < 4; i++) {
		mapaddr_t sx = surroundingXY[i][0], sy = surroundingXY[i][1];
		if (sx >= 0 && sx < map.width && sy >= 0 && sy < map.height) { //Must in the map <-- sx=surroundingX
			function(map,sx,sy,functionData);
		}
	}
}