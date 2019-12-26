#pragma once
#define MSMPI_NO_DEPRECATE_20
#include <mpi.h>
#include <time.h>
#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#define timeType std::chrono::duration<double>
#define duration_cast std::chrono::duration_cast<std::chrono::milliseconds>
enum THREAD { 
	Worker, Dispatcher, OldDispatcher, 
	MapController, OldMapController, Server,
	StartWorker, Main, NONE 
};
extern int rank, size;
extern std::string folderName;
struct Time {
	timeType begin;
	timeType end;
	double GetLifeSpan() {
		return duration_cast(end - begin).count();
	};
};

struct MainInformation {
	Time time;
	THREAD thread;
	MainInformation() {};
	MainInformation(timeType begin, timeType end) {
		time.begin = begin;
		time.end = end;
		thread = NONE;
	};
	MainInformation(timeType begin, timeType end, THREAD thr) {
		time.begin = begin;
		time.end = end;
		thread = thr;
	};
};

struct MessageInformation {
	int sender;
	int reciever;
	int tag;
	MainInformation information;
	MessageInformation(int send, int recv, int t, timeType begin, timeType end) {
		sender = send;
		reciever = recv;
		tag = t;
		information.time.begin = begin;
		information.time.end = end;
		information.thread = NONE;
	};
	
	MessageInformation(int send, int recv, int t, timeType begin, timeType end, THREAD thread) {
		sender = send;
		reciever = recv;
		tag = t;
		information.time.begin = begin;
		information.time.end = end;
		information.thread = thread;
	};
};

struct EventInformation {
	timeType timer;
	THREAD thread;
	std::string event;
	EventInformation(std::string str, THREAD thr) {
		timer = std::chrono::system_clock::now().time_since_epoch();
		event = str;
		thread = thr;
	}
	EventInformation(std::string str) {
		timer = std::chrono::system_clock::now().time_since_epoch();
		event = str;
		thread = NONE;
	}
};

class Profiler {	
public:
	static void AddEvent(std::string event, THREAD thread);	
	static void AddDup(timeType begin, timeType end);
	static void AddAllReduce(timeType begin, timeType end);
	static void AddBarrier(timeType begin, timeType end);
	static void AddWait(timeType begin, timeType end);
	static void AddSend(int sender, int reciever, int tag, timeType begin, timeType end);	
	static void AddRecv(int sender, int reciever, int tag, timeType begin, timeType end);	
	
	static void AddDup(timeType begin, timeType end, THREAD thread);
	static void AddAllReduce(timeType begin, timeType end, THREAD thread);
	static void AddBarrier(timeType begin, timeType end, THREAD thread);
	static void AddWait(timeType begin, timeType end, THREAD thread);
	static void AddSend(int sender, int reciever, int tag, timeType begin, timeType end, THREAD thread);	
	static void AddRecv(int sender, int reciever, int tag, timeType begin, timeType end, THREAD thread);
	
	static void SetStartTime();	
	static void SynchronizeTime(MPI_Comm comm);
	
	static void PrintDup();
	static void PrintAllReduce();
	static void PrintWait();
	static void PrintBarrier();
	static void PrintSend();	
	static void PrintRecv();
	static void PrintEvents();
	
	static void Init();
};

int PROFILE_MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int PROFILE_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PROFILE_MPI_Barrier( MPI_Comm comm );
int PROFILE_MPI_Wait(MPI_Request *request, MPI_Status *status);
int PROFILE_MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int PROFILE_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int PROFILE_MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm, THREAD thread);
int PROFILE_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, THREAD thread);
int PROFILE_MPI_Barrier( MPI_Comm comm, THREAD thread);
int PROFILE_MPI_Wait(MPI_Request *request, MPI_Status *status, THREAD thread);
int PROFILE_MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status, THREAD thread);
int PROFILE_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, THREAD thread);