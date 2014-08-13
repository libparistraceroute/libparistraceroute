#!/bin/sh
#Launch fakeroute, then paris-traceroute, then exit both

rm -rf output.txt
rm -rf log_fkrt.txt
cd ../../paris-traceroute.fakeroute/fakeroute; fakeroute/fakeroute "-4" "random" > log_fkrt.txt &
PID_FKRT=$!
sleep 0.25
cd ../../paris-traceroute.libparistraceroute/libparistraceroute; paris-traceroute/paris-traceroute "-a" "mda" "-n" "-B" $1 $2 > output.txt &
PID_PT=$!
counter=0
while ps -p $PID_PT > /dev/null
do
    counter=$((counter+1))
    if [ $counter -ge 100 ];then
        kill -9 $PID_PT
        kill -9 $PID_FKRT
        exit 23
    fi
    sleep 0.001
done
kill -9 $PID_FKRT
exit 0
