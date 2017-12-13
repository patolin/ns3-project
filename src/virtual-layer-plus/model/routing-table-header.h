/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef ROUTING_TABLE_HEADER_H_
#define ROUTING_TABLE_HEADER_H_

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/mac48-address.h"

namespace ns3 {

    class RoutingTableHeader : public Header {
    public:

        struct RouteEntry {
            Ipv4Address dest;
            Ipv4Address nextHop;
            uint32_t interface;
            uint32_t metric;
            
            RouteEntry()
            {
            }
            
            RouteEntry(Ipv4Address dst, Ipv4Address nxt, uint32_t ifc, uint32_t met):
            dest(dst),
            nextHop(nxt),
            interface(ifc),
            metric(met)
            {
            }
            
            RouteEntry(Ipv4Address dst, Ipv4Address nxt, uint32_t ifc):
            dest(dst),
            nextHop(nxt),
            interface(ifc),
            metric(0)
            {
            }
            
        };
        
        RoutingTableHeader(void);
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        /**
         * \brief Return the instance type identifier.
         * \return instance type ID
         */
        virtual TypeId GetInstanceTypeId(void) const;

        virtual void Print(std::ostream& os) const;

        /**
         * \brief Get the serialized size of the packet.
         * \return size
         */
        virtual uint32_t GetSerializedSize(void) const;

        /**
         * \brief Serialize the packet.
         * \param start Buffer iterator
         */
        virtual void Serialize(Buffer::Iterator start) const;

        /**
         * \brief Deserialize the packet.
         * \param start Buffer iterator
         * \return size of the packet
         */
        virtual uint32_t Deserialize(Buffer::Iterator start);

        std::list<RouteEntry> GetRoutingTable ();
        void SetRoutingTable(std::list<RouteEntry> routingTable);
        void AddRouteToTable(RouteEntry route);

    private:
        std::list<RouteEntry> m_routingTable;

    };

    /**
     * \brief Stream insertion operator.
     *
     * \param os the reference to the output stream
     * \param header the VirtualLayerPlusHeader
     * \returns the reference to the output stream
     */
    std::ostream & operator<<(std::ostream & os, RoutingTableHeader const &header);

}

#endif /* ROUTING_TABLE_HEADER_H_ */
