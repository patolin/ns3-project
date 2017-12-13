/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/pointer.h"
#include "ns3/ibr-helper.h"
/*
#include "ns3/ibr-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
*/
namespace ns3 {
IbrRoutingHelper::IbrRoutingHelper ()
 : Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::IbrRoutingProtocol");
}

IbrRoutingHelper* 
IbrRoutingHelper::Copy (void) const 
{
  return new IbrRoutingHelper (*this);
}

Ptr<Ipv4RoutingProtocol> IbrRoutingHelper::Create (Ptr<Node> node) const
{
  Ptr<IbrRoutingProtocol> agent = m_agentFactory.Create<IbrRoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void 
IbrRoutingHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}



} // namespace ns3
