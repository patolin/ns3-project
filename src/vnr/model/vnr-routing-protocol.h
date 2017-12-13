#ifndef VNR_ROUTING_PROTOCOL_H
#define VNR_ROUTING_PROTOCOL_H


#include <list>
#include <utility>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include <ns3/ipv4-routing-table-entry.h>
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/virtual-layer-plus-module.h"
#include "vnr-dpd.h"
#include <map>

namespace ns3 {
    namespace vnr {

        /**
         * \ingroup vnr
         * 
         * \brief VNR routing protocol
         */

        class RoutingProtocol : public Ipv4RoutingProtocol {
        public:
            /**
             * \brief The interface Id associated with this class.
             * \return type identifier
             */
            static TypeId GetTypeId(void);
            static const uint32_t VNR_PORT;

            /**
             * Assign a fixed random variable stream number to the random variables
             * used by this model.  Return the number of streams (possibly zero) that
             * have been assigned.
             *
             * \param stream first stream index to use
             * \return the number of stream indices assigned by this model
             */
            int64_t AssignStreams(int64_t stream);

            RoutingProtocol();
            virtual ~RoutingProtocol();

            void SetNode(Ptr<Node> node);

            virtual Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

            virtual bool RouteInput(Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                    UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                    LocalDeliverCallback lcb, ErrorCallback ecb);

            virtual void NotifyInterfaceUp(uint32_t interface);
            virtual void NotifyInterfaceDown(uint32_t interface);
            virtual void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address);
            virtual void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address);
            virtual void SetIpv4(Ptr<Ipv4> ipv4);
            virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const;

            /**
             * \brief Add a network route to the static routing table.
             *
             * \param network The Ipv4Address network for this route.
             * \param networkMask The Ipv4Mask to extract the network.
             * \param nextHop The next hop in the route to the destination network.
             * \param interface The network interface index used to send packets to the
             * destination.
             * \param metric Metric of route in case of multiple routes to same destination
             *
             * \see Ipv4Address
             */
            void AddNetworkRouteTo(Ipv4Address network,
                    Ipv4Mask networkMask,
                    Ipv4Address nextHop,
                    uint32_t interface,
                    uint32_t metric = 0);

            /**
             * \brief Add a network route to the static routing table.
             *
             * \param network The Ipv4Address network for this route.
             * \param networkMask The Ipv4Mask to extract the network.
             * \param interface The network interface index used to send packets to the
             * destination.
             * \param metric Metric of route in case of multiple routes to same destination
             *
             * \see Ipv4Address
             */
            void AddNetworkRouteTo(Ipv4Address network,
                    Ipv4Mask networkMask,
                    uint32_t interface,
                    uint32_t metric = 0);

            /**
             * \brief Add a host route to the static routing table.
             *
             * \param dest The Ipv4Address destination for this route.
             * \param nextHop The Ipv4Address of the next hop in the route.
             * \param interface The network interface index used to send packets to the
             * destination.
             * \param metric Metric of route in case of multiple routes to same destination
             *
             * \see Ipv4Address
             */
            void AddHostRouteTo(Ipv4Address dest,
                    Ipv4Address nextHop,
                    uint32_t interface,
                    uint32_t metric = 0);
            /**
             * \brief Add a host route to the static routing table.
             *
             * \param dest The Ipv4Address destination for this route.
             * \param interface The network interface index used to send packets to the
             * destination.
             * \param metric Metric of route in case of multiple routes to same destination
             *
             * \see Ipv4Address
             */
            void AddHostRouteTo(Ipv4Address dest,
                    uint32_t interface,
                    uint32_t metric = 0);
            /**
             * \brief Add a default route to the static routing table.
             *
             * This method tells the routing system what to do in the case where a specific
             * route to a destination is not found.  The system forwards packets to the
             * specified node in the hope that it knows better how to route the packet.
             * 
             * If the default route is set, it is returned as the selected route from 
             * LookupStatic irrespective of destination address if no specific route is
             * found.
             *
             * \param nextHop The Ipv4Address to send packets to in the hope that they
             * will be forwarded correctly.
             * \param interface The network interface index used to send packets.
             * \param metric Metric of route in case of multiple routes to same destination
             *
             * \see Ipv4Address
             * \see Ipv4StaticRouting::Lookup
             */
            void SetDefaultRoute(Ipv4Address nextHop,
                    uint32_t interface,
                    uint32_t metric = 0);

            /**
             * \brief Get the number of individual unicast routes that have been added
             * to the routing table.
             *
             * \warning The default route counts as one of the routes.
             * \return number of entries
             */
            uint32_t GetNRoutes(void) const;

            /**
             * \brief Get the default route with lowest metric from the static routing table.
             *
             * \return If the default route is set, a pointer to that Ipv4RoutingTableEntry is
             * returned, otherwise an empty routing table entry is returned. 
             *  If multiple default routes exist, the one with lowest metric is returned.
             *
             * \see Ipv4RoutingTableEntry
             */
            Ipv4RoutingTableEntry GetDefaultRoute(void);

            /**
             * \brief Get a route from the static unicast routing table.
             *
             * Externally, the unicast static routing table appears simply as a table with
             * n entries.
             *
             * \param i The index (into the routing table) of the route to retrieve.
             * \return If route is set, a pointer to that Ipv4RoutingTableEntry is returned, otherwise
             * a zero pointer is returned.
             *
             * \see Ipv4RoutingTableEntry
             * \see Ipv4StaticRouting::RemoveRoute
             */
            Ipv4RoutingTableEntry GetRoute(uint32_t i) const;

            /**
             * \brief Get a metric for route from the static unicast routing table.
             *
             * \param index The index (into the routing table) of the route to retrieve.
             * \return If route is set, the metric is returned. If not, an infinity metric (0xffffffff) is returned
             *
             */
            uint32_t GetMetric(uint32_t index) const;

            /**
             * \brief Remove a route from the static unicast routing table.
             *
             * Externally, the unicast static routing table appears simply as a table with
             * n entries.
             *
             * \param i The index (into the routing table) of the route to remove.
             *
             * \see Ipv4RoutingTableEntry
             * \see Ipv4StaticRouting::GetRoute
             * \see Ipv4StaticRouting::AddRoute
             */
            void RemoveRoute(uint32_t i);

            /**
             * \brief Add a multicast route to the static routing table.
             *
             * A multicast route must specify an origin IP address, a multicast group and
             * an input network interface index as conditions and provide a vector of
             * output network interface indices over which packets matching the conditions
             * are sent.
             *
             * Typically there are two main types of multicast routes:  routes of the 
             * first kind are used during forwarding.  All of the conditions must be
             * explicitly provided.  The second kind of routes are used to get packets off
             * of a local node.  The difference is in the input interface.  Routes for
             * forwarding will always have an explicit input interface specified.  Routes
             * off of a node will always set the input interface to a wildcard specified
             * by the index Ipv4RoutingProtocol::INTERFACE_ANY.
             *
             * For routes off of a local node wildcards may be used in the origin and
             * multicast group addresses.  The wildcard used for Ipv4Adresses is that 
             * address returned by Ipv4Address::GetAny () -- typically "0.0.0.0".  Usage
             * of a wildcard allows one to specify default behavior to varying degrees.
             *
             * For example, making the origin address a wildcard, but leaving the 
             * multicast group specific allows one (in the case of a node with multiple
             * interfaces) to create different routes using different output interfaces
             * for each multicast group.
             *
             * If the origin and multicast addresses are made wildcards, you have created
             * essentially a default multicast address that can forward to multiple 
             * interfaces.  Compare this to the actual default multicast address that is
             * limited to specifying a single output interface for compatibility with
             * existing functionality in other systems.
             * 
             * \param origin The Ipv4Address of the origin of packets for this route.  May
             * be Ipv4Address:GetAny for open groups.
             * \param group The Ipv4Address of the multicast group or this route.
             * \param inputInterface The input network interface index over which to 
             * expect packets destined for this route.  May be
             * Ipv4RoutingProtocol::INTERFACE_ANY for packets of local origin.
             * \param outputInterfaces A vector of network interface indices used to specify
             * how to send packets to the destination(s).
             *
             * \see Ipv4Address
             */
            void AddMulticastRoute(Ipv4Address origin,
                    Ipv4Address group,
                    uint32_t inputInterface,
                    std::vector<uint32_t> outputInterfaces);

            /**
             * \brief Add a default multicast route to the static routing table.
             *
             * This is the multicast equivalent of the unicast version SetDefaultRoute.
             * We tell the routing system what to do in the case where a specific route
             * to a destination multicast group is not found.  The system forwards 
             * packets out the specified interface in the hope that "something out there"
             * knows better how to route the packet.  This method is only used in 
             * initially sending packets off of a host.  The default multicast route is
             * not consulted during forwarding -- exact routes must be specified using
             * AddMulticastRoute for that case.
             *
             * Since we're basically sending packets to some entity we think may know
             * better what to do, we don't pay attention to "subtleties" like origin
             * address, nor do we worry about forwarding out multiple  interfaces.  If the
             * default multicast route is set, it is returned as the selected route from 
             * LookupStatic irrespective of origin or multicast group if another specific
             * route is not found.
             *
             * \param outputInterface The network interface index used to specify where
             * to send packets in the case of unknown routes.
             *
             * \see Ipv4Address
             */
            void SetDefaultMulticastRoute(uint32_t outputInterface);

            /**
             * \brief Get the number of individual multicast routes that have been added
             * to the routing table.
             *
             * \warning The default multicast route counts as one of the routes.
             * \return number of entries
             */
            uint32_t GetNMulticastRoutes(void) const;

            /**
             * \brief Get a route from the static multicast routing table.
             *
             * Externally, the multicast static routing table appears simply as a table 
             * with n entries.
             * 
             * \param i The index (into the routing table) of the multicast route to
             * retrieve.
             * \return If route \e i is set, a pointer to that Ipv4MulticastRoutingTableEntry is
             * returned, otherwise a zero pointer is returned.
             *
             * \see Ipv4MulticastRoutingTableEntry
             * \see Ipv4StaticRouting::RemoveRoute
             */
            Ipv4MulticastRoutingTableEntry GetMulticastRoute(uint32_t i) const;

            /**
             * \brief Remove a route from the static multicast routing table.
             *
             * Externally, the multicast static routing table appears simply as a table 
             * with n entries.
             * This method causes the multicast routing table to be searched for the first
             * route that matches the parameters and removes it.
             *
             * Wildcards may be provided to this function, but the wildcards are used to
             * exactly match wildcards in the routes (see AddMulticastRoute).  That is,
             * calling RemoveMulticastRoute with the origin set to "0.0.0.0" will not
             * remove routes with any address in the origin, but will only remove routes
             * with "0.0.0.0" set as the the origin.
             *
             * \param origin The IP address specified as the origin of packets for the
             * route.
             * \param group The IP address specified as the multicast group address of
             * the route.
             * \param inputInterface The network interface index specified as the expected
             * input interface for the route.
             * \returns true if a route was found and removed, false otherwise.
             *
             * \see Ipv4MulticastRoutingTableEntry
             * \see Ipv4StaticRouting::AddMulticastRoute
             */
            bool RemoveMulticastRoute(Ipv4Address origin,
                    Ipv4Address group,
                    uint32_t inputInterface);

            /**
             * \brief Remove a route from the static multicast routing table.
             *
             * Externally, the multicast static routing table appears simply as a table 
             * with n entries.
             *
             * \param index The index (into the multicast routing table) of the route to
             * remove.
             *
             * \see Ipv4RoutingTableEntry
             * \see Ipv4StaticRouting::GetRoute
             * \see Ipv4StaticRouting::AddRoute
             */
            void RemoveMulticastRoute(uint32_t index);

        protected:
            virtual void DoDispose(void);

        private:
            /// Container for the network routes
            typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> > NetworkRoutes;

            /// Const Iterator for container for the network routes
            typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> >::const_iterator NetworkRoutesCI;

            /// Iterator for container for the network routes
            typedef std::list<std::pair <Ipv4RoutingTableEntry *, uint32_t> >::iterator NetworkRoutesI;

            /// Container for the multicast routes
            typedef std::list<Ipv4MulticastRoutingTableEntry *> MulticastRoutes;

            /// Const Iterator for container for the multicast routes
            typedef std::list<Ipv4MulticastRoutingTableEntry *>::const_iterator MulticastRoutesCI;

            /// Iterator for container for the multicast routes
            typedef std::list<Ipv4MulticastRoutingTableEntry *>::iterator MulticastRoutesI;


            /// Start protocol operation
            void Start();


            /**
             * \brief Lookup in the forwarding table for destination.
             * \param dest destination address
             * \param oif output interface if any (put 0 otherwise)
             * \return Ipv4Route to route the packet to reach dest address
             */
            Ptr<Ipv4Route> LookupStatic(Ipv4Address dest, Ptr<NetDevice> oif = 0);

            /**
             * \brief Lookup in the multicast forwarding table for destination.
             * \param origin source address
             * \param group group multicast address
             * \param interface interface index
             * \return Ipv4MulticastRoute to route the packet to reach dest address
             */
            Ptr<Ipv4MulticastRoute> LookupStatic(Ipv4Address origin, Ipv4Address group,
                    uint32_t interface);

            /**
             * \brief Choose the source address to use with destination address.
             * \param interface interface index
             * \param dest IPv4 destination address
             * \return IPv4 source address to use
             */
            Ipv4Address SourceAddressSelection(uint32_t interface, Ipv4Address dest);

            /// Create loopback route for given header
            Ptr<Ipv4Route> LoopbackRoute(const Ipv4Address dst, Ptr<NetDevice> oif) const;

            /**
             * \brief the forwarding table for network.
             */
            NetworkRoutes m_networkRoutes;

            /**
             * \brief the forwarding table for multicast.
             */
            MulticastRoutes m_multicastRoutes;

            /**
             * \brief Ipv4 reference.
             */
            Ptr<Ipv4> m_ipv4;

            Ptr<Node> m_node;

            Ptr<VirtualLayerPlusNetDevice> m_vnLayer;

            /// Loopback device
            Ptr<NetDevice> m_lo;

            /// Handle duplicated broadcast/multicast packets
            DuplicatePacketDetection m_dpd;

            /// Provides uniform random variables.
            Ptr<UniformRandomVariable> m_uniformRandomVariable;
        };

    } // Namespace ns3
}
#endif /* VNR_ROUTING_PROTOCOL_H */
