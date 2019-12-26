#include "pmpi_profiler.cpp"

int PROFILE_MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Comm_dup(comm, newcomm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddDup(begin, end);
	return result;
}

int PROFILE_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddAllReduce(begin, end);
	return result;
}

int PROFILE_MPI_Barrier(MPI_Comm comm) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Barrier(comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddBarrier(begin, end);
	return result;
}

int PROFILE_MPI_Wait(MPI_Request *request, MPI_Status *status) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Wait(request, status);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddWait(begin, end);
	return result;
}

int PROFILE_MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Recv(buf, count, datatype, source, tag, comm, status);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	int peer = status->MPI_SOURCE;
	Profiler::AddRecv(peer, rank, tag, begin, end);
	return result;
}

int PROFILE_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	auto  begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Send(buf, count, datatype, dest, tag, comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddSend(rank, dest, tag, begin, end);
	return result;
}

int PROFILE_MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm, THREAD thread) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Comm_dup(comm, newcomm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddDup(begin, end, thread);
	return result;
}

int PROFILE_MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, THREAD thread) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddAllReduce(begin, end, thread);
	return result;
}

int PROFILE_MPI_Barrier(MPI_Comm comm, THREAD thread) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Barrier(comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddBarrier(begin, end, thread);
	return result;
}

int PROFILE_MPI_Wait(MPI_Request *request, MPI_Status *status, THREAD thread) {
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Wait(request, status);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddWait(begin, end, thread);
	return result;
}

int PROFILE_MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status, THREAD thread){
	auto begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Recv(buf, count, datatype, source, tag, comm, status);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	int peer = status->MPI_SOURCE;
	Profiler::AddRecv(peer, rank, tag, begin, end, thread);
	return result;
}

int PROFILE_MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, THREAD thread) {
	auto  begin = std::chrono::system_clock::now().time_since_epoch();
	int result = MPI_Send(buf, count, datatype, dest, tag, comm);
	auto end = std::chrono::system_clock::now().time_since_epoch();
	Profiler::AddSend(rank, dest, tag, begin, end, thread);
	return result;
}