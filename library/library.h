#pragma once
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <queue>
#include <map>
#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <stddef.h>
#include <sstream>
#include "profiler/pmpi.h"
#define MAX_DATA 1000
class ITask {
public:
	int blockNumber;
	void virtual Run() = 0;
	void virtual Clear() = 0;
	void virtual GenerateRecv(int sender, MPI_Comm Comm) = 0;
	void virtual GenerateSend(int reciever, MPI_Comm Comm) = 0;
};
extern std::string folderName;
extern int numberOfConnection;
extern int rank, size, size_old, oldClientRank;
extern int countOfConnect;
extern bool changeExist;
extern std::vector<int> map;
extern std::queue<ITask*> currentTasks, queueRecv;
// Communicators
extern MPI_Comm reduceComm;

void AddTask(ITask* t);

void SendTask(MPI_Status &st, MPI_Comm &CommWorker, MPI_Comm &CommMap);
void LibraryInitialize(int argc, char **argv, bool clientProgram);
void CreateLibraryComponents();
void StartWork(bool clientProgram);
void CloseLibraryComponents();