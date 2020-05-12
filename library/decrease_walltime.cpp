#include "lib_init.h"
#include <unistd.h>
#include <sys/file.h>

std::vector<std::string> clientsList;
void GenerateClientsList() {
	std::fstream inOut;
	inOut.open("id_clients.txt", std::ios::in);
	while(!inOut.eof()){
		std::string id, walltime;
		inOut >> id;
		if (id != "") clientsList.push_back(id);
		//std::cout << id << std::endl;
	}
	inOut.close();
	FILE *fp;
	fp = fopen("id_clients.txt", "w");
	fclose(fp);
}
void* walltimeController(void *me) {
	std::fstream inOut, port;
	GenerateClientsList();
	while(!clientsList.empty()) {		
		sleep(timeDelta);
		FILE *fp;
		fp = fopen("port_name.txt", "r");		
		int fd = fileno(fp); 
		// acquire shared lock
		if (flock(fd, LOCK_SH) == -1) {
			exit(1);
		}

		// non-atomically upgrade to exclusive lock
		// do it in non-blocking mode, i.e. fail if can't upgrade immediately
		if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
			exit(1);
		}

		walltime -= timeDelta;
		int hour = walltime/60/60;
		int min = walltime/60%60;
		int sec = walltime%60;
		std::string qsub = "sh qsub_client.sh " + std::to_string(hour) + ":" + std::to_string(min) + ":" + std::to_string(sec);
		const char *cqsub = qsub.c_str();
		for(int i = 0; i < clientsList.size(); i++) {
			std::string qstat = "qstat -f -F json " + clientsList[i] + " > clientstat.json"; 
			const char *cqstat = qstat.c_str();
			system(cqstat);
			system("python detectStatus.py");
			inOut.open("status.txt", std::ios::in);
			std::string status;
			inOut >> status;
			inOut.close();
			if(status == "Q") {				
				std::string qdel = "qdel " + clientsList[i]; 
				const char *cqdel = qdel.c_str();
				system(cqdel);
				system(cqsub);
			}			
		}
		port.close();
		// release lock
		// lock is also released automatically when close() is called or process exits
		if (flock(fd, LOCK_UN) == -1) {
			exit(1);
		}
		fclose(fp);
		clientsList.clear();
		GenerateClientsList();
	}
}