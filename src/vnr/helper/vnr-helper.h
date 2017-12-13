#ifndef VNR_HELPER_H
#define VNR_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/vnr-routing-protocol.h"

namespace ns3
{
/**
 * \ingroup vnr
 * \brief Helper class that adds VNR routing to nodes.
 */
class VnrHelper : public Ipv4RoutingHelper
{
public:
  VnrHelper();

  /**
   * \returns pointer to clone of this VnrHelper 
   * 
   * \internal
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  VnrHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   * 
   * \todo support installing VNR on the subset of all available IP interfaces
   */
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
  /**
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set.
   *
   * This method controls the attributes of ns3::vnr::RoutingProtocol
   */
  void Set (std::string name, const AttributeValue &value);
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.  The Install() method of the InternetStackHelper
   * should have previously been called by the user.
   *
   * \param stream first stream index to use
   * \param c NodeContainer of the set of nodes for which VNR
   *          should be modified to use a fixed stream
   * \return the number of stream indices assigned by this helper
   */
  int64_t AssignStreams (NodeContainer c, int64_t stream);
  
  /**
   * Try and find the virtual routing protocol as either the main routing
   * protocol or in the list of routing protocols associated with the 
   * Ipv4 provided.
   *
   * \param ipv4 the Ptr<Ipv4> to search for the virtual routing protocol
   * \returns RoutingProtocol pointer or 0 if not found
   */
  Ptr<vnr::RoutingProtocol> GetVirtualRouting (Ptr<Ipv4> ipv4) const;

  /**
   * \brief Add a multicast route to a node and net device using explicit 
   * Ptr<Node> and Ptr<NetDevice>
   */
  void AddMulticastRoute (Ptr<Node> n, Ipv4Address source, Ipv4Address group,
                          Ptr<NetDevice> input, NetDeviceContainer output);

  /**
   * \brief Add a multicast route to a node and device using a name string 
   * previously associated to the node using the Object Name Service and a
   * Ptr<NetDevice>
   */
  void AddMulticastRoute (std::string n, Ipv4Address source, Ipv4Address group,
                          Ptr<NetDevice> input, NetDeviceContainer output);

  /**
   * \brief Add a multicast route to a node and device using a Ptr<Node> and a 
   * name string previously associated to the device using the Object Name Service.
   */
  void AddMulticastRoute (Ptr<Node> n, Ipv4Address source, Ipv4Address group,
                          std::string inputName, NetDeviceContainer output);

  /**
   * \brief Add a multicast route to a node and device using name strings
   * previously associated to both the node and device using the Object Name 
   * Service.
   */
  void AddMulticastRoute (std::string nName, Ipv4Address source, Ipv4Address group,
                          std::string inputName, NetDeviceContainer output);

  /**
   * \brief Add a default route to the virtual routing protocol to forward
   *        packets out a particular interface
   *
   * Functionally equivalent to:
   * route add 224.0.0.0 netmask 240.0.0.0 dev nd
   * \param n node
   * \param nd device of the node to add default route
   */
  void SetDefaultMulticastRoute (Ptr<Node> n, Ptr<NetDevice> nd);

  /**
   * \brief Add a default route to the virtual routing protocol to forward
   *        packets out a particular interface
   *
   * Functionally equivalent to:
   * route add 224.0.0.0 netmask 240.0.0.0 dev nd
   * \param n node
   * \param ndName string with name previously associated to device using the 
   *        Object Name Service
   */
  void SetDefaultMulticastRoute (Ptr<Node> n, std::string ndName);

  /**
   * \brief Add a default route to the virtual routing protocol to forward
   *        packets out a particular interface
   *
   * Functionally equivalent to:
   * route add 224.0.0.0 netmask 240.0.0.0 dev nd
   * \param nName string with name previously associated to node using the 
   *        Object Name Service
   * \param nd device of the node to add default route
   */
  void SetDefaultMulticastRoute (std::string nName, Ptr<NetDevice> nd);

  /**
   * \brief Add a default route to the virtual routing protocol to forward
   *        packets out a particular interface
   *
   * Functionally equivalent to:
   * route add 224.0.0.0 netmask 240.0.0.0 dev nd
   * \param nName string with name previously associated to node using the 
   *        Object Name Service
   * \param ndName string with name previously associated to device using the 
   *        Object Name Service
   */
  void SetDefaultMulticastRoute (std::string nName, std::string ndName);

private:
  /** the factory to create VNR routing object */
  ObjectFactory m_agentFactory;
};

}

#endif /* VNR_HELPER_H */
