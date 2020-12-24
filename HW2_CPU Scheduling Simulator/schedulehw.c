//
// CPU Schedule Simulator Homework
// Student Number : B511169
// Name : 장창훈
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
//#include <io.h>

#define SEED 10

// process states
#define S_IDLE 0
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent
{
	int procid;
	int doneTime;
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} ioDoneEventQueue, *ioDoneEvent;

struct process
{
	int id;
	int len; // for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process *prev;
	struct process *next;
} * procTable;

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute()
{
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	//printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable()
{
	int i;
	for (i = 0; i < NPROC; i++)
	{
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void moveRunProcToReadyQ() {
	runningProc->saveReg0 = cpuReg0;
	runningProc->saveReg1 = cpuReg1;
	readyQueue.prev->next = runningProc;
	runningProc->prev = readyQueue.prev;
	runningProc->next = &readyQueue;
	readyQueue.prev = runningProc;
	readyQueue.len++;
}

void procExecSim(struct process *(*scheduler)())
{
	int pid, qTime = 0, cpuUseTime = 0, nproc = 0, termProc = 0, nioreq = 0;
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;

	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];

	runningProc = &idleProc;
	idleProc.targetServiceTime = -1;

	while (1)
	{
		if (termProc == NPROC) {
			return;
		}
		currentTime++;
		qTime++;
		runningProc->serviceTime++;
		if (runningProc != &idleProc){
			cpuUseTime++;
		}

		// MUST CALL compute() Inside While loop
		compute();

		if (currentTime == nextForkTime)
		{ /* CASE 2 : a new process created */
			if (readyQueue.next == &readyQueue)
			{
				readyQueue.next = &procTable[nproc];
				readyQueue.prev = &procTable[nproc];
				procTable[nproc].prev = &readyQueue;
				procTable[nproc].next = &readyQueue;
			}
			else
			{
				// add new process to readyQueue
				readyQueue.prev->next = &procTable[nproc];
				procTable[nproc].prev = readyQueue.prev;
				procTable[nproc].next = &readyQueue;
				readyQueue.prev = &procTable[nproc];
				// move running process to readyQueue
			}
			readyQueue.len++;
			if (runningProc != &idleProc && runningProc->serviceTime != runningProc->targetServiceTime) {
				if (cpuUseTime != nextIOReqTime && nextIOReqTime != 0 && ioDoneEventQueue.next->doneTime != currentTime) {
					moveRunProcToReadyQ();	
					qTime = 0;
				}
			}
			procTable[nproc].startTime = currentTime;  // or set when it gets CPU?
			nproc++;
			if (nproc < NPROC) {
				nextForkTime = procIntArrTime[nproc] + currentTime;
			} else {
				nextForkTime = 0;
			}
			nextForkTime = procIntArrTime[nproc] + currentTime;
			if (runningProc->serviceTime != runningProc->targetServiceTime ) {
				if (cpuUseTime != nextIOReqTime && nextIOReqTime != 0 && ioDoneEventQueue.next->doneTime != currentTime) {
					scheduler();
					qTime = 0;
				}
			}
		}

		if (qTime == QUANTUM && runningProc != &idleProc && runningProc->serviceTime != runningProc->targetServiceTime && cpuUseTime != nextIOReqTime && ioDoneEventQueue.next->doneTime != currentTime)
		{ /* CASE 1 : The quantum expires */
			moveRunProcToReadyQ();
			qTime = 0;
			scheduler();
		}

		while (ioDoneEventQueue.next->doneTime == currentTime)
		{ /* CASE 3 : IO Done Event */
			struct process *tempBlockedProc;
			tempBlockedProc = blockedQueue.next;
			int isAlreadyTerm = 0;
			int count = 0;
			while(tempBlockedProc->id != ioDoneEventQueue.next->procid && count < blockedQueue.len) {
				tempBlockedProc = tempBlockedProc->next;
				count++;
			}
			if (count >= blockedQueue.len) {
				isAlreadyTerm = 1;
			}
			// 만약 터미네이트일 경우는 어떻게 처리할까.
			if (!isAlreadyTerm) {
				/* delete tempBlockedProc from blockedQueue */
				tempBlockedProc->prev->next = tempBlockedProc->next;
				tempBlockedProc->next->prev = tempBlockedProc->prev;
				/* add tempBlockedProc to readyQueue */
				readyQueue.prev->next = tempBlockedProc;
				tempBlockedProc->prev = readyQueue.prev;
				tempBlockedProc->next = &readyQueue;
				readyQueue.prev = tempBlockedProc;
				
				blockedQueue.len--;
				readyQueue.len++;
			}

			// delete ioDoneEvent from Queue
			ioDoneEventQueue.next = ioDoneEventQueue.next->next;
			ioDoneEventQueue.next->prev = &ioDoneEventQueue;
			ioDoneEventQueue.len--;
			qTime = 0;
			if (runningProc->serviceTime != runningProc->targetServiceTime) {
				if (runningProc != &idleProc) {
					moveRunProcToReadyQ();
				}
				scheduler();
			}
		}

		if (cpuUseTime == nextIOReqTime && nextIOReqTime != 0 && runningProc != &idleProc)
		{ /* CASE 5: reqest IO operations (only when the process does not terminate) */
			/* add running process to blockedQueue */
			int isProcTerm = 1;
			if (runningProc->serviceTime != runningProc->targetServiceTime) {
				runningProc->saveReg0 = cpuReg0;
				runningProc->saveReg1 = cpuReg1;
				if (blockedQueue.next == &blockedQueue) {
					blockedQueue.next = runningProc;
					blockedQueue.prev = runningProc;
					runningProc->prev = &blockedQueue;
					runningProc->next = &blockedQueue;
				} else {
					blockedQueue.prev->next = runningProc;
					runningProc->prev = blockedQueue.prev;
					runningProc->next = &blockedQueue;
					blockedQueue.prev = runningProc;
				}
				blockedQueue.len++;
				isProcTerm = 0;
			}

			/* create ioDoneEvent */
			ioDoneEvent[nioreq].doneTime = ioServTime[nioreq] + currentTime;
			ioDoneEvent[nioreq].procid = runningProc->id;
			/* add ioDoneEvent to Queue */
			if (ioDoneEventQueue.next == &ioDoneEventQueue) {
				ioDoneEventQueue.next = &ioDoneEvent[nioreq];
				ioDoneEventQueue.prev = &ioDoneEvent[nioreq];
				ioDoneEvent[nioreq].prev = &ioDoneEventQueue;
				ioDoneEvent[nioreq].next = &ioDoneEventQueue;
			} else {
				// 순서대로 ioDoneEvent를 큐에 추가.
				struct ioDoneEvent *temp;
				temp = ioDoneEventQueue.next;
				while (ioDoneEvent[nioreq].doneTime > temp->doneTime) {
					temp = temp->next;
				}
				temp->prev->next = &ioDoneEvent[nioreq];
				ioDoneEvent[nioreq].prev = temp->prev;
				ioDoneEvent[nioreq].next = temp;
				temp->prev = &ioDoneEvent[nioreq];
			}
			ioDoneEventQueue.len++;
			nioreq++;
			if (nioreq < NIOREQ) {
				nextIOReqTime = ioReqIntArrTime[nioreq] + cpuUseTime;
			} else {
				nextIOReqTime = 0;
			}
			qTime = 0;
			if (readyQueue.next == &readyQueue) {
				runningProc = &idleProc;
			}
			if (!isProcTerm) {
				scheduler();
			}
		}

		if (runningProc->serviceTime == runningProc->targetServiceTime && runningProc != &idleProc)
		{ /* CASE 4 : the process job done and terminates */
			runningProc->endTime = currentTime;
			runningProc->state = S_TERMINATE;
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			qTime = 0;
			termProc++;
			scheduler();
		}
		// call scheduler() if needed
	}
	 // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
struct process *RRschedule()
{
	if (readyQueue.next != &readyQueue) {
		runningProc = readyQueue.next;
		readyQueue.next->next->prev = &readyQueue;
		readyQueue.next = readyQueue.next->next;
		readyQueue.len--;
		cpuReg0 = runningProc->saveReg0;
		cpuReg1 = runningProc->saveReg1;
	} else {
		runningProc = &idleProc;
	}
}
struct process *SJFschedule()
{
	if (readyQueue.next != &readyQueue) {
		struct process *procLargeTSTime;
		struct process *nextProc;
		procLargeTSTime = readyQueue.next;
		nextProc = readyQueue.next;
		int tsTime = 0;
		while (nextProc->next != &readyQueue){
			nextProc = nextProc->next;
			if (procLargeTSTime->targetServiceTime > nextProc->targetServiceTime) {
				procLargeTSTime = nextProc;
			}
		}
		runningProc = procLargeTSTime;
		procLargeTSTime->next->prev = procLargeTSTime->prev;
		procLargeTSTime->prev->next = procLargeTSTime->next;
		readyQueue.len--;
		cpuReg0 = runningProc->saveReg0;
		cpuReg1 = runningProc->saveReg1;
	} else {
		runningProc = &idleProc;
	}
}
struct process *SRTNschedule()
{
}
struct process *GSschedule()
{
}
struct process *SFSschedule()
{
}

void printResult()
{
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime = 0, totalProcServTime = 0, totalIOReqIntArrTime = 0, totalIOServTime = 0;
	long totalWallTime = 0, totalRegValue = 0;
	for (i = 0; i < NPROC; i++)
	{
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0 + procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
	}
	for (i = 0; i < NPROC; i++)
	{
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for (i = 0; i < NIOREQ; i++)
	{
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}

	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float)totalProcIntArrTime / NPROC, (float)totalProcServTime / NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float)totalIOReqIntArrTime / NIOREQ, (float)totalIOServTime / NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC, NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float)totalWallTime / NPROC, (float)totalRegValue / NPROC);
}

int main(int argc, char *argv[])
{
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;

	if (argc < 12)
	{
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n", argv[0]);
		exit(1);
	}

	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);

	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);

	srandom(SEED);

	// allocate array structures
	procTable = (struct process *)malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *)malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *)malloc(sizeof(int) * NPROC);
	procServTime = (int *)malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *)malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *)malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	// readyQueueHead.next = &readyQueueTail;
	// readyQueueTail.prev = &read	yQueueHead;
	// readyQueueHead.prev = readyQueueTail.next = NULL;

	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len = readyQueue.len = blockedQueue.len = 0;

	// generate process interarrival times
	for (i = 0; i < NPROC; i++)
	{
		procIntArrTime[i] = random() % (MAX_INT_ARRTIME - MIN_INT_ARRTIME + 1) + MIN_INT_ARRTIME;
	}

	// assign service time for each process
	for (i = 0; i < NPROC; i++)
	{
		procServTime[i] = random() % (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];
	}

	ioReqAvgIntArrTime = totalProcServTime / (NIOREQ + 1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);

	// generate io request interarrival time
	for (i = 0; i < NIOREQ; i++)
	{
		ioReqIntArrTime[i] = random() % ((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME) * 2 + 1) + MIN_IOREQ_INT_ARRTIME;
	}

	// generate io request service time
	for (i = 0; i < NIOREQ; i++)
	{
		ioServTime[i] = random() % (MAX_IO_SERVTIME - MIN_IO_SERVTIME + 1) + MIN_IO_SERVTIME;
	}

#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for (i = 0; i < NPROC; i++)
	{
		printf("%d ", procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for (i = 0; i < NPROC; i++)
	{
		printf("%d ", procTable[i].targetServiceTime);
	}
	printf("\n");
#endif

	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n", MIN_IOREQ_INT_ARRTIME,
		   (ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME) * 2 + MIN_IOREQ_INT_ARRTIME);

#ifdef DEBUG
	printf("IO Req Interarrival Time :\n");
	for (i = 0; i < NIOREQ; i++)
	{
		printf("%d ", ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for (i = 0; i < NIOREQ; i++)
	{
		printf("%d ", ioServTime[i]);
	}
	printf("\n");
#endif

	struct process *(*schFunc)();
	switch (SCHEDULING_METHOD)
	{
	case 1:
		schFunc = RRschedule;
		break;
	case 2:
		schFunc = SJFschedule;
		break;
	case 3:
		schFunc = SRTNschedule;
		break;
	case 4:
		schFunc = GSschedule;
		break;
	case 5:
		schFunc = SFSschedule;
		break;
	default:
		printf("ERROR : Unknown Scheduling Method\n");
		exit(1);
	}
	initProcTable();
	procExecSim(schFunc);
	printResult();
}
