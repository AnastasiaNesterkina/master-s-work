import json
# read info from clu
with open("clientstat.json", "r") as read_file:
    data = json.load(read_file)
jobs = data['Jobs']
idsTask = list(jobs)
for job in idsTask:
	f = open('status.txt', 'w')
	f.write(jobs[job]['job_state'])
	f.close()
