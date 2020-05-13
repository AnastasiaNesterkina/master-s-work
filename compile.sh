/mnt/storage/home/magorodnichev/nestyorkina/soft/bin/mpicxx -o server.exe server.cpp task.cpp ./library/*.cpp ./library/profiler/pmpi_functions.cpp -lpthread -std=gnu++11
/mnt/storage/home/magorodnichev/nestyorkina/soft/bin/mpicxx -o client.exe client.cpp task.cpp ./library/*.cpp ./library/profiler/pmpi_functions.cpp -lpthread -std=gnu++11
