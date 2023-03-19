#include "Lee_Router.h"

//********************** 全局变量定义
unsigned long int UIDELAY = 0;
unsigned short int PRIORITY_RANDOM_POSSIBILITY = 80;
unsigned short int NEIGHBOR_RANDOM_POSSIBILITY = 80;
unsigned short int GUI_INTERRESULT = 0;
unsigned long long int MAX_RETRY_INDEX = 2;

int main(int argc, char* argv[]) {

/*******************************************************************************
1 - Get user input: What file I am gonna to process (解析输入的命令行参数)
*******************************************************************************/

	//What is the net file's name?
	if (!(argc > 1)) {
		fputs("Missing argument 1: Input net file name.\n",stderr);
		return -1;
	}
	char* netfile = argv[1];
	
	//Get current time
	time_t currentTime;
	time(&currentTime);
	struct tm* localTime = localtime(&currentTime);
	printf("Task start: %s\n",asctime(localTime));
	
	//Random seed
	if (argc > 2) {
		unsigned short int seed = argv[2][0];
		srand(seed);
		printf("Using seed %hu\n",seed);
	}
	else {
		srand(time(NULL)); //Get current time, and use it as random seed
		puts("Using default seed (time).");
	}
	
	//Get config
	FILE* fp = fopen("./config.cfg","r");
	if (!fp) {
		fputs("Cannot read config file.\n",stderr);
		return -1;
	}
	
//	printf("%hu\n",GUI_INTERRESULT);
	
	char buffer[BUFFER_SIZE];
	char setting[BUFFER_SIZE];
	while ( fgets(buffer,sizeof buffer, fp) != NULL) {
		if (buffer[0] == '#' || strlen(buffer) < 5) //Skip comments
			continue;
		
		//GUI browser config
		if ( sscanf(buffer," gui_path_command = *%[^*]s* ",setting) > 0 ) {
			if (strcmp(setting,"manual") == 0) {
				puts("Manual mode, GUI not auto opened.");
			}
			else {
				char cmd[BUFFER_SIZE];
				sprintf(cmd,setting,"gui.html");
				system(cmd); // 调用命令台（cmd）运行相关命令 
				puts("Browser mode, GUI opened.");
				printf("%s\n",cmd); // 命令内容为 "C:\Program Files\Mozilla Firefox\firefox.exe" gui.html
			}
		}
		
		else if ( sscanf(buffer," gui_delay = %lu ",&UIDELAY) ) {}
		else if ( sscanf(buffer," gui_interresult = %hu ",&GUI_INTERRESULT) ) {
//				printf("%hu\n",GUI_INTERRESULT);
				} // 更新 GUI_INTERRESULT
		else if ( sscanf(buffer," max_retry_index = %llu ",&MAX_RETRY_INDEX) ) {}
		else if ( sscanf(buffer," priority_random_index = %hu ",&PRIORITY_RANDOM_POSSIBILITY) ) {}
		else if ( sscanf(buffer," neighbor_random_index = %hu ",&NEIGHBOR_RANDOM_POSSIBILITY) ) {}
	}
	fclose(fp);
		
	printf("GUI delay: %lu ms.\n",UIDELAY);
	printf("Max retry index: %llu .\n",MAX_RETRY_INDEX);
	printf("Possibility to shuffle the net priority: %hu / 255.\n",PRIORITY_RANDOM_POSSIBILITY);
	printf("Possibility to take different neighbor: %hu / 255.\n",NEIGHBOR_RANDOM_POSSIBILITY);
	
	if (GUI_INTERRESULT) puts("Export intermediate result (wave) to GUI.");
	else puts("Skip to export intermediate result (wave) to GUI.");
	
	puts("");

/*******************************************************************************
2 - Read the empty map and netlist, empty map contains only obstructions
*******************************************************************************/

	printf("Reading netsfile: %s\n",netfile);

	mapdata_t netsize;
	Map emptyMap = parser(netfile,&netsize);
	saveMap(emptyMap,UIDELAY,"Init.");
//	puts("\nInit map:");
//	displayMap(emptyMap);

/*******************************************************************************
3 - Router routine
*******************************************************************************/
	
	Map bestMap = copyMapAsNew(emptyMap); //Best solution
	mapdata_t bestMapNetCount = 0;
	
	mapdata_t priorityNetID = 1;
	
	Map workspaceMap = copyMapAsNew(emptyMap);
	
	char guiSignal[200];
	unsigned long long int maxRunCount = MAX_RETRY_INDEX * netsize;
	for (unsigned int currentTry = 0; currentTry < maxRunCount; currentTry++) {
		printf("\n===== RUN %llu/%llu ==============================\n",(unsigned long long int)(currentTry+1),maxRunCount);
		copyMapM2M(workspaceMap,emptyMap);
		mapdata_t placedCount = 0;
		
		//Create netlist (array of low-priority nets)
		mapdata_t netlist[netsize-1];
		mapdata_t idx = 0;
		for (mapdata_t i = 1; i <= netsize; i++)
			if (i != priorityNetID)
				netlist[idx++] = i;
		
		//Randomlize the netlist
		for (mapdata_t i = 0; i < netsize -2; i++) {
			for (mapdata_t j = i; j < netsize -1; j++) {
				if ( (rand() & 0xFF) < PRIORITY_RANDOM_POSSIBILITY ) { //Possibility = x / 255
					mapdata_t temp = netlist[i];
					netlist[i] = netlist[j];
					netlist[j] = temp;
				}
			}
		}
		
#if(DEBUG)
		puts("Priority list:");
		printf("%llu",(unsigned long long int)priorityNetID);
		for (mapdata_t i = 0; i < netsize -1; i++) {
			printf(" --> %llu",(unsigned long long int)netlist[i]);
		}
		puts("");
#endif
		
		//Place the first net
		if (router(workspaceMap,priorityNetID)) { //Success
			placedCount++;
			sprintf(guiSignal, "Net %llu placed. 1/%llu placed.", (unsigned long long int) placedCount, (unsigned long long int) netsize);
			puts(guiSignal);
//			displayMap(workspaceMap);
			saveMap(workspaceMap,UIDELAY,guiSignal);
		}
		else { //Fail
			puts("Unable to place any net. Retry..."); //We have no net placed in this run
			priorityNetID = rand() % netsize + 1; //Start from another net, hope we can get better result
			continue; //Quit current run
		}
		
		//Place the remaining nets
		uint8_t nestLoopSignal = 0;
		for (mapdata_t i = 0; i < netsize -1; i++) {
			if (router(workspaceMap,netlist[i])) { //Success
				placedCount++;
				sprintf(guiSignal,"Net %llu placed. %llu/%llu placed.",(unsigned long long int) netlist[i], (unsigned long long int) placedCount, (unsigned long long int) netsize);
				puts(guiSignal);
//				displayMap(workspaceMap);
				saveMap(workspaceMap,UIDELAY,guiSignal);
				
				if (placedCount > bestMapNetCount) { //Better solution found
					printf("Better solution found. %llu nets placed.\n",(unsigned long long int)placedCount);
					copyMapM2M(bestMap,workspaceMap);
					bestMapNetCount = placedCount;
				}
				
				if (placedCount == netsize) { //All nets placed
					puts("All nets placed. DONE!");
					nestLoopSignal = 1; //Quit all run
					break;
				}
			}
			else { //Fail
				if ( (rand() & 0xFF) < 80) //Retry, or retry with the fail net as priority net
					priorityNetID = netlist[i];
			
				sprintf(guiSignal,"Net %llu failed. %llu/%llu placed. Retry...",(unsigned long long int) netlist[i], (unsigned long long int) placedCount, (unsigned long long int) netsize);
				puts(guiSignal);
//				displayMap(workspaceMap);
				saveMap(workspaceMap,UIDELAY,guiSignal);
				
				nestLoopSignal = 2; //Quit current run
				break;
			}
		}
		if (nestLoopSignal == 1) break;
		else if (nestLoopSignal == 2) continue;
		
	}
		
	puts("\n===== PROCESS END ==============================");
	sprintf(guiSignal,"Final result: %llu nets placed",(unsigned long long int) bestMapNetCount);
	puts(guiSignal);
//	displayMap(bestMap);
	saveMap(bestMap,UIDELAY,guiSignal);
	
	destroyMap(bestMap);
	destroyMap(workspaceMap);
	return 0;
}