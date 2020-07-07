#include "lib_init.h"
void* mapController(void* me) {
	#ifdef PROFILER
		Profiler::AddEvent("map controller run", MapController);
	#endif
	fprintf(stderr, "%d:: map controller run.\n", rank);
	MPI_Comm Comm = currentComm;
	MPI_Status st;
	MPI_Request s;
	bool close = false;
	int message[2];
	while (!close) {
		#ifdef PROFILER
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st, MapController);
		#else
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st);
		#endif
		// Task place was changed	
		if (message[0] >= 0) {
			int peer = st.MPI_SOURCE;
			int map_id = message[0], rank_id = message[1];
			map[map_id] = rank_id;
			if (peer == rank) {				
				sendedTasksCounter.insert({map_id, size_new - 1});	
				mapMessageCount += size_new - 1;
			} else {
				// Send the message about finished changes; map_id - the number of task
				int map_message[2] = { -2, map_id };
				#ifdef PROFILER			
					std::string str = "task " + std::to_string(map_id) + " pass to " + std::to_string(rank_id);
					Profiler::AddEvent(str, MapController);	
					MPI_Send(&map_message, 2, MPI_INT, peer, MAPCONTROLLER_TAG, Comm, MapController);
				#else
					//fprintf(stderr, "%d:: task %d pass to %d\n", rank, map_id, rank_id);
					MPI_Send(&map_message, 2, MPI_INT, peer, MAPCONTROLLER_TAG, Comm);
				#endif
			}
		} //DeleteSendedTask
		else if (message[0] == -2) {			
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;
			int counter = 0;
			
			mapMessageCount--;	
			//fprintf(stderr, "%d:: start change taskcounter in %d for task %d; counter.\n", rank, peer, taskNumber);
			counter = sendedTasksCounter[taskNumber];
			//fprintf(stderr, "%d:: %d for task %d; counter = %d\n", rank, peer, taskNumber, counter);			
			if (counter != 0) {
				counter--;
				sendedTasksCounter[taskNumber]--;
				#ifdef PROFILER
					std::string str = "change location in " + std::to_string(peer) + " for task " 
					+ std::to_string(taskNumber) + " counter was "  + std::to_string(counter);
					Profiler::AddEvent(str, MapController);			
				#endif
				//fprintf(stderr, "%d:: change location in %d for task %d; counter was %d.\n", rank, peer, taskNumber, counter);
			}
			if (counter == 0) {	
				sendedTasksCounter.erase(taskNumber);
				#ifdef PROFILER
					std::string str = "map changed for task " + std::to_string(taskNumber) 
					+ "; sendedTaskCounter.size = " + std::to_string(sendedTasksCounter.size());
					Profiler::AddEvent(str, MapController);
				#endif
				//fprintf(stderr, "%d:: !!! map changed for task %d; sendedTaskCounter.size = %d.\n", rank, taskNumber, sendedTasksCounter.size());
			}				
		}
		// flag recv SendedTask
		else if (message[0] == -3) {
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;
			//fprintf(stderr, "%d:: recv task %d in %d!\n", rank, taskNumber, peer);				
			pthread_mutex_lock(&mutex_send_task);
			sendedTasks[taskNumber]->Clear();
			sendedTasks.erase(taskNumber);		
			pthread_mutex_unlock(&mutex_send_task);
			#ifdef PROFILER				
				std::string str = "delete task " + std::to_string(taskNumber);
				Profiler::AddEvent(str, MapController);	
			#endif
			//fprintf(stderr, "%d:: delete task %d.\n", rank, taskNumber);			
		}
		// Close mapController
		else if (message[0] == -1) close = true;
		// Communicator changing 
		else if (message[0] == -10) {
			pthread_attr_t attrs;
			if (0 != pthread_attr_init(&attrs)) {
				perror("Cannot initialize attributes");
				abort();
			};
			if (0 != pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED)) {
				perror("Error in setting attributes");
				abort();
			}
			
			// Create map controller which is working in old communicator
			if (0 != pthread_create(&thrs[countOfWorkers + 4], &attrs, oldMapController, &ids[countOfWorkers + 4])) {
				perror("Cannot create a thread");
				abort();
			}	
			int cond = 0;
			MPI_Recv(&cond, 1, MPI_INT, rank, MAPCONTROLLER_CONNECTION_TAG, Comm, &st);
			Comm = newComm;
			mapMessageCount = 0;
			#ifdef PROFILER
				Profiler::AddEvent("communicator changed", MapController);
			#endif
				
		}
	}
	#ifdef PROFILER	
		Profiler::AddEvent("map controller is closed", MapController);
	#endif
	fprintf(stderr, "%d:: map controller is closed.\n", rank);
	return 0;
}

void* oldMapController(void* me) {
	#ifdef PROFILER
		Profiler::AddEvent("old map controller run", OldMapController);
	#endif
	//fprintf(stderr, "%d:: old map controller run.\n", rank);
	MPI_Comm Comm = currentComm;
	MPI_Comm nComm = newComm;
	MPI_Status st;
	MPI_Request s;
	bool close = false;
	int message[2];
	int oldMapMessageCount = mapMessageCount;		
	int cond = -5;	
	MPI_Send(&cond, 1, MPI_INT, rank, MAPCONTROLLER_CONNECTION_TAG, Comm);
	fprintf(stderr, "%d:: old map controller start work.\n", rank);
	
	while (!close || oldMapMessageCount > 0) {
		#ifdef PROFILER
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st, MapController);
		#else
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st);
		#endif
		
		if (message[0] == -1) close = true;
		else {
			MPI_Send(&message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, nComm);
			if (message[0] == -2) oldMapMessageCount--;
		}
	}
	#ifdef PROFILER
		Profiler::AddEvent("old map controller is closed", OldMapController);
	#endif
	fprintf(stderr, "%d:: old map controller is closed.\n", rank);
	
	// continue finish connection
	MPI_Send(&cond, 1, MPI_INT, rank, START_WORK_RECV_TAG, Comm);
	#ifdef PROFILER
		Profiler::AddEvent("connection is done", StartWorker);
	#endif
	fprintf(stderr, "%d:: connection is done.\n", rank);
	return 0;
}