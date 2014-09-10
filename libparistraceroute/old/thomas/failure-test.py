#!/usr/bin/python
#
# Script for tests executing mda with bound calculation
#

import subprocess, os, sys, time

#--------------------------------------------------------
# Globals
#--------------------------------------------------------

# The actual topology as defined by target file
actual = dict()

# Tally of children discovered at each branch point
results = dict()

#--------------------------------------------------------
# Run loop representing single execution of pt and fkrt
#--------------------------------------------------------

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
    discovered = dict()
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

            if not left in discovered:
                discovered[left] = set()
            
            discovered[left].add(right) 

        cur_line = pt_out.readline().strip(' \n\t\r')
    pt_out.close()

    for branch in results.keys():
        tally = results[branch]
        if branch in discovered:
            index = len(discovered[branch])
            tally[index] += 1

    if actual == discovered:
        return "pass"
    else:
        return "fail"    


# Read in actual topology from target file
def read_target(destination):
    fkrt_dir = "../../paris-traceroute.fakeroute/fakeroute/targets/"
    top_file = ""
    for fkrt in os.listdir(fkrt_dir):
        if fkrt.startswith(destination):
            top_file = fkrt

    fkrt_dir += top_file
    topology = open(fkrt_dir, 'r')
    cur_line = topology.readline().strip(' \n\t\r')
    cur_line = topology.readline().strip(' \n\t\r')
    while cur_line:
        interfaces = cur_line.split()
        
        if len(interfaces) > 1:
            if not interfaces[0] in actual:
                actual[interfaces[0]] = set()

            actual[interfaces[0]].add(interfaces[1])

        cur_line = topology.readline().strip(' /n/t/r')
    topology.close()


# Build skeleton for results dictionary
def build_results():
    for branch in actual.keys():
        if len(actual[branch]) > 1:
            results[branch] = [0] * (len(actual[branch]) + 1)
                

def main():
    sys_args = sys.argv
    if (len(sys_args) < 4):
        print "Usage: [failure-test.py] <confidence,max branching,max children> <destination> <repititions>"
        sys.exit(0)

    read_target(sys_args[2])
    build_results()

    num_fail = 0
    runs = int(sys_args[3])
    errors = 0
    start_time = time.time()
    print ("Please wait, executing %d runs of program\n" % runs)

    for i in range(runs):
        ret = run(sys_args[1], sys_args[2])
        if ret == "fail":
            num_fail += 1
        elif ret == "dropped":
            errors += 1
        print ("\rProcessing... %d / %d runs ---> total of %d dropped" % (i + 1, runs, errors)), 
        sys.stdout.flush()

    print
    end_time = time.time()
    total_time = end_time - start_time
    minutes = total_time / 60
    seconds = total_time % 60 
    print ("Tests dropped: %d" % errors)
    print ("Duration: %d minutes %d seconds" % (minutes, seconds))
    print
    print ("%d failures out of %d runs" % (num_fail, runs - errors))
    print ("Failure rate: %.4f" % (num_fail / float(runs - errors)))
    print
    print "------------------------------------------------------"
    print " Branch discovery rate dump"
    print "------------------------------------------------------"
    for branch in results.keys():
        tally = results[branch]
        expected = len(actual[branch])
        print ("At branching point 1: %s" % branch)

        for x in range(expected):
            count = tally[x + 1]
            frac = count / float(runs - errors)
            print ("Discovered %d out of %d children %d / %d times: %.4f" % (x + 1, expected, count, runs - errors, frac))     
        print
    print
    print "/end test data"

if __name__ == "__main__":
    main()
