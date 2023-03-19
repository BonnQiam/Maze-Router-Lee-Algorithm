#include "Lee_Router.h"

extern unsigned long int UIDELAY;
extern unsigned short int GUI_INTERRESULT;


void displayMap(Map map) {
	printf("--> Map: (@%llx)\n",map.map); //Check memory leak
	printf("--> Size = %llu * %llu\n", (unsigned long long int)map.width, (unsigned long long int)map.height);
	for (mapaddr_t y = 0; y < map.height; y++, puts("")) {
		for (mapaddr_t x = 0; x < map.width; x++) {
			printf("%17llx",(unsigned long long int)getMapValueAt(map,x,y));
		}
	}
}

// 返回值为定宽整数类型，在该过程中必然伴随着对 Map 的更新
uint8_t router(Map map, mapdata_t netID) {
	//Find source and dest
	mapdata_t srcX, srcY, destX, destY;
	uint8_t srcFound = 0;
	for (mapaddr_t y = 0; y < map.height; y++) {
		for (mapaddr_t x = 0; x < map.width; x++) {
			if ( getMapSlotType(map,x,y) == mapslot_net && getMapSlotValue(map,x,y) == netID ) {
				if (!srcFound) { //Find the first point: src
					srcX = x;
					srcY = y;
					srcFound = 1;
				}
				else {
					destX = x;
					destY = y;
					x = map.width; //All requirements found, stop search
					y = map.height;
				}
			}
		}
	}
#if(DEBUG)
	printf("> Net %llu. SRC (%llu,%llu). DEST (%llu,%llu)\n",(unsigned long long int)netID,(unsigned long long int)srcX,(unsigned long long int)srcY,(unsigned long long int)destX,(unsigned long long int)destY);
#endif
	
	//Wave
	unsigned long long int placeWave;
	Map newMap = copyMapAsNew(map); //New wave should be read in next iteration; therefore, we read from old map and write to new map
	
	//一次循环为一次扩波操作
	do {
		placeWave = 0; //How mush slots are placed in this iteration
		copyMapM2M(newMap,map);
		
		int count = 1;
		printf("=====================\n");

		for (mapaddr_t y = 0; y < map.height; y++) {
			for (mapaddr_t x = 0; x < map.width; x++) {
#if(DEBUG_ROUTER_WAVE)
				printf("--> Check Point (%llu,%llu):\n",(unsigned long long int)x,(unsigned long long int)y);
#endif
				
				if (getMapSlotType(map,x,y) == mapslot_free) { //For all free slot
					count++;
					struct makeWaveDataXch dataXch;
					dataXch.srcX = srcX;
					dataXch.srcY = srcY;
					dataXch.shouldBePlacedWave = 0; //Unplaced, because we will not place zero-wave. so we use 0 as a flag
					applyNeighbor(map, x, y, makeWave, &dataXch);
					
					if (dataXch.shouldBePlacedWave) { //Placed
						setMapSlotWave(newMap,x,y,dataXch.shouldBePlacedWave);//基于 wave order 设置值，该值会反馈到 TraceBackInit 中
						placeWave++;
					}
				}
				
				else if (x == destX && y == destY) { //For dest

					struct traceBackinitDataXch dataXch;
					dataXch.pathReached = 0;
					applyNeighbor(map, x, y, traceBackInit, &dataXch);//检查扩波操作是否已经产生合法路径
					
					if (dataXch.pathReached) {
#if(DEBUG)
						printf("--> Route found with distance of %llu!\n",(unsigned long long int)dataXch.traceWave);// 读取当前值（即 wave order）来获取路径距离
#endif
						struct traceBackDataXch dataXch2;
						dataXch2.nextX = dataXch.traceX;
						dataXch2.nextY = dataXch.traceY;
						dataXch2.nextWave = dataXch.traceWave;
						
						while (dataXch2.nextWave) {
#if(DEBUG_ROUTER_TRACE)
							printf("----> Mark (%llu,%llu) with wave %llu.\n",(unsigned long long int)dataXch2.nextX,(unsigned long long int)dataXch2.nextY,(unsigned long long int)dataXch2.nextWave);
#endif
							dataXch2.nextWave--; //Multiple route may be found, so nextWave-- cannot be placed in the function
							setMapSlotUsedByNet(map, dataXch2.nextX, dataXch2.nextY, netID);
							applyNeighbor(map, dataXch2.nextX, dataXch2.nextY, traceBack, &dataXch2);
							
						}
						
						//SUCCESS : Clean map
						destroyMap(newMap); //Destroy the workspace map
						cleanMap(map); //Clean all wave
						return 1; //Success
					}
				}
			}
		}
		printf("%d\n",count);
		copyMapM2M(map,newMap);

#if(DEBUG_ROUTER_WAVE)
		printf("> Placed %llu slots.\n",placeWave);
//		displayMap(map);
#endif
		if (GUI_INTERRESULT) {//在config.cfg 文件中配置 gui_interresult = 1
			char guiSignal[200];
			sprintf(guiSignal,"Net %llu waving...",(unsigned long long int) netID);
			//printf("Test!\n");
			saveMap(map,UIDELAY,guiSignal);
		}

	} while (placeWave);

	//FAIL : Clean up map and quit
	destroyMap(newMap); //Destroy the workspace map
	cleanMap(map); //Clean all wave
	return 0; //Fail
}

void makeWave(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct) {
#if(DEBUG_ROUTER_WAVE)
	printf("----> Check neighbor (%llu,%llu): ",(unsigned long long int)x,(unsigned long long int)y);
#endif
						
	//Case 1 - Find surrounding slot is net src
	if (x == (*(struct makeWaveDataXch*)dataStruct).srcX && y == (*(struct makeWaveDataXch*)dataStruct).srcY ) {
		(*(struct makeWaveDataXch*)dataStruct).shouldBePlacedWave = 1;
#if(DEBUG_ROUTER_WAVE)
		fputs("Found SRC: Wave = 1.",stdout);
#endif
	}
						
	//Case 2 - Find surrounding slot is a wave
	else if (getMapSlotType(map,x,y) == mapslot_wave) {
		mapdata_t surroundingWave = getMapSlotValue(map,x,y);
		mapdata_t swp = surroundingWave + 1;// 递增 wave order 顺序
		if ( (*(struct makeWaveDataXch*)dataStruct).shouldBePlacedWave == 0 ) { //Unplaced
			(*(struct makeWaveDataXch*)dataStruct).shouldBePlacedWave = swp;
#if(DEBUG_ROUTER_WAVE)
			printf("Found nearby wave: Wave = %llu.",(unsigned long long int)swp);
#endif
		}
		else { //[R] Placed, then compare
			if ( (*(struct makeWaveDataXch*)dataStruct).shouldBePlacedWave > swp) {
				(*(struct makeWaveDataXch*)dataStruct).shouldBePlacedWave = swp;
#if(DEBUG_ROUTER_WAVE)
				printf(" > Found nearby wave (smaller): Wave = %llu.",(unsigned long long int)swp);
#endif
			}
		}
	}

	//Case 3 - Nothing to Do !
#if(DEBUG_ROUTER_WAVE)
	puts("");
#endif
}

void traceBackInit(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct) {
	if (getMapSlotType(map,x,y) == mapslot_wave) {
		mapdata_t traceWave = getMapSlotValue(map,x,y);// 获取 tracewave 的值，该值在 make wave 过程中设置
		if ( (*(struct traceBackinitDataXch*)dataStruct).pathReached == 0 ) {
			(*(struct traceBackinitDataXch*)dataStruct).pathReached = 1;
			(*(struct traceBackinitDataXch*)dataStruct).traceWave = traceWave;
			(*(struct traceBackinitDataXch*)dataStruct).traceX = x;
			(*(struct traceBackinitDataXch*)dataStruct).traceY = y;
		}
		else { //[R] Multiple path reached, find the one with lowest cost
			if (traceWave < (*(struct traceBackinitDataXch*)dataStruct).traceWave) {
				(*(struct traceBackinitDataXch*)dataStruct).traceWave = traceWave;
				(*(struct traceBackinitDataXch*)dataStruct).traceX = x;
				(*(struct traceBackinitDataXch*)dataStruct).traceY = y;
			}
		}
	}
	// nothing to do !
}

void traceBack(Map map, mapaddr_t x, mapaddr_t y, void* dataStruct) {
	if ( getMapSlotType(map,x,y) == mapslot_wave && getMapSlotValue(map,x,y) == (*(struct traceBackDataXch*)dataStruct).nextWave ) {
		(*(struct traceBackDataXch*)dataStruct).nextX = x;
		(*(struct traceBackDataXch*)dataStruct).nextY = y;
	}
}