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
	int numPageTable;
	// struct pageTableEntry *firstLevelPageTable;
	int *firstLevelPageTable;
	FILE *tracefp;
} * procTable;

struct pageFrame {
	int pid;
	int firstPageTableAddr;
	int id;
	struct pageFrame *prev;
	struct pageFrame *next;
} pageFrameQHead, * pageFrame;

void oneLevelVMSim(int numProcess, int nFrame, int simType) {
	int i, j, k;
	int scanfValue;
	int tableIndex = -1;
	int hit = 0;
	int pageNum = 0;
	int frameCount = 0;
	int fifo = 0;
	int zeroAddrCheck = 0;
	unsigned addr;
	char rw;
	int frameId;
	int test1, test2;
	int update;

	while(1) {
		for (j=0; j < numProcess; j++) {
			scanfValue = fscanf(procTable[j].tracefp, "%x %c", &addr, &rw);
			if (scanfValue <= 0) {
				break;
			}	
			addr = ((addr/16)/16)/16;
			procTable[j].ntraces++;

			for (k=0; k < nFrame; k++) {
				if (pageFrame[k].firstPageTableAddr == addr && pageFrame[k].pid == j) {
					hit = 1;
					update = k;
					break;
				}
			}

			if (hit == 0) {	// Page Fault !!
				procTable[j].numPageFault++;

				if (frameCount < nFrame) {
					pageFrame[frameCount].pid = procTable[j].pid;
					pageFrame[frameCount].firstPageTableAddr = addr;

					pageFrameQHead.prev->next = &pageFrame[frameCount];
					pageFrame[frameCount].prev = pageFrameQHead.prev;
					pageFrameQHead.prev = &pageFrame[frameCount];
					pageFrame[frameCount].next = &pageFrameQHead;

					procTable[j].firstLevelPageTable[addr] = pageFrame[frameCount].id;
					frameCount++;
				} else {
					frameId = pageFrameQHead.next->id;

					procTable[pageFrame[frameId].pid]
						.firstLevelPageTable[pageFrame[frameId].firstPageTableAddr] = -1;
					
					pageFrameQHead.next = pageFrame[frameId].next;
					pageFrame[frameId].next->prev = &pageFrameQHead;

					pageFrameQHead.prev->next = &pageFrame[frameId];
					pageFrame[frameId].prev = pageFrameQHead.prev;
					pageFrame[frameId].next = &pageFrameQHead;
					pageFrameQHead.prev = &pageFrame[frameId];

					pageFrame[frameId].pid = j;
					pageFrame[frameId].firstPageTableAddr = addr;
					procTable[j].firstLevelPageTable[addr] = pageFrame[frameId].id;
				}
			} else {	// Page Hit !!
				// printf("*********** Page hit *************\n");
				if (simType == 1) {
					if (pageFrame[update].next != &pageFrameQHead) {
						pageFrame[update].prev->next = pageFrame[update].next;
						pageFrame[update].next->prev = pageFrame[update].prev;

						pageFrameQHead.prev->next = &pageFrame[update];
						pageFrame[update].prev = pageFrameQHead.prev;
						pageFrame[update].next = &pageFrameQHead;
						pageFrameQHead.prev = &pageFrame[update];
					}
				}
				procTable[j].numPageHit++;
				hit = 0;
			}
		}
		if (scanfValue <= 0) {
			break;
		}
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
	// int i;
	

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
	int i, j, numProcess;
	int c = 4;
	int simType = atoi(argv[1]);
	int firstLevelBits = atoi(argv[2]);
	int phyMemSizeBits = atoi(argv[3]);
	int memoryTest;
	// int nFrame = pow(2, phyMemSizeBits - 12);
	int nFrame = 1 << (phyMemSizeBits - PAGESIZEBITS);
	while(argv[c+1] != NULL) {
		c++;
	}
	numProcess = c - 3;
	// printf("numProcess : %d\n", numProcess);

	memoryTest = nFrame * 10000;
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
	pageFrame = (struct pageFrame *)malloc(sizeof(struct pageFrame) * 1000001);
	pageFrameQHead.next = pageFrameQHead.prev = &pageFrameQHead;
	pageFrameQHead.id = -1;
	for (j=0; j < nFrame; j++) {
		pageFrame[j].id = j; 
		pageFrame[j].firstPageTableAddr = -1;
	}

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
		procTable[i].numPageTable = 0;
		procTable[i].firstLevelPageTable = (int *)malloc(sizeof(int) * 1000001);
	}

	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, nFrame, simType);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(numProcess, nFrame, simType);
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
