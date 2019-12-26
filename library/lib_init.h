#include "../task.h"

#ifdef PROFILER
#define MPI_Send PROFILE_MPI_Send
#define MPI_Recv PROFILE_MPI_Recv
#define MPI_Wait PROFILE_MPI_Wait
#define MPI_Barrier PROFILE_MPI_Barrier
#define MPI_Allreduce PROFILE_MPI_Allreduce
#define MPI_Comm_dup PROFILE_MPI_Comm_dup
#endif


// Descriptors for threads
extern pthread_t thrs[12];
// id for threads
extern int ids[12];

extern int rank_old, size_new;

extern pthread_mutexattr_t attr_set_task, attr_get_task, attr_send_task;
extern pthread_mutex_t mutex_get_task, mutex_set_task, mutex_send_task;
extern pthread_attr_t attrs_dispatcher, attrs_server, attrs_mapController, attrs_workers;
extern pthread_cond_t server_cond, comunicator_cond;
extern pthread_mutex_t server_mutexcond, comunicator_mutexcond;

extern MPI_Comm currentComm, oldComm, newComm, serverComm, barrierComm;

extern std::map<int, ITask*> sendedTasks;
extern std::map<int, int> sendedTasksCounter;
extern std::map<int, bool> sendedTasksSuccessfullyRecv;


// Count of computational threads
extern int countOfWorkers;
// Count of all threads
extern int countOfThreads;
extern int condition;
bool GetTask(ITask **currTask);
int GetRank(int &sign, int &k, int countOfProcess);

void ExecuteOwnTasks();
void ExecuteOtherTask(MPI_Comm &Comm, int id, bool &retry);
void ChangeCommunicator(MPI_Comm &Comm, int &newSize);

void* dispatcher_old(void* me);
void* dispatcher(void* me);
void* worker(void* me);
void* mapController(void* me);
void* oldMapController(void* me);
void* server(void *me);