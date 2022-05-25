/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/log.h"
#include "simgrid/msg.h"
#include "src/surf/network_cm02.hpp"
#include "src/surf/network_wifi.hpp"
#include <simgrid/s4u/Link.hpp>
#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(simulator, "[wifi_usage] 1STA-1LINK-1NODE-CT");

size_t SEND_SIZE = 5500000.*100;
double STOP_TIME = 30;
double START_TIME = 0;


void setup_simulation_1(double dist);
/**
 * @brief Setup the simulation flows
 * @param topo Topology id
 * @param dataSize Amount of data to send in bytes
 * @param flowConf The flow configuration
 */
void setup_simulation_2(double dist1, double dist2, int nbRate1, int nbRate2);
/**
 * @brief Function used as a sending actor
 */
static void send(std::vector<std::string> args);
/**
 * @brief Function used as a receiving actor
 */
static void rcv(std::vector<std::string> args);
/**
 * @brief Set rate of a station on a wifi link
 * @param ap_name Name of the access point (WIFI link)
 * @param node_name Name of the station you want to set the rate
 * @param rate The rate id
 */
void set_rate(std::string ap_name, std::string node_name, short rate);

void dist_to_rate(double dist, std::string ap_name, std::string node_name);

std::vector<simgrid::s4u::CommPtr> recPtr;
std::vector<simgrid::s4u::CommPtr> sndPtr;
void stoper(double timeToStop) {

  simgrid::s4u::this_actor::sleep_until(timeToStop);
  int i=1; // used to separate flows while parsing
  for (simgrid::s4u::CommPtr v : recPtr){
    //XBT_INFO("End %lf left of %zu, done = %lf", v->get_remaining(), SEND_SIZE, SEND_SIZE-v->get_remaining());
    XBT_INFO("%d Sent: %lf Rate: %lf", i, (SEND_SIZE-v->get_remaining()),8*((SEND_SIZE-v->get_remaining())/(STOP_TIME-START_TIME)));
    i++;
    //XBT_INFO("BW=%f", 8*(SEND_SIZE-v->get_remaining())/(STOP_TIME));
  }
  simgrid::s4u::Actor::kill_all();
}

/**
 * Usage ./simulator <platformFile> <topo> <dataSize> <flowConf> <useDecayModel>
 * All arguments are mandatory.
 */
int main(int argc, char **argv) {

  // Build engine
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);

  // Parse arguments
  if(std::stoi(argv[2])==1) { // calibration scenario (1STA with variable dist)
    double dist = std::stod(argv[3]);
    setup_simulation_1(dist);

  }else { // 2 ranges and variable nb of nodes
    double dist1=std::stod(argv[3]);
    double dist2=std::stod(argv[4]);
    int nbSTA1=std::stoi(argv[5]);
    int nbSTA2=std::stoi(argv[6]);
    STOP_TIME = std::stoi(argv[7]);
    XBT_INFO("topo=4 dataSize=%d flowConf=1",SEND_SIZE);

    // Setup/Run simulation
    setup_simulation_2(dist1, dist2, nbSTA1, nbSTA2);
  }


  simgrid::s4u::Actor::create("stopper", engine.get_all_hosts().at(0),
                              stoper, STOP_TIME);
  engine.run();
  XBT_INFO("Simulation took %f s", simgrid::s4u::Engine::get_clock());

  return (0);
}

void setup_simulation_1(double dist) {

  // set rate of the sta
  dist_to_rate(dist, "AP1", "STA0");

  // create actors
  std::vector<std::string> args_STA;
  args_STA.push_back("router");
  args_STA.push_back(std::to_string(SEND_SIZE));
  simgrid::s4u::Actor::create("STA0_sender", simgrid::s4u::Host::by_name("STA0"),
                              send, args_STA);

  std::vector<std::string> noArgs;
  noArgs.push_back(std::to_string(SEND_SIZE));
  simgrid::s4u::Actor::create("router_rcv", simgrid::s4u::Host::by_name("router"),
                            rcv, noArgs);
}

void setup_simulation_2(double dist1, double dist2, int nbRate1, int nbRate2) {
  auto dataSize=SEND_SIZE;//2147483647;
  std::vector<std::string> noArgs;
  noArgs.push_back(std::to_string(dataSize));

  int nHost=sg_host_count();
  int nbSTApositionned=0;

  for(int i=0;i<nHost-1;i+=1){ // -1 because of the router
    std::ostringstream ss;
    ss<<"STA"<<i;
    std::string STA=ss.str();
    std::vector<std::string> args_STA;
    args_STA.push_back("router");
    args_STA.push_back(std::to_string(dataSize));

    simgrid::s4u::Actor::create(STA, simgrid::s4u::Host::by_name(STA),
                                send, args_STA);

    if (nbSTApositionned<nbRate1) {
      XBT_INFO("Set dist %lf for %s", dist1, STA.c_str());
      dist_to_rate(dist1, "AP1", STA);
    } else if (nbSTApositionned < nbRate1+nbRate2) {
      XBT_INFO("Set dist %lf for %s", dist2, STA.c_str());
      //set_rate("AP1", STA, 1);
      dist_to_rate(dist2, "AP1", STA);
    } else {
      XBT_ERROR("problem with nb of nodes in each zone");
    }
    nbSTApositionned++;
    // One receiver for each flow
    simgrid::s4u::Actor::create("outNode"+std::to_string(i), simgrid::s4u::Host::by_name("router"),
                                rcv, noArgs);
  }


}

void set_rate(std::string ap_name, std::string node_name, short rate) {
  simgrid::kernel::resource::NetworkWifiLink *l =
      (simgrid::kernel::resource::NetworkWifiLink *)simgrid::s4u::Link::by_name(
          ap_name)
          ->get_impl();

  // melange link et wifilink degueu
  simgrid::s4u::Link *ll = simgrid::s4u::Link::by_name(ap_name);
  ll->set_host_wifi_rate(simgrid::s4u::Host::by_name(node_name), rate);

}

void dist_to_rate(double dist, std::string ap_name, std::string node_name) {
  /*
   * based on the values measured in ns3:
   *  d <= 40 ~= 21 Mbit/s
   *  40 < d < 50 ~= 7.5 Mbits/s
   *  50 < d ~= 0 Mbits/s (too far away to receive the signal)
   */
  if (dist <= 51.4)
    set_rate(ap_name, node_name, 0);
  //else if (dist < 50)
  //  set_rate(ap_name, node_name, 1);
  else
    set_rate(ap_name, node_name, 2);
}

static void rcv(std::vector<std::string> args) {
  int dataSize = std::atoi(args.at(0).c_str());
  std::string selfName = simgrid::s4u::this_actor::get_host()->get_name();
  simgrid::s4u::Mailbox *selfMailbox = simgrid::s4u::Mailbox::by_name(
    simgrid::s4u::this_actor::get_host()->get_name());

  std::string* msg_content;
  recPtr.push_back(selfMailbox->get_async(&msg_content));

  recPtr.at(recPtr.size()-1)->wait();
}

static void send(std::vector<std::string> args) {
  std::string selfName = simgrid::s4u::this_actor::get_host()->get_name();
  simgrid::s4u::Mailbox *selfMailbox = simgrid::s4u::Mailbox::by_name(
      simgrid::s4u::this_actor::get_host()->get_name());

  simgrid::s4u::Mailbox *dstMailbox =
    simgrid::s4u::Mailbox::by_name(args.at(0));

  int dataSize = std::atoi(args.at(1).c_str());

  simgrid::s4u::this_actor::sleep_until(START_TIME);

  double comStartTime = simgrid::s4u::Engine::get_clock();


  std::string msg_content = std::string("Message ");
  auto* payload = new std::string(msg_content);

  sndPtr.push_back(dstMailbox->put_async(payload, dataSize));
  sndPtr.at(sndPtr.size()-1)->wait();

  double comEndTime = simgrid::s4u::Engine::get_clock();
  XBT_INFO("%s sent %d bytes to %s in %f seconds from %f to %f",
           selfName.c_str(), dataSize, args.at(0).c_str(),
           comEndTime - comStartTime, comStartTime, comEndTime);
}
