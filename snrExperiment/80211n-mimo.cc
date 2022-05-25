/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

// This example is used to validate 802.11n MIMO.
//
// It outputs plots of the throughput versus the distance
// for every HT MCS value and from 1 to 4 MIMO streams.
//
// The simulation assumes a single station in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
// The user can choose whether UDP or TCP should be used and can configure
// some 802.11n parameters (frequency, channel width and guard interval).

#include <fstream>

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  std::ofstream file ("80211n-mimo-throughput.plt");

  std::vector <std::string> modes;
/*  modes.push_back ("HtMcs0");
  modes.push_back ("HtMcs1");
  modes.push_back ("HtMcs2");*/
  modes.push_back ("HtMcs3");
/*  modes.push_back ("HtMcs4");
  modes.push_back ("HtMcs5");
  modes.push_back ("HtMcs6");
  modes.push_back ("HtMcs7");
  modes.push_back ("HtMcs8");
  modes.push_back ("HtMcs9");
  modes.push_back ("HtMcs10");
  modes.push_back ("HtMcs11");
  modes.push_back ("HtMcs12");
  modes.push_back ("HtMcs13");
  modes.push_back ("HtMcs14");
  modes.push_back ("HtMcs15");
  modes.push_back ("HtMcs16");
  modes.push_back ("HtMcs17");
  modes.push_back ("HtMcs18");
  modes.push_back ("HtMcs19");
  modes.push_back ("HtMcs20");
  modes.push_back ("HtMcs21");
  modes.push_back ("HtMcs22");
  modes.push_back ("HtMcs23");
  modes.push_back ("HtMcs24");
  modes.push_back ("HtMcs25");
  modes.push_back ("HtMcs26");
  modes.push_back ("HtMcs27");
  modes.push_back ("HtMcs28");
  modes.push_back ("HtMcs29");
  modes.push_back ("HtMcs30");
  modes.push_back ("HtMcs31");*/

  double simulationTime = 5; //seconds
  double frequency = 5.0; //whether 2.4 or 5.0 GHz
  double step = 1; //meters
  bool shortGuardInterval = false;
  bool channelBonding = false;
  int scenario = 1;

  CommandLine cmd;
  cmd.AddValue ("step", "Granularity of the results to be plotted in meters", step);
  cmd.AddValue ("simulationTime", "Simulation time per step (in seconds)", simulationTime);
  cmd.AddValue ("channelBonding", "Enable/disable channel bonding (channel width = 20 MHz if false, channel width = 40 MHz if true)", channelBonding);
  cmd.AddValue ("shortGuardInterval", "Enable/disable short guard interval", shortGuardInterval);
  cmd.AddValue ("frequency", "Whether working in the 2.4 or 5.0 GHz band (other values gets rejected)", frequency);
  cmd.AddValue ("scenario", "scenario (1=calibration, 2=2 stas)", scenario);
  cmd.Parse (argc,argv);
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (2200));//UintegerValue(100)); // Enable RTS/CTS

  std::ofstream outCSV;
  outCSV.open ("results_ns3.csv");
  // write header
  outCSV << "dist1,dist2,simulator,seed,simTime,throughput1,bytesSent1,throughput2,bytesSent2,throughput3,bytesSent3\n";

  for (uint32_t i = 1; i < 30; i++) //MCS
    {
      RngSeedManager::SetSeed (i);
      std::cout << "seed " << i << std::endl;

      for (double d = 15; d <= 15; ) //distance
      {
        for (double d2 = 15; d2 <= 65;) {
          std::cout << "Distance1 = " << d << "m " << "Distance2 = " << d2 << "m" << std::endl;
          uint32_t payloadSize; //1500 byte IP packet

          payloadSize = 1448; //bytes
          Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

          uint8_t nStreams = 1; //number of MIMO streams

          NodeContainer wifiStaNode;
          if(scenario == 1) {  // for calibration
            wifiStaNode.Create (1);
          } else if (scenario == 2) { // 2 nodes in a variable zone
            wifiStaNode.Create (2);
          } else if (scenario == 3) {
            wifiStaNode.Create (3);
          }

          NodeContainer wifiApNode;
          wifiApNode.Create (1);

          YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
          YansWifiPhyHelper phy = YansWifiPhyHelper ();
          phy.SetChannel (channel.Create ());

          // Set guard interval
          Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (shortGuardInterval));
          //phy.Set ("ShortGuardEnabled", BooleanValue (shortGuardInterval));
          // Set MIMO capabilities
          phy.Set ("Antennas", UintegerValue (nStreams));
          phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (nStreams));
          phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (nStreams));

          WifiMacHelper mac;
          WifiHelper wifi;
          if (frequency == 5.0)
            {
              wifi.SetStandard (WIFI_STANDARD_80211n_5GHZ);
            }
          else if (frequency == 2.4)
            {
              wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);
              Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
            }
          else
            {
              std::cout << "Wrong frequency value!" << std::endl;
              return 0;
            }

          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (modes[0]),
                                        "ControlMode", StringValue (modes[0]));
          Ssid ssid = Ssid ("ns3-80211n");

          mac.SetType ("ns3::StaWifiMac",
                      "Ssid", SsidValue (ssid));
                      /* setting blockack threshold for sta's BE queue */
                     /*"VO_BlockAckThreshold", UintegerValue (0),
                     "BE_MaxAmpduSize", UintegerValue (0));*/


          NetDeviceContainer staDevice;
          staDevice = wifi.Install (phy, mac, wifiStaNode);

          mac.SetType ("ns3::ApWifiMac",
                      "Ssid", SsidValue (ssid));

          NetDeviceContainer apDevice;
          apDevice = wifi.Install (phy, mac, wifiApNode);

          // Set channel width
          if (channelBonding)
            {
              Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
            }

          // mobility.
          MobilityHelper mobility;
          Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

          positionAlloc->Add (Vector (0.0, 0.0, 0.0));
          positionAlloc->Add (Vector (d, 0.0, 0.0));
          // in case of scenario 2, add position of second node in addition to the first one
          if (scenario == 2) {
            //TODO: position d2
            positionAlloc->Add(Vector (d2, 0.0, 0.0));
          }
          if (scenario == 3) {
            positionAlloc->Add (Vector (d, 0.0, 0.0));
            positionAlloc->Add(Vector (d2, 0.0, 0.0));
          }
          mobility.SetPositionAllocator (positionAlloc);

          mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

          mobility.Install (wifiApNode);
          mobility.Install (wifiStaNode);

          /* Internet stack*/
          InternetStackHelper stack;
          stack.Install (wifiApNode);
          stack.Install (wifiStaNode);

          Ipv4AddressHelper address;
          address.SetBase ("192.168.1.0", "255.255.255.0");
          Ipv4InterfaceContainer staNodeInterface;
          Ipv4InterfaceContainer apNodeInterface;
          staNodeInterface = address.Assign (staDevice);
          apNodeInterface = address.Assign (apDevice);

          /* Setting applications */
          ApplicationContainer serverApp;

          //TCP flow
          uint16_t port1 = 50000, port2 = 50001, port3 = 5002;
          Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port1));
          /*PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
          //serverApp = packetSinkHelper.Install (wifiStaNode.Get (0));
          serverApp.Start (Seconds (0.0));
          serverApp.Stop (Seconds (simulationTime + 1));*/

          // add sink on AP
          PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny (), port1)));
          serverApp = packetSinkHelper1.Install(wifiApNode.Get (0));
          PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny (), port2)));
          serverApp.Add(packetSinkHelper2.Install(wifiApNode.Get (0)));
          PacketSinkHelper packetSinkHelper3 ("ns3::TcpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny (), port3)));
          serverApp.Add(packetSinkHelper3.Install(wifiApNode.Get (0)));
          serverApp.Start (Seconds (0.0));
          serverApp.Stop (Seconds (simulationTime + 1));
          // add a second sink

          BulkSendHelper echoClientHelper ("ns3::TcpSocketFactory",InetSocketAddress (apNodeInterface.GetAddress (0), port1));
          ApplicationContainer clientApp = echoClientHelper.Install(wifiStaNode.Get (0));
          clientApp.Start (Seconds (1.0));
          clientApp.Stop (Seconds (simulationTime + 1));

          // add a second data source
          if(scenario == 2) {
            BulkSendHelper echoClientHelper ("ns3::TcpSocketFactory",InetSocketAddress (apNodeInterface.GetAddress (0), port2));
            ApplicationContainer clientApp = echoClientHelper.Install(wifiStaNode.Get (1));
            clientApp.Start (Seconds (1.0));
            clientApp.Stop (Seconds (simulationTime + 1));
          } else if (scenario == 3) {
            BulkSendHelper echoClientHelper ("ns3::TcpSocketFactory",InetSocketAddress (apNodeInterface.GetAddress (0), port2));
            ApplicationContainer clientApp = echoClientHelper.Install(wifiStaNode.Get (1));
            clientApp.Start (Seconds (1.0));
            clientApp.Stop (Seconds (simulationTime + 1));
            BulkSendHelper echoClientHelper3 ("ns3::TcpSocketFactory",InetSocketAddress (apNodeInterface.GetAddress (0), port3));
            clientApp = echoClientHelper3.Install(wifiStaNode.Get (2));
            clientApp.Start (Seconds (1.0));
            clientApp.Stop (Seconds (simulationTime + 1));
          }

          Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

          Simulator::Stop (Seconds (simulationTime + 1));
          //phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
          //phy.EnablePcap("AP_capture_s"+std::to_string(i)+"_d1"+std::to_string(d)+"_d2"+std::to_string(d2), wifiApNode,false);
          //phy.EnablePcap("STA_capture_s"+std::to_string(i)+"_d1"+std::to_string(d)+"_d2"+std::to_string(d2), wifiStaNode,false);

          Simulator::Run ();

          double throughput = 0;
          double throughput2 {0}, byteSent2 {0}, throughput3 {0}, byteSent3 {0};

          //TCP
          uint64_t totalPacketsThrough = DynamicCast<PacketSink> (serverApp.Get (0))->GetTotalRx ();
          throughput = totalPacketsThrough * 8 / (simulationTime * 1000000.0); //Mbit/s

          std::cout << "Th1:" << throughput << " Mbit/s" << std::endl;

          if (scenario >= 2) {
            byteSent2 = DynamicCast<PacketSink> (serverApp.Get (1))->GetTotalRx ();
            throughput2 = byteSent2 * 8 / (simulationTime * 1000000.0); //Mbit/s
            std::cout << "Th2:" << throughput2 << " Mbit/s" << std::endl;
          }
          if (scenario >= 3) {
            byteSent3 = DynamicCast<PacketSink> (serverApp.Get (2))->GetTotalRx ();
            throughput3 = byteSent3 * 8 / (simulationTime * 1000000.0); //Mbit/s
            std::cout << "Th3:" << throughput3 << " Mbit/s" << std::endl;
          }

          outCSV <<d<<","<<d2<<",NS,"<<i<<","<<simulationTime<<","<<throughput*1000000.0<<","<<DynamicCast<PacketSink> (serverApp.Get (0))->GetTotalRx ()<<","<<throughput2*1000000.0<<","<<byteSent2<<","<<throughput3*1000000.0<<","<<byteSent3<<"\n";
          scenario == 1 ? d2 = 100 : d2 +=step;

          Simulator::Destroy ();
        }
        d += step;
      }
    }
    outCSV.close();

  file.close ();

  return 0;
}

