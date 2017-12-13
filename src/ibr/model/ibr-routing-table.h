// ibr routing table


#ifndef IBR_ROUTING_TABLE_H
#define IBR_ROUTING_TABLE_H

#include "ns3/node.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include <list>
#include <vector>

namespace ns3 {

class IbrRoutingTable : public Object
{
public:

  IbrRoutingTable ();
  virtual ~IbrRoutingTable ();

  static TypeId GetTypeId ();
  void AddRoute (Ipv4Address srcAddr, Ipv4Address relayAddr, Ipv4Address dstAddr);
  void AddNode (Ptr<Node> node, Ipv4Address addr);
  Ipv4Address LookupRoute (Ipv4Address srcAddr, Ipv4Address dstAddr);
  void UpdateRoute (double txRange);
  double GetDistance (Ipv4Address srcAddr, Ipv4Address dstAddr);

  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  typedef struct
    {
      Ipv4Address srcAddr;
      Ipv4Address relayAddr;
      Ipv4Address dstAddr;
    }
  SPtableEntry;
  
  typedef struct
    {
      Ptr<Node> node;
      Ipv4Address addr;
    } 
  SpNodeEntry;
  
  double DistFromTable (uint16_t i, uint16_t j);
  
  std::list<SPtableEntry> m_sptable;
  std::vector<SpNodeEntry> m_nodeTable;
  
  uint16_t* m_spNext;
  double*   m_spDist;
  
  double    m_txRange;
};

}

#endif
