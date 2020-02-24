#include "lib_init.h"
#ifdef PROFILER
void SendTask(MPI_Status &st, MPI_Comm &CommWorker, MPI_Comm &CommMap, THREAD thread){
	// reciever rank
	int peer = st.MPI_SOURCE;
	int send = 0;
	ITask *t;
	MPI_Request s;
	// Try get task
	if (GetTask(&t)) {
		send = 1;
		// Send the message about task existing 
		MPI_Send(&send, 1, MPI_INT, peer, DISPATCHER_TASK_INFO_TAG, CommWorker, thread);
		
		int taskNumber = t->blockNumber;
		pthread_mutex_lock(&mutex_send_task);
			sendedTasks.insert({taskNumber, t}); 
			sendedTasksCounter.insert({taskNumber, size_new - 1});	
		pthread_mutex_unlock(&mutex_send_task);		
		
		pthread_mutex_lock(&mutex_map_task);
			mapMessageCount += 1;
		pthread_mutex_unlock(&mutex_map_task);
		
		t->GenerateSend(peer, CommWorker);		
		fprintf(stderr, "%d:: send task %d to %d\n", rank, t->blockNumber, peer);
		int to_map_message[2] = { taskNumber, peer };
		// Send the future task place to all ranks 	
		for (int j = 0; j < size_new; j++) {
			if (j != rank) {
				MPI_Send(&to_map_message, 2, MPI_INT, j, MAPCONTROLLER_TAG, CommMap, thread);
			}
			else map[taskNumber] = peer;
		}
		
	}	// Send the message about task failure
	else MPI_Send(&send, 1, MPI_INT, peer, DISPATCHER_TASK_INFO_TAG, CommWorker, thread);
}
#else
void SendTask(MPI_Status &st, MPI_Comm &CommWorker, MPI_Comm &CommMap){
	// reciever rank
	int peer = st.MPI_SOURCE;
	int send = 0;
	ITask *t;
	MPI_Request s;
	// Try get task
	if (GetTask(&t)) {
		send = 1;
		// Send the message about task existing 
		MPI_Send(&send, 1, MPI_INT, peer, DISPATCHER_TASK_INFO_TAG, CommWorker);
		
		int taskNumber = t->blockNumber;
		pthread_mutex_lock(&mutex_send_task);
			sendedTasks.insert({taskNumber, t}); 
			sendedTasksCounter.insert({taskNumber, size_new - 1});	
		pthread_mutex_unlock(&mutex_send_task);
		
		pthread_mutex_lock(&mutex_map_task);
			mapMessageCount += 1;
		pthread_mutex_unlock(&mutex_map_task);
		
		t->GenerateSend(peer, CommWorker);
	
		int to_map_message[2] = { taskNumber, peer };
		// Send the future task place to all ranks 	
		for (int j = 0; j < size_new; j++) {
			if (j != rank) {
				MPI_Send(&to_map_message, 2, MPI_INT, j, MAPCONTROLLER_TAG, CommMap);
			}
			else map[taskNumber] = peer;
		}
		
	}	// Send the message about task failure
	else MPI_Send(&send, 1, MPI_INT, peer, DISPATCHER_TASK_INFO_TAG, CommWorker);
}
#endif

// Dispatcher for work in old communicator (only tasks sending) 
void* dispatcher_old(void* me) {
	#ifdef PROFILER
		Profiler::AddEvent("old dispatcher run", OldDispatcher);
	#endif
	fprintf(stderr, "%d:: dispetcher_old run\n", rank);	
	MPI_Request req;
	MPI_Comm oldComm_ = currentComm, newComm_ = newComm;
	ITask *t;
	int cond = 2;
	bool close = false;
	while (!close) {
		MPI_Status st;
		#ifdef PROFILER
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, DISPATCHER_TAG, oldComm_, &st, OldDispatcher);			
			// Task request
			if (cond == 0) SendTask(st, currentComm, newComm_, OldDispatcher);
		#else
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, DISPATCHER_TAG, oldComm_, &st);		
			// Task request
			if (cond == 0) SendTask(st, currentComm, newComm_);			
		#endif
			else if (cond == 4) { close = true;	}
	}		
	#ifdef PROFILER
		Profiler::AddEvent("old dispatcher closed", OldDispatcher);
	#endif
	fprintf(stderr, "%d:: old dispatcher is closed.\n", rank);
	return 0;
}

// Dispatcher
void* dispatcher(void* me) {
	size_new = size;
	#ifdef PROFILER
		Profiler::AddEvent("dispatcher run", Dispatcher);
	#endif
	fprintf(stderr, "%d:: dispatcher run.\n", rank);
	MPI_Comm Comm = currentComm;
	ITask *t;
	int cond;
	bool close = false;
	while (!close) {
		MPI_Status st;
		// Get message from any ranks
		#ifdef PROFILER				
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, DISPATCHER_TAG, Comm, &st, Dispatcher);
			// Task request
			if (cond == 0) SendTask(st, Comm, Comm, Dispatcher);	
		#else				
			MPI_Recv(&cond, 1, MPI_INT, MPI_ANY_SOURCE, DISPATCHER_TAG, Comm, &st);
			// Task request
			if (cond == 0) SendTask(st, Comm, Comm);		
		#endif
		// Communicator is changing
		 else if (cond == 1) {
			cond = -10;
			int to_map_message[2] = { cond, cond };
			#ifdef PROFILER
				MPI_Barrier(currentComm, Dispatcher);					
				Profiler::AddEvent("start communication", Dispatcher);
				// Message to mapController about communicator changing
				MPI_Send(&to_map_message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, currentComm, Dispatcher);
				// Communicators should be changed in single time because of map control
				MPI_Barrier(currentComm, Dispatcher);
			#else
				MPI_Barrier(currentComm);				
				// Message to mapController about communicator changing
				MPI_Send(&to_map_message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, currentComm);
				// Communicators should be changed in single time because of map control
				MPI_Barrier(currentComm);
			#endif
			rank_old = rank;
			size_old = size;
			MPI_Request req;
			MPI_Comm oldComm_ = currentComm;
			Comm = newComm;
			MPI_Comm_rank(Comm, &rank);
			MPI_Comm_size(Comm, &size_new);
			#ifdef PROFILER
				// Sending current places of tasks to new ranks
				if (rank == 0) {
					int sizeOfMap = map.size();
					for (int k = size_old; k < size_new; k++) {
						MPI_Send(&sizeOfMap, 1, MPI_INT, k, SIZEOFMAP_TAG, newComm, Dispatcher);
						MPI_Send(map.data(), map.size(), MPI_INT, k, MAP_TAG, newComm, Dispatcher);
					}
				}
				MPI_Barrier(currentComm, Dispatcher);
			#else					
				// Sending current places of tasks to new ranks
				if (rank == 0) {
					int sizeOfMap = map.size();
					for (int k = size_old; k < size_new; k++) {
						MPI_Send(&sizeOfMap, 1, MPI_INT, k, SIZEOFMAP_TAG, newComm);
						MPI_Send(map.data(), map.size(), MPI_INT, k, MAP_TAG, newComm);
					}
				}
				MPI_Barrier(currentComm);
			#endif
			
			MPI_Recv(&cond, 1, MPI_INT, rank, MAPCONTROLLER_TAG, oldComm_, &st);	
			pthread_attr_t attrs;
			if (0 != pthread_attr_init(&attrs)) {
				perror("Cannot initialize attributes");
				abort();
			};
			if (0 != pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED)) {
				perror("Error in setting attributes");
				abort();
			}
			// Create dispatcher which is working in old communicator
			if (0 != pthread_create(&thrs[countOfWorkers + 3], &attrs, dispatcher_old, &ids[countOfWorkers + 3])) {
				perror("Cannot create a thread");
				abort();
			}
			
					
			cond = 2;
			#ifdef PROFILER
				MPI_Send(&cond, 1, MPI_INT, rank_old, START_WORK_RECV_TAG, oldComm_, Dispatcher);			
				Profiler::AddEvent("new dispatcher run", Dispatcher);
			#else
				MPI_Send(&cond, 1, MPI_INT, rank_old, START_WORK_RECV_TAG, oldComm_);		
			#endif
			
	fprintf(stderr, "%d:: map is connected.\n", rank);
		} // Close dispatcher 
		else if (cond == -1) close = true;
		
	}
	#ifdef PROFILER
		Profiler::AddEvent("dispatcher closed", Dispatcher);
	#endif
	fprintf(stderr, "%d:: dispatcher is closed.\n", rank);
	return 0;
}
