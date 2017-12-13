/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef IBR_HELPER_H
#define IBR_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ibr-routing-protocol.h"
#include <map>
#include <set>

namespace ns3
{
class IbrRoutingHelper : public Ipv4RoutingHelper
{
public:
  IbrRoutingHelper();


  IbrRoutingHelper* Copy (void) const;

  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;

  void Set (std::string name, const AttributeValue &value);
  
private:
  ObjectFactory m_agentFactory;
};

}


#endif /* OLSR_HELPER_H */
