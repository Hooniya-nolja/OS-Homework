//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2020
// Student Name: Jang Changhun
// Student Number: B511169
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	// struct pageTableEntry *firstLevelPageTable;
	int *firstLevelPageTable;
	FILE *tracefp;
} * procTable;

struct pageFrame {
	int pid;
	int firstPageTableAddr;
	// int frameCount;
	// struct pageFrame *prev;
	// struct pageFrame *next;
} pageFrameQHead, * pageFrame;

// void initPageFrame(int nFrame) {
// 	int i;
// 	pageFrame = (struct pageFrame *)malloc(sizeof(struct pageFrame) * nFrame);
// 	for(i=0; i < nFrame; i++) {
		
// 	}
// }

void oneLevelVMSim(int numProcess, int nFrame) {
	int i, j;
	int scanfValue;
	int pageNum = 0;
	int frameCount = 0;
	int fifo = 0;
	unsigned addr;
	char rw;
				// scanfValue = fscanf(procTable[j].tracefp, "%x %c", &addr, &rw);

	// int numProcess = numProcess;
	// getLeastTraceNum(numProcess);
	// while(scanf)
	printf("Entered in oneLevelVMSim !!!\n");
	while(1) {

		for (j=0; j < numProcess; j++) {
			printf("beforeScan  ");
			scanfValue = fscanf(procTable[j].tracefp, "%x %c", &addr, &rw);
			// fscanf(procTable[j].tracefp, "%x %c", &addr, &rw);
			if (scanfValue <= 0) {
				printf("scanfValue is %d, so Break !\n", scanfValue);
				break;
			}	
			printf("addr : %x,  %d\n", addr, addr);
			addr = ((addr/16)/16)/16;
			printf("floor addr : %x,  %d\n", addr, addr);
			procTable[j].ntraces++;
			printf("size of firstLevelPageTable : %d\n", sizeof(procTable[j].firstLevelPageTable));

			// if (procTable[j].firstLevelPageTable[addr] == -1 || !procTable[j].firstLevelPageTable[addr]) {	// page fault
			if(1){	printf("********* Page fault !!! ***********\n");
				procTable[j].numPageFault++;

				if (frameCount < nFrame) {
					printf("New Frame Created !!!\n");
					pageFrame[frameCount].pid = procTable[j].pid;
					pageFrame[frameCount].firstPageTableAddr = addr;

					// pageFrameQHead.prev->next = &pageFrame[frameCount];
					// pageFrame[frameCount].prev = pageFrameQHead.prev;
					// pageFrameQHead.prev = &pageFrame[frameCount];
					// pageFrame[frameCount].next = &pageFrameQHead;

					// procTable[j].firstLevelPageTable[addr] = frameCount;
					frameCount++;
				} else {
					printf("Frame Changed !!\n");
					if (fifo == nFrame) {
						fifo = 0;
					}
					procTable[j].firstLevelPageTable[pageFrame[fifo]
						.firstPageTableAddr] = -1;
					pageFrame[fifo].pid = procTable[j].pid;
					pageFrame[fifo].firstPageTableAddr = addr;
					procTable[j].firstLevelPageTable[addr] = fifo;
					fifo++;
				}
			} else {	// page hit
				printf("*********** Page hit *************\n");
				procTable[j].numPageHit++;
			}

			printf("afterScan\n");
		}
		if (scanfValue <= 0) {
			printf("scanfValue is %d, so Break !\n", scanfValue);
			break;
		}
		pageNum++;
		printf("NextWhile gogogo !!!!\n");

	}
	for(i=0; i < procTable[0].ntraces; i++) {
		printf("Table %d ===> %x\n", i, procTable[0].firstLevelPageTable[i]);
	}
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
	
}
void twoLevelVMSim() {
	
	// for(i=0; i < numProcess; i++) {
	// 	printf("**** %s *****\n",procTable[i].traceName);
	// 	printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
	// 	printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
	// 	printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
	// 	printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
	// 	assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	// }
}

void invertedPageVMSim() {

	// for(i=0; i < numProcess; i++) {
	// 	printf("**** %s *****\n",procTable[i].traceName);
	// 	printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
	// 	printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
	// 	printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
	// 	printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
	// 	printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
	// 	printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
	// 	assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	// 	assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	// }
}

// void getLeastTraceNum(int numProcess) {
// 	int i;
// 	int leastTraceNum = procTable[0].ntraces;
// 	for(i=0; i < numProcess; i++) {
// 		procTable[i];
// 	}
// }

int main(int argc, char *argv[]) {
	int i, numProcess;
	int c = 4;
	int simType = atoi(argv[1]);
	int firstLevelBits = atoi(argv[2]);
	int phyMemSizeBits = atoi(argv[3]);
	// int nFrame = pow(2, phyMemSizeBits - 12);
	int nFrame = 1 << (phyMemSizeBits - PAGESIZEBITS);
	while(argv[c+1] != NULL) {
		c++;
	}
	numProcess = c - 3;
	printf("numProcess : %d\n", numProcess);

	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
	pageFrame = (struct pageFrame *)malloc(sizeof(struct pageFrame) * nFrame);
	// pageFrameQHead.next = pageFrameQHead.prev = &pageFrameQHead;
	// pageFrameQHead.frameCount = 0;

	// initialize procTable for Memory Simulations
	for(i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i + 4]);
		procTable[i].tracefp = fopen(argv[i + 4],"r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i+4]);
			exit(1);
		}
		procTable[i].traceName = argv[i + 4];
		procTable[i].pid = i;
		procTable[i].ntraces = 0;
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
		procTable[i].firstLevelPageTable = (int *)malloc(sizeof(int));
	}

	procTable[0].firstLevelPageTable[100000] = 100;
	printf("Test : %d\n", procTable[0].firstLevelPageTable[100000]);

	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, nFrame);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, nFrame);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim();
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim();
	}

	return(0);
}
