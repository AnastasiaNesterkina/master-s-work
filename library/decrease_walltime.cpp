#include "lib_init.h"
#include <unistd.h>
#include <sys/file.h>

std::vector<std::string> clientsList;
void GenerateClientsList() {
	std::fstream inOut;
	inOut.open("./scripts/id_clients.txt", std::ios::in);
	while(!inOut.eof()){
		std::string id, walltime;
		inOut >> id;
		if (id != "") clientsList.push_back(id);
		//std::cout << id << std::endl;
	}
	inOut.close();
	FILE *fp;
	fp = fopen("./scripts/id_clients.txt", "w");
	fclose(fp);
}
void* walltimeController(void *me) {
	std::fstream inOut;
	bool close = false;
	GenerateClientsList();
	while(!clientsList.empty()) {
		sleep(timeDelta);
		FILE *fp;
		fp = fopen("port_name.txt", "r");		
		int fd = fileno(fp); 
		// TO DO:: to handle errors
		while (flock(fd, LOCK_EX) == -1);
		walltime -= timeDelta;
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
		if (closeDecreaseWalltime) close = true;
		for(int i = 0; i < clientsList.size(); i++) {
			std::string qstat = "qstat -f -F json " + clientsList[i] + " > ./scripts/clientstat.json"; 
			const char *cqstat = qstat.c_str();
			system(cqstat);
			system("python ./scripts/detectStatus.py");
			inOut.open("./scripts/status.txt", std::ios::in);
			std::string status;
			inOut >> status;
			inOut.close();
			if(status == "Q") {				
				std::string qdel = "qdel " + clientsList[i]; 
				const char *cqdel = qdel.c_str();
				system(cqdel);		
				if (!close) system(cqsub);
			}			
		}
		if (close) break;
		// release lock
		// lock is also released automatically when close() is called or process exits
		while (flock(fd, LOCK_UN) == -1) {
			//exit(1);
		}
		fclose(fp);
		clientsList.clear();
		GenerateClientsList();
	}
	if (close) {
		std::string qsub = "sh ./scripts/qsub_kill_server.sh";
		const char *cqsub = qsub.c_str();
		closeServer = true;
		system(cqsub);
	}
}
