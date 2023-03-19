#include "Lee_Router.h"

/************* Constructor *************/

//To create an empty map, all slots in this map are empty (not used)
Map createMap(mapaddr_t width, mapaddr_t height) {
	Map newMap;
	newMap.width = width;
	newMap.height = height;
	newMap.map = calloc(width * height, sizeof(mapdata_t));
	if (newMap.map == NULL) {
		fputs("SYSTEM ERROR\tCannot alloc memory.",stderr);
	}
	return newMap;
}

//To create a map exactly like the given map
Map copyMapAsNew(Map srcMap) {
	Map newMap;
	newMap.width = srcMap.width;
	newMap.height = srcMap.height;
	newMap.map = malloc( sizeof(mapdata_t) * newMap.width * newMap.height );
	if (newMap.map == NULL) {
		fputs("SYSTEM ERROR\tCannot alloc memory.",stderr);
	}
	for (mapaddr_t i = 0; i < newMap.height; i++) {
		for (mapaddr_t j = 0; j < newMap.width; j++) {
			setMapValueAt(newMap, j, i, getMapValueAt(srcMap, j, i));
		}
	}
	return newMap;
}

//To copy the content of one map to another, without create a new one, map should be same size
void copyMapM2M(Map destMap, Map srcMap) {
	if (destMap.width != srcMap.width || destMap.height != srcMap.height) {
		fputs("Map not aligned.\n",stderr);
		exit(-2);
	}
	for (mapaddr_t i = 0; i < destMap.height; i++) {
		for (mapaddr_t j = 0; j < destMap.width; j++) {
			setMapValueAt(destMap, j, i, getMapValueAt(srcMap, j, i));
		}
	}
}

//Destroy a map, release the map slot memory space
void destroyMap(Map targetMap) {
	free(targetMap.map);
}

/************* Getter, setter, checker *************/

//Set a slot to be obstruction
void setMapSlotObstruction(Map writeMap, mapaddr_t x, mapaddr_t y) {
	setMapValueAt(writeMap,x,y,MAPDATA_T_HALF);
}

//Set a slot to be used by a net
void setMapSlotUsedByNet(Map writeMap, mapaddr_t x, mapaddr_t y, mapdata_t netID) {
	setMapValueAt(writeMap,x,y,MAPDATA_T_HALF|netID);
}

void setMapSlotWave(Map writeMap, mapaddr_t x, mapaddr_t y, mapdata_t wave) {
	setMapValueAt(writeMap,x,y,(MAPDATA_T_HALF-1)&wave);
}

void setMapSlotFree(Map writeMap, mapaddr_t x, mapaddr_t y) {
	setMapValueAt(writeMap,x,y,0);
}

//Get map slot attribute (free, wave, obstruction, used by net)
mapslot_type getMapSlotType(Map checkMap, mapaddr_t x, mapaddr_t y) {
	mapdata_t value = getMapValueAt(checkMap,x,y);
	if (!value)				return mapslot_free;
	else if (value == MAPDATA_T_HALF)	return mapslot_obstruction;
	else if (value&MAPDATA_T_HALF)		return mapslot_net;
	else					return mapslot_wave;
}

//After check the map slot type, get the value of it, either netID or wave, or 0 for free/obstruction
mapdata_t getMapSlotValue(Map checkMap, mapaddr_t x, mapaddr_t y) {
	return getMapValueAt(checkMap,x,y) & (MAPDATA_T_HALF-1);
}

/************* Util *************/

//Get the content of a slot in the map, using x-y position
mapdata_t getMapValueAt(Map readMap, mapaddr_t x, mapaddr_t y) {
	return readMap.map[ y * readMap.width + x ];
}

//Set the content of a slot in the map, using x-y position
void setMapValueAt(Map writeMap, mapaddr_t x, mapaddr_t y, mapdata_t value) {
	writeMap.map[ y * writeMap.width + x ] = value;
}
