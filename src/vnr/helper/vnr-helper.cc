#include "vnr-helper.h"
#include <vector>
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/assert.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/node-list.h"
#include "src/core/model/log-macros-enabled.h"


namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("VirtualRoutingHelper");

    VnrHelper::VnrHelper() :
    Ipv4RoutingHelper() {
        m_agentFactory.SetTypeId("ns3::vnr::RoutingProtocol");
    }

    VnrHelper*
    VnrHelper::Copy(void) const {
        return new VnrHelper(*this);
    }

    Ptr<Ipv4RoutingProtocol>
    VnrHelper::Create(Ptr<Node> node) const {
        Ptr<vnr::RoutingProtocol> agent = m_agentFactory.Create<vnr::RoutingProtocol> ();
        agent->SetNode(node);
        node->AggregateObject(agent);
        return agent;
    }

    void
    VnrHelper::Set(std::string name, const AttributeValue &value) {
        m_agentFactory.Set(name, value);
    }

    int64_t
    VnrHelper::AssignStreams(NodeContainer c, int64_t stream) {
        int64_t currentStream = stream;
        Ptr<Node> node;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
            node = (*i);
            Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
            NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");
            Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
            NS_ASSERT_MSG(proto, "Ipv4 routing not installed on node");
            Ptr<vnr::RoutingProtocol> vnr = DynamicCast<vnr::RoutingProtocol> (proto);
            if (vnr) {
                currentStream += vnr->AssignStreams(currentStream);
                continue;
            }
            // Vnr may also be in a list
            Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (proto);
            if (list) {
                int16_t priority;
                Ptr<Ipv4RoutingProtocol> listProto;
                Ptr<vnr::RoutingProtocol> listVnr;
                for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++) {
                    listProto = list->GetRoutingProtocol(i, priority);
                    listVnr = DynamicCast<vnr::RoutingProtocol> (listProto);
                    if (listVnr) {
                        currentStream += listVnr->AssignStreams(currentStream);
                        break;
                    }
                }
            }
        }
        return (currentStream - stream);
    }

    Ptr<vnr::RoutingProtocol>
    VnrHelper::GetVirtualRouting(Ptr<Ipv4> ipv4) const {
        NS_LOG_FUNCTION(this);
        Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol();
        NS_ASSERT_MSG(ipv4rp, "No routing protocol associated with Ipv4");
        if (DynamicCast<vnr::RoutingProtocol> (ipv4rp)) {
            NS_LOG_LOGIC("Virtual routing found as the main IPv4 routing protocol.");
            return DynamicCast<vnr::RoutingProtocol> (ipv4rp);
        }
        if (DynamicCast<Ipv4ListRouting> (ipv4rp)) {
            Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
            int16_t priority;
            for (uint32_t i = 0; i < lrp->GetNRoutingProtocols(); i++) {
                NS_LOG_LOGIC("Searching for virtual routing in list");
                Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol(i, priority);
                if (DynamicCast<vnr::RoutingProtocol> (temp)) {
                    NS_LOG_LOGIC("Found static routing in list");
                    return DynamicCast<vnr::RoutingProtocol> (temp);
                }
            }
        }
        NS_LOG_LOGIC("Virtual routing not found");
        return 0;
    }

    void
    VnrHelper::AddMulticastRoute(
            Ptr<Node> n,
            Ipv4Address source,
            Ipv4Address group,
            Ptr<NetDevice> input,
            NetDeviceContainer output) {
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();

        // We need to convert the NetDeviceContainer to an array of interface 
        // numbers
        std::vector<uint32_t> outputInterfaces;
        for (NetDeviceContainer::Iterator i = output.Begin(); i != output.End(); ++i) {
            Ptr<NetDevice> nd = *i;
            int32_t interface = ipv4->GetInterfaceForDevice(nd);
            NS_ASSERT_MSG(interface >= 0,
                    "VnrHelper::AddMulticastRoute(): "
                    "Expected an interface associated with the device nd");
            outputInterfaces.push_back(interface);
        }

        int32_t inputInterface = ipv4->GetInterfaceForDevice(input);
        NS_ASSERT_MSG(inputInterface >= 0,
                "VnrHelper::AddMulticastRoute(): "
                "Expected an interface associated with the device input");
        VnrHelper helper;
        Ptr<vnr::RoutingProtocol> ipv4VirtualRouting = helper.GetVirtualRouting(ipv4);
        if (!ipv4VirtualRouting) {
            NS_ASSERT_MSG(ipv4VirtualRouting,
                    "VnrHelper::SetDefaultMulticastRoute(): "
                    "Expected an vnr::RoutingProtocol associated with this node");
        }
        ipv4VirtualRouting->AddMulticastRoute(source, group, inputInterface, outputInterfaces);
    }

    void
    VnrHelper::AddMulticastRoute(
            Ptr<Node> n,
            Ipv4Address source,
            Ipv4Address group,
            std::string inputName,
            NetDeviceContainer output) {
        Ptr<NetDevice> input = Names::Find<NetDevice> (inputName);
        AddMulticastRoute(n, source, group, input, output);
    }

    void
    VnrHelper::AddMulticastRoute(
            std::string nName,
            Ipv4Address source,
            Ipv4Address group,
            Ptr<NetDevice> input,
            NetDeviceContainer output) {
        Ptr<Node> n = Names::Find<Node> (nName);
        AddMulticastRoute(n, source, group, input, output);
    }

    void
    VnrHelper::AddMulticastRoute(
            std::string nName,
            Ipv4Address source,
            Ipv4Address group,
            std::string inputName,
            NetDeviceContainer output) {
        Ptr<NetDevice> input = Names::Find<NetDevice> (inputName);
        Ptr<Node> n = Names::Find<Node> (nName);
        AddMulticastRoute(n, source, group, input, output);
    }

    void
    VnrHelper::SetDefaultMulticastRoute(
            Ptr<Node> n,
            Ptr<NetDevice> nd) {
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        int32_t interfaceSrc = ipv4->GetInterfaceForDevice(nd);
        NS_ASSERT_MSG(interfaceSrc >= 0,
                "VnrHelper::SetDefaultMulticastRoute(): "
                "Expected an interface associated with the device");
        VnrHelper helper;
        Ptr<vnr::RoutingProtocol> ipv4VirtualRouting = helper.GetVirtualRouting(ipv4);
        if (!ipv4VirtualRouting) {
            NS_ASSERT_MSG(ipv4VirtualRouting,
                    "VnrHelper::SetDefaultMulticastRoute(): "
                    "Expected an vnr::RoutingProtocol associated with this node");
        }
        ipv4VirtualRouting->SetDefaultMulticastRoute(interfaceSrc);
    }

    void
    VnrHelper::SetDefaultMulticastRoute(
            Ptr<Node> n,
            std::string ndName) {
        Ptr<NetDevice> nd = Names::Find<NetDevice> (ndName);
        SetDefaultMulticastRoute(n, nd);
    }

    void
    VnrHelper::SetDefaultMulticastRoute(
            std::string nName,
            Ptr<NetDevice> nd) {
        Ptr<Node> n = Names::Find<Node> (nName);
        SetDefaultMulticastRoute(n, nd);
    }

    void
    VnrHelper::SetDefaultMulticastRoute(
            std::string nName,
            std::string ndName) {
        Ptr<Node> n = Names::Find<Node> (nName);
        Ptr<NetDevice> nd = Names::Find<NetDevice> (ndName);
        SetDefaultMulticastRoute(n, nd);
    }


}
