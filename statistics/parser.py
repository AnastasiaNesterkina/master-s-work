import json
import csv
import datetime

months = {
    'Jan': '01',
    'Feb': '02',
    'Mar': '03',
    'Apr': '04',
    'May': '05',
    'Jun': '06',
    'Jul': '07',
    'Aug': '08',
    'Sep': '09',
    'Oct': '10',
    'Nov': '11',
    'Dec': '12'
}

def GetDate(date):
	words = date.split()
	result = words[2] + '.' + months[words[1]] + '.' + words[4] + ' ' + words[3]
	return result

def GetDiffDate(laterTime, firstTime):
	strBeg = firstTime.split()
	dateBeg = strBeg[0].split(".")
	timeBeg = strBeg[1].split(":")
	begin = datetime.datetime(int(dateBeg[2]), int(dateBeg[1]), int(dateBeg[0]), 
		hour=int(timeBeg[0]), minute=int(timeBeg[1]), second=int(timeBeg[2]), microsecond=0, tzinfo=None)
	
	strEnd = laterTime.split()
	dateEnd = strEnd[0].split(".")
	timeEnd = strEnd[1].split(":")
	end = datetime.datetime(
		int(dateEnd[2]), int(dateEnd[1]), int(dateEnd[0]), 
		hour=int(timeEnd[0]), minute=int(timeEnd[1]), second=int(timeEnd[2]), microsecond=0, tzinfo=None)
	
	difference = end - begin
	secondsInDay = 24 * 60 * 60
	ms = divmod(difference.days * secondsInDay + difference.seconds, 60)
	hm = divmod(ms[0], 60)
	diff = str(hm[0]) + ':' + str(hm[1]) + ':' + str(ms[1])
	return diff

listOfJobs = {}

line = 0
#read previous tasks
with open('qstatinfo.csv') as csvfile:
	readCSV = csv.reader(csvfile, delimiter=',')
	for row in readCSV:
		if (line > 0) and (len(row) > 0):
			listOfJobs[row[0]] = {
				'nodes': row[1],
				'ncpus': row[2],
				'walltime': row[3],
				'qtime': row[4],
				'rtime': row[5]
			}
		line += 1
# read info from clu
with open("qstatinfo.json", "r") as read_file:
    data = json.load(read_file)
jobs = data['info']['Jobs']
idsTask = list(jobs)
for job in idsTask:
	# qtime - the time that the job entered the current queue.
	# stime - Time the job started. Updated when job is restarted.
	if jobs[job]['job_state'] == 'R':
		listOfJobs[job] = {			
			'nodes': jobs[job]['Resource_List']['select'].split(':')[0],
			'ncpus': jobs[job]['Resource_List']['ncpus'],
			'walltime': jobs[job]['Resource_List']['walltime'],
			'qtime': GetDiffDate(GetDate(jobs[job]['stime']), GetDate(jobs[job]['qtime'])),
			'rtime': GetDiffDate(data['date'], GetDate(jobs[job]['stime']))
		}

listOfRunningJobs = [['Job', 'nodes', 'ncpus', 'walltime', 'qtime', 'rtime']]
keys = list(listOfJobs)

for key in keys:
	task = listOfJobs[key]	
	listOfRunningJobs.append([key, task['nodes'], task['ncpus'], task['walltime'], task['qtime'], task['rtime']])

file = open('qstatinfo.csv', 'w')
with file:
    writer = csv.writer(file)
    writer.writerows(listOfRunningJobs)
