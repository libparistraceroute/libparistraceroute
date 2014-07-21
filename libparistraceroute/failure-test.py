#!/usr/bin/python
#
# Script for tests executing mda with bound calculation
#

import subprocess, os, sys, time

def run(bound_args, destination):
    pt_args = ['./launch_fkrt_pt.sh', bound_args, destination]
    process = subprocess.Popen(pt_args)
    process.wait()
    if process.returncode == 23:
        return "dropped"
    pt_out = open('output.txt', 'r')

    cur_line = pt_out.readline().strip(' \n\t\r')
    while cur_line != "Lattice:":
        cur_line = pt_out.readline().strip(' \n\t\r')

    cur_line = pt_out.readline().strip(' \n\t\r')
    discovered = set()
    while cur_line:
        interfaces = cur_line.split()
        left = interfaces[0]
        left = left.strip(',')
        if left == "None":
            left = "start"

        for i in range((len(interfaces) - 4)):
            right = interfaces[3 + i]
            right = right.strip(',')
            if right == destination:
                right = "end"

            discovered.add((left, right))
        cur_line = pt_out.readline().strip(' \n\t\r')
    pt_out.close()

    fkrt_dir = "../../paris-traceroute.fakeroute/fakeroute/targets/"
    top_file = ""
    for fkrt in os.listdir(fkrt_dir):
        if fkrt.startswith(destination):
            top_file = fkrt

    fkrt_dir += top_file
    topology = open(fkrt_dir, 'r')
    actual = set()
    cur_line = topology.readline().strip(' \n\t\r')
    cur_line = topology.readline().strip(' \n\t\r')
    while cur_line:
        interfaces = cur_line.split()
        
        if len(interfaces) > 1:
            actual.add((interfaces[0], interfaces[1]))

        cur_line = topology.readline().strip(' /n/t/r')
    topology.close()

    if actual == discovered:
        return "pass"
    else:
        return "fail"    

def main():
    sys_args = sys.argv
    if (len(sys_args) < 4):
        print "Usage: [failure-test.py] <confidence,max branching,max children> <destination> <repititions>"
        sys.exit(0)

    num_fail = 0
    runs = int(sys_args[3])
    errors = 0
    start_time = time.time()
    for i in range(runs):
        ret = run(sys_args[1], sys_args[2])
        if ret == "fail":
            num_fail += 1
        elif ret == "dropped":
            errors += 1
        if (i % 10) == 0:
            print ("Processing... %d / %d runs ---> total of %d dropped" % (i, runs, errors)) 
    end_time = time.time()
    total_time = end_time - start_time
    minutes = total_time / 60
    seconds = total_time % 60 
    print 
    print ("%d failures out of %d runs" % (num_fail, runs - errors))
    print ("Failure rate: %.4f" % (num_fail / float(runs - errors)))
    print
    print ("Tests dropped: %d" % errors)
    print ("Duration: %d minutes %d seconds" % (minutes, seconds))

if __name__ == "__main__":
    main()
