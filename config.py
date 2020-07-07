import json
import subprocess

def GetSec(time):
    t = time.split(':')
    tsec = int(t[0])*60*60 + int(t[1])*60 + int(t[2])
    return tsec

# read info from clu
with open("config.json", "r") as read_file:
    data = json.load(read_file)
    
serverParams = data['server_group']
clientParams = data['client_group']

snodes = str(serverParams['n_nodes'])
sncpus = str(serverParams['n_cpus'])
smem = serverParams['mem']
stime = serverParams['walltime']
nConnect = str(serverParams['countOfConnect'])
cnodes = str(clientParams['n_nodes'])
cncpus = str(clientParams['n_cpus'])
cmem = clientParams['mem']
sectime =  str(GetSec(serverParams['walltime']))

command = "./scripts/qsub.sh"
p = subprocess.call(["sh", command, snodes, sncpus, smem, stime, nConnect, cnodes, cncpus, cmem, sectime])
