#ifndef MODULES_HPP
#define MODULES_HPP

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/node-list.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/flow-monitor-module.h"
#include <memory>
#include "ns3/netanim-module.h" // To generate NetAnim xml file
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"

using namespace ns3;


// ---------- Global Variables ---------
extern std::string mcs;
extern ApplicationContainer Sinks;
extern bool pcapCapture;
extern int simTime;
extern int seed;
extern uint32_t nNode;
extern uint32_t nNodePair;
extern uint32_t dataSize;
// replacing Sinks in case of scenarios 4 and 5
extern ApplicationContainer SinksAP;
extern ApplicationContainer SinksSTA;

// energy measurement
extern DeviceEnergyModelContainer consumedEnergyAP;
extern DeviceEnergyModelContainer consumedEnergySTA;

// ---------- Callbacks ---------
void ApRxTrace (std::string context, Ptr<const Packet> p);


// ---------- Typedef ----------
typedef std::pair<NetDeviceContainer,NetDeviceContainer> WifiNetDev;

// ---------- platform.cc ----------
void buildT1(short nNodePair, short flowConf, int dataSize, double radius);
void buildT2(short nNode, short flowConf, int dataSize, double radius);
void buildT3(short nNodePair, short flowConf, int dataSize, double radius);
void buildT4(int nSTAInFirstRange, int nSTAInSecondRange, double radius, double radius2, int flowConf);
#endif
