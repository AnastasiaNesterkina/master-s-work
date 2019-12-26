#include "pmpi.h"
#include <fstream>
#include <iostream>
#include <sstream>
std::string extension = ".txt";
pthread_mutex_t mutex_send, mutex_recv, mutex_event, 
mutex_dup, mutex_allreduce, mutex_barrier, mutex_wait;

std::vector<MainInformation> barrier;
std::vector<MainInformation> allReduce;
std::vector<MainInformation> wait;
std::vector<MainInformation> dup;

std::vector<MessageInformation> recv;
std::vector<MessageInformation> send;

std::vector<EventInformation> events;

double startTime, startSystemTime, diffTime;

void Profiler::AddEvent(std::string event, THREAD thread) {
	pthread_mutex_lock(&mutex_event);
	events.push_back(EventInformation(event, thread));
	pthread_mutex_unlock(&mutex_event);
}

void Profiler::AddDup(timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_dup);
	dup.push_back(MainInformation(begin, end));
	pthread_mutex_unlock(&mutex_dup);
}

void Profiler::AddAllReduce(timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_allreduce);
	allReduce.push_back(MainInformation(begin, end));
	pthread_mutex_unlock(&mutex_allreduce);
}

void Profiler::AddBarrier(timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_barrier);
	barrier.push_back(MainInformation(begin, end));
	pthread_mutex_unlock(&mutex_barrier);
}

void Profiler::AddWait(timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_wait);
	wait.push_back(MainInformation(begin, end));
	pthread_mutex_unlock(&mutex_wait);
}

void Profiler::AddSend(int sender, int reciever, int tag, timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_send);
	send.push_back(MessageInformation(sender, reciever, tag, begin, end));
	pthread_mutex_unlock(&mutex_send);
};

void Profiler::AddRecv(int sender, int reciever, int tag, timeType begin, timeType end) {
	pthread_mutex_lock(&mutex_recv);
	recv.push_back(MessageInformation(sender, reciever, tag, begin, end));
	pthread_mutex_unlock(&mutex_recv);
};

void Profiler::AddDup(timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_dup);
	dup.push_back(MainInformation(begin, end, thread));
	pthread_mutex_unlock(&mutex_dup);
}

void Profiler::AddAllReduce(timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_allreduce);
	allReduce.push_back(MainInformation(begin, end, thread));
	pthread_mutex_unlock(&mutex_allreduce);
}

void Profiler::AddBarrier(timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_barrier);
	barrier.push_back(MainInformation(begin, end, thread));
	pthread_mutex_unlock(&mutex_barrier);
}

void Profiler::AddWait(timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_wait);
	wait.push_back(MainInformation(begin, end, thread));
	pthread_mutex_unlock(&mutex_wait);
}

void Profiler::AddSend(int sender, int reciever, int tag, timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_send);
	send.push_back(MessageInformation(sender, reciever, tag, begin, end, thread));
	pthread_mutex_unlock(&mutex_send);
};

void Profiler::AddRecv(int sender, int reciever, int tag, timeType begin, timeType end, THREAD thread) {
	pthread_mutex_lock(&mutex_recv);
	recv.push_back(MessageInformation(sender, reciever, tag, begin, end, thread));
	pthread_mutex_unlock(&mutex_recv);
};

void Profiler::SetStartTime() {
	startTime = duration_cast(std::chrono::system_clock::now().time_since_epoch()).count();
};

void Profiler::SynchronizeTime(MPI_Comm comm) {
	double minTime = 0;
	MPI_Barrier(comm);
	double time = duration_cast(std::chrono::system_clock::now().time_since_epoch()).count();	
	MPI_Allreduce(&time, &minTime, 1, MPI_DOUBLE, MPI_MIN, comm);
	diffTime = time - minTime;
	startTime -= diffTime;
	MPI_Allreduce(&startTime, &startSystemTime, 1, MPI_DOUBLE, MPI_MIN, comm);
	MPI_Allreduce(&time, &minTime, 1, MPI_DOUBLE, MPI_MAX, comm);
	if(rank == 0) {
		std::string fileName = "src/source/" + folderName + "info.txt";
		std::ofstream f(fileName);
		f <<"size,maxTime\n" << size << "," << minTime - startSystemTime;
	}
};

void Profiler::PrintWait() {
	std::string fileName = "src/source/" + folderName + "wait/wait" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < wait.size(); i++) {
		auto begin = duration_cast(wait[i].time.begin).count() - startSystemTime;
		auto end = duration_cast(wait[i].time.end).count() - startSystemTime;
		f << wait[i].thread << "," << begin << "," << end
		<< "," << wait[i].time.GetLifeSpan() << "\n";
	}
	f.close();
	wait.clear();
};

void Profiler::PrintAllReduce() {
	std::string fileName = "src/source/" + folderName + "allReduce/allReduce" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < allReduce.size(); i++) {
		auto begin = duration_cast(allReduce[i].time.begin).count() - startSystemTime;
		auto end = duration_cast(allReduce[i].time.end).count() - startSystemTime;
		f << allReduce[i].thread << "," << begin << "," << end
		<< "," << allReduce[i].time.GetLifeSpan() << "\n";
	}
	f.close();
	allReduce.clear();
};

void Profiler::PrintDup() {
	std::string fileName = "src/source/" + folderName + "dup/dup" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < dup.size(); i++) {
		auto begin = duration_cast(dup[i].time.begin).count() - startSystemTime;
		auto end = duration_cast(dup[i].time.end).count() - startSystemTime;
		f << dup[i].thread << "," << begin << "," << end
		<< "," << dup[i].time.GetLifeSpan() << "\n";
	}
	f.close();
	dup.clear();
};

void Profiler::PrintBarrier() {
	std::string fileName = "src/source/" + folderName + "barrier/barrier" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < barrier.size(); i++) {
		auto begin = duration_cast(barrier[i].time.begin).count() - startSystemTime;
		auto end = duration_cast(barrier[i].time.end).count() - startSystemTime;
		f << barrier[i].thread << "," << begin << "," << end
		<< "," << barrier[i].time.GetLifeSpan() << "\n";
	}
	f.close();
	barrier.clear();
};

void Profiler::PrintSend() {
	std::string fileName = "src/source/" + folderName + "send/send" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << ",tag,"  << "sender" << "," 
		<< "reciever" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < send.size(); i++) {
		auto begin = duration_cast(send[i].information.time.begin).count() - startSystemTime;
		auto end = duration_cast(send[i].information.time.end).count() - startSystemTime;
		f << send[i].information.thread << "," << send[i].tag << "," << send[i].sender << "," 
		<< send[i].reciever << "," << begin << "," << end
		<< "," << send[i].information.time.GetLifeSpan() << "\n";
	}
	f.close();
	send.clear();
};

void Profiler::PrintRecv() {
	std::string fileName = "src/source/" + folderName + "recv/recv" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "thread" << ",tag," << "sender" << "," 
		<< "reciever" << "," << "begin" << "," << "end"
		<< "," << "lifeSpan" << "\n";
	for (int i = 0; i < recv.size(); i++) {
		auto begin = duration_cast(recv[i].information.time.begin).count() - startSystemTime;
		auto end = duration_cast(recv[i].information.time.end).count() - startSystemTime;
		f << recv[i].information.thread << "," << recv[i].tag << "," << recv[i].sender << "," 
		<< recv[i].reciever << "," <<  begin << "," <<  end
		<< "," << recv[i].information.time.GetLifeSpan() << "\n";
	}
	f.close();
	recv.clear();
};

void Profiler::PrintEvents() {
	std::string fileName = "src/source/" + folderName + "events/events" + std::to_string(rank) + extension;
	std::ofstream f(fileName);
	f << "," << "thread" << "," << "time" << "," 
		<< "event" << "\n";
	for (int i = 0; i < events.size(); i++) {
		auto time = duration_cast(events[i].timer).count() - startSystemTime;
		f << "," << events[i].thread << "," << time << "," 
		<< events[i].event << "\n";
	}
	f.close();
	events.clear();
};

void Profiler::Init() {
	pthread_mutex_init(&mutex_send, NULL);
	pthread_mutex_init(&mutex_recv, NULL);
	pthread_mutex_init(&mutex_event, NULL);
	pthread_mutex_init(&mutex_allreduce, NULL);
	pthread_mutex_init(&mutex_barrier, NULL);
	pthread_mutex_init(&mutex_dup, NULL);
	pthread_mutex_init(&mutex_wait, NULL);
};