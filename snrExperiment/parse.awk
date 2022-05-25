/Rate: / {
  simTime
  throughput=$8
  bytesSent=$6
}

/Simulation took / {
  simTime=$5
}

END {
  print("simTime="simTime";throughput="throughput";bytesSent="bytesSent)
}
