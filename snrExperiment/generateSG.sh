#!/usr/bin/bash

date=$(date "+%s")
logDir=logs/${date}
mkdir -p ${logDir}

#scenario 1
#echo "dist, simulator, simTime, throughput, bytesSent" > ${logDir}/results1.csv
#for dist in $(seq 1 1 100)
#do
#  make run  ARGS="wifi_snr.xml 1 ${dist} --log=root.app:file:${logDir}/res1_${dist}.log"
#  vals=$(awk -f parse.awk ${logDir}/res1_${dist}.log)
#  eval "$vals"

#  echo "${dist},SG,${simTime},${throughput},${bytesSent}" >> ${logDir}/results1.csv
#done

#scenario 2
nbSTA1=1
nbSTA2=1
echo "dist1, dist2, simulator, simTime, throughput1, bytesSent1, throughput2, bytesSent2" > ${logDir}/results2.csv
for dist1 in $(seq 1 1 1)
do
  for dist2 in $(seq 50 0.1 55)
  do
    make run  ARGS="wifi_snr.xml 2 ${dist1} ${dist2} ${nbSTA1} ${nbSTA2} 5 --log=root.app:file:${logDir}/res2_${dist1}_${dist2}_${nbSTA1}_${nbSTA2}.log"
    vals=$(awk -f parse2.awk ${logDir}/res2_${dist1}_${dist2}_${nbSTA1}_${nbSTA2}.log)
    eval "$vals"

    echo "${dist1},${dist2},SG,${simTime},${throughput1},${bytesSent1},${throughput2},${bytesSent2}" >> ${logDir}/results2.csv
  done
done
