#include "lib_init.h"

/*int DISPATCHER_TAG = MPI_TAG_UB;
int DISPATCHER_TASK_INFO_TAG = MPI_TAG_UB - 1;
int MAPCONTROLLER_TAG = MPI_TAG_UB - 2;
int SIZEOFMAP_TAG = MPI_TAG_UB - 3;
int MAP_TAG = MPI_TAG_UB - 4;
int CONDITION_TAG = MPI_TAG_UB - 5;
int FOLDER_TAG = MPI_TAG_UB - 6;
int NUMBEROFCONNECTION_TAG = MPI_TAG_UB - 7;
int CONNECTION_FINISH_TAG = MPI_TAG_UB - 8;
int START_WORK_RECV_TAG = MPI_TAG_UB - 9;
int WORKER_CALC_TAG = MPI_TAG_UB - 10;
int WORKER_CHANGE_TAG = MPI_TAG_UB - 11;*/

int DISPATCHER_TAG = 10000000;
int DISPATCHER_TASK_INFO_TAG = 100000001;
int MAPCONTROLLER_TAG = 100000002;
int SIZEOFMAP_TAG = 100000003;
int MAP_TAG = 100000004;
int CONDITION_TAG = 100000005;
int FOLDER_TAG = 100000006;
int NUMBEROFCONNECTION_TAG = 100000007;
int CONNECTION_FINISH_TAG = 100000008;
int START_WORK_RECV_TAG = 100000009;
int WORKER_CALC_TAG = 100000010;
int WORKER_CHANGE_TAG = 100000011;

// Descriptors for threads
pthread_t thrs[12];
// id for threads
int ids[12] = { 0,1,2,3,4,5,6,7,8,9,10, 11 };
int numberOfConnection = 0;
int rank, size, rank_old, size_old, size_new, oldClientRank;
std::string folderName = "";
// Count of computational threads
int countOfWorkers = 1;
// Count of all threads
int countOfThreads = 3;
int condition = 0;
bool changeExist = false;
std::vector<int> map;
std::queue<ITask*> currentTasks, queueRecv;
std::map<int, ITask*> sendedTasks;
std::map<int, int> sendedTasksCounter;
std::map<int, bool> sendedTasksSuccessfullyRecv;
// Communicators
MPI_Comm currentComm = MPI_COMM_WORLD;
MPI_Comm oldComm, newComm, serverComm, reduceComm;
MPI_Comm barrierComm;
pthread_mutexattr_t attr_set_task, attr_get_task, attr_send_task;
pthread_mutex_t mutex_get_task, mutex_set_task, mutex_send_task;
pthread_attr_t attrs_dispatcher, attrs_server, attrs_mapController, attrs_workers;
pthread_cond_t server_cond, comunicator_cond;
pthread_mutex_t server_mutexcond, comunicator_mutexcond;

void CreateLibraryComponents() {
	// Create dispatcher
	if (0 != pthread_create(&thrs[countOfWorkers], &attrs_dispatcher, dispatcher, &ids[countOfWorkers])) {
		perror("Cannot create a thread");
		abort();
	}
	// Create mapController
	if (0 != pthread_create(&thrs[countOfWorkers + 1], &attrs_mapController, mapController, &ids[countOfWorkers + 1])) {
		perror("Cannot create a thread");
		abort();
	}
	// Create computational treads
	for (int i = 0; i < countOfWorkers; i++)
		if (0 != pthread_create(&thrs[i], &attrs_workers, worker, &ids[i])) {
			perror("Cannot create a thread");
			abort();
		}
	// Create server
	if (0 != pthread_create(&thrs[countOfWorkers + 2], &attrs_server, server, &ids[countOfWorkers + 2])) {
		perror("Cannot create a thread");
		abort();
	}
}

void LibraryInitialize(int argc, char **argv, bool clientProgram) {
	#ifdef PROFILER
	Profiler::SetStartTime();
	#endif
	int provided = MPI_THREAD_SINGLE;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	if (provided != MPI_THREAD_MULTIPLE) {
		std::cerr << "not MPI_THREAD_MULTIPLE";
		exit(0);
	}
	MPI_Comm_rank(currentComm, &rank);
	MPI_Comm_size(currentComm, &size);
	size_old = size;
	if (rank == 0) std::cout << MPI_TAG_UB << std::endl;
	if (argc > 1) {
		folderName = argv[argc - 1];
		folderName += "/";
		//std::cout << folderName.size() << std::endl;
	}
	
	pthread_mutexattr_init(&attr_get_task);
	pthread_mutex_init(&mutex_get_task, &attr_get_task);	
	pthread_mutexattr_init(&attr_set_task);
	pthread_mutex_init(&mutex_set_task, &attr_set_task);
	pthread_mutexattr_init(&attr_send_task);
	pthread_mutex_init(&mutex_send_task, &attr_send_task);
	
	#ifdef PROFILER
	// Profiler's mutex
	Profiler::Init();
	#endif

	pthread_mutex_init(&server_mutexcond, NULL);
	pthread_mutex_init(&comunicator_mutexcond, NULL);
	pthread_cond_init(&server_cond, NULL);
	pthread_cond_init(&comunicator_cond, NULL);
	newComm = currentComm;
	if (0 != pthread_attr_init(&attrs_workers)) {
		perror("Cannot initialize attributes");
		abort();
	};
	if (0 != pthread_attr_setdetachstate(&attrs_workers, PTHREAD_CREATE_JOINABLE)) {
		perror("Error in setting attributes");
		abort();
	}
	if (0 != pthread_attr_init(&attrs_dispatcher)) {
		perror("Cannot initialize attributes");
		abort();
	};
	if (0 != pthread_attr_setdetachstate(&attrs_dispatcher, PTHREAD_CREATE_JOINABLE)) {
		perror("Error in setting attributes");
		abort();
	}
	if (0 != pthread_attr_init(&attrs_server)) {
		perror("Cannot initialize attributes");
		abort();
	};
	if (0 != pthread_attr_setdetachstate(&attrs_server, PTHREAD_CREATE_JOINABLE)) {
		perror("Error in setting attributes");
		abort();
	}
	if (0 != pthread_attr_init(&attrs_mapController)) {
		perror("Cannot initialize attributes");
		abort();
	};
	if (0 != pthread_attr_setdetachstate(&attrs_mapController, PTHREAD_CREATE_JOINABLE)) {
		perror("Error in setting attributes");
		abort();
	}
	if (clientProgram) {
		MPI_Comm server;
		MPI_Status st;
		double buf[MAX_DATA];

		char port_name[MPI_MAX_PORT_NAME];
		std::ifstream fPort("port_name.txt");
		for (int i = 0; i < MPI_MAX_PORT_NAME; i++)
		fPort >> port_name[i];
		fPort.close();
		oldClientRank = rank;
		MPI_Comm_connect(port_name, MPI_INFO_NULL, 0, currentComm, &server);
		MPI_Intercomm_merge(server, true, &currentComm);
		#ifdef PROFILER
		Profiler::AddEvent("connect to server success", Main);
		#endif
		//fprintf(stderr, "%d:: connect to server success\n", rank);

		MPI_Comm_rank(currentComm, &rank);
		MPI_Comm_size(currentComm, &size);
		rank_old = rank;
		size_old = size;
		//fprintf(stderr, "%d:: new rank = %d, new_size = %d\n", rank_old, rank, size);

		int sizeOfMap;
		MPI_Recv(&numberOfConnection, 1, MPI_INT, 0, NUMBEROFCONNECTION_TAG, currentComm, &st);
		char b[20];
		MPI_Recv(&b, 20, MPI_CHAR, 0, FOLDER_TAG, currentComm, &st); 
		folderName = b;
		//fprintf(stderr, "%d:: numberOfConnection = %d\n", rank, numberOfConnection);
		MPI_Recv(&sizeOfMap, 1, MPI_INT, 0, SIZEOFMAP_TAG, currentComm, &st);
		if (sizeOfMap) {
			map.resize(sizeOfMap);
			MPI_Recv(map.data(), sizeOfMap, MPI_INT, 0, MAP_TAG, currentComm, &st);
			MPI_Recv(&condition, 1, MPI_INT, 0, CONDITION_TAG, currentComm, &st);
			MPI_Comm_dup(currentComm, &serverComm);
			MPI_Comm_dup(currentComm, &reduceComm);			
			MPI_Comm_dup(currentComm, &barrierComm);
			CreateLibraryComponents();
		}
	}	
	else {
		MPI_Comm_dup(currentComm, &serverComm);
		MPI_Comm_dup(currentComm, &reduceComm);
		MPI_Comm_dup(currentComm, &barrierComm);
		CreateLibraryComponents();
	}
}

void CloseLibraryComponents() {
	MPI_Status st;
	MPI_Request s;
	int exit = -1;
	
	// Close dispatcher
	MPI_Isend(&exit, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm, &s);	
	
	// Close workers
	for (int i = 0; i < countOfWorkers; i++)
		MPI_Isend(&exit, 1, MPI_INT, rank, WORKER_CALC_TAG, currentComm, &s);
	
	// Close map controller
	int to_map_message[2] = { exit, exit };
	MPI_Isend(&to_map_message, 2, MPI_INT, rank, MAPCONTROLLER_TAG, currentComm, &s);	
	
	while (numberOfConnection < countOfConnect) {
		int cond;
		MPI_Recv(&cond, 1, MPI_INT, rank, DISPATCHER_TAG, currentComm, &st);
		if (rank == 0) {
			size_old = size;
			MPI_Comm_size(newComm, &size_new);
			cond = 0;
			for (int k = size_old; k < size_new; k++)
				MPI_Send(&cond, 1, MPI_INT, k, SIZEOFMAP_TAG, newComm);
		}
		MPI_Send(&cond, 1, MPI_INT, rank, CONNECTION_FINISH_TAG, currentComm);
	}
	
	pthread_join(thrs[countOfWorkers], NULL);	
	pthread_join(thrs[countOfWorkers + 3], NULL);
	for (int i = 0; i < countOfWorkers; i++)
		pthread_join(thrs[i], NULL);	
	pthread_join(thrs[countOfWorkers + 1], NULL);
	pthread_join(thrs[countOfWorkers + 2], NULL);
	
	pthread_attr_destroy(&attrs_dispatcher);
	pthread_attr_destroy(&attrs_server);
	pthread_attr_destroy(&attrs_mapController);
	pthread_attr_destroy(&attrs_workers);
	#ifdef PROFILER
	Profiler::SynchronizeTime(currentComm);
	Profiler::PrintSend();
	Profiler::PrintRecv();
	Profiler::PrintWait();
	Profiler::PrintAllReduce();
	Profiler::PrintBarrier();
	Profiler::PrintDup();
	Profiler::PrintEvents();
	#endif
}
