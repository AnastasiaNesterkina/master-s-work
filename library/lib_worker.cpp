#include "lib_init.h"
void AddTask(ITask* t){
	pthread_mutex_lock(&mutex_get_task);
		currentTasks.push(t);
	pthread_mutex_unlock(&mutex_get_task);
}

bool GetTask(ITask **currTask) {
	pthread_mutex_lock(&mutex_get_task);
	if (currentTasks.empty()) {
		pthread_mutex_unlock(&mutex_get_task);
		return false;
	}
	else {
		*currTask = currentTasks.front();
		currentTasks.pop();
	}
	pthread_mutex_unlock(&mutex_get_task);
	return true;
}

int GetRank(int &sign, int &k, int countOfProcess) {
	if (sign == 1) {
		sign = -1;
		k++;
	}
	else sign = 1;
	int id = rank + sign * k;
	if (id > countOfProcess - 1) id -= countOfProcess;
	else if (id < 0) id += countOfProcess;
	return id;
}
void ExecuteOwnTasks() {
	ITask *currTask;
	// Execution of own tasks
	while (GetTask(&currTask)) {
		// It doesn't matter what communicator is current
		currTask->Run();
		// Creating the queue of executed tasks
		pthread_mutex_lock(&mutex_set_task);
		queueRecv.push(currTask);
		pthread_mutex_unlock(&mutex_set_task);
	}
}
void ExecuteOtherTask(MPI_Comm &Comm, int id, bool &retry) {
	// Send task request
	int existTask = 0;	
	MPI_Status st;
	MPI_Request s;
	#ifdef PROFILER		
		MPI_Send(&existTask, 1, MPI_INT, id, DISPATCHER_TAG, Comm, Worker);
		// Get information about task existing
		MPI_Recv(&existTask, 1, MPI_INT, id, DISPATCHER_TASK_INFO_TAG, Comm, &st, Worker);
	#else		
		MPI_Send(&existTask, 1, MPI_INT, id, DISPATCHER_TAG, Comm);
		// Get information about task existing
		MPI_Recv(&existTask, 1, MPI_INT, id, DISPATCHER_TASK_INFO_TAG, Comm, &st);
	#endif
	std::string str = "";
	// If task exist, worker recieve and execute it
	if (existTask) {
		Task *t = new Task();		
		t->GenerateRecv(id, Comm);		
		
		pthread_mutex_lock(&mutex_set_task);
		queueRecv.push(t);
		pthread_mutex_unlock(&mutex_set_task);
		
		int to_map_message[2] = { -3, t->blockNumber };
		MPI_Isend(&to_map_message, 2, MPI_INT, id, MAPCONTROLLER_TAG, Comm, &s);	
		
		t->Run();
		//fprintf(stderr, "%d:: worker executed %d task.\n", rank, t->blockNumber );
		// This rank can have new task for work
		retry = true; 
		#ifdef PROFILER		
			str += "worker executed " + std::to_string(t->blockNumber) + " task";
			Profiler::AddEvent(str, Worker);
			MPI_Wait(&s, &st, Worker);		
		#else
			MPI_Wait(&s, &st);
		#endif
		return;
		
	}
	else retry = false;
	#ifdef PROFILER
		str += "rank " + std::to_string(id) + " haven't tasks";
		Profiler::AddEvent(str, Worker);
	#endif
	//fprintf(stderr, "%d:: rank %d haven't tasks.\n", rank, id );
}
void ChangeCommunicator(MPI_Comm &Comm, int &newSize) {
	#ifdef PROFILER
		Profiler::AddEvent("worker started changing communicator", Worker);
	#endif
	//fprintf(stderr, "%d:: worker is changing communicator.\n", rank);
	int message = 3;
	MPI_Request req;
	// The message about finished changing of communicator
	//fprintf(stderr, "%d:: worker send message to %d ranks.\n", rank, newSize);
	int s = size_new;
	Comm = newComm;
	#ifdef PROFILER
		for (int i = 0; i < newSize; i++)
			MPI_Send(&message, 1, MPI_INT, i, START_WORK_RECV_TAG, oldComm, Worker);
	#else
		for (int i = 0; i < newSize; i++)
			MPI_Send(&message, 1, MPI_INT, i, START_WORK_RECV_TAG, oldComm);
	#endif
	
	newSize = s;
	#ifdef PROFILER
		Profiler::AddEvent("worker finished changing communicator", Worker);
	#endif
	//fprintf(stderr, "%d:: worker finished changing communicator.\n", rank);
}
// Computational thread
void* worker(void* me) {
	//fprintf(stderr, "%d:: worker run.\n", rank);
	bool close = false;
	MPI_Status st;
	MPI_Request reqCalc, reqChange;
	MPI_Comm Comm = currentComm;
	int flagChange = 0, flagCalc = 0;
	int cond, message;
	// Get message from own rank
	MPI_Irecv(&message, 1, MPI_INT, rank, WORKER_CHANGE_TAG, Comm, &reqChange);	
	MPI_Irecv(&cond, 1, MPI_INT, rank, WORKER_CALC_TAG, Comm, &reqCalc);
	int countOfProcess, newSize = size;	
	while (!close) {
		MPI_Test(&reqChange, &flagChange, &st);
		MPI_Test(&reqCalc, &flagCalc, &st);		
		if (flagCalc != 0){
			if (cond == 1) {
				//fprintf(stderr, "%d:: worker is executing own tasks.\n", rank);
				#ifdef PROFILER
					Profiler::AddEvent("worker is executing own tasks", Worker);
				#endif
				ExecuteOwnTasks();
				countOfProcess = newSize;
				// If new ranks comes, their queue is empty	
				int sign = 1, id, k = 0;
				bool retry = false;
				// Task request from another ranks
				#ifdef PROFILER
					Profiler::AddEvent("worker is executing another tasks", Worker);
				#endif
				//fprintf(stderr, "%d:: worker is executing another tasks.\n", rank);
				id = GetRank(sign, k, countOfProcess);
				for (int i = 0; i < countOfProcess - 1;) {
					MPI_Test(&reqChange, &flagChange, &st);
					if (flagChange) {
						ChangeCommunicator(Comm, newSize);
						MPI_Irecv(&message, 1, MPI_INT, rank, WORKER_CHANGE_TAG, Comm, &reqChange);
						flagChange = false;
					}
					#ifdef PROFILER
						std:: string str = "worker try execute another tasks from " + std::to_string(id) + " rank";
						Profiler::AddEvent(str, Worker);
					#endif
					//fprintf(stderr, "%d:: worker try execute another tasks from %d rank.\n", rank, id);
					ExecuteOtherTask(Comm, id, retry);
					if (!retry) { 
						id = GetRank(sign, k, countOfProcess);
						i++;
					}
				}
				#ifdef PROFILER
					MPI_Send(&cond, 1, MPI_INT, rank, START_WORK_RECV_TAG, Comm, Worker);
					Profiler::AddEvent("worker finished job", Worker);
				#else
					MPI_Send(&cond, 1, MPI_INT, rank, START_WORK_RECV_TAG, Comm);
				#endif
				//fprintf(stderr, "%d:: worker finished job.\n", rank);
				MPI_Irecv(&cond, 1, MPI_INT, rank, WORKER_CALC_TAG, Comm, &reqCalc);
				flagCalc = 0;
			}
			else if (cond == -1) close = true;
		}
		if (flagChange != 0) {
			ChangeCommunicator(Comm, newSize);
			MPI_Irecv(&message, 1, MPI_INT, rank, WORKER_CHANGE_TAG, Comm, &reqChange);
			MPI_Irecv(&cond, 1, MPI_INT, rank, WORKER_CALC_TAG, Comm, &reqCalc);
			flagChange = false;
		}
	}
	#ifdef PROFILER
		Profiler::AddEvent("worker is closed", Worker);
	#endif
	//fprintf(stderr, "%d:: worker is closed.\n", rank);
	return 0;
}

void ChangeMainCommunicator() {
	changeExist = true;
	MPI_Request req;
	int cond = 4;
	// Send message to close old dispatcher
	#ifdef PROFILER		
	MPI_Send(&cond, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm, StartWorker);
	#else		
	MPI_Send(&cond, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm);
	#endif
	cond = -1;
	int to_map_message[2] = { cond, cond };
	
	// Close old mapController 
	#ifdef PROFILER			
	MPI_Send(&to_map_message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, currentComm, StartWorker);
	#else	
	MPI_Send(&to_map_message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, currentComm);
	#endif
	currentComm = newComm;
}

void StartWork(bool clientProgram) {
	MPI_Status st;
	MPI_Request req;
	bool barrier = true;
	int cond = 1, message = 1;
	int count = 0, countOfConnectedWorkers = 0;
	bool connection = false;
	
	size_old = size;
	std::vector<int> flags(size);
	std::vector<int> globalFlags(size);
	if (!clientProgram || condition) {
		MPI_Barrier(barrierComm);
		for (int i = 0; i < countOfWorkers; i++)
			#ifdef PROFILER					
			MPI_Send(&message, 1, MPI_INT, rank, WORKER_CALC_TAG, currentComm, StartWorker);
			#else					
			MPI_Send(&message, 1, MPI_INT, rank, WORKER_CALC_TAG, currentComm);
			#endif
		while (count < countOfWorkers || connection) {
			//fprintf(stderr, "%d:: waiting for message...\n", rank);
			#ifdef PROFILER	
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st, StartWorker);
			#else			
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st);
			#endif
			//fprintf(stderr, "%d:: get condition %d.\n", rank, cond);
			if (cond == 2) {
				//fprintf(stderr, "%d:: connection....\n", rank);
				#ifdef PROFILER
				Profiler::AddEvent("start connection", StartWorker);
				#endif
				connection = true;
				condition = 2;
				for (int i = 0; i < globalFlags.size(); i++) 
					flags[i] = 0;
				flags[rank] = condition;
				MPI_Allreduce(flags.data(), globalFlags.data(), globalFlags.size(), MPI_INT, MPI_SUM, reduceComm);
				
				for (int i = 0; i < globalFlags.size(); i++)
					if (globalFlags[i] == 0) condition = 0;
					
				// Send the message about calculation condition
				if (rank == 0) {
					//fprintf(stderr, "%d:: send condition to client size_old = %d, size = %d.\n", rank, size, size_new);
					for (int k = size; k < size_new; k++)
						#ifdef PROFILER							
						MPI_Send(&condition, 1, MPI_INT, k, CONDITION_TAG, newComm, StartWorker);
						#else									
						MPI_Send(&condition, 1, MPI_INT, k, CONDITION_TAG, newComm);
						#endif
				}
				MPI_Comm_dup(newComm, &serverComm);
				MPI_Comm_dup(newComm, &reduceComm);
				MPI_Comm_dup(newComm, &barrierComm);
				for (int i = 0; i < countOfWorkers; i++)
					#ifdef PROFILER							
					MPI_Send(&cond, 1, MPI_INT, rank_old, WORKER_CHANGE_TAG, currentComm, StartWorker);
					#else									
					MPI_Send(&cond, 1, MPI_INT, rank_old, WORKER_CHANGE_TAG, currentComm);
					#endif
			}
			else if (cond == 3) {
				countOfConnectedWorkers++;
				//fprintf(stderr, "%d:: %d connected workers. sizeOld = %d\n", rank, countOfConnectedWorkers, size);
				if (countOfConnectedWorkers == size * countOfWorkers) {
					ChangeMainCommunicator();
					connection = false;
					countOfConnectedWorkers = 0;
					if (condition) MPI_Barrier(barrierComm);
					size = size_new;
					
					flags.resize(size); globalFlags.resize(size);
				}
			}
			else if (cond == 1) count++;
		}
	}
	while (barrier) {
		condition = 0;
		// Exchange of condition
		for (int i = 0; i < globalFlags.size(); i++) 
				flags[i] = 0;
		MPI_Allreduce(flags.data(), globalFlags.data(), globalFlags.size(), MPI_INT, MPI_SUM, reduceComm);
		 
		bool hasConnection = false;
		for (int i = 0; i < globalFlags.size(); i++)
			if (globalFlags[i] == 2) {
				hasConnection = true;
				break;
			}
		if (!hasConnection) barrier = false;
		if (hasConnection) {
			// message from dispatcher 
			#ifdef PROFILER											
				MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st, StartWorker);
				#else									
				MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st);
				#endif
			// Send the message about calculation condition
			if (rank == 0) {
				//fprintf(stderr, "%d:: send condition to client size_old = %d, size = %d.\n", rank, size, size_new);
				for (int k = size; k < size_new; k++)
					#ifdef PROFILER							
					MPI_Send(&condition, 1, MPI_INT, k, CONDITION_TAG, newComm, StartWorker);
					#else									
					MPI_Send(&condition, 1, MPI_INT, k, CONDITION_TAG, newComm);
					#endif
			}
			MPI_Comm_dup(newComm, &serverComm);
			MPI_Comm_dup(newComm, &reduceComm);
			MPI_Comm_dup(newComm, &barrierComm);
			for (int i = 0; i < countOfWorkers; i++)
				#ifdef PROFILER							
				MPI_Send(&cond, 1, MPI_INT, rank_old, WORKER_CHANGE_TAG, currentComm, StartWorker);
				#else									
				MPI_Send(&cond, 1, MPI_INT, rank_old, WORKER_CHANGE_TAG, currentComm);
				#endif
			connection = true;
			while (connection) {
				#ifdef PROFILER											
				MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st, StartWorker);
				#else									
				MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, START_WORK_RECV_TAG, currentComm, &st);
				#endif
				if (cond == 3) {
					countOfConnectedWorkers++;
					//fprintf(stderr, "%d:: %d connected workers after calculations. sizeOld = %d\n", rank, countOfConnectedWorkers, size);
					if (countOfConnectedWorkers == size * countOfWorkers) {
						ChangeMainCommunicator();
						connection = false;
						countOfConnectedWorkers = 0;		
						//MPI_Barrier(barrierComm);
						size = size_new;						
						flags.resize(size); globalFlags.resize(size);
					}
				}
			}
		}
	}
	//fprintf(stderr, "%d:: sended tasks count = %d\n", rank, sendedTasksCounter.size());
	
	while (!sendedTasksCounter.empty());
	
	//fprintf(stderr, "%d:: barrier after calculations\n", rank);
	MPI_Barrier(reduceComm);
	#ifdef PROFILER
		Profiler::AddEvent("tasks.size = " + std::to_string(queueRecv.size()), StartWorker);
	#endif
}
