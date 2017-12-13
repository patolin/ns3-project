/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// IBR Routing Protocol
// Inherits IPV4RoutingProtocol
// Used for the simulation with VNLayer module


#include <float.h>
#include <math.h>
#include <iostream> 


#include "ns3/ibr-routing-protocol.h"
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
#include "ns3/ipv4.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-route.h"
#include "ns3/routing-table-header.h"
#include "ns3/ibr-region-host-table.h"
#include "ns3/ibr-hostlist-trailer.h"
#include "ns3/ibr-header.h"
#include "ns3/names.h"
#include "ns3/socket-factory.h"
#include "ns3/virtual-layer-plus-header.h"

#define TIME_SYNC_HOST_LIST ns3::Seconds(1.0)
#define TIME_START_PINGPONG ns3::Seconds(1.0)
#define TIME_INCOMING_PACKET ns3::Seconds(5.0)	// timer to wait for incoming packets. restart pingpong as watchdog
#define TIME_GET_NODE_POSITION ns3::Seconds(POSITION_TIMER_INTERVAL)
#define IBR_PORT_NUMBER 6666

#define IBR_DEBUG_PINGPONG_NODE 555 /*total*/ //269 //559 //UINT32_MAX		// UINT32_MAX to disable, node number to start pingpong in one waysegment
#define IBR_DEBUG false 			// selective debug messages

NS_LOG_COMPONENT_DEFINE("IbrRoutingProtocol");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(IbrRoutingProtocol);
		
	IbrRoutingProtocol::IbrRoutingProtocol() { 
		m_timerSyncHostsRegion.SetFunction(&IbrRoutingProtocol::TimerSyncHostTable, this);
		m_timerIncomingPacket.SetFunction(&IbrRoutingProtocol::TimerIncomingPacket, this);
		m_timerSyncHostsRegion.SetFunction(&IbrRoutingProtocol::TimerSyncHostTable, this);
		NS_LOG_FUNCTION_NOARGS ();
	}

	IbrRoutingProtocol::~IbrRoutingProtocol() {
  		//NS_LOG_FUNCTION_NOARGS ();
	}

	TypeId IbrRoutingProtocol::GetTypeId (void) {
		static TypeId tid = TypeId ("ns3::IbrRoutingProtocol")
			.SetParent<Ipv4RoutingProtocol> ()
			.AddConstructor<IbrRoutingProtocol> ()
			.AddAttribute ("RoutingTable", "Pointer to Routing Table.",
                   PointerValue (),
                   MakePointerAccessor (&IbrRoutingProtocol::SetRtable),
                   MakePointerChecker<IbrRoutingTable> ())
			.AddAttribute ("WaysegmentTable", "Table of waysegments.",
				   PointerValue (),
				   MakePointerAccessor (&IbrRoutingProtocol::SetWsTable	),
				   MakePointerChecker<IbrWaysegmentTable>() 
				   )
				   ;
	  	return tid;
	}



	// IPV4 routing virtual functions
	Ptr<Ipv4Route>	IbrRoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, enum Socket::SocketErrno &sockerr) {
		// RouteOutput function
		// required by the Routing process
		// Actually does nothing in this setup
				
		Ptr<Ipv4Route> route = Create<Ipv4Route> ();
		NS_LOG_FUNCTION (this << header.GetSource () << "->" << "->" << header.GetDestination ());
		  /*
		  Ipv4Address relay = m_rtable->LookupRoute (m_address, header.GetDestination ());
		  relay = m_address;
		  NS_LOG_FUNCTION (this << header.GetSource () << "->" << relay << "->" << header.GetDestination ());
		  NS_LOG_INFO ("Relay to " << relay);
		  if (m_address == relay)
			{
			  //NS_LOG_DEBUG ("Can't find route!!");
			}
		  route->SetGateway (relay);
		  route->SetSource (m_address);
		  route->SetDestination (header.GetDestination ());
		  route->SetOutputDevice (m_ipv4->GetNetDevice (m_ifaceId));
		  
		  sockerr = Socket::ERROR_NOTERROR;
		*/  
		  return route;
	}	

	bool IbrRoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev, UnicastForwardCallback ucb, MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb){
		 	// RouteInput function
			// required by the Routing process
			// Actually does nothing in this setup
			
			NS_LOG_FUNCTION (this << header.GetDestination ());
		/*
		  if (header.GetDestination () == m_address)
			{
			  //NS_LOG_DEBUG ("I'm the destination");
			  lcb (p, header, m_ipv4->GetInterfaceForDevice (idev));
			  return true;
			}
		  else if (header.GetDestination () == m_broadcast)
			{
			  //NS_LOG_DEBUG ("It's broadcast");
			  return true;
			}
		  else
			{
			  Ipv4Address relay = m_rtable->LookupRoute (m_address, header.GetDestination ());
			  NS_LOG_FUNCTION (this << m_address << "->" << relay << "->" << header.GetDestination ());
			  //NS_LOG_DEBUG ("Relay to " << relay);
			  if (m_address == relay)
				{
				  //NS_LOG_DEBUG ("Can't find a route!!");
				}
			  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
			  route->SetGateway (relay);
			  route->SetSource (header.GetSource ());
			  route->SetDestination (header.GetDestination ());
			  route->SetOutputDevice (m_ipv4->GetNetDevice (m_ifaceId));
			  ucb (route, p, header);
			  return true;
			}
			
		  return false;
		 */
		  return true;
	}
	
	void IbrRoutingProtocol::NotifyInterfaceUp (uint32_t interface) {
      NS_LOG_FUNCTION (this << interface);
    }
    
	void IbrRoutingProtocol::NotifyInterfaceDown (uint32_t interface)  {
      NS_LOG_FUNCTION (this << interface);
    }
    
	void IbrRoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {
      NS_LOG_FUNCTION(this << interface << address << m_rtable);
      m_ifaceId = interface;
      m_address = address.GetLocal ();
      m_broadcast = address.GetBroadcast ();
    }
    
	void IbrRoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {
      NS_LOG_FUNCTION(this << interface << address);
    }
    
	void IbrRoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4) {
      NS_LOG_FUNCTION(this << ipv4);
      m_ipv4 = ipv4;
	}
    
	void IbrRoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const {
      *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ();
      m_rtable->Print (stream);
    }
    
	void IbrRoutingProtocol::SetRtable (Ptr<IbrRoutingTable> p) {
      NS_LOG_FUNCTION(p);
      m_rtable = p;
    }
	
	void IbrRoutingProtocol::SetWsTable (Ptr<IbrWaysegmentTable> wsT) {
	  NS_LOG_FUNCTION(wsT);
	  m_wsTable = wsT;
	}
	
	
	void IbrRoutingProtocol::RecvIbr (Ptr<Socket> socket) {
		  // Function defined for IBR packet reception and processing
		  // Uses the socket defined at RoutingProtocol startup
		   
		  //NS_LOG_DEBUG ("Node: " << ibr_node->GetId() << "\tReceived an IBR packet");
		  Ptr<Packet> receivedPacket;
		  Address sourceAddress;
		  receivedPacket = socket->RecvFrom (sourceAddress);

		  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
		  Ipv4Address receiverIfaceAddr = m_socketAddresses[socket].GetLocal ();
		  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());

		  // All routing messages are sent from and to port RT_PORT,
		  // so we check it.
		  NS_ASSERT (inetSourceAddr.GetPort () == IBR_PORT_NUMBER);

		  Ptr<Packet> packet1 = receivedPacket;
		  ////NS_LOG_DEBUG("Passing packet to RecvPacket function");
		  // if not from waysegment, discard
		  Ptr<Packet> copyPkt = receivedPacket->Copy();
		  IbrHostListTrailer ibrHostListTrailer;
		  copyPkt->RemoveHeader(ibrHostListTrailer);
		  uint32_t ori_ws = ibrHostListTrailer.GetWaysegment();

		  if (IbrRoutingProtocol::m_wsTable->isRegionInWaySegment(ibr_region, ori_ws)) {
	  		  IbrRoutingProtocol::RecvPacket(receivedPacket /*packet1*/);
		  }
	}

	
	void IbrRoutingProtocol::SendPacket (Ptr<Packet> packet) {
		  // IBR Packet send function
		  // Used with the defined socket
		  
		  //NS_LOG_DEBUG ("Node: " << ibr_node->GetId() << "\tsending an IBR packet");

		  // Trace it
		  // m_txPacketTrace (header, containedMessages);

		  // Send it
		  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
				 m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
			{
			  Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
			  i->first->SendTo (packet, 0, InetSocketAddress (bcast, IBR_PORT_NUMBER));
			  NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:IbrRP-PktTx\tNode:"<<ibr_node->GetId()<<"\tRegion:"<<ibr_region<<"\tPktSize:"<<packet->GetSize());
			}
	}


	void IbrRoutingProtocol::DoInitialize() {

	  // get node from main program/ 
	  ibr_node = m_ipv4->GetObject<Node>();
	  hostListSyncDirection=false; 
	  // get net device
	  // ibr_netDevice = m_ipv4->GetObject<NetDevice>();
	  ibr_netDevice = 0;	// default for net device
        for (uint32_t i = 0; i < ibr_node->GetNDevices(); i++) {
                if (dynamic_cast<NetDevice*> (PeekPointer(ibr_node->GetDevice(i)))) {
                    //if (IBR_DEBUG) //NS_LOG_DEBUG("NetDevice found in the node " << ibr_node->GetId());
                    ibr_netDevice=dynamic_cast<NetDevice*> (PeekPointer(ibr_node->GetDevice(i)));
					//ibr_vnLayer=ibr_node->GetObject<VirtualLayerPlusNetDevice> ();
                    break;
                }
        }
	    
		// get the virtual layer plus net device
		ibr_vnLayer = 0;
        for (uint32_t i = 0; i < ibr_node->GetNDevices(); i++) {
                if (dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(ibr_node->GetDevice(i)))) {
                    //if (IBR_DEBUG) //NS_LOG_DEBUG("Virtual Layer found in the node " << ibr_node->GetId());

                    ibr_vnLayer=dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(ibr_node->GetDevice(i)));
                    break;
                }
        }


	  // setup the socket communication between nodes
	  //if (IBR_DEBUG) //NS_LOG_DEBUG("Setting up IBR Socket in node: " << ibr_node->GetId() );
	  Ipv4Address loopback ("127.0.0.1");
	  if (m_mainAddress == Ipv4Address ())  {
			  //if (IBR_DEBUG) //NS_LOG_DEBUG(m_mainAddress);
			  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
				{
				  // Use primary address, if multiple
				  Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
				  if (addr != loopback)
					{
					  m_mainAddress = addr;
					  break;
					}
				}

			  NS_ASSERT (m_mainAddress != Ipv4Address ());
 	  }

//	  //if (IBR_DEBUG) //NS_LOG_DEBUG("Setting up IBR Socket list: " << IBR_PORT_NUMBER << "\tNInterfaces: " << m_ipv4->GetNInterfaces () );
	  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++) {
			  Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
			  if (addr == loopback)
				{
				  continue;
				}
			  // Create a socket to listen only on this interface
			  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
			  socket->SetAllowBroadcast (true);
			  InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal ()/* Ipv4Address::GetAny() */, IBR_PORT_NUMBER);
			  //socket->SetRecvCallback ( MakeCallback (&IbrRoutingProtocol::RecvIbr,  this));
			  // Setup Accept and Receive Callbacks
			  socket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>,const Address &> (),MakeCallback(&IbrRoutingProtocol::accept, this));
			  socket->SetRecvCallback ( MakeCallback (&IbrRoutingProtocol::RecvIbr,  this));
			  if (socket->Bind (inetAddr))
				{
				  NS_FATAL_ERROR ("Failed to bind() Ibr socket");
				}
			  socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
			  m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);
			  //if (IBR_DEBUG) //NS_LOG_DEBUG("Binding to " << m_ipv4->GetAddress(i,0));
      }

	  // Wait 1s to start the pingpong process
	  // because its needed that the vnLayer gets the region information
	  		m_PingPongStart.SetFunction(&IbrRoutingProtocol::TimerPingPongStart, this);
	  		m_PingPongStart.Schedule(TIME_START_PINGPONG);
	  	
	}

	void IbrRoutingProtocol::TimerPingPongStart() {
		// Timer for the PingPong process, to capture all the nodes in region information with the VNLayer
		ibr_region=ibr_vnLayer->GetRegion();
		ibr_state=ibr_vnLayer->GetState();
	    ibr_virtualNodeLevel=ibr_vnLayer->GetVirtualNodeLevel(ibr_region);
	  	ibr_waysegment=m_wsTable->getWaySegment(ibr_region);

		////NS_LOG_DEBUG("Checking if region: " << ibr_region << " is L2");  
		if (m_wsTable->isL2Region(ibr_region, true)==true) {
			// for debugging, check debug ws flag and enable PingPong
		  	IbrRoutingProtocol::PingPongStart();
			// arm the IncomingPacket timer and set it to load PingPongStart as callback
			// checking if not in debug mode
			if (IBR_DEBUG_PINGPONG_NODE==UINT32_MAX) {
				m_timerIncomingPacket.Schedule(TIME_INCOMING_PACKET);	
			} else {
				if (ibr_node->GetId()==IBR_DEBUG_PINGPONG_NODE) {
					m_timerIncomingPacket.Schedule(TIME_INCOMING_PACKET);			
				}
			}
		}

	}
	
	void IbrRoutingProtocol::TimerIncomingPacket() {
		// TimerIncomingPacket
		// To wait for a incoming packet
		
		if (IBR_DEBUG_PINGPONG_NODE==UINT32_MAX) {
			// start pingpong if its not in debug mode
			// //NS_LOG_DEBUG("Re Scheduling timer incoming packet on node: " << ibr_node->GetId());
			IbrRoutingProtocol::PingPongStart();
			m_timerIncomingPacket.Schedule(TIME_INCOMING_PACKET);
		}
	}
	
	
	void IbrRoutingProtocol::accept(Ptr<Socket> socket,const ns3::Address& from) {
	    //if (IBR_DEBUG) //NS_LOG_DEBUG("Connection accepted");
		socket->SetRecvCallback (MakeCallback (&IbrRoutingProtocol::RecvIbr,  this));
	}


	// ***************************************
	
		
	void IbrRoutingProtocol::PingPongStart() {
		// PingPong start process
		// Required to scan all the nodes in the RoadSegment, with the VNLayer
		
		//m_timerSyncHostsRegion.Schedule(TIME_SYNC_HOST_LIST);
		float t1 = ((rand()%100+100.0)/100.0);
		ibr_virtualIp = GetVirtualIpOf(ibr_region);
		ibr_waysegment = m_wsTable->getWaySegment(ibr_region);	
		// check for ibr debug ws
		
		if (IBR_DEBUG_PINGPONG_NODE==UINT32_MAX) {
			//if (IBR_DEBUG) //NS_LOG_DEBUG("Starting PING PONG on node: " << ibr_node->GetId() << " in Region: " << ibr_region << " with timeout: " << t1 );
			m_timerSyncHostsRegion.Schedule(ns3::Seconds(t1));
		} else {
			if (ibr_node->GetId()==IBR_DEBUG_PINGPONG_NODE) {	
				//if (IBR_DEBUG) //NS_LOG_DEBUG("Starting PING PONG on node: " << ibr_node->GetId() << " in Region: " << ibr_region << " with timeout: " << t1 );
	 			////NS_LOG_DEBUG("WS Check: " << ibr_waysegment << ":" << IBR_DEBUG_PINGPONG_NODE);
				m_timerSyncHostsRegion.SetFunction(&IbrRoutingProtocol::TimerSyncHostTable, this);
				m_timerSyncHostsRegion.Schedule(ns3::Seconds(t1));
			}
		}

		// **************
		
				//DoInitialize();
	}

	bool IbrRoutingProtocol::addHostToTable(IbrRegionHostTable::HostEntry host) {
		// adds host to the main host table
		ibr_hostsTable.push_back(host);
		return true;
	}

	bool IbrRoutingProtocol::addHostToTableDiscovery(IbrRegionHostTable::HostEntry host) {
		// adds host to the main host table
		ibr_hostsTable.push_back(host);
		return true;
	}

	Ipv4Address IbrRoutingProtocol::GetVirtualIpOf(uint32_t region) {
        	return Ipv4Address(VIRTUAL_IP_BASE_IBR + region + 1);
	}

	void IbrRoutingProtocol::TimerSyncHostTable() {
		// start the discovery table transmission
		// remove the hosts from the actual region from the discovery table
		//
		// update the ibr_region value from vnLayer
		// get the virtual layer plus net device
	  	
		ibr_region=ibr_vnLayer->GetRegion();
		ibr_state=ibr_vnLayer->GetState();
		ibr_virtualNodeLevel=ibr_vnLayer->GetVirtualNodeLevel(ibr_region);		

		//if (IBR_DEBUG) //NS_LOG_DEBUG("\tTimerSyncHostTable \tnode: " << ibr_node->GetId() << " (" << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() << ")");
		if (ibr_state==VirtualLayerPlusHeader::LEADER) {
			// remove the nodes from the actual region	
				if (IbrRoutingProtocol::ibr_hostsTableDiscovery.size()>0) {
						for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); i!=IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); ++i) {
							if (i->m_region==ibr_region) {
								i=IbrRoutingProtocol::ibr_hostsTableDiscovery.erase(i);
							}
						}
				}
			//if (IBR_DEBUG) //NS_LOG_DEBUG("\tTimerSyncHostTable*\tnode: " << ibr_node->GetId() << " (" << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() << ")");
			// add the hosts table to the discovery table
				IbrRoutingProtocol::ibr_hostsTable=IbrRoutingProtocol::ibr_vnLayer->ibr_hostsTable;

				
				if (IbrRoutingProtocol::ibr_hostsTable.size()>0) {
					//if (IBR_DEBUG) //NS_LOG_DEBUG("-- Hosts (" << IbrRoutingProtocol::ibr_hostsTable.size() << ")  detected on vnLayer node: " << ibr_node->GetId());
					for (std::list<IbrRegionHostTable::HostEntry>::iterator i = IbrRoutingProtocol::ibr_hostsTable.begin(); i!=IbrRoutingProtocol::ibr_hostsTable.end(); ++i) {
							
							IbrHostListTrailer::PayloadHostListEntry host(i->waySegment, i->m_region, i->ip, i->mac, ibr_ip, ibr_mac, ibr_virtualIp, i->BW, i->RSAT);
							//if (IBR_DEBUG) //NS_LOG_DEBUG("-- Added: " << i->ip);
							IbrRoutingProtocol::ibr_hostsTableDiscovery.push_back(host);
					}


				}

			//if (IBR_DEBUG) //NS_LOG_DEBUG("\tTimerSyncHostTable**\tnode: " << ibr_node->GetId() << " (" << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() << ")");


				if (ibr_virtualNodeLevel==2) {
					hostListSyncDirection=!hostListSyncDirection;
				}
				// send the host table
				////NS_LOG_DEBUG("Sending Packet with IBRSocket from node: " << ibr_node->GetId() << "\tregion: " << ibr_region);

				Ipv4Address bcast = Ipv4Address::GetBroadcast();
				Address macBcast = ibr_netDevice->GetBroadcast();
				
				IbrRoutingProtocol::SendSyncHostTable(VirtualLayerPlusHeader::VirtualToIbr, VirtualLayerPlusHeader::IbrDiscovery, bcast, macBcast);
				
				////NS_LOG_DEBUG("PingPong TX in IBR region: " << ibr_region << "\tws: " << ibr_waysegment << "[ " <<  GetWaySegment(ibr_region) << " ]\tnode: " << ibr_node->GetId() << "\tsize: " << ibr_hostsTable.size() << "\tDir: " << hostListSyncDirection );
				//m_timerSyncHostsRegion.Schedule(Seconds(1.0));
		}
	}


	bool IbrRoutingProtocol::SendSyncHostTable(VirtualLayerPlusHeader::type ibrMsgType, VirtualLayerPlusHeader::subType ibrMsgSubType, Ipv4Address dstIp, Address dstMac) {
		// Send the host table to the next leader
		Ptr<Packet> pkt;
		// create the trailer with the host list
		IbrHostListTrailer ibrHostListTrailer = IbrHostListTrailer();
		ibrHostListTrailer.ClearHostList();
		ibrHostListTrailer.SetData(IbrHostListTrailer::IbrDiscovery, ibr_waysegment, ibr_region, hostListSyncDirection);
		//if (IBR_DEBUG_PINGPONG_NODE!=UINT32_MAX) {
			//if (IBR_DEBUG) //NS_LOG_DEBUG("Sending ibr_hostsTableDiscovery table (" << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() << ") from node: " << ibr_node->GetId() );
		//}
		if (IbrRoutingProtocol::ibr_hostsTableDiscovery.size()>0) {
			for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); i != IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); ++i) {
						ibrHostListTrailer.AddHost(i->waySegment, i->m_region, i->leaderIp, i->leaderMac, i->hostIp, i->hostMac, i->virtualIp, i->BW, i->RSAT);
			}
		}

		// create the ibrhostlist packet and send over socket	
		Ptr<Packet> pkt1;
		pkt1=Create<Packet>();
		pkt1->AddHeader(ibrHostListTrailer);
		IbrRoutingProtocol::SendPacket(pkt1);
		return true;
	}

	bool IbrRoutingProtocol::RecvPacket(Ptr<Packet> pkt) {
		// Receive packet from virtual layer
		// Independent from the IBR socket communication
		Ptr<Packet> copyPkt = pkt->Copy();
	
		IbrHostListTrailer ibrHostListTrailer;

		copyPkt->RemoveHeader(ibrHostListTrailer);
		//uint32_t ori_ws = ibrHostListTrailer.GetWaysegment();
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:IbrRP-PktRx\tNode:"<<ibr_node->GetId()<<"\tRegion:"<<ibr_region<<"\tPktSize:"<<pkt->GetSize());
				if (ibrHostListTrailer.GetType()==IbrHostListTrailer::IbrDiscovery) {
				// process the HostList PingPong
					if (ibr_state==VirtualLayerPlusHeader::LEADER) {
							// process for the LEADER STATE
							////NS_LOG_DEBUG("Received Packet IbrDiscovery in region: " << ibr_region );
							
							//if (virtualLayerPlusHdr.GetSubType()==VirtualLayerPlusHeader::IbrDiscovery) {
							uint32_t ori_waysegment = ibrHostListTrailer.GetWaysegment();
							uint32_t ori_region = ibrHostListTrailer.GetRegion();
							hostListSyncDirection=ibrHostListTrailer.GetDirection();

							uint32_t pre_region = m_wsTable->GetRxSourceRegion(hostListSyncDirection, ibr_region);
							
							// check if the origin region is the allowed, depending on direction
							if (ibr_waysegment==ori_waysegment && ori_region==pre_region) {
								
									IbrRoutingProtocol::ibr_hostsTableDiscovery.clear();
									IbrRoutingProtocol::ibr_hostsTableDiscovery = ibrHostListTrailer.GetHostsList();	

									//if (IBR_DEBUG) //NS_LOG_DEBUG("Received ibr_hostsTableDiscovery table (" <<  IbrRoutingProtocol::ibr_hostsTableDiscovery.size() << ") in node: " << ibr_node->GetId() );

								
								// enable timer for next transmission
								if (ibr_virtualNodeLevel>1) {
									if (m_timerSyncHostsRegion.IsRunning()) {
										m_timerSyncHostsRegion.Cancel();
									}
									m_timerSyncHostsRegion.Schedule(TIME_SYNC_HOST_LIST);
								}

								// reschedule the incomingpacket timer
								if (m_timerIncomingPacket.IsRunning()) {
									m_timerIncomingPacket.Cancel();
									m_timerIncomingPacket.Schedule(TIME_INCOMING_PACKET);
								}
							}

							// ***********************************************************************
							// check if the packet comes from an intersection neighbour (node level 2)
							// and updates the intersection node list
							
							if (ibrHostListTrailer.GetNumHosts()>=0) {
								if (m_wsTable->IsIntersectionNeighbour(ibr_region, ori_region, hostListSyncDirection)) {
									//if (IBR_DEBUG) //NS_LOG_DEBUG("Intersection node: " << ibr_node->GetId() << "\t >> Receiving packet in region:" << ibr_region << "\tfrom region: " << ori_region); 
									// remove the nodes in the discovery list, from the actual waysegment
									uint16_t numNodesDelete=0;
									for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); i != IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); ++i) {
										if (i->waySegment==ori_waysegment) {
											i=IbrRoutingProtocol::ibr_hostsTableDiscovery.erase(i);
											numNodesDelete++;
										}				
									}
									////NS_LOG_DEBUG("Erasing nodes in intersection: " << ibr_region << " from WS: " << ibr_waysegment << " - "  << numNodesDelete << " nodes "); 
									
									
									// update the discovery list on this intersection node	reading the received packet		
									
									std::list<IbrHostListTrailer::PayloadHostListEntry> rxHostList = ibrHostListTrailer.GetHostsList();
									
									for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = rxHostList.begin(); i!=rxHostList.end(); i++) {
										// remove all items with the same IP address from the list
										Ipv4Address ipTemp = i->leaderIp;

										for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator j = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); j!=IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); j++) {
											if (ipTemp.IsEqual(j->hostIp)) {
												j=IbrRoutingProtocol::ibr_hostsTableDiscovery.erase(j);
											}
										}
											
										IbrHostListTrailer::PayloadHostListEntry host(i->waySegment, i->m_region, i->hostIp, i->hostMac, i->leaderIp, i->leaderMac, i->virtualIp, i->BW, i->RSAT);
										IbrRoutingProtocol::ibr_hostsTableDiscovery.push_back(host);
									}
								

									// remove the hosts in the local intersection 
									for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator j = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); j!=IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); j++) {
										if (j->m_region==ibr_region) {
											j=IbrRoutingProtocol::ibr_hostsTableDiscovery.erase(j);
										}
									}

									
									// update the discovery list, to add nodes in the local intersection

									if (ibr_hostsTable.size()>0) {
										for (std::list<IbrRegionHostTable::HostEntry>::iterator i = ibr_hostsTable.begin(); i!=ibr_hostsTable.end(); ++i) {
											// remove all items with the same IP address from the list
											Ipv4Address ipTemp1 = i->ip; 	// check refactoring of the list to correct the index of host and leader IP
											for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator j = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); j!=IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); j++) {
												if (ipTemp1.IsEqual(j->hostIp)) {
													j=IbrRoutingProtocol::ibr_hostsTableDiscovery.erase(j);
												}
											}

											IbrHostListTrailer::PayloadHostListEntry host(i->waySegment, i->m_region, ibr_ip, ibr_mac, i->ip, i->mac,  ibr_virtualIp, i->BW, i->RSAT);
											IbrRoutingProtocol::ibr_hostsTableDiscovery.push_back(host);
										}

									}
									
									
									// remove all items with the same IP address from the list
									////NS_LOG_DEBUG("Hosts befor: " << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() ) ;
									IbrRoutingProtocol::ibr_hostsTableDiscovery.unique();
									////NS_LOG_DEBUG("Hosts after: " << IbrRoutingProtocol::ibr_hostsTableDiscovery.size() ) ;
									
									// copy the host table to the vnLayer
									////NS_LOG_DEBUG("Copying intersection host table in node: " << ibr_node->GetId() << " to vnLayer");
									ibr_vnLayer->ReceiveHostsTableIbr(IbrRoutingProtocol::ibr_hostsTableDiscovery);
									
									// print the host table in the intersection
									printDiscoveryHostsTable();
									NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:IbrRP-InterUpdateList\tNode:"<<ibr_node->GetId()<<"\tRegion:"<<ibr_region<<"\tHostNum:"<<ibr_hostsTableDiscovery.size());
								}

							}
							
						}
				}
				
				if (ibrHostListTrailer.GetType()==IbrHostListTrailer::IbrHostToIntersection) {
					// take a packet from a host and transmit it from leader to leader
					// to the intersection in the actual pingpong direction
					if (ibr_state==VirtualLayerPlusHeader::LEADER) {
						if (ibr_region==ibrHostListTrailer.GetRegion()) {
							//NS_LOG_DEBUG("*** Received packet IbrHostToIntersection in region " << ibr_region << " in node: " << ibr_node->GetId());	
							// send the packet to the next leader
							pkt = Create<Packet>();
							
							// create the trailer with the host list
							IbrHostListTrailer ibrHostListTrailer = IbrHostListTrailer();
							ibrHostListTrailer.ClearHostList();
							ibrHostListTrailer.SetData(IbrHostListTrailer::IbrResourceRequest, ibr_waysegment, ibr_region, hostListSyncDirection);

							// copy the scma packet
							Ptr<Packet> copyPktScma = copyPkt->Copy();

							pkt->AddHeader(ibrHostListTrailer);
							//pkt->AddHeader(virtualLayerPlusHdr);
							pkt->AddAtEnd(copyPktScma);

							//ibr_netDevice->Send(pkt, ibr_netDevice->GetBroadcast(), 0);
							IbrRoutingProtocol::SendPacket(pkt);


							//NS_LOG_DEBUG("*** Sending IbrToIntersection from node: " << ibr_node->GetId() << " Direction: " << hostListSyncDirection);
						
						}
					}
				}
			
				if (ibrHostListTrailer.GetType()==IbrHostListTrailer::IbrResourceRequest) {
					// packet sent to intersection with SCMA information
				
						
					if (ibr_state==VirtualLayerPlusHeader::LEADER) {

						////NS_LOG_DEBUG("** Received IbrToIntersection packet in node: " << ibr_node->GetId());
						uint32_t ori_waysegment = ibrHostListTrailer.GetWaysegment();
						uint32_t ori_region = ibrHostListTrailer.GetRegion();
						hostListSyncDirection=ibrHostListTrailer.GetDirection();
						//uint32_t pre_region = GetRxSourceRegion(hostListSyncDirection);
						uint32_t pre_region = m_wsTable->GetRxSourceRegion(hostListSyncDirection, ibr_region);
						// if its from a neighbour region, tx to the next one
						if (ibr_waysegment==ori_waysegment && ori_region==pre_region)  {
								//NS_LOG_DEBUG("**** Received IbrToIntersection packet in node: " << ibr_node->GetId() << " Reg: " << ibr_region << " reg: " << ori_region ) ;

								// create a new packet, add the actual one and send to the next leader

								//IbrRoutingProtocol::SendSyncHostTable(VirtualLayerPlusHeader::VirtualToIbr, VirtualLayerPlusHeader::IbrDiscovery, Ipv4Address::GetBroadcast(), ibr_netDevice->GetBroadcast());
								pkt = Create<Packet>();
								VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();
								virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::VirtualToIbr);
								virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::IbrDiscovery);
								virtualLayerPlusHdr.SetRegion(ibr_region);
								virtualLayerPlusHdr.SetSrcAddress(ibr_ip);
								virtualLayerPlusHdr.SetSrcMAC(ibr_mac);
								virtualLayerPlusHdr.SetDstAddress(Ipv4Address::GetBroadcast());
								virtualLayerPlusHdr.SetDstMAC(ibr_netDevice->GetBroadcast());
								virtualLayerPlusHdr.SetRol(VirtualLayerPlusHeader::LEADER);
								
								// create the trailer with the host list
								IbrHostListTrailer ibrHostListTrailer = IbrHostListTrailer();
								ibrHostListTrailer.ClearHostList();
								ibrHostListTrailer.SetData(IbrHostListTrailer::IbrResourceRequest, ibr_waysegment, ibr_region, hostListSyncDirection);

								pkt->AddHeader(ibrHostListTrailer);
							
								Ptr<Packet> copyPkt1 = copyPkt->Copy();
								
								pkt->AddAtEnd(copyPkt1);
								IbrRoutingProtocol::SendPacket(pkt);

								//NS_LOG_DEBUG("**** Sending IbrToIntersection packet from node: " << ibr_node->GetId() << " Direction: " << hostListSyncDirection);
						}
						// if its received on the intersection, send the resources
						
						if (m_wsTable->IsIntersectionNeighbour(ibr_region, ori_region, !hostListSyncDirection)==true) {
								// uses !hostListSyncDirection because the packet is sent to the intersection without changing direction, as in the discovery process.
								//NS_LOG_DEBUG("***** Received packet on intersection node: " << ibr_node->GetId());

						}



					}

				}
		//}
		return true;
	}

	void IbrRoutingProtocol::printDiscoveryHostsTable() {
		// print the intersection node table
		int numRegs=0;
		if (IbrRoutingProtocol::ibr_hostsTableDiscovery.size()>0) {

			NS_LOG_INFO("--Discovery Table on Intersection - Region: " << ibr_region << " node: " << ibr_node->GetId());					
				NS_LOG_INFO("#\tWS\tReg\tlIp            \thIp            \tvIp       \tBW\tRSAT");	
				for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = IbrRoutingProtocol::ibr_hostsTableDiscovery.begin(); i != IbrRoutingProtocol::ibr_hostsTableDiscovery.end(); ++i) {
					NS_LOG_INFO(numRegs << " \t" << i->waySegment << "\t" << i->m_region << "\t" << i->leaderIp << "\t" << i->hostIp << "\t" << i->virtualIp << "\t" << i->BW << "\t" << i->RSAT);					
					numRegs++;
				}
				NS_LOG_INFO(" \t--\t---\t---            \t---           \t---      \t--\t----");	
		}
	}

		
}

