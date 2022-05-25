#!/bin/bash
set -e -o pipefail

local_only=0
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
#csv="${wai}/results_${date}_${suffix}.csv"
argsFile="${wai}/args_${date}_${suffix}.sh"

aHost=$(cat $hostFile|head -n1)
bindLibPath="${wai}/../../analysis/" && source "${bindLibPath}/Rbindings.sh"
g5k_user="ccourageuxsudan"

range(){
    min=$(( $1 * 1000000 ))
    max=$(( $2 * 1000000 ))
    count=$3
    range=$(( (max - min + 1) / count))
    seq $min $range $max
}

##### Fixed Parameters
nbCells="$(seq 1 1 10)"
AP_BW="38.0Mbps" # 44.23
seedsPlatf="$(seq 1 1 4)"
seedsNS="$(seq 1 1 30)"
verbose_flow_th=1

startTime="10"
endTime="10"
simTime="150"
useNS3=1

type="rand"

### options for "rand"
minSTAPerCell="20"
maxSTAPerCell="20"
minSize="10000000"
maxSize="30000000"
minNbMsg=1
maxNbMsg=1
minIntervalBetweenMsg=10
maxIntervalBetweenMsg=20

### options for "norm"
meanStaPerCell=7
devStaPerCell=3
meanSize="1500000"
devSize="100000"
avgNbMsg=50
devNbMsg=10
meanIntervalBetweenMsg=28
devIntervalBetweenMsg=7


ECHO (){
    echo -e "\e[32m----------| $@ |----------\e[0m"
}

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
for nbCell in ${nbCells}
do
    for minSTA in ${minSTAPerCell}
    do
        for maxSTA in ${maxSTAPerCell}
        do
            for BW in ${AP_BW}
            do
                for minS in ${minSize}
                do
                    for maxS in ${maxSize}
                    do
                        for seedP in ${seedsPlatf}
                        do
                        for seedN in ${seedsNS}
                        do
                            for stt in ${startTime}
                            do
                                for edt in ${endTime}
                                do
                                    echo "nbCells=${nbCell};minSTAPerCell=${minSTA};useNS3=${useNS3};maxSTAPerCell=${maxSTA};AP_BW=${BW};minSize=${minS};maxSize=${maxS};seedP=${seedP};seedNS=${seedN};startTime=${stt};endTime=${edt};simTime=${simTime};minNbMsg=${minNbMsg};maxNbMsg=${maxNbMsg};meanSize=${meanSize};devSize=${devSize};avgNbMsg=${avgNbMsg};devNbMsg=${devNbMsg};meanIntervalBetweenMsg=${meanIntervalBetweenMsg};devIntervalBetweenMsg=${devIntervalBetweenMsg};meanStaPerCell=${meanStaPerCell};devStaPerCell=${devStaPerCell};type=${type};minIntervalBetweenMsg=${minIntervalBetweenMsg};maxIntervalBetweenMsg=${maxIntervalBetweenMsg};verbose_flow_th=${verbose_flow_th}" >> $argsFile
                                done
                            done
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
    ssh ${g5k_user}@$host "cd ~/project/run/largeExperiment/ && tmux -l new-session -s 'simu' -d './run.sh'"
    rm $tmp
done

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
            [ ! -e "${wai}/logs/$date/results.csv" ] && head -n1 "$tmp" > "${wai}/logs/$date/results.csv"
            tail -n +2 "$tmp" >> "${wai}/logs/$date/results.csv"
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
fetch_results
