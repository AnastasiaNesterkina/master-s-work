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
		} //DeleteSendedTask
		else if (message[0] == -2) {			
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;
			
			if (sendedTasksCounter[taskNumber] != 0) {
				sendedTasksCounter[taskNumber]--;
				#ifdef PROFILER
					std::string str = "change location in " + std::to_string(peer) + " for task " 
					+ std::to_string(taskNumber) + " counter was "  + std::to_string(sendedTasksCounter[taskNumber]);
					Profiler::AddEvent(str, MapController);			
				#endif
				fprintf(stderr, "%d:: change location in %d for task %d; counter was %d.\n", rank, peer, taskNumber, sendedTasksCounter[taskNumber]);
			}
			if (sendedTasksCounter[taskNumber] == 0) {
				pthread_mutex_lock(&mutex_map_task);		
				mapMessageCount--;
				pthread_mutex_unlock(&mutex_map_task);		
				pthread_mutex_lock(&mutex_send_task);		
					sendedTasksCounter.erase(taskNumber);
				pthread_mutex_unlock(&mutex_send_task);
				#ifdef PROFILER
					std::string str = "map changed for task " + std::to_string(taskNumber) 
					+ "; sendedTaskCounter.size = " + std::to_string(sendedTasksCounter.size());
					Profiler::AddEvent(str, MapController);
				#endif
				fprintf(stderr, "%d:: !!! map changed for task %d; sendedTaskCounter.size = %d.\n", rank, taskNumber, sendedTasksCounter.size());
			}				
		}
		// flag recv SendedTask
		else if (message[0] == -3) {
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;
			fprintf(stderr, "%d:: recv task %d in %d!\n", rank, taskNumber, peer);	
			sendedTasks[taskNumber]->Clear();
			pthread_mutex_lock(&mutex_send_task);
				sendedTasks.erase(taskNumber);
			pthread_mutex_unlock(&mutex_send_task);		
			#ifdef PROFILER				
				std::string str = "delete task " + std::to_string(taskNumber);
				Profiler::AddEvent(str, MapController);	
			#endif
			fprintf(stderr, "%d:: delete task %d.\n", rank, taskNumber);			
		}
		// Close mapController
		else if (message[0] == -1) close = true;
		// Communicator changing 
		else if (message[0] == -10) {
			Comm = newComm;
				
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
	//fprintf(stderr, "%d:: map controller run.\n", rank);
	MPI_Comm Comm = currentComm;
	MPI_Status st;
	MPI_Request s;
	bool close = false;
	int message[2];
	int oldMapMessageCount = 0;
	int cond = -1;
	pthread_mutex_lock(&mutex_map_task);		
		oldMapMessageCount += mapMessageCount;
		mapMessageCount = 0;
	pthread_mutex_unlock(&mutex_map_task);
	MPI_Send(&cond, 1, MPI_INT, rank, MAPCONTROLLER_TAG, Comm);
	
	while (!close || oldMapMessageCount > 0) {
		#ifdef PROFILER
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st, MapController);
		#else
			MPI_Recv(&message, 2, MPI_INT, MPI_ANY_SOURCE, MAPCONTROLLER_TAG, Comm, &st);
		#endif
		// Message from worker in old communicator
		if (message[0] == -3) {			
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;				
			fprintf(stderr, "%d:: om:: recv task %d in %d!\n", rank, taskNumber, peer);	
			sendedTasks[taskNumber]->Clear();
			pthread_mutex_lock(&mutex_send_task);
				sendedTasks.erase(taskNumber);
			pthread_mutex_unlock(&mutex_send_task);	
			#ifdef PROFILER
				std::string str = "delete task " + std::to_string(taskNumber);
				Profiler::AddEvent(str, OldMapController);	
			#endif
			fprintf(stderr, "%d:: om:: delete task %d.\n", rank, taskNumber);			
		}
		// Message from mapController in old communicator
		else if (message[0] == -2) {			
			int taskNumber = message[1];			
			int peer = st.MPI_SOURCE;
			
			if (sendedTasksCounter[taskNumber] != 0) {				
				pthread_mutex_lock(&mutex_send_task);		
				sendedTasksCounter[taskNumber]--;				
				pthread_mutex_unlock(&mutex_send_task);			
				#ifdef PROFILER
				std::string str = "change location in " + std::to_string(peer) + " for task " 
				+ std::to_string(taskNumber) + " counter was "  + std::to_string(sendedTasksCounter[taskNumber]);
				Profiler::AddEvent(str, OldMapController);
				#endif
				fprintf(stderr, "%d:: om:: change location in %d for task %d; counter was %d.\n", rank, peer, taskNumber, sendedTasksCounter[taskNumber]);
			}
			if (sendedTasksCounter[taskNumber] == 0) {
				pthread_mutex_lock(&mutex_map_task);		
				oldMapMessageCount--;
				pthread_mutex_unlock(&mutex_map_task);	
				sendedTasksCounter.erase(taskNumber);
				#ifdef PROFILER
					std::string str = "map changed for task " + std::to_string(taskNumber) 
					+ "; sendedTaskCounter.size = " + std::to_string(sendedTasksCounter.size());
					Profiler::AddEvent(str, OldMapController);
				#endif
				fprintf(stderr, "%d:: om:: !!! map changed for task %d; sendedTaskCounter.size = %d.\n", rank, taskNumber, sendedTasksCounter.size());
			}		
		}
		// Close mapController
		else if (message[0] == -1) close = true;
	}
	
	
	#ifdef PROFILER
		Profiler::AddEvent("old map controller is closed", OldMapController);
	#endif
	fprintf(stderr, "%d:: old map controller is closed.\n", rank);
	
	// Send message to server about changed communicator
	MPI_Send(&cond, 1, MPI_INT, rank, CONNECTION_FINISH_TAG, Comm);
	#ifdef PROFILER
		Profiler::AddEvent("connection is done", StartWorker);
	#endif
	fprintf(stderr, "%d:: connection is done.\n", rank);
	return 0;
}