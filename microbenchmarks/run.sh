#!/bin/bash
set -e -o pipefail

########## Pay attention: Please install R lib in personal directory before running this script

wai=$(dirname $(readlink -f $0))
[ -e "/tmp/args.sh" ] && onG5K=1 || onG5K=0
sg="${wai}/simgrid/"
ns="${wai}/ns-3/"
[ $onG5K -eq 1 ] && csv="/tmp/results.csv" || csv="${wai}/results_local.csv"
nSub=32 # Number of parrallel simulations

init_csv(){
    if ([ "$topo" = "3" ] && [ "$flowConf" = "1" ]) || ([ "$topo" = "3" ] && [ "$flowConf" = "2" ])
    then
        header="simulator,simTime,simTimeOffset,topo,dataSize,nSTA,bitRate,date,corrFactor,radius,seed,name,useDecayModel,useNS3,duration,flowConf,nNode,nNodePair,dataSent,dataReceived,appSimTime,swappedCount,cpuTimeU,cpuTimeS,wallClock,peakMemUsage,throughput,MacTxDrop,MacRxDrop,PhyTxDrop,PhyRxDrop,avgSignal,avgNoise,avgSNR,avgThroughputAP,avgThroughputSTA,dataReceivedAP,dataReceivedSTA,nbSinkAP, nbSinkSTA,nbSinkTot,totEnergyConsumption,energyDyn,energyStat,durIDLE,durRXTX" && echo "$header" > "$csv"
    elif ([ "$topo" = "4" ])
    then
        header="simulator,simTime,simTimeOffset,topo,dataSize,nSTA,bitRate,date,corrFactor,radius,seed,name,useDecayModel,useNS3,duration,flowConf,nNode,nNodePair,dataSent,dataReceived,appSimTime,swappedCount,cpuTimeU,cpuTimeS,wallClock,peakMemUsage,throughput,MacTxDrop,MacRxDrop,PhyTxDrop,PhyRxDrop,avgSignal,avgNoise,avgSNR,nbSinkTot,totEnergyConsumption,energyDyn,energyStat,durIDLE,durRXTX,nbSTASNR1,nbSTASNR2,radius1,radius2" && echo "$header" > "$csv"
    else
        header="simulator,simTime,simTimeOffset,topo,dataSize,nSTA,bitRate,date,corrFactor,radius,seed,name,useDecayModel,useNS3,duration,flowConf,nNode,nNodePair,dataSent,dataReceived,appSimTime,swappedCount,cpuTimeU,cpuTimeS,wallClock,peakMemUsage,throughput,MacTxDrop,MacRxDrop,PhyTxDrop,PhyRxDrop,avgSignal,avgNoise,avgSNR,nbSinkTot,totEnergyConsumption,energyDyn,energyStat,durIDLE,durRXTX" && echo "$header" > "$csv"
    fi
}


[ $onG5K -eq 1 ] && date=$(cat "${wai}/ts.txt") || date=$(date "+%s")
[ $onG5K -eq 1 ] && logs="/tmp/logs/" || logs="${wai}/logs/$date/"
rm -rf "$logs" && mkdir -p "$logs"

run_once() {
    args="T${topo}_dataSize${dataSize}_nNode${nNode}_nNodePair${nNodePair}_radius${radius}_seed${seed}_radius${radius}_flowConf${flowConf}_radius2${radius2}"
    nsout="${logs}/NS3_${args}.log"
    sgout="${logs}/SG_${args}.log"

    [ $topo -eq 1 ] && nSTA=$(( nNodePair * 2 )) || nSTA=$nNode
    [ $topo -eq 3 ] && nSTA=$(( nNodePair * 3 ))

    echo -e '\e[1;32m---------- RUN ---------\e[0m' >&2
    # Run ns-3 simulations
    tStart=$(date "+%s")
    [ $useNS3 -eq 1 ] && { /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $ns ARGS="--topo=$topo --flow=$flowConf --dataSize=$dataSize --nNode=$nNode --nNodePair=$nNodePair --radius=$radius --seed=$seed --simTime=${simTime} --radius2=${radius2} --nSTAInFirstRange=${nSTAInFirstRange} --nSTAInSecondRange=${nSTAInSecondRange} --threshRTSCTS=$threshRTSCTS" run |& tee $nsout >&2; }
    tNS3=$(( $(date "+%s") - tStart))
    # try running ns3 withing simgrid
    if [ $useNS3 -eq 2 ]
    then
        tStart=$(date "+%s")
        platform=/tmp/${args}_platform_ns3.xml
        $sg/gen-platform_ns3.py $topo $nNodePair $nNode $bitRate 1 ${seed} > ${platform}
        /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $sg ARGS="${platform} $topo $dataSize $flowConf $useDecayModel $simTime 1 --cfg=network/model:ns-3" run |& tee $nsout >&2
    fi


    # Run simgrid simulations
    tStart=$(date "+%s")
    platform=/tmp/${args}_platform.xml
    $sg/gen-platform.py $topo $nNodePair $nNode $bitRate 1 > ${platform}
    /usr/bin/time -f 'swappedCount:%W cpuTimeU:%U cpuTimeS:%S wallClock:%e peakMemUsage:%M' make -sC $sg ARGS="${platform} $topo $dataSize $flowConf $useDecayModel $simTime 0 --cfg=plugin:link_energy_wifi" run |& tee $sgout >&2
    tSG=$(( $(date "+%s") - tStart))
    echo -e '\e[1;32m---------- RUN END ---------\e[0m\n' >&2

    # Parse logs
    #sg_bash=$(cat $sgout|$sg/parse.awk)
    #eval "$sg_bash"

    [ ! -e "$csv" ] && init_csv;

    ##### Gen CSV #####
    exec 100>>"$csv"
    flock -x 100 # Lock the file

        if ([ "$topo" = "3" ] && [ "$flowConf" = "1" ]) || ([ "$topo" = "3" ] && [ "$flowConf" = "2" ])
        then
            if [ $useNS3 -eq 1 ]
            then
                ns_bash=$(cat $nsout|$ns/parse_hybrid.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,$simTimeOffset,$topo,$dataSize,$nSTA,NA,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tNS3,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$appSimTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,$throughput,$MacTxDrop,$MacRxDrop,$PhyTxDrop,$PhyRxDrop,$avgSignal,$avgNoise,$avgSNR,$avgThroughputAP,$avgThroughputSTA,$dataReceivedAP,$dataReceivedSTA,$nbSinkAP,$nbSinkSTA,$nbSinkTot,$totEnergyConsumption,NA,NA,NA,NA" >&100
            fi
            if [ $useNS3 -eq 2 ]
            then
                ns_bash=$(cat  $sgout|$sg/parse.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,NA,$topo,$dataSize,$nSTA,$bitRate,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tSG,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$simTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,$totEnergyConsumption,$energyDyn,$energyStat,$durIDLE,$durRXTX" >&100
            fi
            sg_bash=$(cat $sgout|$sg/parse.awk)
            eval "$sg_bash"
            echo "sg,$simTime,NA,$topo,$dataSize,$nSTA,$bitRate,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tSG,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$simTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,$totEnergyConsumption,$energyDyn,$energyStat,$durIDLE,$durRXTX" >&100
        elif ([ "$topo" = "4" ])
        then
            if [ $useNS3 -eq 1 ]
            then
                ns_bash=$(cat $nsout|$ns/parse_topo4.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,$simTimeOffset,$topo,$dataSize,$nSTA,NA,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tNS3,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$appSimTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,$throughput,$MacTxDrop,$MacRxDrop,$PhyTxDrop,$PhyRxDrop,$avgSignal,$avgNoise,$avgSNR,$nbSinkTot,$totEnergyConsumption,NA,NA,NA,NA,$nbSTASNR1,$nbSTASNR2,$radius1,$radius2" >&100
            fi
            if [ $useNS3 -eq 2 ]
            then
                ns_bash=$(cat $nsout|$sg/parse.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,$simTimeOffset,$topo,$dataSize,$nSTA,NA,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tNS3,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$appSimTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,$throughput,$MacTxDrop,$MacRxDrop,$PhyTxDrop,$PhyRxDrop,$avgSignal,$avgNoise,$avgSNR,$nbSinkTot,$totEnergyConsumption,NA,NA,NA,NA,$nbSTASNR1,$nbSTASNR2,$radius1,$radius2" >&100
            fi
        else
            if [ $useNS3 -eq 1 ]
            then
                ns_bash=$(cat $nsout|$ns/parse.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,$simTimeOffset,$topo,$dataSize,$nSTA,NA,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tNS3,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$appSimTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,$throughput,$MacTxDrop,$MacRxDrop,$PhyTxDrop,$PhyRxDrop,$avgSignal,$avgNoise,$avgSNR,$nbSinkTot,$totEnergyConsumption,NA,NA,NA,NA" >&100
            fi
            if [ $useNS3 -eq 2 ]
            then
                ns_bash=$(cat $nsout|$sg/parse.awk)
                eval "$ns_bash"
                echo "ns3,$simTime,NA,$topo,$dataSize,$nSTA,$bitRate,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tSG,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$simTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,NA,NA,NA,NA,NA,NA,NA,NA,NA,$totEnergyConsumption,$energyDyn,$energyStat,$durIDLE,$durRXTX" >&100
            fi
            sg_bash=$(cat $sgout|$sg/parse.awk)
            eval "$sg_bash"
            echo "sg,$simTime,NA,$topo,$dataSize,$nSTA,$bitRate,$date,NA,$radius,$seed,$name,$useDecayModel,$useNS3,$tSG,$flowConf,$nNode,$nNodePair,$dataSent,$dataReceived,$simTime,$swappedCount,$cpuTimeU,$cpuTimeS,$wallClock,$peakMemUsage,NA,NA,NA,NA,NA,NA,NA,NA,NA,$totEnergyConsumption,$energyDyn,$energyStat,$durIDLE,$durRXTX" >&100
        fi

    flock -u 100 # Unlock the file
}


if [ $onG5K -eq 1 ]
then
    rm -rf /tmp/finish.txt
    rm -rf "$csv"
    #echo "$header" > "$csv"
    over=$(cat /tmp/args.sh|wc -l)
    i=0
    while IFS= read -r line
    do
        eval "$line"
        ( run_once; ) & # Launch as subprocess
        i=$(( i + 1 ))
        echo $i over $over > /tmp/avancement.txt
        while [ $(pgrep -P $$ | wc -l) -ge $nSub ] # Until we have the max of subprocess we wait
        do
            sleep 3
        done
    done < /tmp/args.sh
    wait # Be sure evething is done

else
    echo "Using args file: ${localOnlyArgsFile}"
    rm -rf /tmp/finish.txt
    rm -rf "$csv"
    #echo "$header" > "$csv"
    over=$(cat ${wai}/${localOnlyArgsFile}|wc -l)
    i=0
    while IFS= read -r line
    do
        eval "$line" #&& useNS3=0
        ( run_once; ) & # Launch as subprocess
        i=$(( i + 1 ))
        echo $i over $over > ${wai}/avancement.txt
        while [ $(pgrep -P $$ | wc -l) -ge $nSub ] # Until we have the max of subprocess we wait
        do
            sleep 3
        done
    done < ${wai}/${localOnlyArgsFile}
    wait # Be sure evething is done

fi

if [ $onG5K -ne 1 ]
then
    exit 0 # Skip this step
    # Do data analysis
    echo -e '\n\e[1;32m---------- Running R data analysis ---------\e[0m'
    R --no-save -q -e 'source("'${wai}/analysis.R'")'
    echo -e '\e[1;32m---------- Done -----------\e[0m'

    # Generate pdf
    echo -e '\n\e[1;32m---------- Generating PDF ---------\e[0m'
    ${wai}/gen-pdf.sh
    pkill -SIGHUP mupdf # Refresh pdf viewer
    echo -e '\e[1;32m---------- Done -----------\e[0m'
else
    touch /tmp/finish.txt
fi
