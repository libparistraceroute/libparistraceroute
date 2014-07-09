#!/usr/bin/python
#
# Script for tests executing mda with bound calculation
#

import subprocess, os, sys

def main():
    sys_args = sys.argv
    if (len(sys_args) < 4):
        print "Usage: [failure-test.py] <confidence,max branching,max children> <destination> <repititions>"
        sys.exit(0)

    num_fail = 0
    for i in range(int(sys_args[3])):
        ret = run(sys_args[1], sys_args[2])
        if not ret:
            num_fail += 1
        if (i % (int(sys_args[3]) / 10)) == 0:
            print "Processing..."
    print num_fail

def run(bound_args, destination):
    pt_out = open('output.txt', 'w+')
    pt_args = ['./paris-traceroute/paris-traceroute', '-a', 'mda', '-n', '-B', bound_args, destination]
    process = subprocess.Popen(pt_args, stdout=pt_out)
    process.wait()
    pt_out.truncate()
    pt_out.seek(0)

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
        return True
    else:
        return False    


if __name__ == "__main__":
    main()
