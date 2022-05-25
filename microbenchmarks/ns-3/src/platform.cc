#include "modules.hpp"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/applications-module.h"

#include <math.h>
#include <random>


#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"

ApplicationContainer Sinks;

// only used in topology 3
ApplicationContainer SinksAP;
ApplicationContainer SinksSTA;

// for energy measurements
DeviceEnergyModelContainer consumedEnergyAP;
DeviceEnergyModelContainer consumedEnergySTA;

/**
 * @brief Make build application
 * @param dst Destination IP address
 * @param port Communication port
 * @param dataSize Amount of data to send
 * @param nodes Node to install the application
 */
void txApp(Ipv4Address dst, int port,int dataSize,NodeContainer nodes, bool limitSize=false){
  for(int i=0; i< nodes.GetN(); i++)
    NS_LOG_UNCOND("Creating sender on Node "<<nodes.Get(i)->GetId());

   BulkSendHelper echoClientHelper ("ns3::TcpSocketFactory",InetSocketAddress (dst, port));
   if(limitSize == true)
    echoClientHelper.SetAttribute ("MaxBytes", UintegerValue (dataSize)); //remove maxbytes to observe an amount of bytes over duration instead of the duration over an amount of bytes sent

  // Warning: comment the line 295 from src/application/model/on-off-application.cc
  // otherwise ns-3 will exit(1) when byte send >= MaxBytes
//  OnOffHelper echoClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (dst, port));
//  echoClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//  echoClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//  echoClientHelper.SetConstantRate (DataRate ("100Mb/s"));
//  echoClientHelper.SetAttribute ("MaxBytes", UintegerValue (dataSize));
//  echoClientHelper.SetAttribute ("PacketSize", UintegerValue (1500));

  Ptr<UniformRandomVariable> trafficStart = CreateObject<UniformRandomVariable> ();
  double trafStar = trafficStart->GetValue(10.0, 10.0 + 0.01);
  ApplicationContainer apps = echoClientHelper.Install(nodes);
  apps.Start (Seconds (trafStar)); // App start somewhere at 10s !!
  //apps.Stop (Seconds (simTime+100));
}

/**
 * @brief Setup position/mobility on a wifi cell
 *
 * Node are spread homogeneously in circle around the access point.
 *
 * @param STA Stations in the wifi cell
 * @param AP Access Point of the wifi cell
 * @param The Stations circle radius
 * @param isTopo1 Topo1 require special node positioning
 */
void SetupMobility(NodeContainer STA, NodeContainer AP, double radius, bool isTopo1){
  // Setup AP position (on the origin)
  MobilityHelper mobilityAP;
  Ptr<ListPositionAllocator> posAP = CreateObject <ListPositionAllocator>();
  posAP ->Add(Vector(0, 0, 0));
  mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
  mobilityAP.SetPositionAllocator(posAP);
  mobilityAP.Install(NodeContainer(AP));

  // Set station positions homogeneously in circle around the origin
  MobilityHelper mobilitySTA;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  double step=2*M_PI/STA.GetN();
  double theta=0;
  /*if(isTopo1){ // For topo 1 we align communicating stations to limit side effects
    for(int i=0;i<STA.GetN();i++){
      double x=cos(theta)*radius;
      double y=sin(theta)*radius;
      if(i%2!=0){
        theta-=step;
        x=cos(theta+M_PI)*radius;
        y=sin(theta+M_PI)*radius;
      }
      positionAlloc ->Add(Vector(x,y, 0)); // node1
      theta+=step;
    }
  }
  else {
    for(int i=0;i<STA.GetN();i++){
      positionAlloc ->Add(Vector(cos(theta)*radius,sin(theta)*radius, 0)); // node1
      theta+=step;
    }
  }*/

  for(int i=0;i<STA.GetN();i++){
    double r = radius * sqrt(((double) rand() / (RAND_MAX)));
    double theta = ((double) rand() / (RAND_MAX)) * 2 * M_PI;
    positionAlloc ->Add(Vector(cos(theta)*r,sin(theta)*r, 0)); //
  }
  mobilitySTA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySTA.SetPositionAllocator(positionAlloc);
  mobilitySTA.Install(STA);
}

void SetupMobilitySNR(NodeContainer STA1, NodeContainer STA2, NodeContainer AP, double radius, double radius2){
  // Setup AP position (on the origin)
  MobilityHelper mobilityAP;
  Ptr<ListPositionAllocator> posAP = CreateObject <ListPositionAllocator>();
  posAP ->Add(Vector(0, 0, 0));
  mobilityAP.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // Sensors are fixed
  mobilityAP.SetPositionAllocator(posAP);
  mobilityAP.Install(NodeContainer(AP));

  // Set station positions homogeneously in circle around the origin
  MobilityHelper mobilitySTA;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  double step=2*M_PI/(STA1.GetN()+STA2.GetN());
  double theta=0;

  for(int i=0;i<STA1.GetN();i++){
    positionAlloc ->Add(Vector(cos(theta)*radius,sin(theta)*radius, 0));
    theta+=step;
  }

  for(int i=STA1.GetN();i<STA2.GetN();i++){
    positionAlloc ->Add(Vector(cos(theta)*radius2,sin(theta)*radius2, 0));
    theta+=step;
  }

  mobilitySTA.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySTA.SetPositionAllocator(positionAlloc);
  mobilitySTA.Install(NodeContainer(STA1,STA2));
}


/**
 * @brief Setup a 802.11n stack on STA and AP.
 * @param STA The cell's stations
 * @param AP the cell's Acces Point
 * @return The wifi cell net devices
 */
WifiNetDev SetupWifi(NodeContainer STA, NodeContainer AP){
  SpectrumWifiPhyHelper spectrumPhy;

  Ptr<MultiModelSpectrumChannel> spectrumChannel
    = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<FriisPropagationLossModel> lossModel
    = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (5.180e9);
  spectrumChannel->AddPropagationLossModel (lossModel);

  Ptr<ConstantSpeedPropagationDelayModel> delayModel
    = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);

  spectrumPhy.SetChannel (spectrumChannel);
  spectrumPhy.SetErrorRateModel ("ns3::TableBasedErrorRateModel");
  spectrumPhy.Set ("ChannelWidth", UintegerValue (40));
  spectrumPhy.Set ("Frequency", UintegerValue (5180));
  spectrumPhy.Set ("TxPowerStart", DoubleValue (20)); // dBm  (1.26 mW)
  spectrumPhy.Set ("TxPowerEnd", DoubleValue (20));

  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n_5GHZ);

  // We setup this according to:
  // this examples/wireless/wifi-spectrum-per-example.cc
  // and https://en.wikipedia.org/wiki/IEEE_802.11n-2009
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (mcs), // Before was HtMcs7
                                      "ControlMode", StringValue (mcs));


  WifiMacHelper wifiMac;
  Ssid ssid = Ssid ("net1");
  /* Configure STA */
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer sensorsNetDevices;
  sensorsNetDevices = wifiHelper.Install (/*wifiPhy*/spectrumPhy, wifiMac, STA);
  /* Configure AP */
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apNetDevice;
  apNetDevice = wifiHelper.Install (/*wifiPhy*/spectrumPhy, wifiMac, AP);
//  for(int i=0;i<apNetDevice.GetN();i++){
//	  Ptr<WifiMac> macAp = DynamicCast<WifiNetDevice> (apNetDevice.Get(i))->GetMac();
//	//  macAp->TraceConnect ("MacRx", "p",MakeCallback(&ApRxTrace));
//
//	  DoubleValue v;
//	  DynamicCast<WifiNetDevice> (apNetDevice.Get(i))->GetPhy()->GetAttribute ("RxSensitivity",v);
//	  NS_LOG_UNCOND("dd:"<<v.Get());
//  }
  if(pcapCapture){
    spectrumPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
    //spectrumPhy.EnablePcap("STA_capture_s"+std::to_string(seed), STA,true);
    spectrumPhy.EnablePcap("AP_capture_s"+std::to_string(seed)+"_n"+std::to_string(nNode)+"_p"+std::to_string(nNodePair), AP,true);
  }
    return(std::make_pair(apNetDevice, sensorsNetDevices));
}

/**
 * @brief Configure Applications (flow from SimGrid perspective) for topology 1
 *
 * @param AP The wifi Access Point
 * @param STA The wifi stations
 * @param APNET The @a AP net device
 * @param STANET The @a STA net devices
 * @param flowConf if 1 unidirectional and if 2 bidirectional
 * @param dataSize Number of data to send per application
 * @param nNodepair Number of pair of node (can be deduce from STANET but I am lazy)
 */
void SetFlowConfT1(NodeContainer AP,NodeContainer STA, NetDeviceContainer APNET,NetDeviceContainer STANET,short flowConf, int dataSize, short nNodePair){
  // Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (NodeContainer(AP,STA));

  Ipv4InterfaceContainer interfaces;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces=ipv4.Assign(NetDeviceContainer(APNET,STANET));

  int port=80;
  PacketSinkHelper STASink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));

  for(int i=0;i<(nNodePair*2-1);i+=2){
    Ipv4Address IPnode1,IPnode2; // Will be fill in the following for loop
    IPnode1=interfaces.GetAddress(i+1); // Note first interface == AP!!!
    IPnode2=interfaces.GetAddress(i+2);

    if(flowConf==1){
      txApp(IPnode2,port,dataSize,STA.Get(i));
      Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node
    }
    else if(flowConf==2){
      txApp(IPnode2,port,dataSize,STA.Get(i));
      Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node

      txApp(IPnode1,port,dataSize,STA.Get(i+1));
      Sinks.Add(STASink.Install(STA.Get(i))); // Install sink on the first node
    }
  }
}


/**
 * @brief Configure Applications (flow from SimGrid perspective) for topology 1
 *
 * @param AP The wifi Access Point
 * @param STA The wifi stations
 * @param APNET The @a AP net device
 * @param STANET The @a STA net devices
 * @param flowConf if 1 upward and if 2 downwards
 * @param dataSize Number of data to send per application
 */
void SetFlowConfT2(NodeContainer AP,NodeContainer STA, NetDeviceContainer APNET,NetDeviceContainer STANET,short flowConf){
  // Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (NodeContainer(AP,STA));
  Ipv4InterfaceContainer interfaces;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces=ipv4.Assign(NetDeviceContainer(APNET,STANET));

  Ipv4Address IPAP; // Will be fill in the following for loop
  IPAP=interfaces.GetAddress(0);
  int port=80;

  /* 1 = upwards approach*/
  if (flowConf == 1) {
    txApp(IPAP,port,1,STA);

    PacketSinkHelper apSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
    Sinks.Add(apSink.Install(AP.Get(0))); // Install sink on AP
  } else if (flowConf == 2) {
    for (int i=0; i < STA.GetN(); i++) {
      // create app on AP to send to sink on STA (downlink communication)
      txApp(interfaces.GetAddress(1+i),port, 1, AP.Get(0));
      PacketSinkHelper staSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
      Sinks.Add(staSink.Install(STA.Get(i))); // Install sink on AP
    }
  } else if (flowConf == 0) {
    txApp(IPAP,port,dataSize,STA,true);
    PacketSinkHelper apSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
    Sinks.Add(apSink.Install(AP.Get(0))); // Install sink on AP
  }
}

/**
 * @brief Configure Applications (flow from SimGrid perspective) for topology 3 (mixed T1+T2) @ref buildT3()
 * @param AP The wifi Access Point
 * @param STA The wifi stations
 * @param APNET The @a AP net device
 * @param STANET The @a STA net devices
 * @param flowConf @ref bluildT3()
 * @param dataSize Number of data to send per application
 * @param nNodepair Number of pair of node (can be deduce from STANET but I am lazy)
 */
void SetFlowConfT3(NodeContainer AP,NodeContainer STA, NetDeviceContainer APNET,NetDeviceContainer STANET,short flowConf, int dataSize, short nNodePair){
  // Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (NodeContainer(AP,STA));


  Ipv4InterfaceContainer interfaces;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces=ipv4.Assign(NetDeviceContainer(APNET,STANET));

  Ipv4Address IPAP; // Will be fill in the following for loop
  IPAP=interfaces.GetAddress(0);

  int port=80;
  PacketSinkHelper apSink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
  //Sinks.Add(apSink.Install(AP.Get(0)));
  SinksAP.Add(apSink.Install(AP.Get(0)));
  PacketSinkHelper STASink("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));



  // Configure T1 AND T2
  for(int i=0;i<(nNodePair*3-1);i+=3){
    Ipv4Address IPnode1,IPnode2; // Will be fill in the following for loop
    IPnode1=interfaces.GetAddress(i+1); // Note first interface == AP!!!
    IPnode2=interfaces.GetAddress(i+2);

    // T1
    if(flowConf==1){
      txApp(IPnode2,port,dataSize,STA.Get(i));
      //Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node
      SinksSTA.Add(STASink.Install(STA.Get(i+1)));
    }
    else if(flowConf==2){
      txApp(IPnode2,port,dataSize,STA.Get(i));
      //Sinks.Add(STASink.Install(STA.Get(i+1))); // Install sink on last node
      SinksSTA.Add(STASink.Install(STA.Get(i+1)));

      txApp(IPnode1,port,dataSize,STA.Get(i+1));
      //Sinks.Add(STASink.Install(STA.Get(i))); // Install sink on the first node
      SinksSTA.Add(STASink.Install(STA.Get(i)));

    }

    // T2
    txApp(IPAP,port,dataSize,STA.Get(i+2));
  }
}

/**
 * Set up energy measurement capacity
 * (provision 'infinite' batteries and put them on the devices)
 */
void setupEnergyMetrics(NodeContainer aps, NodeContainer stas, WifiNetDev netDev) {
    // energy model
   BasicEnergySourceHelper energySource;
   // put very high value to have "non-battery" nodes..
   energySource.Set("BasicEnergySourceInitialEnergyJ", DoubleValue (10000000));

   EnergySourceContainer batteryAP = energySource.Install(aps);
   EnergySourceContainer batterySTA = energySource.Install(stas);

   WifiRadioEnergyModelHelper radioEnergyHelper;
   //radioEnergyHelper.Set("IdleCurrentA", DoubleValue (0.0000001)); //to change energy value

   consumedEnergyAP = radioEnergyHelper.Install(netDev.first, batteryAP);
   consumedEnergySTA =  radioEnergyHelper.Install(netDev.second, batterySTA);
}

/**
 * @brief Build Topology 1
 * @param nNodePair Number of pair of nodes which will communicate together
 * @param flowConf if 1 unidirectional and if 2 bidirectional
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT1(short nNodePair, short flowConf, int dataSize, double radius){
  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STA;
  STA.Create(2*nNodePair);

  NS_LOG_UNCOND("created nodes, call setupwifi");
  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  NS_LOG_UNCOND("setupwifi finished, call setupmobility");
  SetupMobility(STA, AP, radius,true);

  NS_LOG_UNCOND("Setup energy measurement");
  //setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  NS_LOG_UNCOND("call setflowconf");
  SetFlowConfT1(AP,STA,netDev.first,netDev.second,flowConf,dataSize,nNodePair);

}


/**
 * @brief Build Topology 2
 * @param nNodePair Number of nodes which will communicate together
 * @param flowConf if 1 unidirectional and if 2 bidirectional
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT2(short nNode, short flowConf, int dataSize, double radius){
  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STA;
  STA.Create(nNode);

  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  SetupMobility(STA, AP,radius,false);

  NS_LOG_UNCOND("Setup energy measurement");
  //setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  SetFlowConfT2(AP,STA,netDev.first,netDev.second,flowConf);
}


/**
 * @brief Build Topology 2
 * @param nNodePair Number of pair of nodes which will communicate together
 * @param flowConf if 1 unidirectional and if 2 bidirectional
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT4(int nSTAInFirstRange, int nSTAInSecondRange, double radius, double radius2, int flowConf){
  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STARange1;
  STARange1.Create(nSTAInFirstRange);
  NodeContainer STARange2;
  STARange2.Create(nSTAInSecondRange);
  NodeContainer STA = NodeContainer(STARange1, STARange2);

  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  SetupMobilitySNR(STARange1, STARange2, AP,radius,radius2);

  NS_LOG_UNCOND("Setup energy measurement");
  //setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  SetFlowConfT2(AP,STA,netDev.first,netDev.second,flowConf);
}



/**
 * @brief Build Topology 3
 *
 * If nNodePair == 1, then it will create 1 pair of node + 1 node that send data to the AP
 * If nNodePair == 2, then it will create 2 pair of node + 2 node that send data to the AP
 * @param nNodePair Number of pair of nodes + a single node to create
 * @param flowConf if 1 unidirectional and if 2 bidirectional (for the node pair only)
 * @param dataSize Number of data to send per application
 * @param radius The radius of the wifi cell
 */
void buildT3(short nNodePair, short flowConf, int dataSize, double radius){
  // Create nodes
  NodeContainer AP;
  AP.Create(1);
  NodeContainer STA;
  STA.Create(3*nNodePair);

  // Setup wifi
  WifiNetDev netDev=SetupWifi(STA,AP);
  SetupMobility(STA, AP, radius,false);

  NS_LOG_UNCOND("Setup energy measurement");
  //setupEnergyMetrics(AP,STA,netDev);

  /* Set Flow Configuration  */
  SetFlowConfT3(AP,STA,netDev.first,netDev.second,flowConf,dataSize,nNodePair);
}

