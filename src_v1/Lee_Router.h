#ifndef LEE_ROUTER_H
#define LEE_ROUTER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//************************ 常量定义
#define DEBUG 1
#define DEBUG_PARSER 1
#define DEBUG_ROUTER_WAVE 1
#define DEBUG_ROUTER_TRACE 1

#define BUFFER_SIZE 255

#define MAPDATA_T_HALF (~(((mapdata_t)-1)>>1)) //~0b0111111...=0b10000000...

#define MAP_OUTPUT_FILE "./map.html"

#define BUFFER_SIZE 255

//********************* Typedef
typedef unsigned long int mapdata_t;//Date type used for each slot in the map
typedef unsigned long int mapaddr_t;//Data type used to represent the x-y position of map
typedef enum mapslot_type {mapslot_free, mapslot_wave, mapslot_obstruction, mapslot_net} mapslot_type;

//******************** 结构体定义
//Structure used to represents a map
typedef struct Map {
	mapaddr_t width;
	mapaddr_t height;
	mapdata_t* map;
} Map; 

//Find the correct wave value that should be write to the current slot
struct makeWaveDataXch {
	mapaddr_t srcX;
	mapaddr_t srcY;
	mapdata_t shouldBePlacedWave;
};

//Check if any path reaches the dest
struct traceBackinitDataXch {
	mapaddr_t traceX;
	mapaddr_t traceY;
	mapdata_t traceWave;
	uint8_t pathReached;
};

//Find a nearby slot that has the correct wave value
struct traceBackDataXch {
	mapaddr_t nextX;
	mapaddr_t nextY;
	mapdata_t nextWave;
};

//********************* Map 相关函数声明

//********************* Parser 相关函数声明
Map parser(char* filename, mapdata_t *netCount);//Read a file contains map size, obstructions and nets, return the map

//? Constructor/
Map createMap(mapaddr_t width, mapaddr_t height);//To create an empty map, all slots in this map are empty (not used)
Map copyMapAsNew(Map srcMap);//To create a map exactly like the given map
void copyMapM2M(Map destMap, Map srcMap);//To copy the content of one map to another, without create a new one, map should be same size
void destroyMap(Map targetMap);//Destroy a map, release the map slot memory space
//? Getter, setter, checker/
void setMapSlotObstruction(Map writeMap, mapaddr_t x, mapaddr_t y);//Set a slot to be obstruction
void setMapSlotUsedByNet(Map writeMap, mapaddr_t x, mapaddr_t y, mapdata_t netID);//Set a slot to be used by a net
void setMapSlotWave(Map writeMap, mapaddr_t x, mapaddr_t y, mapdata_t wave);
void setMapSlotFree(Map writeMap, mapaddr_t x, mapaddr_t y);
mapslot_type getMapSlotType(Map checkMap, mapaddr_t x, mapaddr_t y);//Get map slot attribute (free, wave, obstruction, used by net)
mapdata_t getMapSlotValue(Map checkMap, mapaddr_t x, mapaddr_t y);//After check the map slot type, get the value of it, either netID or wave, or 0 for free/obstruction
//? Util /
mapdata_t getMapValueAt(Map, mapaddr_t, mapaddr_t);//Get the content of a slot in the map, using x-y position
void setMapValueAt(Map, mapaddr_t, mapaddr_t, mapdata_t);//Set the content of a slot in the map, using x-y position

//********************* XMap 相关函数声明
void cleanMap(Map map);//Clean a map, remove all waves
void applyNeighbor(Map map, mapaddr_t selfX, mapaddr_t selfY, void (*function)(), void *functionData);//For all neberhood, do function. Using *functionData to pass value to the function by reference


//********************* Map2Html 相关函数声明
void saveMap(Map map, unsigned long int delayTime, char* signal);

//********************* Core_Function 相关函数声明

void displayMap(Map);//Display map in cmd
uint8_t router(Map, mapdata_t);//Give the map, return 1 if net placed, return 0 if fail. Map will be modified (mark slots used by net)//, or CONTAMINATED IF FAIL (Map slots is pass by reference)
void makeWave(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct);
void traceBackInit(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct);
void traceBack(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct);

#endif