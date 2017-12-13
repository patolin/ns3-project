/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "routing-table-header.h"


namespace ns3 {


    /*
     * RoutingTableHeader
     */
    NS_OBJECT_ENSURE_REGISTERED(RoutingTableHeader);

    RoutingTableHeader::RoutingTableHeader() {
    }

    TypeId RoutingTableHeader::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::RoutingTableHeader").SetParent<Header> ().AddConstructor<RoutingTableHeader> ();
        return tid;
    }

    TypeId RoutingTableHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    void RoutingTableHeader::Print(std::ostream & os) const {
        os << "NumRoutes: " << m_routingTable.size();
    }

    uint32_t RoutingTableHeader::GetSerializedSize() const {
        uint32_t serializedSize = 4 + static_cast<uint32_t> (m_routingTable.size()) * 16;
        
        return serializedSize;
    }

    void RoutingTableHeader::Serialize(Buffer::Iterator start) const {

        Buffer::Iterator iter = start;
        
        iter.WriteHtonU32(static_cast<uint32_t> (m_routingTable.size()));

        for (std::list<RouteEntry>::const_iterator i = m_routingTable.begin(); i != m_routingTable.end(); ++i) {
            iter.WriteHtonU32(i->dest.Get()); // Destination
            iter.WriteHtonU32(i->nextHop.Get()); // Next Hop
            iter.WriteHtonU32(i->interface); // Interface
            iter.WriteHtonU32(i->metric); // Metric
        }

    }

    uint32_t RoutingTableHeader::Deserialize(Buffer::Iterator start) {

        Buffer::Iterator iter = start;



        uint32_t numBackUp = iter.ReadNtohU32();

        for (int i = 0; i < (int)numBackUp; i++) {
            RouteEntry routeEntry;

            routeEntry.dest.Set(iter.ReadNtohU32()); // Destination
            routeEntry.nextHop.Set(iter.ReadNtohU32()); // Next Hop
            routeEntry.interface = iter.ReadNtohU32(); // Interface
            routeEntry.metric = iter.ReadNtohU32(); // Metric
            
            m_routingTable.push_back(routeEntry);
        }

        return GetSerializedSize();
    }

    std::list<RoutingTableHeader::RouteEntry> RoutingTableHeader::GetRoutingTable() {
        return m_routingTable;
    }

    void RoutingTableHeader::SetRoutingTable(std::list<RoutingTableHeader::RouteEntry> routingTable) {
        m_routingTable = routingTable;
    }

    void RoutingTableHeader::AddRouteToTable(RouteEntry route) {
        m_routingTable.push_back(route);
    }

    std::ostream & operator<<(std::ostream & os, const RoutingTableHeader & h) {
        h.Print(os);
        return os;
    }

}

