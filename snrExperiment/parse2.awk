/1 Sent: / {
  throughput1=$8
  bytesSent1=$6
}

/2 Sent: / {
  throughput2=$8
  bytesSent2=$6
}

/Simulation took / {
  simTime=$5
}

END {
  print("simTime="simTime";throughput1="throughput1";bytesSent1="bytesSent1";throughput2="throughput2";bytesSent2="bytesSent2)
}
