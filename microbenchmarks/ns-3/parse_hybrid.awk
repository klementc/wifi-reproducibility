#!/usr/bin/awk -f

BEGIN {
    simTime=0
    simTimeOffset=10
    dataSent=0
    dataReceived=0
    MacTxDrop=0
    MacRxDrop=0
    PhyTxDrop=0
    PhyRxDrop=0
    # Throughput
    ntp=0
    tp=0
    # Applicative simTime
    appSimTime=0
    # /usr/bin/time
    swappedCount=-1
    cpuTimeU=-1
    cpuTimeS=-1
    wallClock=-1
    peakMemUsage=-1
    avgThroughput=-1
    avgSignal=-1
    avgNoise=-1
    avgSnr=-1
    totSink=-1
    avgThroughputAP=-1
    avgThroughputSTA=-1
    nbSinkAP=-1
    nbSinkSTA=-1
    dataReceivedAP=-1
    dataReceivedSTA=-1
    totEnergyConsumption=0
}

/Device consumed/ {
    totEnergyConsumption+=$4
}

/Average throughput AP:/ {
    avgThroughputAP=$4+0
}

/Average throughput STA:/ {
    avgThroughputSTA=$4+0
}

/Flow/ && /ends at/ {
    if($5-0 > simTime) # -0 to cast into a number
        simTime=$5-0
}

/NbSinkTot:/ {
    totSink=$2+0
}

/SinkAP amount:/ {
    nbSinkAP=$3+0
}

/SinkSTA amount:/ {
    nbSinkSTA=$3+0
}

/Sending check:/ {
    dataSent+=$7
}

/SinkAP [0-9]+ received/ {
    dataReceived+=$4
    dataReceivedAP+=$4
}
/SinkSTA [0-9]+ received/ {
    dataReceived+=$4
    dataReceivedSTA+=$4
}

/MacTxDrop:/ {
    MacTxDrop=$2
}

/MacRxDrop:/ {
    MacRxDrop=$2
}

/PhyTxDrop:/ {
    PhyTxDrop=$2
}

/PhyRxDrop:/ {
    PhyRxDrop=$2
}

/AvgSNR:/ {
    avgSnr=$2
}

/AvgNoise:/ {
    avgNoise=$2
}


/AvgSignal:/ {
    avgSignal=$2
}

/Average throughput/ {
    avgThroughput=$3 +0
}

/Applicative simulation time:/ {
    appSimTime=$4+0
}


/Overall APs throughput at/ {
    cur_tp=$6+0
    if(cur_tp!=0){
        tp+=cur_tp
        ntp++
    }
}

/swappedCount/ {
    split($1,a,":")
    swappedCount=a[2]+0
    split($2,a,":")
    cpuTimeU=a[2]+0
    split($3,a,":")
    cpuTimeS=a[2]+0
    split($4,a,":")
    wallClock=a[2]+0
    split($5,a,":")
    peakMemUsage=a[2]+0
}


END {
    final_tp=0
    if(ntp!=0)
        final_tp=tp/ntp
    print("simTime="simTime-simTimeOffset";simTimeOffset="simTimeOffset\
          ";dataSent="dataSent";dataReceived="dataReceived";MacTxDrop="MacTxDrop";MacRxDrop="MacRxDrop";PhyTxDrop="PhyTxDrop";PhyRxDrop="PhyRxDrop\
          ";throughput="avgThroughput";appSimTime="appSimTime-simTimeOffset\
        ";swappedCount="swappedCount";cpuTimeU="cpuTimeU";cpuTimeS="cpuTimeS";wallClock="wallClock";peakMemUsage="peakMemUsage";avgSignal="avgSignal";avgNoise="avgNoise";avgSNR="avgSnr";nbSinkTot="totSink";avgThroughputAP="avgThroughputAP";avgThroughputSTA="avgThroughputSTA";nbSinkAP="nbSinkAP";nbSinkSTA="nbSinkSTA";dataReceivedSTA="dataReceivedSTA";dataReceivedAP="dataReceivedAP";totEnergyConsumption="totEnergyConsumption)
}