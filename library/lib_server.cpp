#include "lib_init.h"
bool kill = false;
void* server(void *me) {
	#ifdef PROFILER
	Profiler::AddEvent("server run", Server);
	#endif
	//fprintf(stderr, "%d:: server run.\n", rank);
	MPI_Comm client;
	MPI_Status st;
	int cond;
	char port_name[MPI_MAX_PORT_NAME];
	int old_size, new_size;
	bool blockConnection = false, close = false;
	newComm = currentComm;
	// Open port
	if (rank == 0) {
		MPI_Open_port(MPI_INFO_NULL, port_name);
		std::ofstream fPort("port_name.txt");
		for (int i = 0; i < MPI_MAX_PORT_NAME; i++)
			fPort << port_name[i];
		fPort.close();
		
		int hour = walltime/60/60;
		int min = walltime/60%60;
		int sec = walltime%60;
		
		std::string qsub = "sh ./scripts/qsub_client.sh " 
		+  std::to_string(nodes) + " " 
		+  std::to_string(ncpus) + " " 
		+ memory + " " 
		+ std::to_string(hour) + ":" + std::to_string(min) + ":" + std::to_string(sec) + " "		
		+ std::to_string(countOfConnect);
		
		const char *cqsub = qsub.c_str();
		for(int i = 0; i < countOfConnect; i++) {			
        	    system(cqsub);
	        }
		if (countOfConnect > 0) {
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
			if (0 != pthread_create(&thrs[countOfWorkers + 5], &attrs, walltimeController, &ids[countOfWorkers + 5])) {
				perror("Cannot create a thread");
				abort();
			}
		}
	}
	for (; numberOfConnection < countOfConnect; ) {
		#ifdef PROFILER
		Profiler::AddEvent("server are ready for connection", Server);
		#endif
		fprintf(stderr, "%d:: server are ready for connection.\n", rank);
		old_size = size;
		// Waiting for new ranks
		MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, serverComm, &client);
		if(closeServer) close = true;		
		MPI_Bcast(&close, 1, MPI_C_BOOL, 0, serverComm);
		#ifdef PROFILER
		Profiler::AddEvent("start communication", Server);
		#endif
		oldComm = currentComm;
		// Creating new communicator for joint ranks group
		MPI_Intercomm_merge(client, false, &newComm);
		MPI_Comm_size(newComm, &new_size);
		MPI_Request req;
		int message = 1;
		numberOfConnection++;
		if (close) message = 0;
		// send to new ranks information about connections count
		#ifdef PROFILER
			if (rank == 0)
				for (int k = old_size; k < new_size; k++) {
					MPI_Send(&numberOfConnection, 1, MPI_INT, k, NUMBEROFCONNECTION_TAG, newComm, Server);
					MPI_Send(folderName.c_str(), 24, MPI_CHAR, k, FOLDER_TAG,  newComm, Server);
				}
			// Send to dispatcher message about new communicator
			MPI_Send(&message, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm, Server);
			// The previous connection must be finished
			MPI_Recv(&cond, 1, MPI_INT, rank, CONNECTION_FINISH_TAG, oldComm, &st, Server);
		#else
			if (rank == 0)
				for (int k = old_size; k < new_size; k++){
					MPI_Send(&numberOfConnection, 1, MPI_INT, k, NUMBEROFCONNECTION_TAG, newComm);
					MPI_Send(folderName.c_str(), 24, MPI_CHAR, k, FOLDER_TAG, newComm);
				}
			// Send to dispatcher message about new communicator
			MPI_Send(&message, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm);
			// The previous connection must be finished
			MPI_Recv(&cond, 1, MPI_INT, rank, CONNECTION_FINISH_TAG, oldComm, &st);	
		#endif
		if (close) break;
	}
	#ifdef PROFILER
	Profiler::AddEvent("server is closed", Server);
	#endif
	fprintf(stderr, "%d:: server is closed;\n", rank);
	return 0;
}
/*
void killServer() {
	kill = true;
	if(rank == 0) {
		//barrier
		MPI_Comm server;
		MPI_Status st;
		double buf[MAX_DATA];
		char port_name[MPI_MAX_PORT_NAME];
		std::ifstream fPort("port_name.txt");
		for (int i = 0; i < MPI_MAX_PORT_NAME; i++)
			fPort >> port_name[i];
		fPort.close();
		port_name;
		fprintf(stderr, "%d:: try connect. %s\n", rank, port_name);
		MPI_Comm_connect(port_name, MPI_INFO_NULL, 0, currentComm, &server);
		fprintf(stderr, "%d:: connect!.\n", rank);
	}
}*/