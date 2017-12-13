#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4 && m_ipv4->GetObject<Node> ()) { \
      std::clog << Simulator::Now ().GetSeconds () \
                << " [node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }


#include <iomanip>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-route.h"
#include "vnr-routing-protocol.h"
#include "src/core/model/log.h"
#include "src/virtual-layer-plus/model/virtual-layer-plus-net-device.h"

using std::make_pair;

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("VirtualNetworkRoutingProtocol");

    namespace vnr {

        NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);

        /// UDP Port for VNR control traffic
        const uint32_t RoutingProtocol::VNR_PORT = 6900;

        TypeId
        RoutingProtocol::GetTypeId(void) {
            static TypeId tid = TypeId("ns3::vnr::RoutingProtocol")
                    .SetParent<Ipv4RoutingProtocol> ()
                    .AddConstructor<RoutingProtocol> ()
                    ;
            return tid;
        }

        RoutingProtocol::RoutingProtocol()
        : m_ipv4(0),
        m_dpd(Seconds(5)) {
            NS_LOG_FUNCTION(this);
        }

        int64_t
        RoutingProtocol::AssignStreams(int64_t stream) {
            NS_LOG_FUNCTION(this << stream);
            m_uniformRandomVariable->SetStream(stream);
            return 1;
        }

        void RoutingProtocol::SetNode(Ptr<Node> node) {
            m_node = node;
        }

        void
        RoutingProtocol::Start() {

            m_vnLayer = 0;
            for (uint32_t i = 0; i < m_node->GetNDevices(); i++) {
                if (dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_node->GetDevice(i)))) {
                    NS_LOG_INFO("Virtual Layer found in the node " << m_node->GetId());

                    m_vnLayer = dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_node->GetDevice(i)));
                    break;
                }
            }

            NS_ASSERT_MSG(m_vnLayer, "Virtual Layer was not found in the node " << m_node->GetId());


        }

        void
        RoutingProtocol::AddNetworkRouteTo(Ipv4Address network,
                Ipv4Mask networkMask,
                Ipv4Address nextHop,
                uint32_t interface,
                uint32_t metric) {
            NS_LOG_FUNCTION(this << network << " " << networkMask << " " << nextHop << " " << interface << " " << metric);
            Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry();
            *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo(network,
                    networkMask,
                    nextHop,
                    interface);
            m_networkRoutes.push_back(make_pair(route, metric));
        }

        void
        RoutingProtocol::AddNetworkRouteTo(Ipv4Address network,
                Ipv4Mask networkMask,
                uint32_t interface,
                uint32_t metric) {
            NS_LOG_FUNCTION(this << network << " " << networkMask << " " << interface << " " << metric);
            Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry();
            *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo(network,
                    networkMask,
                    interface);
            m_networkRoutes.push_back(make_pair(route, metric));
        }

        void
        RoutingProtocol::AddHostRouteTo(Ipv4Address dest,
                Ipv4Address nextHop,
                uint32_t interface,
                uint32_t metric) {
            NS_LOG_FUNCTION(this << dest << " " << nextHop << " " << interface << " " << metric);
            AddNetworkRouteTo(dest, Ipv4Mask::GetOnes(), nextHop, interface, metric);
        }

        void
        RoutingProtocol::AddHostRouteTo(Ipv4Address dest,
                uint32_t interface,
                uint32_t metric) {
            NS_LOG_FUNCTION(this << dest << " " << interface << " " << metric);
            AddNetworkRouteTo(dest, Ipv4Mask::GetOnes(), interface, metric);
        }

        void
        RoutingProtocol::SetDefaultRoute(Ipv4Address nextHop,
                uint32_t interface,
                uint32_t metric) {
            NS_LOG_FUNCTION(this << nextHop << " " << interface << " " << metric);
            AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask::GetZero(), nextHop, interface, metric);
        }

        void
        RoutingProtocol::AddMulticastRoute(Ipv4Address origin,
                Ipv4Address group,
                uint32_t inputInterface,
                std::vector<uint32_t> outputInterfaces) {
            NS_LOG_FUNCTION(this << origin << " " << group << " " << inputInterface << " " << &outputInterfaces);
            Ipv4MulticastRoutingTableEntry *route = new Ipv4MulticastRoutingTableEntry();
            *route = Ipv4MulticastRoutingTableEntry::CreateMulticastRoute(origin, group,
                    inputInterface, outputInterfaces);
            m_multicastRoutes.push_back(route);
        }

        // default multicast routes are stored as a network route
        // these routes are _not_ consulted in the forwarding process-- only
        // for originating packets

        void
        RoutingProtocol::SetDefaultMulticastRoute(uint32_t outputInterface) {
            NS_LOG_FUNCTION(this << outputInterface);
            Ipv4RoutingTableEntry *route = new Ipv4RoutingTableEntry();
            Ipv4Address network = Ipv4Address("224.0.0.0");
            Ipv4Mask networkMask = Ipv4Mask("240.0.0.0");
            *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo(network,
                    networkMask,
                    outputInterface);
            m_networkRoutes.push_back(make_pair(route, 0));
        }

        uint32_t
        RoutingProtocol::GetNMulticastRoutes(void) const {
            NS_LOG_FUNCTION(this);
            return m_multicastRoutes.size();
        }

        Ipv4MulticastRoutingTableEntry
        RoutingProtocol::GetMulticastRoute(uint32_t index) const {
            NS_LOG_FUNCTION(this << index);
            NS_ASSERT_MSG(index < m_multicastRoutes.size(),
                    "Ipv4StaticRouting::GetMulticastRoute ():  Index out of range");

            if (index < m_multicastRoutes.size()) {
                uint32_t tmp = 0;
                for (MulticastRoutesCI i = m_multicastRoutes.begin();
                        i != m_multicastRoutes.end();
                        i++) {
                    if (tmp == index) {
                        return *i;
                    }
                    tmp++;
                }
            }
            return 0;
        }

        bool
        RoutingProtocol::RemoveMulticastRoute(Ipv4Address origin,
                Ipv4Address group,
                uint32_t inputInterface) {
            NS_LOG_FUNCTION(this << origin << " " << group << " " << inputInterface);
            for (MulticastRoutesI i = m_multicastRoutes.begin();
                    i != m_multicastRoutes.end();
                    i++) {
                Ipv4MulticastRoutingTableEntry *route = *i;
                if (origin == route->GetOrigin() &&
                        group == route->GetGroup() &&
                        inputInterface == route->GetInputInterface()) {
                    delete *i;
                    m_multicastRoutes.erase(i);
                    return true;
                }
            }
            return false;
        }

        void
        RoutingProtocol::RemoveMulticastRoute(uint32_t index) {
            NS_LOG_FUNCTION(this << index);
            uint32_t tmp = 0;
            for (MulticastRoutesI i = m_multicastRoutes.begin();
                    i != m_multicastRoutes.end();
                    i++) {
                if (tmp == index) {
                    delete *i;
                    m_multicastRoutes.erase(i);
                    return;
                }
                tmp++;
            }
        }

        Ptr<Ipv4Route>
        RoutingProtocol::LookupStatic(Ipv4Address dest, Ptr<NetDevice> oif) {
            NS_LOG_FUNCTION(this << dest << " " << oif);
            Ptr<Ipv4Route> rtentry = 0;
            uint16_t longest_mask = 0;
            uint32_t shortest_metric = 0xffffffff;
            /* when sending on local multicast, there have to be interface specified */
            if (dest.IsLocalMulticast()) {
                NS_ASSERT_MSG(oif, "Try to send on link-local multicast address, and no interface index is given!");

                rtentry = Create<Ipv4Route> ();
                rtentry->SetDestination(dest);
                rtentry->SetGateway(Ipv4Address::GetZero());
                rtentry->SetOutputDevice(oif);
                rtentry->SetSource(m_ipv4->GetAddress(oif->GetIfIndex(), 0).GetLocal());
                return rtentry;
            }

            // Virtual IP
            if (m_vnLayer->IsVirtual(dest)) {

                NS_LOG_INFO("Buscando ruta hacia la IP virtual " << dest << " (Región " << m_vnLayer->GetRegionIdOf(dest) << ")");

                m_vnLayer->PrintNeighborLeadersTable();

                if (m_vnLayer->GetRegionIdOf(dest) == m_vnLayer->GetRegion()) {
                    if ((m_vnLayer->GetState() == VirtualLayerPlusHeader::LEADER) || ((m_vnLayer->GetState() == VirtualLayerPlusHeader::INTERIM) && !m_vnLayer->IsReplaced())) {
                        NS_LOG_INFO("La región destino es mi propia región y yo actúo como líder de la misma --> El paquete es para mi");

                        return LoopbackRoute(dest, oif);

                    } else {
                        NS_LOG_INFO("La región destino es mi propia región --> Enviando paquete...");

                        rtentry = Create<Ipv4Route> ();
                        rtentry->SetDestination(dest);
                        rtentry->SetGateway(dest);
                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                        rtentry->SetSource(m_vnLayer->GetIp());
                        return rtentry;
                    }
                } else if (m_vnLayer->IsNeighborRegion(m_vnLayer->GetRegionIdOf(dest))) {

                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionIdOf(dest)) > 0) {

                        NS_LOG_INFO("La región destino es una región vecina con líder activo --> Enviando paquete...");
                        //Crear ruta hacia la IP virtual directamente

                        rtentry = Create<Ipv4Route> ();
                        rtentry->SetDestination(dest);
                        rtentry->SetGateway(dest);
                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                        rtentry->SetSource(m_vnLayer->GetIp());
                        return rtentry;

                    } else {
                        NS_LOG_INFO("La región destino es una región vecina sin un líder activo --> PAQUETE DESCARTADO");
                    }


                } else {

                    uint32_t nextRegion;

                    Vector2D arrowsColumns = m_vnLayer->GetCoordinateLimits();

                    Vector2D coordOrig = m_vnLayer->GetCoordinatesOfRegion(m_vnLayer->GetRegion());
                    Vector2D coordDest = m_vnLayer->GetCoordinatesOfRegion(m_vnLayer->GetRegionIdOf(dest));

                    Vector2D v = Vector2D(coordDest.x - coordOrig.x, coordDest.y - coordOrig.y);

                    if (coordOrig.x > 0) { // Tiene regiones vecinas debajo
                        if (coordOrig.x < (arrowsColumns.x - 1)) { // Tiene regiones vecinas encima
                            if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                                if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 8 regiones

                                    if (v.x == 0) { // Misma fila

                                        if (v.y > 0) { // Hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));
                                                //Crear ruta hacia la IP virtual de la región vecina de la derecha
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            }

                                        } else { // Hacia la izquierda
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            }
                                        }

                                    } else if (v.x > 0) { // Hacia arriba

                                        if (v.y == 0) { // Sólo hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else if (v.y > 0) { // Hacia arriba a la derecha
                                            if (v.x > v.y) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                }
                                            } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }
                                        } else { // Hacia arriba a la izquierda
                                            if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }
                                            } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }
                                        }


                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;



                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;


                                            }
                                        } else if (v.y > 0) { // Hacia abajo a la derecha

                                            if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }


                                            } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        } else { // Hacia abajo a la izquierda

                                            if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                }


                                            } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        }
                                    }



                                } else { // Columna N --> No tiene regiones vecinas a la derecha --> 5 regiones

                                    if (v.x == 0) { // Misma fila --> Hacia la izquierda

                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                            // Izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                            // Arriba a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                            // Abajo a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }


                                    } else if (v.x > 0) { // Hacia arriba

                                        if (v.y == 0) { // Sólo hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else { // Hacia arriba a la izquierda
                                            if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }
                                            } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }
                                        }


                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else { // Hacia abajo a la izquierda

                                            if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }


                                            } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        }
                                    }

                                }

                            } else { // Columna 0 --> No hay regiones vecinas a la izquierda --> 5 regiones vecinas

                                if (v.x == 0) { // Misma fila --> Hacia la derecha

                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                        // Derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                        // Arriba a la derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                        // Abajo a la derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    }


                                } else if (v.x > 0) { // Hacia arriba

                                    if (v.y == 0) { // Sólo hacia arriba
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de encima
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    } else { // Hacia arriba a la derecha
                                        if (v.x > v.y) { // Más hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }
                                    }


                                } else { // Hacia abajo
                                    if (v.y == 0) { // Sólo hacia abajo
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                            // Abajo

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de abajo
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                            // Abajo a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    } else { // Hacia abajo a la derecha

                                        if (((-1) * v.x) > v.y) { // Más hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }


                                        } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }

                                    }
                                }

                            }
                        } else { // Fila N --> No tiene regiones vecinas encima
                            if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                                if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 5 regiones

                                    if (v.x == 0) { // Misma fila

                                        if (v.y > 0) { // Hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Hacia la izquierda
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        }

                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else if (v.y > 0) { // Hacia abajo a la derecha

                                            if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }


                                            } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        } else { // Hacia abajo a la izquierda

                                            if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }


                                            } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        }
                                    }

                                } else { // Columna N --> No tiene regiones vecinas a la derecha --> 3 regiones

                                    if (v.x == 0) { // Misma fila --> Hacia la izquierda
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                            // Izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                            // Abajo a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }

                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }
                                        } else { // Hacia abajo a la izquierda

                                            if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }


                                            } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(dest);
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                    return rtentry;

                                                }

                                            }

                                        }
                                    }

                                }
                            } else { // Columna 0 --> No hay regiones a la izquierda --> 3 regiones vecinas

                                if (v.x == 0) { // Misma fila --> Hacia la derecha
                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                        // Derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                        // Abajo a la derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    }

                                } else { // Hacia abajo
                                    if (v.y == 0) { // Sólo hacia abajo
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                            // Abajo

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de abajo
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                            // Abajo a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    } else { // Hacia abajo a la derecha

                                        if (((-1) * v.x) > v.y) { // Más hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }


                                        } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;


                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;


                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }

                                    }
                                }

                            }
                        }

                    } else { // Fila 0 --> No hay regiones debajo
                        if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                            if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 5 regiones

                                if (v.x == 0) { // Misma fila

                                    if (v.y > 0) { // Hacia la derecha

                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }

                                    } else { // Hacia la izquierda
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                            // Izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                            // Arriba a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    }

                                } else { // Hacia arriba
                                    if (v.y == 0) { // Sólo hacia arriba
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de abajo
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                            // Arriba a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    } else if (v.y > 0) { // Hacia arriba a la derecha

                                        if (v.x > v.y) { // Más hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }


                                        } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }

                                    } else { // Hacia arriba a la izquierda

                                        if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }


                                        } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la izquierda

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;


                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }

                                    }
                                }

                            } else { // Columna N --> No tiene regiones vecinas a la derecha --> 3 regiones

                                if (v.x == 0) { // Misma fila --> Hacia la izquierda
                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                        // Izquierda
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                        // Arriba a la izquierda
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    }

                                } else { // Hacia arriba
                                    if (v.y == 0) { // Sólo hacia arriba
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de encima
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                            // Arriba a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }
                                    } else { // Hacia arriba a la izquierda

                                        if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }


                                        } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;


                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        } else { // Más hacia la izquierda

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(dest);
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                                return rtentry;

                                            }

                                        }

                                    }
                                }

                            }
                        } else { // Columna 0 --> No hay regiones a la izquierda --> 3 regiones vecinas

                            if (v.x == 0) { // Misma fila --> Hacia la derecha
                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                    // Derecha
                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                    rtentry = Create<Ipv4Route> ();
                                    rtentry->SetDestination(dest);
                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                    rtentry->SetSource(m_vnLayer->GetIp());
                                    return rtentry;

                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                    // Arriba a la derecha
                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                    rtentry = Create<Ipv4Route> ();
                                    rtentry->SetDestination(dest);
                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                    rtentry->SetSource(m_vnLayer->GetIp());
                                    return rtentry;

                                }

                            } else { // Hacia arriba
                                if (v.y == 0) { // Sólo hacia arriba
                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                        // Arriba

                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                        //Crear ruta hacia la IP virtual de la región vecina de arriba
                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                        // Arriba a la derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(dest);
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());
                                        return rtentry;

                                    }
                                } else { // Hacia arriba a la derecha

                                    if (v.x > v.y) { // Más hacia arriba
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de encima
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }


                                    } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción

                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;


                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }

                                    } else { // Más hacia la derecha

                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(dest);
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());
                                            return rtentry;

                                        }

                                    }

                                }
                            }

                        }
                    }
                }
                NS_LOG_INFO("No se pudo encontrar un nodo virtual hacia el que realizar el siguiente salto.");
            }


            for (NetworkRoutesI i = m_networkRoutes.begin();
                    i != m_networkRoutes.end();
                    i++) {
                Ipv4RoutingTableEntry *j = i->first;
                uint32_t metric = i->second;
                Ipv4Mask mask = (j)->GetDestNetworkMask();
                uint16_t masklen = mask.GetPrefixLength();
                Ipv4Address entry = (j)->GetDestNetwork();
                NS_LOG_LOGIC("Searching for route to " << dest << ", checking against route to " << entry << "/" << masklen);
                if (mask.IsMatch(dest, entry)) {
                    NS_LOG_LOGIC("Found global network route " << j << ", mask length " << masklen << ", metric " << metric);
                    if (oif != 0) {
                        if (oif != m_ipv4->GetNetDevice(j->GetInterface())) {
                            NS_LOG_LOGIC("Not on requested interface, skipping");
                            continue;
                        }
                    }
                    if (masklen < longest_mask) // Not interested if got shorter mask
                    {
                        NS_LOG_LOGIC("Previous match longer, skipping");
                        continue;
                    }
                    if (masklen > longest_mask) // Reset metric if longer masklen
                    {
                        shortest_metric = 0xffffffff;
                    }
                    longest_mask = masklen;
                    if (metric > shortest_metric) {
                        NS_LOG_LOGIC("Equal mask length, but previous metric shorter, skipping");
                        continue;
                    }
                    shortest_metric = metric;
                    Ipv4RoutingTableEntry* route = (j);
                    uint32_t interfaceIdx = route->GetInterface();
                    rtentry = Create<Ipv4Route> ();
                    rtentry->SetDestination(route->GetDest());
                    rtentry->SetSource(SourceAddressSelection(interfaceIdx, route->GetDest()));
                    rtentry->SetGateway(route->GetGateway());
                    rtentry->SetOutputDevice(m_ipv4->GetNetDevice(interfaceIdx));
                }
            }
            if (rtentry != 0) {
                NS_LOG_LOGIC("Matching route via " << rtentry->GetGateway() << " at the end");
                NS_LOG_INFO("Matching route via " << rtentry->GetGateway() << " at the end");
            } else {
                NS_LOG_LOGIC("No matching route to " << dest << " found");
                NS_LOG_INFO("No matching route to " << dest << " found");
            }
            return rtentry;
        }

        Ptr<Ipv4MulticastRoute>
        RoutingProtocol::LookupStatic(
                Ipv4Address origin,
                Ipv4Address group,
                uint32_t interface) {
            NS_LOG_FUNCTION(this << origin << " " << group << " " << interface);
            Ptr<Ipv4MulticastRoute> mrtentry = 0;

            for (MulticastRoutesI i = m_multicastRoutes.begin();
                    i != m_multicastRoutes.end();
                    i++) {
                Ipv4MulticastRoutingTableEntry *route = *i;
                //
                // We've been passed an origin address, a multicast group address and an 
                // interface index.  We have to decide if the current route in the list is
                // a match.
                //
                // The first case is the restrictive case where the origin, group and index
                // matches.
                //
                if (origin == route->GetOrigin() && group == route->GetGroup()) {
                    // Skipping this case (SSM) for now
                    NS_LOG_LOGIC("Found multicast source specific route" << *i);
                }
                if (group == route->GetGroup()) {
                    if (interface == Ipv4::IF_ANY ||
                            interface == route->GetInputInterface()) {
                        NS_LOG_LOGIC("Found multicast route" << *i);
                        mrtentry = Create<Ipv4MulticastRoute> ();
                        mrtentry->SetGroup(route->GetGroup());
                        mrtentry->SetOrigin(route->GetOrigin());
                        mrtentry->SetParent(route->GetInputInterface());
                        for (uint32_t j = 0; j < route->GetNOutputInterfaces(); j++) {
                            if (route->GetOutputInterface(j)) {
                                NS_LOG_LOGIC("Setting output interface index " << route->GetOutputInterface(j));
                                mrtentry->SetOutputTtl(route->GetOutputInterface(j), Ipv4MulticastRoute::MAX_TTL - 1);
                            }
                        }
                        return mrtentry;
                    }
                }
            }
            return mrtentry;
        }

        uint32_t
        RoutingProtocol::GetNRoutes(void) const {
            NS_LOG_FUNCTION(this);
            return m_networkRoutes.size();
            ;
        }

        Ipv4RoutingTableEntry
        RoutingProtocol::GetDefaultRoute() {
            NS_LOG_FUNCTION(this);
            // Basically a repeat of LookupStatic, retained for backward compatibility
            Ipv4Address dest("0.0.0.0");
            uint32_t shortest_metric = 0xffffffff;
            Ipv4RoutingTableEntry *result = 0;
            for (NetworkRoutesI i = m_networkRoutes.begin();
                    i != m_networkRoutes.end();
                    i++) {
                Ipv4RoutingTableEntry *j = i->first;
                uint32_t metric = i->second;
                Ipv4Mask mask = (j)->GetDestNetworkMask();
                uint16_t masklen = mask.GetPrefixLength();
                if (masklen != 0) {
                    continue;
                }
                if (metric > shortest_metric) {
                    continue;
                }
                shortest_metric = metric;
                result = j;
            }
            if (result) {
                return result;
            } else {
                return Ipv4RoutingTableEntry();
            }
        }

        Ipv4RoutingTableEntry
        RoutingProtocol::GetRoute(uint32_t index) const {
            NS_LOG_FUNCTION(this << index);
            uint32_t tmp = 0;
            for (NetworkRoutesCI j = m_networkRoutes.begin();
                    j != m_networkRoutes.end();
                    j++) {
                if (tmp == index) {
                    return j->first;
                }
                tmp++;
            }
            NS_ASSERT(false);
            // quiet compiler.
            return 0;
        }

        uint32_t
        RoutingProtocol::GetMetric(uint32_t index) const {
            NS_LOG_FUNCTION(this << index);
            uint32_t tmp = 0;
            for (NetworkRoutesCI j = m_networkRoutes.begin();
                    j != m_networkRoutes.end();
                    j++) {
                if (tmp == index) {
                    return j->second;
                }
                tmp++;
            }
            NS_ASSERT(false);
            // quiet compiler.
            return 0;
        }

        void
        RoutingProtocol::RemoveRoute(uint32_t index) {
            NS_LOG_FUNCTION(this << index);
            uint32_t tmp = 0;
            for (NetworkRoutesI j = m_networkRoutes.begin();
                    j != m_networkRoutes.end();
                    j++) {
                if (tmp == index) {
                    delete j->first;
                    m_networkRoutes.erase(j);
                    return;
                }
                tmp++;
            }
            NS_ASSERT(false);
        }

        Ptr<Ipv4Route>
        RoutingProtocol::RouteOutput(Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno & sockerr) {
            NS_LOG_FUNCTION(this << p << header << oif << sockerr);
            Ipv4Address destination = header.GetDestination();
            Ptr<Ipv4Route> rtentry = 0;

            // Multicast goes here
            if (destination.IsMulticast()) {
                // Note:  Multicast routes for outbound packets are stored in the
                // normal unicast table.  An implication of this is that it is not
                // possible to source multicast datagrams on multiple interfaces.
                // This is a well-known property of sockets implementation on 
                // many Unix variants.
                // So, we just log it and fall through to LookupStatic ()
                NS_LOG_LOGIC("RouteOutput()::Multicast destination");
            }
            rtentry = LookupStatic(destination, oif);
            if (rtentry) {
                sockerr = Socket::ERROR_NOTERROR;
                NS_LOG_INFO("El próximo salto es " << rtentry->GetGateway());
            } else {
                sockerr = Socket::ERROR_NOROUTETOHOST;
            }
            return rtentry;
        }

        bool
        RoutingProtocol::RouteInput(Ptr<const Packet> p, const Ipv4Header &ipHeader, Ptr<const NetDevice> idev,
                UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                LocalDeliverCallback lcb, ErrorCallback ecb) {
            NS_LOG_FUNCTION(this << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination() << idev << &ucb << &mcb << &lcb << &ecb);

            NS_ASSERT(m_ipv4 != 0);
            // Check if input device supports IP 
            NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
            uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

            // Virtual IP
            if (m_vnLayer->IsVirtual(ipHeader.GetDestination())) {

                NS_LOG_INFO("Buscando ruta hacia la IP virtual " << ipHeader.GetDestination() << " (Región " << m_vnLayer->GetRegionIdOf(ipHeader.GetDestination()) << ")");

                m_vnLayer->PrintNeighborLeadersTable();

                if (m_vnLayer->GetRegionIdOf(ipHeader.GetDestination()) == m_vnLayer->GetRegion()) {
                    if ((m_vnLayer->GetState() == VirtualLayerPlusHeader::LEADER) ||
                            ((m_vnLayer->GetState() == VirtualLayerPlusHeader::INTERIM) && (!m_vnLayer->IsReplaced()))) {

                        if (m_dpd.IsDuplicate(p, ipHeader)) {
                            NS_LOG_INFO("Duplicated packet " << p->GetUid() << " from " << ipHeader.GetSource() << ". Drop.");
                            return true;
                        }
                        lcb(p, ipHeader, iif);
                        return true;
                    } else {
                        Ptr<Ipv4Route> rtentry = 0;

                        if (m_vnLayer->GetNumOfLeaders() > 0) {

                            rtentry = Create<Ipv4Route> ();
                            rtentry->SetDestination(ipHeader.GetDestination());
                            rtentry->SetGateway(ipHeader.GetDestination());
                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                            rtentry->SetSource(m_vnLayer->GetIp());

                        }

                        if (rtentry != 0) {
                            NS_LOG_LOGIC("Found unicast destination- calling unicast callback");
                            NS_LOG_INFO("El próximo salto es " << rtentry->GetGateway());
                            ucb(rtentry, p, ipHeader); // unicast forwarding callback
                            return true;
                        } else {
                            NS_LOG_LOGIC("Did not find unicast destination- returning false");
                            return false; // Let other routing protocols try to handle this
                        }

                    }
                } else {

                    // Check if input device supports IP forwarding
                    if (m_ipv4->IsForwarding(iif) == false) {
                        NS_LOG_LOGIC("Forwarding disabled for this interface");
                        ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
                        return false;
                    }
                    Ptr<Ipv4Route> rtentry = 0;

                    if (m_vnLayer->IsNeighborRegion(m_vnLayer->GetRegionIdOf(ipHeader.GetDestination()))) {

                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionIdOf(ipHeader.GetDestination())) > 0) {

                            //Crear ruta hacia la IP virtual directamente

                            rtentry = Create<Ipv4Route> ();
                            rtentry->SetDestination(ipHeader.GetDestination());
                            rtentry->SetGateway(ipHeader.GetDestination());
                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                            rtentry->SetSource(m_vnLayer->GetIp());

                        }


                    } else {

                        uint32_t nextRegion;

                        Vector2D arrowsColumns = m_vnLayer->GetCoordinateLimits();

                        Vector2D coordOrig = m_vnLayer->GetCoordinatesOfRegion(m_vnLayer->GetRegion());
                        Vector2D coordDest = m_vnLayer->GetCoordinatesOfRegion(m_vnLayer->GetRegionIdOf(ipHeader.GetDestination()));

                        Vector2D v = Vector2D(coordDest.x - coordOrig.x, coordDest.y - coordOrig.y);

                        if (coordOrig.x > 0) { // Tiene regiones vecinas debajo
                            if (coordOrig.x < (arrowsColumns.x - 1)) { // Tiene regiones vecinas encima
                                if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                                    if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 8 regiones

                                        if (v.x == 0) { // Misma fila

                                            if (v.y > 0) { // Hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));
                                                    //Crear ruta hacia la IP virtual de la región vecina de la derecha
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                }

                                            } else { // Hacia la izquierda
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                }
                                            }

                                        } else if (v.x > 0) { // Hacia arriba

                                            if (v.y == 0) { // Sólo hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            } else if (v.y > 0) { // Hacia arriba a la derecha
                                                if (v.x > v.y) { // Más hacia arriba
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                        // Arriba a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    }
                                                } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                        // Arriba a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la derecha

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                        // Arriba a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }
                                            } else { // Hacia arriba a la izquierda
                                                if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }
                                                } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }
                                            }


                                        } else { // Hacia abajo
                                            if (v.y == 0) { // Sólo hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());



                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                }
                                            } else if (v.y > 0) { // Hacia abajo a la derecha

                                                if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }


                                                } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la derecha

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }

                                            } else { // Hacia abajo a la izquierda

                                                if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    }


                                                } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }

                                            }
                                        }



                                    } else { // Columna N --> No tiene regiones vecinas a la derecha --> 5 regiones

                                        if (v.x == 0) { // Misma fila --> Hacia la izquierda

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }


                                        } else if (v.x > 0) { // Hacia arriba

                                            if (v.y == 0) { // Sólo hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            } else { // Hacia arriba a la izquierda
                                                if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }
                                                } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                        // Arriba a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                        // Arriba
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }
                                            }


                                        } else { // Hacia abajo
                                            if (v.y == 0) { // Sólo hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                }
                                            } else { // Hacia abajo a la izquierda

                                                if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    }


                                                } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());


                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());
                                                    }

                                                }

                                            }
                                        }

                                    }

                                } else { // Columna 0 --> No hay regiones vecinas a la izquierda --> 5 regiones vecinas

                                    if (v.x == 0) { // Misma fila --> Hacia la derecha

                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                            // Abajo a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        }


                                    } else if (v.x > 0) { // Hacia arriba

                                        if (v.y == 0) { // Sólo hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        } else { // Hacia arriba a la derecha
                                            if (v.x > v.y) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }
                                        }


                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());
                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        } else { // Hacia abajo a la derecha

                                            if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());
                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }


                                            } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }

                                        }
                                    }

                                }
                            } else { // Fila N --> No tiene regiones vecinas encima
                                if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                                    if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 5 regiones

                                        if (v.x == 0) { // Misma fila

                                            if (v.y > 0) { // Hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Hacia la izquierda
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            }

                                        } else { // Hacia abajo
                                            if (v.y == 0) { // Sólo hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            } else if (v.y > 0) { // Hacia abajo a la derecha

                                                if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }


                                                } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la derecha

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                        // Derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                        // Abajo a la derecha
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }

                                            } else { // Hacia abajo a la izquierda

                                                if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }


                                                } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }

                                            }
                                        }

                                    } else { // Columna N --> No tiene regiones vecinas a la derecha --> 3 regiones

                                        if (v.x == 0) { // Misma fila --> Hacia la izquierda
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                // Abajo a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }

                                        } else { // Hacia abajo
                                            if (v.y == 0) { // Sólo hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                    // Abajo a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }
                                            } else { // Hacia abajo a la izquierda

                                                if (((-1) * v.x) > ((-1) * v.y)) { // Más hacia abajo
                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo

                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                        //Crear ruta hacia la IP virtual de la región vecina de encima
                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }


                                                } else if (((-1) * v.x) == ((-1) * v.y)) { // Hacia abajo e izquierda en igual proporción 

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                } else { // Más hacia la izquierda

                                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                        // Izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1))) > 0) {
                                                        // Abajo a la izquierda
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y - 1));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                        // Abajo
                                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                        rtentry = Create<Ipv4Route> ();
                                                        rtentry->SetDestination(ipHeader.GetDestination());
                                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                        rtentry->SetSource(m_vnLayer->GetIp());

                                                    }

                                                }

                                            }
                                        }

                                    }
                                } else { // Columna 0 --> No hay regiones a la izquierda --> 3 regiones vecinas

                                    if (v.x == 0) { // Misma fila --> Hacia la derecha
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                            // Derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                            // Abajo a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        }

                                    } else { // Hacia abajo
                                        if (v.y == 0) { // Sólo hacia abajo
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                // Abajo

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                // Abajo a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        } else { // Hacia abajo a la derecha

                                            if (((-1) * v.x) > v.y) { // Más hacia abajo
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }


                                            } else if (((-1) * v.x) == v.y) { // Hacia abajo y derecha en igual proporción

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1))) > 0) {
                                                    // Abajo a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y))) > 0) {
                                                    // Abajo
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x - 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }

                                        }
                                    }

                                }
                            }

                        } else { // Fila 0 --> No hay regiones debajo
                            if (coordOrig.y > 0) { // Tiene regiones vecinas a la izquierda
                                if (coordOrig.y < (arrowsColumns.y - 1)) { // Tiene regiones vecinas a la derecha --> 5 regiones

                                    if (v.x == 0) { // Misma fila

                                        if (v.y > 0) { // Hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }

                                        } else { // Hacia la izquierda
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                // Izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        }

                                    } else { // Hacia arriba
                                        if (v.y == 0) { // Sólo hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de abajo
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        } else if (v.y > 0) { // Hacia arriba a la derecha

                                            if (v.x > v.y) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }


                                            } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la derecha

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                    // Derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                    // Arriba a la derecha
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }

                                        } else { // Hacia arriba a la izquierda

                                            if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }


                                            } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }

                                        }
                                    }

                                } else { // Columna N --> No tiene regiones vecinas a la derecha --> 3 regiones

                                    if (v.x == 0) { // Misma fila --> Hacia la izquierda
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                            // Izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                            // Arriba a la izquierda
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        }

                                    } else { // Hacia arriba
                                        if (v.y == 0) { // Sólo hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                // Arriba a la izquierda
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }
                                        } else { // Hacia arriba a la izquierda

                                            if (v.x > ((-1) * v.y)) { // Más hacia arriba
                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba

                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                    //Crear ruta hacia la IP virtual de la región vecina de encima
                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }


                                            } else if (v.x == ((-1) * v.y)) { // Hacia arriba e izquierda en igual proporción 

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());


                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            } else { // Más hacia la izquierda

                                                if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1))) > 0) {
                                                    // Izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1))) > 0) {
                                                    // Arriba a la izquierda
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y - 1));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                    // Arriba
                                                    nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                    rtentry = Create<Ipv4Route> ();
                                                    rtentry->SetDestination(ipHeader.GetDestination());
                                                    rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                    rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                    rtentry->SetSource(m_vnLayer->GetIp());

                                                }

                                            }

                                        }
                                    }

                                }
                            } else { // Columna 0 --> No hay regiones a la izquierda --> 3 regiones vecinas

                                if (v.x == 0) { // Misma fila --> Hacia la derecha
                                    if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                        // Derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(ipHeader.GetDestination());
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());

                                    } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                        // Arriba a la derecha
                                        nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                        rtentry = Create<Ipv4Route> ();
                                        rtentry->SetDestination(ipHeader.GetDestination());
                                        rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                        rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                        rtentry->SetSource(m_vnLayer->GetIp());

                                    }

                                } else { // Hacia arriba
                                    if (v.y == 0) { // Sólo hacia arriba
                                        if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                            // Arriba

                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                            //Crear ruta hacia la IP virtual de la región vecina de arriba
                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                            // Arriba a la derecha
                                            nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                            rtentry = Create<Ipv4Route> ();
                                            rtentry->SetDestination(ipHeader.GetDestination());
                                            rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                            rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                            rtentry->SetSource(m_vnLayer->GetIp());

                                        }
                                    } else { // Hacia arriba a la derecha

                                        if (v.x > v.y) { // Más hacia arriba
                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba

                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));
                                                //Crear ruta hacia la IP virtual de la región vecina de encima
                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }


                                        } else if (v.x == v.y) { // Hacia arriba y derecha en igual proporción

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }

                                        } else { // Más hacia la derecha

                                            if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1))) > 0) {
                                                // Derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1))) > 0) {
                                                // Arriba a la derecha
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y + 1));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());


                                            } else if (m_vnLayer->GetNumOfNeighborLeadersIn(m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y))) > 0) {
                                                // Arriba
                                                nextRegion = m_vnLayer->GetRegionForCoordinates(Vector2D(coordOrig.x + 1, coordOrig.y));

                                                rtentry = Create<Ipv4Route> ();
                                                rtentry->SetDestination(ipHeader.GetDestination());
                                                rtentry->SetGateway(m_vnLayer->GetVirtualIpOf(nextRegion));
                                                rtentry->SetOutputDevice(dynamic_cast<NetDevice*> (PeekPointer(m_vnLayer)));
                                                rtentry->SetSource(m_vnLayer->GetIp());

                                            }

                                        }

                                    }
                                }

                            }
                        }
                    }

                    if (rtentry != 0) {
                        NS_LOG_LOGIC("Found unicast destination- calling unicast callback");
                        NS_LOG_INFO("El próximo salto es " << rtentry->GetGateway() << " (Región " << m_vnLayer->GetRegionIdOf(rtentry->GetGateway()) << ")");
                        ucb(rtentry, p, ipHeader); // unicast forwarding callback
                        return true;
                    } else {
                        NS_LOG_LOGIC("Did not find unicast destination- returning false");
                        NS_LOG_INFO("No se encontró ningún nodo virtual hacia el que direccionar el paquete.");
                        return false; // Let other routing protocols try to handle this
                    }

                }
            }

            // Multicast recognition; handle local delivery here
            //
            if (ipHeader.GetDestination().IsMulticast()) {
                NS_LOG_LOGIC("Multicast destination");
                Ptr<Ipv4MulticastRoute> mrtentry = LookupStatic(ipHeader.GetSource(),
                        ipHeader.GetDestination(), m_ipv4->GetInterfaceForDevice(idev));

                if (mrtentry) {
                    NS_LOG_LOGIC("Multicast route found");
                    mcb(mrtentry, p, ipHeader); // multicast forwarding callback
                    return true;
                } else {
                    NS_LOG_LOGIC("Multicast route not found");
                    return false; // Let other routing protocols try to handle this
                }
            }
            if (ipHeader.GetDestination().IsBroadcast()) {
                NS_LOG_LOGIC("For me (Ipv4Addr broadcast address)");
                /// \todo Local Deliver for broadcast
                /// \todo Forward broadcast
            }

            NS_LOG_LOGIC("Unicast destination");
            /// \todo Configurable option to enable \RFC{1222} Strong End System Model
            // Right now, we will be permissive and allow a source to send us
            // a packet to one of our other interface addresses; that is, the
            // destination unicast address does not match one of the iif addresses,
            // but we check our other interfaces.  This could be an option
            // (to remove the outer loop immediately below and just check iif).
            for (uint32_t j = 0; j < m_ipv4->GetNInterfaces(); j++) {
                for (uint32_t i = 0; i < m_ipv4->GetNAddresses(j); i++) {
                    Ipv4InterfaceAddress iaddr = m_ipv4->GetAddress(j, i);
                    Ipv4Address addr = iaddr.GetLocal();
                    if (addr.IsEqual(ipHeader.GetDestination())) {
                        if (j == iif) {
                            NS_LOG_LOGIC("For me (destination " << addr << " match)");
                        } else {
                            NS_LOG_LOGIC("For me (destination " << addr << " match) on another interface " << ipHeader.GetDestination());
                        }
                        if (m_dpd.IsDuplicate(p, ipHeader)) {
                            NS_LOG_DEBUG("Duplicated packet " << p->GetUid() << " from " << ipHeader.GetSource() << ". Drop.");
                            return true;
                        }
                        lcb(p, ipHeader, iif);
                        return true;
                    }
                    if (ipHeader.GetDestination().IsEqual(iaddr.GetBroadcast())) {
                        NS_LOG_LOGIC("For me (interface broadcast address)");
                        if (m_dpd.IsDuplicate(p, ipHeader)) {
                            NS_LOG_DEBUG("Duplicated packet " << p->GetUid() << " from " << ipHeader.GetSource() << ". Drop.");
                            return true;
                        }
                        lcb(p, ipHeader, iif);
                        return true;
                    }
                    NS_LOG_LOGIC("Address " << addr << " not a match");
                }
            }
            // Check if input device supports IP forwarding
            if (m_ipv4->IsForwarding(iif) == false) {
                NS_LOG_LOGIC("Forwarding disabled for this interface");
                ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
                return false;
            }
            // Next, try to find a route
            Ptr<Ipv4Route> rtentry = LookupStatic(ipHeader.GetDestination());
            if (rtentry != 0) {
                NS_LOG_LOGIC("Found unicast destination- calling unicast callback");
                ucb(rtentry, p, ipHeader); // unicast forwarding callback
                return true;
            } else {
                NS_LOG_LOGIC("Did not find unicast destination- returning false");
                return false; // Let other routing protocols try to handle this
            }
        }

        RoutingProtocol::~RoutingProtocol() {
            NS_LOG_FUNCTION(this);
        }

        void
        RoutingProtocol::DoDispose(void) {
            NS_LOG_FUNCTION(this);
            for (NetworkRoutesI j = m_networkRoutes.begin();
                    j != m_networkRoutes.end();
                    j = m_networkRoutes.erase(j)) {
                delete (j->first);
            }
            for (MulticastRoutesI i = m_multicastRoutes.begin();
                    i != m_multicastRoutes.end();
                    i = m_multicastRoutes.erase(i)) {
                delete (*i);
            }
            m_ipv4 = 0;
            Ipv4RoutingProtocol::DoDispose();
        }

        void
        RoutingProtocol::NotifyInterfaceUp(uint32_t i) {
            NS_LOG_FUNCTION(this << i);
            // If interface address and network mask have been set, add a route
            // to the network of the interface (like e.g. ifconfig does on a
            // Linux box)
            for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++) {
                if (m_ipv4->GetAddress(i, j).GetLocal() != Ipv4Address() &&
                        m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask() &&
                        m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask::GetOnes()) {
                    AddNetworkRouteTo(m_ipv4->GetAddress(i, j).GetLocal().CombineMask(m_ipv4->GetAddress(i, j).GetMask()),
                            m_ipv4->GetAddress(i, j).GetMask(), i);
                }
            }
        }

        void
        RoutingProtocol::NotifyInterfaceDown(uint32_t i) {
            NS_LOG_FUNCTION(this << i);
            // Remove all static routes that are going through this interface
            for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();) {
                if (it->first->GetInterface() == i) {
                    delete it->first;
                    it = m_networkRoutes.erase(it);
                } else {
                    it++;
                }
            }
        }

        void
        RoutingProtocol::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) {
            NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
            if (!m_ipv4->IsUp(interface)) {
                return;
            }

            Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
            Ipv4Mask networkMask = address.GetMask();
            if (address.GetLocal() != Ipv4Address() &&
                    address.GetMask() != Ipv4Mask()) {
                AddNetworkRouteTo(networkAddress,
                        networkMask, interface);
            }
        }

        void
        RoutingProtocol::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) {
            NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
            if (!m_ipv4->IsUp(interface)) {
                return;
            }
            Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
            Ipv4Mask networkMask = address.GetMask();
            // Remove all static routes that are going through this interface
            // which reference this network
            for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();) {
                if (it->first->GetInterface() == interface
                        && it->first->IsNetwork()
                        && it->first->GetDestNetwork() == networkAddress
                        && it->first->GetDestNetworkMask() == networkMask) {
                    delete it->first;
                    it = m_networkRoutes.erase(it);
                } else {
                    it++;
                }
            }
        }

        void
        RoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4) {
            NS_LOG_FUNCTION(this << ipv4);
            NS_ASSERT(m_ipv4 == 0 && ipv4 != 0);
            m_ipv4 = ipv4;

            m_lo = m_ipv4->GetNetDevice(0);
            NS_ASSERT(m_lo != 0);

            for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++) {
                if (m_ipv4->IsUp(i)) {
                    NotifyInterfaceUp(i);
                } else {
                    NotifyInterfaceDown(i);
                }
            }

            Simulator::ScheduleWithContext(m_node->GetId(), FemtoSeconds(1), &RoutingProtocol::Start, this);
        }

        // Formatted like output of "route -n" command

        void
        RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const {
            NS_LOG_FUNCTION(this << stream);
            std::ostream* os = stream->GetStream();
            if (GetNRoutes() > 0) {
                *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface" << std::endl;
                for (uint32_t j = 0; j < GetNRoutes(); j++) {
                    std::ostringstream dest, gw, mask, flags;
                    Ipv4RoutingTableEntry route = GetRoute(j);
                    dest << route.GetDest();
                    *os << std::setiosflags(std::ios::left) << std::setw(16) << dest.str();
                    gw << route.GetGateway();
                    *os << std::setiosflags(std::ios::left) << std::setw(16) << gw.str();
                    mask << route.GetDestNetworkMask();
                    *os << std::setiosflags(std::ios::left) << std::setw(16) << mask.str();
                    flags << "U";
                    if (route.IsHost()) {
                        flags << "HS";
                    } else if (route.IsGateway()) {
                        flags << "GS";
                    }
                    *os << std::setiosflags(std::ios::left) << std::setw(6) << flags.str();
                    *os << std::setiosflags(std::ios::left) << std::setw(7) << GetMetric(j);
                    // Ref ct not implemented
                    *os << "-" << "      ";
                    // Use not implemented
                    *os << "-" << "   ";
                    if (Names::FindName(m_ipv4->GetNetDevice(route.GetInterface())) != "") {
                        *os << Names::FindName(m_ipv4->GetNetDevice(route.GetInterface()));
                    } else {
                        *os << route.GetInterface();
                    }
                    *os << std::endl;
                }
            }
        }

        Ipv4Address
        RoutingProtocol::SourceAddressSelection(uint32_t interfaceIdx, Ipv4Address dest) {
            NS_LOG_FUNCTION(this << interfaceIdx << " " << dest);
            if (m_ipv4->GetNAddresses(interfaceIdx) == 1) // common case
            {
                return m_ipv4->GetAddress(interfaceIdx, 0).GetLocal();
            }
            // no way to determine the scope of the destination, so adopt the
            // following rule:  pick the first available address (index 0) unless
            // a subsequent address is on link (in which case, pick the primary
            // address if there are multiple)
            Ipv4Address candidate = m_ipv4->GetAddress(interfaceIdx, 0).GetLocal();
            for (uint32_t i = 0; i < m_ipv4->GetNAddresses(interfaceIdx); i++) {
                Ipv4InterfaceAddress test = m_ipv4->GetAddress(interfaceIdx, i);
                if (test.GetLocal().CombineMask(test.GetMask()) == dest.CombineMask(test.GetMask())) {
                    if (test.IsSecondary() == false) {
                        return test.GetLocal();
                    }
                }
            }
            return candidate;
        }

        Ptr<Ipv4Route>
        RoutingProtocol::LoopbackRoute(const Ipv4Address dst, Ptr<NetDevice> oif) const {
            NS_LOG_FUNCTION(this << dst);
            NS_ASSERT(m_lo != 0);
            Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
            rt->SetDestination(dst);
            //
            // Source address selection here is tricky.  The loopback route is
            // returned when VNR does not have a route; this causes the packet
            // to be looped back and handled (cached) in RouteInput() method
            // while a route is found. However, connection-oriented protocols
            // like TCP need to create an endpoint four-tuple (src, src port,
            // dst, dst port) and create a pseudo-header for checksumming.  So,
            // VNR needs to guess correctly what the eventual source address
            // will be.
            //
            // For single interface, single address nodes, this is not a problem.
            // When there are possibly multiple outgoing interfaces, the policy
            // implemented here is to pick the first available VNR interface.
            // If RouteOutput() caller specified an outgoing interface, that 
            // further constrains the selection of source address
            //

            if (oif) {
                for (NetworkRoutesCI it = m_networkRoutes.begin(); it != m_networkRoutes.end();) {

                    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
                    if (l3->GetNAddresses(it->first->GetInterface()) > 1) {
                        NS_LOG_WARN("VNR does not work with more then one address per each interface.");
                    }
                    Ipv4InterfaceAddress iface = l3->GetAddress(it->first->GetInterface(), 0);
                    if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
                        continue;

                    Ipv4Address addr = iface.GetLocal();

                    if (oif == m_ipv4->GetNetDevice(it->first->GetInterface())) {
                        rt->SetSource(addr);
                        break;
                    }

                }
            } else {
                Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
                Ipv4InterfaceAddress iface = l3->GetAddress(m_networkRoutes.begin()->first->GetInterface(), 0);
                rt->SetSource(iface.GetLocal());
            }

            NS_ASSERT_MSG(rt->GetSource() != Ipv4Address(), "Valid VNR source address not found");
            rt->SetGateway(Ipv4Address("127.0.0.1"));
            rt->SetOutputDevice(m_lo);
            return rt;
        }

    }
} // namespace ns3
