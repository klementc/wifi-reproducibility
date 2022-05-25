#include "modules.hpp"
#ifndef NS3_VERSION
#define NS3_VERSION "unknown"
#endif

// Good tuto: https://www.nsnam.org/docs/release/3.7/tutorial/tutorial_27.html

NS_LOG_COMPONENT_DEFINE ("WIFISensorsSimulator");

// Global variables for use in callbacks.
long MacTxDrop=0,PhyTxDrop=0;
long MacRxDrop=0,PhyRxDrop=0;
long APRx=0;
float TimeLastRxPacket=-1;
int simTime = 15;
int seed;
std::string mcs = "HtMcs3";
uint32_t nNode;
uint32_t nNodePair;
uint32_t dataSize;


// wifi-spectrum-per-example.cc
double g_signalDbmAvg=0;
double g_noiseDbmAvg=0;
uint32_t g_samples=0;

bool pcapCapture = false;

void MonitorSniffRx (Ptr<const Packet> packet,
                     uint16_t channelFreqMhz,
                     WifiTxVector txVector,
                     MpduInfo aMpdu,
                     SignalNoiseDbm signalNoise,
                     uint16_t staId)

{
  g_samples++;
  g_signalDbmAvg += ((signalNoise.signal - g_signalDbmAvg) / g_samples);
  g_noiseDbmAvg += ((signalNoise.noise - g_noiseDbmAvg) / g_samples);
}

void MacTxDropCallback(std::string context, Ptr<const Packet> p)
{
	MacTxDrop++;
}
void MacRxDropCallback(std::string context, Ptr<const Packet> p)
{
	MacRxDrop++;
}
void PhyTxDropCallback(std::string context,Ptr<const Packet> p)
{
	PhyTxDrop++;
}
void PhyRxDropCallback(std::string context,Ptr<const Packet> p,WifiPhyRxfailureReason r)
{
	PhyRxDrop++;
	//NS_LOG_UNCOND("Packet drop at rx. Reason: "<< r);
}

void PacketSinkRx(/*std::string path, */Ptr<const Packet> p, const Address &address){
	float simTime=Simulator::Now().GetSeconds();
	if(TimeLastRxPacket<simTime)
		TimeLastRxPacket=simTime;
	APRx+=p->GetSize();
}

void PrintThroughput(){
	uint32_t simTime=Simulator::Now().GetSeconds();
	if(APRx!=0){
		double throughput=APRx*8;
		NS_LOG_UNCOND("Overall APs throughput at " << simTime <<"s: " << throughput <<"bps");
		APRx=0;
	}
	Simulator::Schedule (Seconds (1.0), &PrintThroughput);
}

/**
 * To get more details about functions please have a look at modules/modules.hpp
 * make ARGS="--dataSize=12022949 --seed=18 --radius=15 --topo=1 --nNodePair=5 --flow=1" run
 */
int main(int argc, char* argv[]){
  uint32_t topo=1; // Choose you topology
  uint32_t flow=1; // 1=one way 1=bidirectional
  dataSize=1000; // Choose the amount of data sended by nodes
  nNodePair=1; // Number of sender/receiver pair
  nNode=1; // Number of sender/receiver pair
  uint32_t threshRTSCTS=100; // minimum frame size before using RTS/CTS
  double radius=10; // Choose the distance between AP and STA
  seed=10; // Choose the used by ns-3
  bool anim=false; // Generate animation.xml ?

  CommandLine cmd;
  cmd.AddValue ("topo", "Which topology to use", topo);
  cmd.AddValue ("flow", "Which flow configuration to use", flow);
  cmd.AddValue ("dataSize", "Packet data size sended by senders", dataSize);
  cmd.AddValue ("nNodePair", "Number of sender/receiver pairs", nNodePair);
  cmd.AddValue ("nNode", "Number of node", nNode);
  cmd.AddValue ("threshRTSCTS", "Set the RTS threshold", threshRTSCTS);
  cmd.AddValue ("radius", "Radius between STA and AP", radius);
  cmd.AddValue ("seed", "Simulator seed", seed);
  cmd.AddValue ("anim", "Generate animate.xml", anim);
  cmd.AddValue ("pcap", "Generate PCAP", pcapCapture);
  cmd.AddValue ("simTime", "Communication duration", simTime);
  cmd.AddValue ("mcs", "MCS to use for data and control", mcs);

  int nSTAInFirstRange = -1;
  int nSTAInSecondRange = -1;
  double radius2 = -1;
  cmd.AddValue ("nSTAInFirstRange", "how many stations are radius meters away from the ap", nSTAInFirstRange);
  cmd.AddValue ("nSTAInSecondRange", "how many stations are radius2 meters away from the ap", nSTAInSecondRange);
  cmd.AddValue ("radius2", "range for the second SNR", radius2);

  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(threshRTSCTS)); // Enable RTS/CTS

  ns3::RngSeedManager::SetSeed (seed);
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1500));
  NS_LOG_UNCOND("WARNING: Transmissions start at T>10s. Thus, this delay should be removed from each line of logs. Note that if you use parse.awk, this delay is automatically removed."); LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //  LogComponentEnable("PacketSink", LOG_LEVEL_INFO);


  // ---------- Setup Simulations ----------
  if(topo==1)
    buildT1(nNodePair,flow,dataSize,radius);
  else if(topo==2)
    buildT2(nNode,flow,dataSize,radius);
  else if(topo==3)
    buildT3(nNodePair,flow,dataSize,radius);
  else if(topo==4)
    buildT4(nSTAInFirstRange, nSTAInSecondRange, radius, radius2,flow);

  // Setup channel size (BW) it seems to be at 20MHz by default
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));

  // Disable Short Guard Interval: Add ghost signal to the carrier in order to reduce the collision probability with the data (multipath fading etc..)
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (false));
 // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDropCallback));
  Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback(&MacRxDropCallback));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRx));

  Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback (&MonitorSniffRx));

  // Setup routing tables
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Enable flow monitoring
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // Run Simulations
  Simulator::Stop (Seconds (simTime));
  if(anim)
    AnimationInterface anim ("animation.xml");
  Simulator::Schedule (Seconds (11.0), &PrintThroughput);
  Simulator::Run ();

  for(auto &flow: monitor->GetFlowStats ()){
    NS_LOG_UNCOND("Flow "<< flow.first <<\
    		" ends at " << flow.second.timeLastRxPacket.GetSeconds() << "s" <<\
    		" first rx pkt " << flow.second.timeFirstTxPacket.GetSeconds() << "s" <<\
			" nb lost pkt " << flow.second.lostPackets << " rxBytes " << flow.second.rxBytes << " txBytes " << flow.second.txBytes );
  }
  // Un comment to save flows into xml
  //  monitor->SerializeToXmlFile("NameOfFile.xml", true, true);

  // Echo seed informations
  int i=1;
  long totalRx=0;
  int nbSinkTot=0;

  // differentiate between pair sink and ap sink in the case of scenarios 4 and 5
  if((topo==3 && flow==1) || (topo==3 && flow==2)){
    long totalRxAP = 0;
    long totalRxSTA = 0;
    for(ApplicationContainer::Iterator n = SinksAP.Begin (); n != SinksAP.End (); n++){
      ns3::Ptr<Application> app=*n;
      PacketSink& ps=dynamic_cast<PacketSink&>(*app);
      NS_LOG_UNCOND("SinkAP "<< i << " received " << ps.GetTotalRx() << " bytes");
      i++;
      totalRxAP+=ps.GetTotalRx();
      nbSinkTot++;
    }
    for(ApplicationContainer::Iterator n = SinksSTA.Begin (); n != SinksSTA.End (); n++){
      ns3::Ptr<Application> app=*n;
      PacketSink& ps=dynamic_cast<PacketSink&>(*app);
      NS_LOG_UNCOND("SinkSTA "<< i << " received " << ps.GetTotalRx() << " bytes");
      i++;
      totalRxSTA+=ps.GetTotalRx();
      nbSinkTot++;
    }
    totalRx = totalRxAP + totalRxSTA;
    NS_LOG_UNCOND("SinkAP amount: " << SinksAP.GetN());
    NS_LOG_UNCOND("SinkSTA amount: " << SinksSTA.GetN());
    NS_LOG_UNCOND("Average throughput AP: " << (totalRxAP*8)/((TimeLastRxPacket-10)));
    NS_LOG_UNCOND("Average throughput STA: " << (totalRxSTA*8)/((TimeLastRxPacket-10)));
    NS_LOG_UNCOND("Average throughput: " << (totalRx*8)/((TimeLastRxPacket-10)));

  }else{
    for(ApplicationContainer::Iterator n = Sinks.Begin (); n != Sinks.End (); n++){
      ns3::Ptr<Application> app=*n;
      PacketSink& ps=dynamic_cast<PacketSink&>(*app);
      NS_LOG_UNCOND("Sink "<< i << " received " << ps.GetTotalRx() << " bytes");
      i++;
      totalRx+=ps.GetTotalRx();
      nbSinkTot++;
    }
    NS_LOG_UNCOND("Sink amount: " << Sinks.GetN());
    NS_LOG_UNCOND("Average throughput: " << (totalRx*8)/((TimeLastRxPacket-10)));

  }

  NS_LOG_UNCOND("radius1: " << radius);
  NS_LOG_UNCOND("radius2: " << radius2);
  NS_LOG_UNCOND("nSTAInFirstRange: " << nSTAInFirstRange);
  NS_LOG_UNCOND("nSTAInSecondRange: " << nSTAInSecondRange);
  NS_LOG_UNCOND("NbSinkTot: " << nbSinkTot);
  NS_LOG_UNCOND("MacTxDrop: " << MacTxDrop);
  NS_LOG_UNCOND("MacRxDrop: " << MacRxDrop);
  NS_LOG_UNCOND("PhyTxDrop: " << PhyTxDrop);
  NS_LOG_UNCOND("PhyRxDrop: " << PhyRxDrop);
  NS_LOG_UNCOND("AvgSignal: "<< g_signalDbmAvg);
  NS_LOG_UNCOND("AvgNoise: "<< g_noiseDbmAvg);
  NS_LOG_UNCOND("AvgSNR: "<< (g_signalDbmAvg - g_noiseDbmAvg));
  NS_LOG_UNCOND("Applicative simulation time: " << TimeLastRxPacket << "s");

  // energy output
  /*DeviceEnergyModelContainer::Iterator energyIt = consumedEnergySTA.Begin();
  while(energyIt!=consumedEnergySTA.End()) {
    NS_LOG_UNCOND("STA Device consumed " <<  (*energyIt)->GetTotalEnergyConsumption() << " W");
    energyIt++;
  }
  NS_LOG_UNCOND("AP Device consumed " <<(*consumedEnergyAP.Begin())->GetTotalEnergyConsumption()<<" W");
  */
  // Finish
  Simulator::Destroy ();
  return(0);
}
