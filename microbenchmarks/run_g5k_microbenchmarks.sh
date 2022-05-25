#!/bin/bash
set -e -o pipefail

[ $# -ne 1 ] && { echo -e "Invalide arguments.\nUsage: ./$0 <experiment-id>\nExperiment id range from 1 to 6."; exit 1; }

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
dataSizes=15000001 #`seq 2500000 2000000 15000001`
bitRates=22.2 #44.23 # bitRate in Mbps (avant 42.10)
#bitRates=54  # test for decay
useNS3=1
local_only=1
name="seed"
useDecayModel=1
threshRTSCTS=100
seeds=1 #"$(seq 1 1 200)"
radiuses=15 #"5 10 15 30"
simTime=70 #70
nSTAInFirstRange=-1
nSTAInSecondRange=-1
radius2=-1
########## PARAMETERS ##########
case $1 in
    1 )
        #Topo 1 Flow Conf 1
        echo "Pair of station in one direction"
        topos=1
        flowConfs=1
        nNodes=0 # Not used
        nNodePairs=$(seq 1 1 35) # 10 15"
	#nNodePairs="$(seq 51 1 75)" # "1 5 10 15 20 30 40"
        ;;
    2)
        ##### Topo 1 Flow Conf 2
        echo "Pair of station in two directions"
        #local_only=1
        topos=1
        flowConfs=2
        nNodes=0 # Not used
        #nNodePairs="1 2 4 6 8 10 15 30"
	nNodePairs="$(seq 1 1 35)"
        ;;
    3)
        ##### Topo 2
        echo "Station to AP"
        #local_only=1
        topos=2
        flowConfs=1 # Not used
        #nNodes="1 2 4 6 8 10 15"
	nNodes="$(seq 1 1 70)" #"1 5 10 15 20 30 40 50 60 70 90"
        nNodePairs=0 # Not used
        ;;
    4)
        ##### Topo 3 Flow Conf 1
        echo "Pair of station in one direction + station to the AP"
        #local_only=1
        topos=3
        flowConfs=1
        nNodes=0 # Not used
        nNodePairs="$(seq 1 1 50)"
        ;;
        ###### Topo 2
        #echo "Station to AP"
        ##local_only=1
        #topos=2
        #flowConfs=0 # Not used
        ##nNodes="1 2 4 6 8 10 15"
	    #nNodes="$(seq 122 1 151)" #"1 5 10 15 20 30 40 50 60 70 90"
        #nNodePairs=0 # Not used
        #;;
    5)
        ##### Topo 3 Flow Conf 2
        echo "Pair of station in two directions + station to the AP"
        #local_only=1
        topos=3
        flowConfs=2
        nNodes=0 # Not used
        nNodePairs="$(seq 1 1 50)"
        ;;
    6)
        ##### Calib
        #local_only=1
        #date=1597209987
        dataSizes=10000000 #$(randRange 100 10000000 100000000)
        #bitRates=38.13
        echo "Station to AP"
        topos=2
        flowConfs=0 # Not used
        nNodes=1
        nNodePairs=0 # Not used
        ;;
    7)
        ##### Topo 2, downwards
        echo "AP to stations"
        #local_only=1
        topos=2
        flowConfs=2 # Not used
        #nNodes="1 2 4 6 8 10 15"
	    nNodes="$(seq 1 1 70)" #"1 5 10 15 20 30 40 50 60 70 90"
        nNodePairs=0 # Not used
        ;;
    fetch)
        break
        ;;
    *)
        echo "Invalid experiment range"
        exit 1
        ;;
esac

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
