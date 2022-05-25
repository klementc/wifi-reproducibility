/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/host.h"
#include "simgrid/kernel/routing/NetZoneImpl.hpp" // full type for NetZoneImpl object
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/zone.h"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/include/surf/surf.hpp"
#include "xbt/config.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/include/catch.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

static const char* string_action(simgrid::kernel::resource::Action::State state)
{
  switch (state) {
    case simgrid::kernel::resource::Action::State::INITED:
      return "SURF_ACTION_INITED";
    case simgrid::kernel::resource::Action::State::STARTED:
      return "SURF_ACTION_RUNNING";
    case simgrid::kernel::resource::Action::State::FAILED:
      return "SURF_ACTION_FAILED";
    case simgrid::kernel::resource::Action::State::FINISHED:
      return "SURF_ACTION_DONE";
    case simgrid::kernel::resource::Action::State::IGNORED:
      return "SURF_ACTION_IGNORED";
    default:
      return "INVALID STATE";
  }
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  simgrid::s4u::Engine::set_config("network/model:CM02");

  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  e.load_platform(argv[1]);

  const_sg_netzone_t as_zone = e.netzone_by_name_or_null("world");
  auto net_model             = as_zone->get_impl()->get_network_model();

  XBT_DEBUG("Network model: %p", net_model.get());
  simgrid::s4u::Host* hostA = sg_host_by_name("STA0");
  simgrid::s4u::Host* hostB = sg_host_by_name("STA1");
  simgrid::s4u::Host* hostAP = sg_host_by_name("router");

  simgrid::s4u::Link* link = sg_link_by_name("1");

  /* Let's do something on it */
  simgrid::kernel::resource::Action* actionA = net_model->communicate(hostA, hostAP, 100e7, link->get_bandwidth());
  simgrid::kernel::resource::Action* actionB = net_model->communicate(hostB, hostAP, 100e7, link->get_bandwidth());

  //actionA->set_sharing_penalty(2);
  //actionB->set_sharing_penalty(1);

  surf_solve(-1.0);
  do {
    XBT_INFO("Next Event : %g", simgrid::s4u::Engine::get_clock());

    simgrid::kernel::resource::Action::StateSet* action_list;

    action_list = net_model->get_failed_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   Network Failed action");
      XBT_DEBUG("\t * Failed : %p", &action);
      action.unref();
    }

    action_list = net_model->get_finished_action_set();
    while (not action_list->empty()) {
      simgrid::kernel::resource::Action& action = action_list->front();
      XBT_INFO("   Network Done action");
      XBT_DEBUG("\t * Done : %p", &action);
      action.unref();
    }
  } while ((net_model->get_started_action_set()->size()) &&
           surf_solve(-1.0) >= 0.0);

  XBT_DEBUG("Simulation Terminated");

  return 0;
}
