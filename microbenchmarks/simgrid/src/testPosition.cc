
#include "simgrid/plugins/position.h"
#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(test_wifi, "Wifi position demo");

static void sender()
{
  //const auto* l = simgrid::s4u::Link::by_name("AP1");
  //std::vector<double> v{-1,100};
  //sg_wifi_link_positions_set_snr_dist_vect(l, v);
  // start sending after 5 seconds
  simgrid::s4u::this_actor::sleep_until(5);

  std::string mbName = "MailBoxRCV";
  simgrid::s4u::Mailbox *dst = simgrid::s4u::Mailbox::by_name(mbName);

  int size = 6750000;

  XBT_INFO("SENDING 1 msg of size %d to %s", size, mbName.c_str());
  static std::string message = "message";
  dst->put(&message, size);
  XBT_INFO("finished sending");
}

static void receiver()
{
  std::string mbName = "MailBoxRCV";
  XBT_INFO("RECEIVING on mb %s", mbName.c_str());
  simgrid::s4u::Mailbox *myBox = simgrid::s4u::Mailbox::by_name(mbName);
  myBox->get<std::string>();

  XBT_INFO("received all messages");
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);

  sg_wifi_link_positions_plugin_init();
  engine.load_platform(argv[1]);

  // setup WiFi bandwidths
  const auto* l = engine.link_by_name("AP1");
  std::vector<double> v{10,100};
  sg_wifi_link_positions_set_host_position(l, engine.host_by_name("Station 1"), 10,10,10);
  l->set_host_wifi_rate(engine.host_by_name("Station 1"), 0);
  l->set_host_wifi_rate(engine.host_by_name("Station 2"), 0);

  sg_wifi_link_positions_set_snr_dist_vect(l, v);

  // create the two actors for the test
  simgrid::s4u::Actor::create("act0", engine.host_by_name("Station 1"), sender);
  simgrid::s4u::Actor::create("act1", engine.host_by_name("Station 2"), receiver);

  engine.run();

  return 0;
}