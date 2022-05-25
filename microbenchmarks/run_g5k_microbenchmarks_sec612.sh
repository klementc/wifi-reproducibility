#!/bin/bash
set -e -o pipefail

wai=$(dirname $(readlink -f $0))
date=$(date "+%s")

if [[ -z ${hostFile} ]]
then
  echo "You must specify HostFile!"
  exit 1
fi
echo "Using hostFile: ${hostFile}"
#="${wai}/hosts.txt"

[ -z "${suffix}" ] && suffix="DEFAULT"
csv="${wai}/results_${date}_${suffix}.csv"
argsFile="${wai}/args_${date}_${suffix}.sh"

aHost=$(cat $hostFile|head -n1)
bindLibPath="${wai}/../analysis/" && source "${bindLibPath}/Rbindings.sh"
g5k_user="ccourageuxsudan"

range(){
    min=$(( $1 * 1000000 ))
    max=$(( $2 * 1000000 ))
    count=$3
    range=$(( (max - min + 1) / count))
    seq $min $range $max
}

##### Fixed Parameters

echo "Reproduce Scenario: Flow Concurrency"
echo -e "\033[0;31m/!\ ns-3 MCS in main.cc and set the correct callback degradation values in SimGrid code if you modify the MCS /!\\033[0m"
dataSizes=15000001
bitRates=44.23
useNS3=1
local_only=0
name="figconcurrency_msc5"
useDecayModel=1
threshRTSCTS=100
seeds="$(seq 1 1 35)"
radiuses=15 #"5 10 15 30"
simTime=150 #70
nSTAInFirstRange=-1
nSTAInSecondRange=-1
radius2=-1


#Topo 1 Flow Conf 1
echo "Pair of station in one direction"
topos=1
flowConfs=1
nNodes=0 # Not used
nNodePairs=$(seq 1 1 25)



ECHO (){
    echo -e "\e[32m----------| $@ |----------\e[0m"
}

fetch_results() {
    ##### Wait for last exp finish
    ECHO "Waiting for the experiments to end..."
    finish=0
    while [ $finish -eq 0 ]
    do
        for host in $(cat $hostFile)
        do
            finish=0
            ssh ${g5k_user}@$host "test -e /tmp/finish.txt" && finish=1 || { finish=0; break; }
        done
        sleep 10
    done

    ##### Fetch results
    ECHO "Gathering results"
    for host in $(cat $hostFile)
    do
        echo "Fetch results from $host"
        {
            tmp=$(mktemp)
            rsync -qavh "${g5k_user}@$host:/tmp/results.csv" "$tmp"
            [ ! -e "$csv" ] && head -n1 "$tmp" > "$csv"
            tail -n +2 "$tmp" >> "$csv"
            rm "$tmp"
        } || {
            echo -e "\e[31mCan't fetch results from $host\e[0m"
        }
        {
            mkdir -p "${wai}/logs/$date/"
            rsync -qavh "${g5k_user}@$host:/tmp/logs/" "${wai}/logs/$date/"
        } || {
            echo -e "\e[31mCan't fetch log from $host\e[0m"
        }
    done
}

[ "$1" == "fetch" ] && { fetch_results; exit 0; }

# First refresh simulator
if [ $local_only -ne 1 ]
then
    ECHO "Refreshing simulators"
    #rsync -avh simgrid --exclude simgrid/simgrid/ ${g5k_user}@$aHost:./project/
    #rsync -avh ns-3 --exclude ns-3/ns-3/ ${g5k_user}@$aHost:./project/
    #ssh ${g5k_user}@$aHost "cd project/ns-3 && make clean &&  make"
    #ssh ${g5k_user}@$aHost "cd project/simgrid && make clean && make"
    ECHO "Refreshing R bindings"
    rsync -avh --exclude "plots/" --exclude "decayVSnodecay" $(realpath -s ${bindLibPath}) ${g5k_user}@$aHost:./project/
    ECHO "Refreshing run.sh"
    rsync -avh run.sh ${g5k_user}@$aHost:./project/
fi

ECHO "Generating Arguments"
echo -en > $argsFile
for dataSize in ${dataSizes}
do
    for topo in ${topos}
    do
        for nNode in ${nNodes}
        do
            for nNodePair in ${nNodePairs}
            do
                for bitRate in ${bitRates}
                do
                    for radius in ${radiuses}
                    do
                        for seed in ${seeds}
                        do
                            for flowConf in ${flowConfs}
                            do
                                echo "threshRTSCTS=$threshRTSCTS;dataSize=$dataSize;topo=$topo;nNode=$nNode;nNodePair=$nNodePair;bitRate=$bitRate;useNS3=$useNS3;radius=$radius;seed=$seed;name=$name;useNS3=${useNS3};flowConf=${flowConf};useDecayModel=${useDecayModel};simTime=${simTime};radius2=${radius2};nSTAInFirstRange=-1;nSTAInSecondRange=-1" >> $argsFile
                            done
                        done
                    done
                done
            done
        done
    done
done

[ $local_only -eq 1 ] && exit 0

# Shuffle lines (more homogeneous load repartition)
tmp=$(mktemp)
cat $argsFile|shuf > $tmp
mv $tmp $argsFile

# Give the experiment timestamp
ECHO "Starting simulations $date"
ssh ${g5k_user}@$aHost "echo $date > ~/project/ts.txt"


nHost=$(cat $hostFile|wc -l)
nSim=$(cat $argsFile|wc -l)
chunk=$(( nSim / nHost ))

start=1
end=$chunk
for i in $(seq 1 $nHost)
do
    tmp=$(mktemp)
    if [ $i -ne $nHost ]
    then
        cat $argsFile | sed -n "${start},${end}p" > $tmp
    else
        cat $argsFile | tail --lines="+${start}" > $tmp
    fi
    start=$((start + chunk))
    end=$((end + chunk))

    host=$(cat $hostFile | sed -n "${i}p")
    rsync -qavh $tmp ${g5k_user}@$host:/tmp/args.sh
    ssh ${g5k_user}@$host "{ tmux kill-session -t simu 2>/dev/null && echo killing previous experiment on $host; } || true "
    echo "Launching simulation on host $host ($i/$nHost)"
    ssh ${g5k_user}@$host tmux -l new-session -s "simu" -d "./project/run/run.sh"
    rm $tmp
done


fetch_results
