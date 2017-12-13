/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef IBR_ROUTING_PROTOCOL_H
#define IBR_ROUTING_PROTOCOL_H


#include <list>
#include <utility>
#include <stdint.h>

//#include "ns3/ipv4-address.h"
//#include "ns3/ipv4-header.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/timer.h"
#include "ibr-hostlist-trailer.h"
#include "ns3/scma-statemachine.h"
#include <map>
#include "ns3/ibr-routing-table.h"
#include "ns3/ibr-waysegment-table.h"
#include "ns3/ibr-header.h"

#include "ns3/virtual-layer-plus-net-device.h"
#include "ns3/virtual-layer-plus-header.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/abort.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "src/network/utils/ipv4-address.h"
#include "src/network/model/packet.h"
#include "ns3/ipv4-route.h"
#include "ns3/routing-table-header.h"

#include "ns3/names.h"


#ifndef VIRTUAL_IP_BASE_IBR
#define VIRTUAL_IP_BASE_IBR   (0xF0000000) 
#endif

namespace ns3 {
	class IbrRoutingProtocol: public Ipv4RoutingProtocol {
		public:

			
		Ptr<VirtualLayerPlusNetDevice> ibr_vnLayer; // copiado al .cc para que funcione (loco!)
		Ptr<Node> ibr_node;
		Ptr<NetDevice> ibr_netDevice; 


		struct HostEntry {
            Ipv4Address ip;
            Address mac;
            Timer timerLifeHostEntry;
            
            Ipv4Address leaderIp;
            Address leaderMac;
            
            Ipv4Address virtualIp;
			
			uint32_t waySegment;
			uint32_t m_region;
		
			double BW;
			Time RSAT;

			

			HostEntry(Ipv4Address ipAddr, Address macAddr) :
            ip(ipAddr),
            mac(macAddr),
            timerLifeHostEntry(Timer::CANCEL_ON_DESTROY) {
            }
            void HostEntryRegion(Ipv4Address ipAddr, Address macAddr, Ipv4Address leadIpAddr, Address leadMacAddr, Ipv4Address virtualIpAddr, uint32_t ws, uint32_t reg, double bw, Time rsat) {
				ip=(ipAddr);
				mac=(macAddr);
				timerLifeHostEntry=(Timer::CANCEL_ON_DESTROY);
				leaderIp=(leadIpAddr);
				leaderMac=(leadMacAddr);
				virtualIp=(virtualIpAddr);
				waySegment=(ws);
				m_region=(reg);
				BW=bw;
				RSAT=rsat;
			}
            
        };
		
		std::list<IbrRegionHostTable::HostEntry> ibr_hostsTable;
		//std::list<HostEntry> ibr_hostsTableDiscovery;
		std::list<IbrHostListTrailer::PayloadHostListEntry> ibr_hostsTableDiscovery;


		static TypeId GetTypeId();
		Timer m_timerSyncHostsRegion;
		Timer m_timerIncomingPacket;
		Timer m_PingPongStart;
		
		void TimerSyncHostTable();
		void TimerPingPongStart();
		void TimerIncomingPacket();
	
		
		void PingPongStart();
		// host table functions
		bool addHostToTable(IbrRegionHostTable::HostEntry host);
		bool addHostToTableDiscovery(IbrRegionHostTable::HostEntry host);
		//bool removeHostFromTable(std::list<HostEntry>::iterator host);
		// waysegment functions
		uint32_t GetWaySegment(uint32_t region);
		bool IsRegionInWaySegment(uint32_t region, uint32_t waysegment);
		//uint32_t GetActualWaySegment(uint32_t region) 
		// virtual IP functions
		Ipv4Address GetVirtualIpOf(uint32_t region);

		// Tx Rx Functions
		bool SendSyncHostTable(VirtualLayerPlusHeader::type ibrMsgType, VirtualLayerPlusHeader::subType ibrMsgSubType, Ipv4Address dstIp, Address dstMac);
		bool RecvPacket(Ptr<Packet> pkt); 
		uint32_t GetRxSourceRegion(bool direction);
		
		uint32_t ibr_waysegment;
		uint32_t ibr_region;
		uint32_t ibr_virtualNodeLevel;
		Ipv4Address ibr_ip;
		Address ibr_mac;
		Ipv4Address ibr_virtualIp;
		VirtualLayerPlusHeader::typeState ibr_state;
		bool hostListSyncDirection;
		//bool IsPacketFromIntersectionNeighbour(uint32_t regionLocal, uint32_t region, bool dir);
		
		

		void printDiscoveryHostsTable();

		// Function for the RoutingProtocol implementation
		// *********************************************************
		
		// ipv4 routing functions
		
		
		protected:
			virtual void DoInitialize (void);	

		public:
			
				// virtual function from Ipv4RoutingProtocol
				//
				IbrRoutingProtocol();
				virtual ~IbrRoutingProtocol();

				virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

				virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, 
										 Ptr<const NetDevice> idev,
										 UnicastForwardCallback ucb, MulticastForwardCallback mcb,
										 LocalDeliverCallback lcb, ErrorCallback ecb);
				virtual void NotifyInterfaceUp (uint32_t interface);
				virtual void NotifyInterfaceDown (uint32_t interface);
				virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
				virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
				virtual void SetIpv4 (Ptr<Ipv4> ipv4);
				virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;
				
				void SetRtable (Ptr<IbrRoutingTable> prt);
				void SetWsTable (Ptr<IbrWaysegmentTable> wsT);

				// ****************************************
				
		

		private:
				Ptr<IbrRoutingTable> m_rtable;
				Ipv4Address m_address;
				Ipv4Address m_broadcast;
				Ptr<Ipv4> m_ipv4;
				Ptr<Node> m_node;
				uint32_t m_ifaceId; 
				Ptr<IbrWaysegmentTable> m_wsTable;
				
				void SendPacket(Ptr<Packet>);
				void RecvIbr (Ptr<Socket> socket);
				Ipv4Address m_mainAddress; 
				std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses; 
				void accept(Ptr<Socket> socket,const ns3::Address& from) ;
		

		};
	

}

#endif /* IBR_H */

