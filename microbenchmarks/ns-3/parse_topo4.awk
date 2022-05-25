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
    totEnergyConsumption=0
    nbSTASNR1=0
    nbSTASNR2=0
    radius1=0
    radius2=0
}

/nSTAInFirstRange/ {
    nbSTASNR1=$2-0
}

/nSTAInSecondRange/ {
    nbSTASNR2=$2-0
}

/radius1/ {
    radius1=$2-0
}

/radius2/ {
    radius2=$2-0
}

/Device consumed/ {
    totEnergyConsumption+=$4
}

/Flow/ && /ends at/ {
    if($5-0 > simTime) # -0 to cast into a number
        simTime=$5-0
}

/NbSinkTot:/ {
    totSink=$2+0
}

/Sending check:/ {
    dataSent+=$7
}

/Sink [0-9]+ received/ {
    dataReceived+=$4
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
    print("simTime="simTime-simTimeOffset";radius1="radius1";radius2="radius2";nbSTASNR1="nbSTASNR1";nbSTASNR2="nbSTASNR2";simTimeOffset="simTimeOffset\
          ";dataSent="dataSent";dataReceived="dataReceived";MacTxDrop="MacTxDrop";MacRxDrop="MacRxDrop";PhyTxDrop="PhyTxDrop";PhyRxDrop="PhyRxDrop\
          ";throughput="avgThroughput";appSimTime="appSimTime-simTimeOffset\
        ";swappedCount="swappedCount";cpuTimeU="cpuTimeU";cpuTimeS="cpuTimeS";wallClock="wallClock";peakMemUsage="peakMemUsage";avgSignal="avgSignal";avgNoise="avgNoise";avgSNR="avgSnr";nbSinkTot="totSink";totEnergyConsumption="totEnergyConsumption)
}
