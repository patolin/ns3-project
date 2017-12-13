/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <float.h>
#include <math.h>
#include <iostream> 


#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-l3-protocol.h"

#include "virtual-layer-plus-payload-header.h"
#include "virtual-layer-plus-header.h"
#include "virtual-layer-plus-net-device.h"
//#include "ns3/scma-statemachine.h"

#include "ns3/mobility-model.h"
#include "ns3/virtual-layer-plus-net-device.h"
#include "src/network/utils/ipv4-address.h"
#include "src/network/model/packet.h"
#include "helper/virtual-layer-plus-helper.h"
#include "src/core/model/log.h"
#include "src/internet/model/arp-l3-protocol.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/routing-table-header.h"

//#include "ns3/ibr-routing-protocol.h"
#include "ns3/ibr-hostlist-trailer.h"


#define TIME_HEARTBEAT ns3::Seconds(2)
#define TIME_REQUEST_WAIT ns3::Seconds(0.15)
#define TIME_RELINQUISHMENT ns3::Seconds(0.5)
#define TIME_RETRY_SYNC_DATA ns3::Seconds(0.05)
#define TIME_LIFE_PACKET_ENTRY ns3::Seconds(1)
#define TIME_HELLO_INTERVAL ns3::Seconds(1.0)
#define TIME_LIFE_HOST_ENTRY 2*TIME_HELLO_INTERVAL
#define TIME_LEADER_CEASE ns3::Seconds(1)
#define TIME_WAIT_FOR_NEW_LEADER ns3::Seconds(1)
#define TIME_LIFE_NEIGHBOR_LEADER_ENTRY 2*TIME_HEARTBEAT
#define CURRENT_TIME ns3::Simulator::Now()

#define BACKUPS_PERCENTAGE 0.5
#define MIN_NUM_BACKUPS_PER_LEADER 2

#define INITIAL_QUEUE_SIZE_THRESHOLD 10
#define MIN_QUEUE_SIZE_THRESHOLD 5

#define MAX_LAST_LEADERS_PER_ID 3

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

#define POSITION_TIMER_INTERVAL 0.1
#define TIME_DISCOVERY ns3::Seconds(0.5)
#define TIME_GET_NODE_POSITION ns3::Seconds(POSITION_TIMER_INTERVAL)

// define ID of node in the center of the map, to have wifi access
// automatic sets 4 nodes in upper and lower intersections
#define WIFI_CENTER_REGION 544 		// UINT32MAX // to disable
#define WIFI_OFFSET_REGION_1 256
#define WIFI_OFFSET_REGION_2 272
#define WIFI_BW	24						// WIFI bandwidth for defined regions


#define TIME_LAUNCH_AUGMENTATION ns3::Seconds(10.0)
#define SCMA_DEBUG_REGION 543
#define SCMA_NODE_TEST 1//111


NS_LOG_COMPONENT_DEFINE("VirtualLayerPlusNetDevice");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(VirtualLayerPlusNetDevice);

    TypeId VirtualLayerPlusNetDevice::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::VirtualLayerPlusNetDevice")
                .SetParent<NetDevice> ()
                .AddConstructor<VirtualLayerPlusNetDevice> ()
                .AddAttribute("Long",
                "The long of the simulation scenario",
                UintegerValue(1000),
                MakeDoubleAccessor(&VirtualLayerPlusNetDevice::m_long),
                MakeDoubleChecker<double> ())
                .AddAttribute("Width",
                "The width of the simulation scenario",
                UintegerValue(1000),
                MakeDoubleAccessor(&VirtualLayerPlusNetDevice::m_width),
                MakeDoubleChecker<double>())
                .AddAttribute("Rows",
                "The number of rows of the simulation scenario",
                UintegerValue(10),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_rows),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("Columns",
                "The number of columns of the simulation scenario",
                UintegerValue(10),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_columns),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("scnGridX",
                "X grid value of the Manhatann simulation scenario",
                UintegerValue(7),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_scnGridX),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("scnGridY",
                "Y grid value of the Manhatann simulation scenario",
                UintegerValue(7),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_scnGridY),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("scnRegBetIntX",
                "Regions between intersections X value of the Manhatann simulation scenario",
                UintegerValue(1),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_scnRegBetIntX),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("scnRegBetIntY",
                "Regions between intersections Y value of the Manhatann simulation scenario",
                UintegerValue(1),
                MakeUintegerAccessor(&VirtualLayerPlusNetDevice::m_scnRegBetIntY),
                MakeUintegerChecker<uint32_t> ())
                .AddAttribute("Supportive_Backups",
                "Make backups more supportive of the leader's work",
                BooleanValue(false),
                MakeBooleanAccessor(&VirtualLayerPlusNetDevice::m_supportiveBackups),
                MakeBooleanChecker())
                .AddAttribute("Multiple_Leaders",
                "More than one leader can exist in one region in a overloading situation",
                BooleanValue(false),
                MakeBooleanAccessor(&VirtualLayerPlusNetDevice::m_multipleLeaders),
                MakeBooleanChecker())
                .AddTraceSource("Tx", "Send - packet (including VNLayer header), VirtualLayerPlusNetDevice Ptr, interface index.",
                MakeTraceSourceAccessor(&VirtualLayerPlusNetDevice::m_txTrace),
                "ns3::VirtualLayerPlusNetDevice::RxTxTracedCallback")
                .AddTraceSource("Rx", "Receive - packet (including VNLayer header), VirtualLayerPlusNetDevice Ptr, interface index.",
                MakeTraceSourceAccessor(&VirtualLayerPlusNetDevice::m_rxTrace),
                "ns3::VirtualLayerPlusNetDevice::RxTxTracedCallback")
                .AddTraceSource("Drop", "Drop - DropReason, packet (including VNLayer header), VirtualLayerPlusNetDevice Ptr, interface index.",
                MakeTraceSourceAccessor(&VirtualLayerPlusNetDevice::m_dropTrace),
                "ns3::VirtualLayerPlusNetDevice::DropTracedCallback")
                .AddTraceSource("Region", "Value of the region identifier to trace.",
                MakeTraceSourceAccessor(&VirtualLayerPlusNetDevice::m_region),
                "ns3::Traced::Value::Uint32Callback")
                .AddTraceSource("State", "Value of the state to trace.",
                MakeTraceSourceAccessor(&VirtualLayerPlusNetDevice::m_stateTrace),
                "ns3::VirtualLayerPlusNetDevice::StateTracedCallback")
				// waysegment table from main program
				.AddAttribute ("WaysegmentTable", "Table of waysegments.",
				   PointerValue (),
				   MakePointerAccessor (&VirtualLayerPlusNetDevice::SetWsTable	),
				   MakePointerChecker<IbrWaysegmentTable>() 
				   )
				// *******************************
				;
        return tid;
    }

    VirtualLayerPlusNetDevice::VirtualLayerPlusNetDevice()
    : m_timerRequestWait(Timer::CANCEL_ON_DESTROY),
    m_timerHeartbeat(Timer::CANCEL_ON_DESTROY),
    m_timerGetLocation(Timer::CANCEL_ON_DESTROY),
    m_timerBackUpRequest(Timer::CANCEL_ON_DESTROY),
    m_timerRelinquishment(Timer::CANCEL_ON_DESTROY),
    m_timerRetrySyncData(Timer::CANCEL_ON_DESTROY),
    m_timerLeaderCease(Timer::CANCEL_ON_DESTROY),
    m_timerHello(Timer::CANCEL_ON_DESTROY),
    m_node(0),
    m_netDevice(0),
    m_ifIndex(0) {

        NS_LOG_FUNCTION(this);
        //PacketMetadata::Enable();
        m_rng = CreateObject<UniformRandomVariable> ();
        srand(time(NULL));

        m_timerRequestWait.SetFunction(&VirtualLayerPlusNetDevice::TimerRequestWaitExpire, this);
        m_timerHeartbeat.SetFunction(&VirtualLayerPlusNetDevice::TimerHeartbeatExpire, this);
        m_timerGetLocation.SetFunction(&VirtualLayerPlusNetDevice::TimerGetLocationExpire, this);
        m_timerBackUpRequest.SetFunction(&VirtualLayerPlusNetDevice::TimerBackUpRequestExpire, this);
        m_timerRelinquishment.SetFunction(&VirtualLayerPlusNetDevice::TimerRelinquishmentExpire, this);
        m_timerRetrySyncData.SetFunction(&VirtualLayerPlusNetDevice::TimerRetrySyncDataExpire, this);
        m_timerLeaderCease.SetFunction(&VirtualLayerPlusNetDevice::TimerLeaderCeaseExpire, this);
        m_timerHello.SetFunction(&VirtualLayerPlusNetDevice::TimerHelloExpire, this);


		m_timerLaunchAugmentation.SetFunction(&VirtualLayerPlusNetDevice::TimerLaunchAugmentation, this);


        m_region = 0; //unknown;
        m_stateTrace(m_state, VirtualLayerPlusHeader::INITIAL, m_region);
        m_state = VirtualLayerPlusHeader::INITIAL;
        m_timeInState = 0;
        m_expires = 0;

        m_leaderID = UINT32_MAX;
        m_leaderTable.clear();
        m_newLeaderActive = false;
        m_replaced = false;

        m_neighborLeadersTable.clear();

        m_lastLeadersTable.clear(); //******

        ClearBackUp();

        for (int i = 0; i < 20; i++) {
            m_emptyMAC[i] = 0x00;
        }



    }

    Ptr<NetDevice> VirtualLayerPlusNetDevice::GetNetDevice() const {
        NS_LOG_FUNCTION(this);
        return m_netDevice;
    }

    void VirtualLayerPlusNetDevice::SetNetDevice(Ptr<NetDevice> device) {
        NS_LOG_FUNCTION(this << device);
        m_netDevice = device;

        //NS_LOG_DEBUG("RegisterProtocolHandler for " << device->GetInstanceTypeId().GetName());

        uint16_t protocolType = 0;

        m_node->RegisterProtocolHandler(MakeCallback(&VirtualLayerPlusNetDevice::ReceiveFromDevice,
                this),
                protocolType, device, false);

        m_node->RegisterProtocolHandler(MakeCallback(&VirtualLayerPlusNetDevice::PromiscReceiveFromDevice,
                this),
                protocolType, device, true);

        Simulator::ScheduleNow(&VirtualLayerPlusNetDevice::Start, this);
    }

    int64_t VirtualLayerPlusNetDevice::AssignStreams(int64_t stream) {
        NS_LOG_FUNCTION(this << stream);
        m_rng->SetStream(stream);
        return 1;
    }

    void VirtualLayerPlusNetDevice::DoDispose() {
        NS_LOG_FUNCTION(this);

        m_netDevice = 0;
        m_node = 0;

        NetDevice::DoDispose();
    }

    void VirtualLayerPlusNetDevice::ReceiveFromDevice(Ptr<NetDevice> incomingPort,
            Ptr<const Packet> packet,
            uint16_t protocol,
            Address const &src,
            Address const &dst,
            PacketType packetType) {
        NS_LOG_FUNCTION(this << incomingPort << packet << protocol << src << dst);
        ////NS_LOG_DEBUG("UID is " << packet->GetUid());

        Ptr<Packet> copyPkt = packet->Copy();

		//NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:vnLayerPND-PktRx\tNode:"<<m_node->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<copyPkt->GetSize());
        
		m_rxTrace(copyPkt, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

        ////NS_LOG_DEBUG("Packet received: " << *copyPkt);
        ////NS_LOG_DEBUG("Packet length: " << copyPkt->GetSize());
	

		// *************************
		// Check if its a SCMA packet
		/*
	 	bool scmaProcessed=false;	
		ScmaMessageHeader scmaHeader;
		copyPkt->RemoveHeader(scmaHeader);

		switch(scmaHeader.GetType()){
			case ScmaMessageHeader::T_SCMA:
				// Packet from App Layer redirected to SCMA layer
				////NS_LOG_DEBUG("***\tTSCMA pkt in node: " << m_node->GetId());
				nodeSM.AugmentationRequest(packet->Copy());
				scmaProcessed=true;
				break;
			default:
				break;

		}		
		if (scmaProcessed) {
			return;
		}
		*/
		// *************************
		
		
		copyPkt = packet->Copy();


        VirtualLayerPlusHeader virtualLayerPlusHdr;
        copyPkt->RemoveHeader(virtualLayerPlusHdr);


        switch (virtualLayerPlusHdr.GetType()) {
			
			case VirtualLayerPlusHeader::VirtualToScma:
				// send packet to scma layer
				nodeSM.RecvPkt(copyPkt->Copy());	
				break;
			/*
			case VirtualLayerPlusHeader::VirtualToIbr:
				//Send packet to IBR layer, to the PingPong process;
				//Verify if its on the same waysegment, to send to the ibr layer
				//NS_LOG_DEBUG("Recv IBR in node: " << m_node->GetId() << "\tWS: " << m_waysegment);

				if (IsRegionInWaySegment(virtualLayerPlusHdr.GetRegion(), m_waysegment)==true) {					 
									//if (m_state==VirtualLayerPlusHeader::LEADER) {
				
					// VirtualLayerPlusNetDevice::ibr.RecvPacket(copyPkt);
					
					
					//VirtualLayerPlusNetDevice::ibr.RecvPacket(packet->Copy());						
				}
				break;
            */
			case VirtualLayerPlusHeader::LeaderElection:
                RecvLeaderElection(packet->Copy());
                break;
            case VirtualLayerPlusHeader::Synchronization:
                RecvSynchronization(packet->Copy());
                break;
            case VirtualLayerPlusHeader::Hello:
                RecvHello(packet->Copy());
                break;
            case VirtualLayerPlusHeader::Application:
                switch (virtualLayerPlusHdr.GetSubType()) {
                    case VirtualLayerPlusHeader::ArpRequest:
                    {
                        // Ignoramos paquetes ARP que no sean de nuestra región ni de una región vecina
                        if (!IsNeighborRegion(virtualLayerPlusHdr.GetRegion()) && (virtualLayerPlusHdr.GetRegion() != m_region)) break;

                        ArpHeader arpHdr;
                        copyPkt->PeekHeader(arpHdr);

                        if (m_state == VirtualLayerPlusHeader::LEADER) {
                            // Va dirigido a la IP virtual de mi región y soy líder
                            if (arpHdr.GetDestinationIpv4Address().Get() == VIRTUAL_IP_BASE + m_region + 1) {
                                ArpHeader arpReplyHdr;
                                NS_LOG_INFO("ARP: sending reply from node " << m_node->GetId() <<
                                        "|| src: " << GetVirtualAddressOf(m_region) <<
                                        " / " << arpHdr.GetDestinationIpv4Address() <<
                                        " || dst: " << arpHdr.GetSourceHardwareAddress() << " / " << arpHdr.GetSourceIpv4Address());
                                arpReplyHdr.SetReply(GetVirtualAddressOf(m_region), arpHdr.GetDestinationIpv4Address(), arpHdr.GetSourceHardwareAddress(), arpHdr.GetSourceIpv4Address());
                                Ptr<Packet> pkt = Create<Packet> ();
                                pkt->AddHeader(arpReplyHdr);

                                VirtualLayerPlusHeader virtualLayerPlusReplyHdr = VirtualLayerPlusHeader();

                                virtualLayerPlusReplyHdr.SetType(VirtualLayerPlusHeader::Application);
                                virtualLayerPlusReplyHdr.SetSubType(VirtualLayerPlusHeader::ArpReply);
                                virtualLayerPlusReplyHdr.SetRegion(m_region);
                                virtualLayerPlusReplyHdr.SetSrcAddress(m_ip);
                                virtualLayerPlusReplyHdr.SetSrcMAC(m_mac);
                                virtualLayerPlusReplyHdr.SetDstAddress(arpReplyHdr.GetDestinationIpv4Address());
                                virtualLayerPlusReplyHdr.SetDstMAC(arpReplyHdr.GetDestinationHardwareAddress());
                                virtualLayerPlusReplyHdr.SetRol(m_state);
                                virtualLayerPlusReplyHdr.SetLeaderID(m_leaderID);
                                virtualLayerPlusReplyHdr.SetTimeInState(m_timeInState);
                                virtualLayerPlusReplyHdr.SetNumBackUp(m_backUpTable.size());
                                virtualLayerPlusReplyHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
                                virtualLayerPlusReplyHdr.SetBackUpPriority(m_backUpPriority);

                                pkt->AddHeader(virtualLayerPlusReplyHdr);

                                m_txTrace(pkt, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

                                m_netDevice->Send(pkt, arpReplyHdr.GetDestinationHardwareAddress(), protocol);

                                break; // Si no enviamos respuesta seguiríamos en el siguiente case (default)
                            }

                        }
                    }

                    default:
                        if ((virtualLayerPlusHdr.GetSubType() == VirtualLayerPlusHeader::ArpReply)) {
                            ArpHeader arpHdr;
                            copyPkt->PeekHeader(arpHdr);

                            if (IsVirtual(arpHdr.GetSourceHardwareAddress()) && IsNeighborRegion(virtualLayerPlusHdr.GetRegion())) {
                                NS_LOG_INFO("Recibido ArpReply del nodo virtual " << arpHdr.GetSourceIpv4Address() << " en el nodo " << m_node->GetId());
                                PutNeighborLeader(virtualLayerPlusHdr.GetRegion(), virtualLayerPlusHdr.GetLeaderID(), virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                                PrintNeighborLeadersTable();
                            }
                        }
                        NS_LOG_INFO("Paquete de aplicación recibido por la VNLayer! --> Enviándolo hacia arriba...");
                        if (!m_promiscRxCallback.IsNull()) {
                            m_promiscRxCallback(this, copyPkt, protocol, src, dst, packetType);
                        }
                        m_rxCallback(this, copyPkt, protocol, src);
                }

                break;
            
            default:;


        }

        // Comprobamos tamaño de la cola de transmisión del líder
        if ((m_state == VirtualLayerPlusHeader::LEADER) && (m_multipleLeaders)) {
            // Habría que evitar que un mismo líder genere más de uno?

            Ptr<WifiNetDevice> dev = dynamic_cast<WifiNetDevice*> (PeekPointer(m_netDevice));
            NS_ASSERT(dev != NULL);
            Ptr<OcbWifiMac> mac = dynamic_cast<OcbWifiMac*> (PeekPointer(dev->GetMac()));
            //Ptr<AdhocWifiMac> mac = dynamic_cast<AdhocWifiMac*> (PeekPointer(dev->GetMac()));
            NS_ASSERT(mac != NULL);
            PointerValue ptr;
            mac->GetAttribute("DcaTxop", ptr);
            Ptr<DcaTxop> dca = ptr.Get<DcaTxop> ();
            NS_ASSERT(dca != NULL);
            Ptr<WifiMacQueue> queue = dca->GetQueue();
            NS_ASSERT(queue != NULL);

            if (queue->GetSize() == 0) {
                if ((m_leaderID != 0) && IsTheLatestLeader(m_leaderID))
                    if (!m_timerLeaderCease.IsRunning()) m_timerLeaderCease.Schedule(TIME_LEADER_CEASE);
            } else {
                if (m_timerLeaderCease.IsRunning()) m_timerLeaderCease.Cancel();

                if (queue->GetSize() > MIN(MIN_QUEUE_SIZE_THRESHOLD, std::floor((float) INITIAL_QUEUE_SIZE_THRESHOLD / (float) m_leaderTable.size()))) {
                    NS_LOG_INFO("****************** Queue size threshold exceeded in " << (queue->GetSize() - MIN(MIN_QUEUE_SIZE_THRESHOLD, std::floor((float) INITIAL_QUEUE_SIZE_THRESHOLD / (float) m_leaderTable.size()))) << " packets at leader " << m_leaderID << " ******************");

                    // Enviamos al Priority BackUp una petición para que nos ayude (actúe como líder)
                    BackUpTableHeader::BackUpEntry priorityBackUp = GetPriorityBackUpOf(m_leaderID);

                    if (!priorityBackUp.ip.IsBroadcast() && priorityBackUp.isSynced && (!m_newLeaderActive) && (m_leaderID == 0)) {

                        NS_LOG_INFO("Requesting new leader becoming...");
                        SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderBecomeRequest, priorityBackUp.ip, priorityBackUp.mac);
                    }
                }
            }
        }
        return;
    }

    void VirtualLayerPlusNetDevice::PromiscReceiveFromDevice(Ptr<NetDevice> incomingPort,
            Ptr<const Packet> packet,
            uint16_t protocol,
            Address const &src,
            Address const &dst,
            PacketType packetType) {

        if (m_promiscuousMode) {

            Ptr<Packet> copyPkt = packet->Copy();

            VirtualLayerPlusHeader virtualLayerPlusHdr;
            copyPkt->RemoveHeader(virtualLayerPlusHdr);

            if (((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) && (m_backUpState == VirtualLayerPlusNetDevice::SYNC)) {

                LeaderEntry lider = GetLeader(m_supportedLeaderID);

                // Paquete (Que no sea propio del protocolo) para el líder (O para alguno de sus predecesores más recientes)
                if ((!lider.ip.IsBroadcast()) && // En un momento dado el líder correspondiente a SupportedLeaderID puede estar caído
                        ((virtualLayerPlusHdr.GetDstMAC() == lider.mac) || (IsInLastLeadersTable(virtualLayerPlusHdr.GetDstMAC(), m_supportedLeaderID))) &&
                        (virtualLayerPlusHdr.GetType() == VirtualLayerPlusHeader::Application) &&
                        (virtualLayerPlusHdr.GetSubType() != VirtualLayerPlusHeader::ArpRequest) &&
                        (virtualLayerPlusHdr.GetSubType() != VirtualLayerPlusHeader::ArpReply)) {

                    NS_LOG_INFO("Paquete para el líder " << m_supportedLeaderID << " recibido en MODO PROMISCUO en el nodo " << m_node->GetId() << "!!!\n");

                    NS_LOG_FUNCTION(this << incomingPort << packet << protocol << src << dst);
                    //NS_LOG_DEBUG("UID is " << packet->GetUid());

                    m_rxTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

                    //NS_LOG_DEBUG("Packet received: " << *packet);
                    //NS_LOG_DEBUG("Packet length: " << packet->GetSize());

                    Ipv4Header ipHdr;

                    copyPkt->PeekHeader(ipHdr);

                    if (ipHdr.GetDestination().IsEqual(lider.ip) || ipHdr.GetDestination().IsEqual(GetVirtualIpOf(m_region))) {
                        if (virtualLayerPlusHdr.GetDstMAC() != lider.mac) {
                            if (ipHdr.GetDestination().IsEqual(GetVirtualIpOf(m_region))) {
                                NS_LOG_INFO("El destino final del paquete " << packet->GetUid() << " es nuestro VN pero va dirigido un líder anterior --> Lo reenviamos directamente al líder actual\n");

                                NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << copyPkt);
                                Send(copyPkt, GetVirtualAddressOf(m_region), protocol);
                            } else {
                                NS_LOG_INFO("El destino final del paquete " << packet->GetUid() << " es líder anterior referenciado como entidad física --> Lo procesamos\n");
                                EnqueuePacket(packet->Copy(), protocol, false);
                                // Enviamos el paquete hacia arriba para que el protocolo de enrutado consulte a dónde enviarlo (RouteInput).
                                // De este modo mantenemos las tablas de enrutado actualizadas en el BackUp aunque después no tengamos que enviarlo.
                                if (!m_promiscRxCallback.IsNull()) {
                                    m_promiscRxCallback(this, copyPkt, protocol, src, dst, packetType);
                                }
                                m_rxCallback(this, copyPkt, protocol, src);
                            }
                        } else {
                            NS_LOG_INFO("El destino final del paquete " << packet->GetUid() << " es el propio líder --> No lo procesamos\n");
                        }
                    } else {
                        NS_LOG_INFO("Encolando y procesando paquete " << packet->GetUid() << " en el nodo " << m_node->GetId() << " (BackUp)...\n");

                        // Encolamos el paquete por si fuese retransmitido antes de que lo hayamos procesado
                        EnqueuePacket(packet->Copy(), protocol, false);
                        // Enviamos el paquete hacia arriba para que el protocolo de enrutado consulte a dónde enviarlo (RouteInput).
                        // De este modo mantenemos las tablas de enrutado actualizadas en el BackUp aunque después no tengamos que enviarlo.
                        if (!m_promiscRxCallback.IsNull()) {
                            m_promiscRxCallback(this, copyPkt, protocol, src, dst, packetType);
                        }
                        m_rxCallback(this, copyPkt, protocol, src);

                    }

                    // Paquete del líder o de uno de sus backups (Que no sea propio del protocolo)
                } else if ((((virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::LEADER) || (virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::INTERIM)) &&
                        (virtualLayerPlusHdr.GetLeaderID() == m_supportedLeaderID) &&
                        (virtualLayerPlusHdr.GetRegion() == m_region) &&
                        (virtualLayerPlusHdr.GetSubType() == VirtualLayerPlusHeader::ForwardedByLeader)) ||
                        (((virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::NONLEADER) || (virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::REQUEST)) &&
                        (virtualLayerPlusHdr.GetLeaderID() == m_supportedLeaderID) &&
                        (virtualLayerPlusHdr.GetRegion() == m_region) &&
                        (virtualLayerPlusHdr.GetSubType() == VirtualLayerPlusHeader::ForwardedByBackUp))) {

                    // Si no conocíamos al Líder que hizo el reenvío, lo añadimos a la LeaderTable
                    if ((virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::LEADER) && (!IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress()))) {
                        PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                        PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                        PrintLeaderTable();
                    }

                    // Si no conocíamos al BackUp que hizo el reenvío, lo añadimos a la BackUpTable
                    if ((virtualLayerPlusHdr.GetRol() == VirtualLayerPlusHeader::NONLEADER) && (!IsInBackUpTable(virtualLayerPlusHdr.GetSrcAddress()))) {
                        PutBackUp(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetBackUpPriority());
                        SyncBackUp(virtualLayerPlusHdr.GetSrcAddress());
                        PrintBackUpTable();
                    }

                    NS_LOG_INFO("Paquete del líder " << m_supportedLeaderID << " recibido en MODO PROMISCUO en el nodo " << m_node->GetId() << "!!!\n");

                    NS_LOG_FUNCTION(this << incomingPort << packet << protocol << src << dst);
                    //NS_LOG_DEBUG("UID is " << packet->GetUid());

                    m_rxTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

                    //NS_LOG_DEBUG("Packet received: " << *packet);
                    //NS_LOG_DEBUG("Packet length: " << packet->GetSize());

                    NS_LOG_INFO("Desencolando paquete " << packet->GetUid() << " en el nodo " << m_node->GetId() << "...\n");
                    DequeuePacketWithUid(packet->GetUid());
                }
            } else if (m_state == VirtualLayerPlusHeader::LEADER) {
                /*
                if ((virtualLayerPlusHdr.GetRegion() == m_region)) {
                    if ((virtualLayerPlusHdr.GetSubType() != VirtualLayerPlusHeader::BackUpLeft) && (virtualLayerPlusHdr.GetSubType() != VirtualLayerPlusHeader::LeaderLeft)) {
                        NS_LOG_INFO("----------> Insertamos " << virtualLayerPlusHdr.GetSrcAddress() << " en la HostTable");
                        PutHost(virtualLayerPlusHdr.GetSrcAddress());
                    } else QuitHost(virtualLayerPlusHdr.GetSrcAddress());
                }
                 */
            }
        }
    }

    void VirtualLayerPlusNetDevice::SetIfIndex(const uint32_t index) {

        NS_LOG_FUNCTION(this << index);
        m_ifIndex = index;
    }

    uint32_t VirtualLayerPlusNetDevice::GetIfIndex(void) const {
        NS_LOG_FUNCTION(this);

        return m_ifIndex;
    }

    Ptr<Channel> VirtualLayerPlusNetDevice::GetChannel(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->GetChannel();
    }

    void VirtualLayerPlusNetDevice::SetAddress(Address address) {

        NS_LOG_FUNCTION(this << address);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        m_netDevice->SetAddress(address);
    }

    Address VirtualLayerPlusNetDevice::GetAddress(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->GetAddress();
    }

    bool VirtualLayerPlusNetDevice::SetMtu(const uint16_t mtu) {
        NS_LOG_FUNCTION(this << mtu);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->SetMtu(mtu);
    }

    uint16_t VirtualLayerPlusNetDevice::GetMtu(void) const {
        NS_LOG_FUNCTION(this);

        uint16_t mtu = m_netDevice->GetMtu();

        // Minimum MTU for Ipv4
        if (mtu < 576) {

            mtu = 576;
        }
        return mtu;
    }

    bool VirtualLayerPlusNetDevice::IsLinkUp(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->IsLinkUp();
    }

    void VirtualLayerPlusNetDevice::AddLinkChangeCallback(Callback<void> callback) {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->AddLinkChangeCallback(callback);
    }

    bool VirtualLayerPlusNetDevice::IsBroadcast(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->IsBroadcast();
    }

    Address VirtualLayerPlusNetDevice::GetBroadcast(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->GetBroadcast();
    }

    bool VirtualLayerPlusNetDevice::IsMulticast(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->IsMulticast();
    }

    Address VirtualLayerPlusNetDevice::GetMulticast(Ipv4Address multicastGroup) const {
        NS_LOG_FUNCTION(this << multicastGroup);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->GetMulticast(multicastGroup);
    }

    Address VirtualLayerPlusNetDevice::GetMulticast(Ipv6Address addr) const {
        NS_LOG_FUNCTION(this << addr);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->GetMulticast(addr);
    }

    bool VirtualLayerPlusNetDevice::IsPointToPoint(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->IsPointToPoint();
    }

    bool VirtualLayerPlusNetDevice::IsBridge(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->IsBridge();
    }

    bool VirtualLayerPlusNetDevice::Send(Ptr<Packet> packet,
            const Address& dest,
            uint16_t protocolNumber) {
        NS_LOG_FUNCTION(this << *packet << dest << protocolNumber);
        bool ret = false;
        Address src;

        ret = DoSend(packet, src, dest, protocolNumber, false);

        return ret;
    }

    bool VirtualLayerPlusNetDevice::SendFrom(Ptr<Packet> packet,
            const Address& src,
            const Address& dest,
            uint16_t protocolNumber) {
        NS_LOG_FUNCTION(this << *packet << src << dest << protocolNumber);
        bool ret = false;

        ret = DoSend(packet, src, dest, protocolNumber, true);

        return ret;
    }

    bool VirtualLayerPlusNetDevice::DoSend(Ptr<Packet> packet,
            const Address& src,
            const Address& dest,
            uint16_t protocolNumber,
            bool doSendFrom) {
        NS_LOG_FUNCTION(this << *packet << src << dest << protocolNumber << doSendFrom);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);
		//NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:vnLayerPND-PktTx\tNode:"<<m_node->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<packet->GetSize());

        Ptr<Packet> copyPacket = packet->Copy();
        bool ret = false;



        if (protocolNumber == Ipv4L3Protocol::PROT_NUMBER) {

            Address dst = dest;

            if (IsVirtual(dest)) {
                // A que líder enviamos el paquete??
                dst = GetLeaderMacForRtx(GetRegionIdOf(dest), packet->GetUid());

                if (dst == m_netDevice->GetBroadcast()) {
                    NS_LOG_INFO("Paquete descartado en el nodo " << m_node->GetId() << " por no conocer ningún líder con MAC " << dest << " (Región " << GetRegionIdOf(dest) << ")");
                    m_dropTrace(VirtualLayerPlusNetDevice::DROP_NO_LEADER_RTX, copyPacket, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                    return ret;
                }

            }

            Ipv4Header ipv4Header;
            copyPacket->RemoveHeader(ipv4Header);

            VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();

            if (ipv4Header.GetSource().IsEqual(m_ip)) {
                NS_LOG_INFO("Paquete de Aplicación (Ipv4L3) recibido en la VNLayer del nodo " << m_node->GetId() << " para " << dst << ", enviándolo hacia el NetDevice...");

                virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::Application);
                virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::Client);
                virtualLayerPlusHdr.SetRegion(m_region);
                virtualLayerPlusHdr.SetSrcAddress(ipv4Header.GetSource());
                if (doSendFrom) virtualLayerPlusHdr.SetSrcMAC(src);
                else virtualLayerPlusHdr.SetSrcMAC(m_mac);
                virtualLayerPlusHdr.SetDstAddress(GetIpOfHost(dst));
                virtualLayerPlusHdr.SetDstMAC(dst);
                virtualLayerPlusHdr.SetRol(m_state);
                virtualLayerPlusHdr.SetLeaderID(m_leaderID);
                virtualLayerPlusHdr.SetTimeInState(m_timeInState);
                virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
                virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
                virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);

                packet->AddHeader(virtualLayerPlusHdr);

                m_txTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                if (doSendFrom) {
                    NS_LOG_INFO("VirtualLayerPlusNetDevice::SendFrom " << m_node->GetId() << " " << *packet);
                    ret = m_netDevice->SendFrom(packet, src, dst, protocolNumber);
                } else {
                    NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *packet);
                    ret = m_netDevice->Send(packet, dst, protocolNumber);
                }
            } else {
                if ((m_state == VirtualLayerPlusHeader::LEADER) || ((m_state == VirtualLayerPlusHeader::INTERIM) && !m_replaced)) {
                    virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::Application);
                    virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::ForwardedByLeader);
                    virtualLayerPlusHdr.SetRegion(m_region);
                    virtualLayerPlusHdr.SetSrcAddress(m_ip);
                    if (doSendFrom) virtualLayerPlusHdr.SetSrcMAC(src);
                    else virtualLayerPlusHdr.SetSrcMAC(m_mac);
                    virtualLayerPlusHdr.SetDstAddress(GetIpOfHost(dst));
                    virtualLayerPlusHdr.SetDstMAC(dst);
                    virtualLayerPlusHdr.SetRol(m_state);
                    virtualLayerPlusHdr.SetLeaderID(m_leaderID);
                    virtualLayerPlusHdr.SetTimeInState(m_timeInState);
                    virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
                    virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
                    virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);

                    packet->AddHeader(virtualLayerPlusHdr);

                    m_txTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                    if (doSendFrom) {
                        NS_LOG_INFO("VirtualLayerPlusNetDevice::SendFrom " << m_node->GetId() << " " << *packet);
                        ret = m_netDevice->SendFrom(packet, src, dst, protocolNumber);
                    } else {
                        NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *packet);
                        ret = m_netDevice->Send(packet, dst, protocolNumber);
                    }
                } else if (((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) && (m_backUpState == VirtualLayerPlusNetDevice::SYNC)) {
                    virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::Application);
                    virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::ForwardedByBackUp);
                    virtualLayerPlusHdr.SetRegion(m_region);
                    virtualLayerPlusHdr.SetSrcAddress(m_ip);
                    if (doSendFrom) virtualLayerPlusHdr.SetSrcMAC(src);
                    else virtualLayerPlusHdr.SetSrcMAC(m_mac);
                    virtualLayerPlusHdr.SetDstAddress(GetIpOfHost(dst));
                    virtualLayerPlusHdr.SetDstMAC(dst);
                    virtualLayerPlusHdr.SetRol(m_state);
                    virtualLayerPlusHdr.SetLeaderID(m_supportedLeaderID); // Ponemos al que estamos soportando para eliminar incertidumbre en el resto de BackUps (Pueden no tenerlo en su BackUpTable)
                    virtualLayerPlusHdr.SetTimeInState(m_timeInState);
                    virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
                    virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
                    virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);

                    packet->AddHeader(virtualLayerPlusHdr);

                    // Encolamos el paquete de respuesta a la espera de que el líder lo transmita o no
                    NS_LOG_INFO("Reencolando paquete " << packet->GetUid() << " procesado en el nodo " << m_node->GetId() << " (BackUp)...\n");
                    EnqueuePacket(packet, protocolNumber, doSendFrom);
                }
            }

        } else if (protocolNumber == ArpL3Protocol::PROT_NUMBER) {

            ArpHeader arpHeader;
            copyPacket->RemoveHeader(arpHeader);

            NS_LOG_INFO("Paquete de Aplicación (ArpL3: " << arpHeader.GetSourceIpv4Address() << " -> " << arpHeader.GetDestinationIpv4Address() << " ) recibido en la VNLayer del nodo " << m_node->GetId() << " para " << dest << ", enviándolo hacia el NetDevice...");

            VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();

            virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::Application);
            if (arpHeader.IsRequest()) virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::ArpRequest);
            else if (arpHeader.IsReply()) virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::ArpReply);
            virtualLayerPlusHdr.SetRegion(m_region);
            virtualLayerPlusHdr.SetSrcAddress(arpHeader.GetSourceIpv4Address());
            virtualLayerPlusHdr.SetSrcMAC(arpHeader.GetSourceHardwareAddress());
            virtualLayerPlusHdr.SetDstAddress(arpHeader.GetDestinationIpv4Address());
            virtualLayerPlusHdr.SetDstMAC(arpHeader.GetDestinationHardwareAddress());
            virtualLayerPlusHdr.SetRol(m_state);
            virtualLayerPlusHdr.SetLeaderID(m_leaderID);
            virtualLayerPlusHdr.SetTimeInState(m_timeInState);
            virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
            virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
            virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);

            packet->AddHeader(virtualLayerPlusHdr);

            m_txTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
            if (doSendFrom) {
                NS_LOG_INFO("VirtualLayerPlusNetDevice::SendFrom " << m_node->GetId() << " " << *packet);
                ret = m_netDevice->SendFrom(packet, src, dest, protocolNumber);
            } else {
                NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *packet);
                ret = m_netDevice->Send(packet, dest, protocolNumber);
            }
        } else {
            NS_LOG_INFO("Paquete de Aplicación (" << protocolNumber << ") recibido en la VNLayer del nodo " << m_node->GetId() << " para " << dest << ", enviándolo hacia el NetDevice...");

            VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();

            virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::Application);
            virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::Client);
            virtualLayerPlusHdr.SetRegion(m_region);
            virtualLayerPlusHdr.SetSrcAddress(m_ip);
            if (doSendFrom) virtualLayerPlusHdr.SetSrcMAC(src);
            else virtualLayerPlusHdr.SetSrcMAC(m_mac);
            virtualLayerPlusHdr.SetDstAddress(GetIpOfHost(dest));
            virtualLayerPlusHdr.SetDstMAC(dest);
            virtualLayerPlusHdr.SetRol(m_state);
            virtualLayerPlusHdr.SetLeaderID(m_leaderID);
            virtualLayerPlusHdr.SetTimeInState(m_timeInState);
            virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
            virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
            virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);

            packet->AddHeader(virtualLayerPlusHdr);

            m_txTrace(packet, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
            if (doSendFrom) {
                NS_LOG_INFO("VirtualLayerPlusNetDevice::SendFrom " << m_node->GetId() << " " << *packet);
                ret = m_netDevice->SendFrom(packet, src, dest, protocolNumber);
            } else {

                NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *packet);
                ret = m_netDevice->Send(packet, dest, protocolNumber);
            }
        }

        return ret;
    }

    Ptr<Node> VirtualLayerPlusNetDevice::GetNode(void) const {
        NS_LOG_FUNCTION(this);

        return m_node;
    }

    void VirtualLayerPlusNetDevice::SetNode(Ptr<Node> node) {

        NS_LOG_FUNCTION(this << node);
        m_node = node;
    }

    bool VirtualLayerPlusNetDevice::NeedsArp(void) const {
        NS_LOG_FUNCTION(this);
        NS_ASSERT_MSG(m_netDevice != 0, "VirtualLayer: can't find any lower-layer protocol " << m_netDevice);

        return m_netDevice->NeedsArp();
    }

    void VirtualLayerPlusNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {

        NS_LOG_FUNCTION(this << &cb);
        m_rxCallback = cb;
    }

    void VirtualLayerPlusNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) {

        NS_LOG_FUNCTION(this << &cb);
        m_promiscRxCallback = cb;
    }

    bool VirtualLayerPlusNetDevice::SupportsSendFrom() const {
        NS_LOG_FUNCTION(this);

        return true;
    }

    VirtualLayerPlusHeader::typeState VirtualLayerPlusNetDevice::GetState() {
        return m_state;
    }

    uint32_t VirtualLayerPlusNetDevice::GetRegion() {
        return m_region;
    }

    Ipv4Address VirtualLayerPlusNetDevice::GetIp() {
        return m_ip;
    }

    Address VirtualLayerPlusNetDevice::GetMac() {
        return m_mac;
    }
    
     /* funciones nuevas */
  
	  bool VirtualLayerPlusNetDevice::IsLeader() {
		return m_isLeader;
	  }

	  void VirtualLayerPlusNetDevice::SetLeader(bool leader) {
		m_isLeader=leader;
		if (m_isLeader==true) {
			m_state = VirtualLayerPlusHeader::LEADER;
			m_leaderID=m_node->GetId();
		} else {
			m_state=VirtualLayerPlusHeader::NONLEADER;
		}
		
	  }

    void VirtualLayerPlusNetDevice::Start() {
	
      
        NS_LOG_INFO("::Arranca el protocolo en el nodo " << m_node->GetId());
        
        int32_t interface = m_node->GetObject<Ipv4>()->GetInterfaceForDevice(this);

        NS_ASSERT_MSG((interface != -1), "Ipv4 Interface not found for device");
        m_ipIfIndex = static_cast<uint32_t> (interface);

        m_ip = m_node->GetObject<Ipv4>()->GetAddress(m_ipIfIndex, 0).GetLocal();
        m_mac = this->GetAddress();

        NS_LOG_INFO("Dirección IP: " << m_ip << " Dirección MAC: " << m_mac << "\n");

        m_arpCache = m_node->GetObject<Ipv4L3Protocol>()->GetInterface(m_ipIfIndex)->GetArpCache();
        if (m_arpCache == NULL)
            m_arpCache = CreateObject<ArpCache>();

        std::ostringstream oss;
        oss << "/NodeList/" << m_node->GetId() << "/$ns3::MobilityModel/CourseChange";

        //Config::Connect(oss.str(), MakeCallback(&VirtualLayerPlusNetDevice::CourseChange, this));

        m_timerGetLocation.Schedule(TIME_GET_NODE_POSITION);
        
        // posicion actual del objeto, para calculo de velocidad
        // obtenemos la posicion del nodo
		Vector3D pos = m_node->GetObject<MobilityModel>()->GetPosition();
		// colocamos la posicion inicial y final iguales
		m_posXLast=pos.x;
		m_posYLast=pos.y;
		
		
		
		// pass the netdevice to the scma layer	
		nodeSM.m_netDeviceBase=m_netDevice;
		nodeSM.m_nodeBase=m_node;

		// setup the waymembers list on the scma
		//nodeSM.waySegmentMembers = waySegmentMembers;
		nodeSM.m_ip = m_ip;
		nodeSM.m_mac = m_mac;

		nodeSM.m_wsTable = m_wsTable;
		// removed to test the ibr routing
		// VirtualLayerPlusNetDevice::ibr.scmaStateMachine=&nodeSM;
		
		// start the augmentationrequest timer (debug/running on each line)
		
		//if ( (m_node->GetId()%10)==0 && m_node->GetId()<300 ) {		// 10% of nodes
		//if (m_node->GetId()==SCMA_NODE_TEST) {							// specific node
		//	NS_LOG_UNCOND("Task timer assigned to node: " << m_node->GetId());
		//	m_timerLaunchAugmentation.Schedule(TIME_LAUNCH_AUGMENTATION);
		//}
		

    }

    void VirtualLayerPlusNetDevice::CourseChange(std::string context, Ptr<const MobilityModel> mobility) {
		//NS_LOG_DEBUG("Cambio de curso en nodo: " << m_node->GetId());
        Time exitTime = GetExitTime();

        if (m_state == VirtualLayerPlusHeader::LEADER) {
            if (exitTime > TIME_RELINQUISHMENT) {
                NS_LOG_INFO("Reprogramando Timer Relinquishment con: " << (exitTime.GetSeconds() - static_cast<Time> (TIME_RELINQUISHMENT).GetSeconds()) << " segundos\n");
                m_timerRelinquishment.Cancel();
                Time relinq = exitTime - TIME_RELINQUISHMENT;
                NS_ASSERT_MSG(relinq.IsPositive(), "Programando timer relinquishment con tiempo negativo" << relinq);
                m_timerRelinquishment.Schedule(exitTime - TIME_RELINQUISHMENT);
            } else {

                NS_LOG_INFO("Reprogramando Timer Relinquishment con 0 segundos\n");
                m_timerRelinquishment.Cancel();
                m_timerRelinquishment.Schedule(Seconds(0.0));
            }
        }

        m_timerGetLocation.Cancel();
        NS_ASSERT_MSG(exitTime.IsPositive(), "Programando timer GetLocation con tiempo negativo" << exitTime);
        m_timerGetLocation.Schedule(exitTime);
    }

    void VirtualLayerPlusNetDevice::TimerRequestWaitExpire(uint32_t leaderID) {

        NS_LOG_INFO("Timer RequestWait expirado en el nodo " << m_node->GetId() << ", tenemos nuevo líder!\n");

        m_stateTrace(m_state, VirtualLayerPlusHeader::LEADER, m_region);
        m_state = VirtualLayerPlusHeader::LEADER;
        m_timeInState = CURRENT_TIME.GetDouble();
        m_leaderID = leaderID;

        if (leaderID == UINT32_MAX) m_leaderID = PutLeader(m_ip, m_mac);
        else PutLeader(m_ip, m_mac, leaderID);

        PushLastLeader(m_ip, m_mac, m_leaderID); //********

        PrintLeaderTable();

        m_promiscuousMode = true;

        if (NewLeaderActive()) {
            m_newLeaderActive = true;
        }

        if (m_backUpState != VirtualLayerPlusNetDevice::NONE) { // Si era BackUp
            if (m_multipleLeaders) QuitBackUp(m_ip, false); // Se borra de la tabla sin actualizar prioridades
            else QuitBackUp(m_ip, true); // Se borra de la tabla actualizando prioridades
            PrintBackUpTable();

            // Enviar mensajes almacenados del anterior líder?
            m_backUpState = VirtualLayerPlusNetDevice::NONE;
            m_backUpPriority = UINT32_MAX;
            m_supportedLeaderID = UINT32_MAX;
        }

       
		SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::Heartbeat, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());

        Time exitTime = GetExitTime();
        if (exitTime > TIME_RELINQUISHMENT) {
            NS_LOG_INFO("Reprogramando Timer Relinquishment con: " << (exitTime.GetSeconds() - static_cast<Time> (TIME_RELINQUISHMENT).GetSeconds()) << " segundos\n");
            m_timerRelinquishment.Cancel();
            Time relinq = exitTime - TIME_RELINQUISHMENT;
            NS_ASSERT_MSG(relinq.IsPositive(), "Programando timer relinquishment con tiempo negativo" << relinq);
            m_timerRelinquishment.Schedule(exitTime - TIME_RELINQUISHMENT);
        } else {

            NS_LOG_INFO("Reprogramando Timer Relinquishment con 0 segundos\n");
            m_timerRelinquishment.Cancel();
            m_timerRelinquishment.Schedule(Seconds(0.0));
        }

		
        m_timerGetLocation.Cancel();
        NS_ASSERT_MSG(exitTime.IsPositive(), "Programando timer GetLocation con tiempo negativo" << exitTime);
        //m_timerGetLocation.Schedule(exitTime);
		m_timerGetLocation.Schedule(TIME_GET_NODE_POSITION);
        m_timerHeartbeat.Cancel();
        m_timerHeartbeat.Schedule(TIME_HEARTBEAT);

    }

    void VirtualLayerPlusNetDevice::TimerHeartbeatExpire() {


        switch (m_state) {
            case VirtualLayerPlusHeader::NONLEADER:
                m_expires++;
                 ////NS_LOG_DEBUG("++ Heartbeat expirado en el nodo " << m_node->GetId() << " -> " << m_expires);
                if (m_expires == 2) {
                    m_stateTrace(m_state, VirtualLayerPlusHeader::REQUEST, m_region);
                    m_state = VirtualLayerPlusHeader::REQUEST;
                    CancelTimers();
                    double Rand = (rand() / (double) RAND_MAX) * 0.1;
                    if ((m_state == VirtualLayerPlusHeader::NONLEADER) && (m_backUpState != VirtualLayerPlusNetDevice::NONE)) {
                        m_timerRequestWait.SetArguments(GetSupportedLeaderIdOf(m_ip));
                    } else {
                        m_timerRequestWait.SetArguments(static_cast<uint32_t> (0));
                    }
                    Time req = TIME_REQUEST_WAIT + Seconds(Rand);
                    NS_ASSERT_MSG(req.IsPositive(), "Programando timer RequestWait con tiempo negativo" << req);
                    m_timerRequestWait.Schedule(TIME_REQUEST_WAIT + Seconds(Rand));
                    m_expires = 0;
                    NS_LOG_INFO("Timer Heartbeat expirado en el nodo " << m_node->GetId() << " -> pasa al estado REQUEST\n");
                } else {
                    m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                    NS_LOG_INFO("Timer Heartbeat expirado en el nodo " << m_node->GetId() << " -> reiniciamos timer...\n");
                    // forzamos a que nunca expire el contador de heartbeats
                    m_expires=0;
                }
                break;
            case VirtualLayerPlusHeader::LEADER:
                SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::Heartbeat, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
                m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                NS_LOG_INFO("Timer Heartbeat expirado en el nodo " << m_node->GetId() << " -> enviando Heartbeat...\n");
                break;
            case VirtualLayerPlusHeader::INTERIM:
            {

                BackUpTableHeader::BackUpEntry priorityBackUp = GetPriorityBackUpOf(m_leaderID);
                // Le envía el LeaderLeft al Priority BackUp y de no haberlo lo envía por broadcast para iniciar una nueva contienda
                SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderLeft, priorityBackUp.ip, priorityBackUp.mac);
                m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                NS_LOG_INFO("Timer Heartbeat expirado en el nodo " << m_node->GetId() << " -> enviando LeaderLeft...\n");
            }
                break;
            default:;
        }

    }


  
  
  /*
    void VirtualLayerPlusNetDevice::TimerGetLocationExpire() {

        NS_LOG_INFO("Timer GetLocation expirado en el nodo " << m_node->GetId() << ", actualizando región...");

        uint32_t region = GetLocation();

        if (region != m_region) {
            NS_LOG_INFO("DETECTADO CAMBIO DE REGIÓN EN EL NODO  " << m_node->GetId());

            CancelTimers();
            
            

            if ((m_state == VirtualLayerPlusHeader::LEADER) || (m_state == VirtualLayerPlusHeader::INTERIM)) { // Si estaba actuando como lider envía un LeaderLeft
                BackUpTableHeader::BackUpEntry priorityBackUp = GetPriorityBackUpOf(m_leaderID);

                // Si hay BackUp se lo envía a él para que le sustituya. Si no, lo envía por broadcast para iniciar una nueva contienda
                SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderLeft, priorityBackUp.ip, priorityBackUp.mac);

            } else if (m_backUpState != VirtualLayerPlusNetDevice::NONE) { // Si es BackUp envía BackUpLeft
                SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::BackUpLeft, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
            }


            

            m_region = region;
                
            std::cout << "ok\n" ;


            m_virtualNodeLevel = GetVirtualNodeLevel(m_region);

            

            m_leaderID = UINT32_MAX;
            for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
                if (i->timerLifeLeaderEntry.IsRunning()) i->timerLifeLeaderEntry.Cancel();
                i = m_leaderTable.erase(i);
                i--;
            }



            m_leaderTable.clear();

            m_lastLeadersTable.clear();

            m_newLeaderActive = false;
            m_replaced = false;

            ClearBackUp();
            UpdateNeighborLeadersTable();

            if (m_state != VirtualLayerPlusHeader::INITIAL) SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderRequest, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
            double Rand = (rand() / (double) RAND_MAX)*0.1;
            NS_LOG_INFO("Iniciamos el timer RequestWait con: " << Rand + 0.2 << " segundos");
            m_timerRequestWait.Cancel();
            m_timerRequestWait.SetArguments(static_cast<uint32_t> (0));
            Time req = TIME_REQUEST_WAIT + Seconds(Rand);
            NS_ASSERT_MSG(req.IsPositive(), "Programando timer RequestWait con tiempo negativo" << req);
            m_timerRequestWait.Schedule(TIME_REQUEST_WAIT + Seconds(Rand));
            NS_LOG_INFO("El nodo " << m_node->GetId() << " pasa a estado REQUEST");
            m_stateTrace(m_state, VirtualLayerPlusHeader::REQUEST, m_region);
            m_timeInState = 0;
            m_expires = 0;
            m_state = VirtualLayerPlusHeader::REQUEST;

            // Una vez conocemos nuestra región por primera vez (salimos del estado INIT) comenzamos a enviar Hellos
            if (!m_timerHello.IsRunning()) {
                Rand = (rand() / (double) RAND_MAX)*0.1;
                m_timerHello.Schedule(Seconds(Rand));
            }
        }

        Time ext = GetExitTime();
        NS_ASSERT_MSG(ext.IsPositive(), "Programando timer GetLocation con tiempo negativo" << ext);
        //m_timerGetLocation.Schedule(GetExitTime());
        m_timerGetLocation.Schedule(ext);

        NS_LOG_INFO("Región: " << m_region << "\n");

    }
  */
    void VirtualLayerPlusNetDevice::TimerBackUpRequestExpire() {

        switch (m_state) {
            case VirtualLayerPlusHeader::LEADER:
                NS_LOG_INFO("Timer BackUp expirado en el nodo " << m_node->GetId() << " (líder), purgando BackUp Table...\n");
                if (m_multipleLeaders) PurgeBackUp(true);
                else PurgeBackUp(false);
                PrintBackUpTable();
                break;
            case VirtualLayerPlusHeader::INTERIM:
                NS_LOG_INFO("Timer BackUp expirado en el nodo " << m_node->GetId() << " (interino), purgando BackUp Table...\n");
                if (m_multipleLeaders) PurgeBackUp(true);
                else PurgeBackUp(false);
                PrintBackUpTable();
                break;
            case VirtualLayerPlusHeader::NONLEADER:
                if (m_backUpState == VirtualLayerPlusNetDevice::REQUESTING) { // Si está pidiendo ser BackUp envía SyncRequest

                    NS_LOG_INFO("Timer BackUp expirado en el nodo " << m_node->GetId() << ", enviando SyncRequest...\n");
                    SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::SyncRequest, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
                }
                break;
            default:;
        }

    }

    void VirtualLayerPlusNetDevice::TimerRelinquishmentExpire() {

        Time exitTime = GetExitTime();
        m_timerGetLocation.Cancel();
        NS_ASSERT_MSG(exitTime.IsPositive(), "Programando timer GetLocation con tiempo negativo" << exitTime);
        m_timerGetLocation.Schedule(exitTime);

        if (exitTime == Seconds(0.0)) return; // Si ya ha salido de la región se salta el estado INTERIM
        else if (exitTime > TIME_RELINQUISHMENT) { // Si todavía no está abandonando la región esperamos a que lo esté
            NS_LOG_INFO("Timer Relinquishment expirado en el nodo " << m_node->GetId() << ", aún no está saliendo...\n");
            Time relinq = exitTime - TIME_RELINQUISHMENT;
            NS_ASSERT_MSG(relinq.IsPositive(), "Programando timer relinquishment con tiempo negativo" << relinq);
            m_timerRelinquishment.Schedule(exitTime - TIME_RELINQUISHMENT);
        } else { // Inicia la renuncia

            //NS_LOG_DEBUG("---> Timer Relinquishment expirado en el nodo " << m_node->GetId() << ", se inicia la RENUNCIA\n");
            CancelTimers();

            m_stateTrace(m_state, VirtualLayerPlusHeader::INTERIM, m_region);
            m_state = VirtualLayerPlusHeader::INTERIM;
            m_timeInState = DBL_MAX;

            BackUpTableHeader::BackUpEntry priorityBackUp = GetPriorityBackUpOf(m_leaderID);
            //Si hay BackUp se lo envía a él para que le sustituya. Si no, lo envía por broadcast para iniciar una nueva contienda
            SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderLeft, priorityBackUp.ip, priorityBackUp.mac);

            m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
        }

    }

    void VirtualLayerPlusNetDevice::TimerRetrySyncDataExpire(Ipv4Address ip, Address mac) {

        NS_LOG_INFO("Timer RetrySyncData expirado en el nodo " << m_node->GetId() << ", volviendo a enviar SyncData...\n");
        SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::SyncData, ip, mac);
    }

    void VirtualLayerPlusNetDevice::TimerLifePacketEntryExpire(uint64_t uid) {

        NS_LOG_INFO("Timer LifePacketEntry expirado en el nodo " << m_node->GetId() << ", reenviando paquete " << uid << "...\n");
        ForwardPacketWithUid(uid);
    }

    void VirtualLayerPlusNetDevice::TimerLifeHostEntryExpire(Ipv4Address ip) {

        //NS_LOG_DEBUG("Timer LifeHostEntry expirado en el nodo " << m_node->GetId() << ", eliminando host " << ip << " de la HostTable...");
        QuitHost(ip);
    }

    void VirtualLayerPlusNetDevice::TimerLeaderCeaseExpire() {
        NS_LOG_INFO("Timer LeaderCease expirado en el líder " << m_leaderID << ". Deja de ser líder, enviando CeaseLeaderAnnouncement...");

        if (!IsTheLatestLeader(m_leaderID)) return;

        SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::CeaseLeaderAnnouncement, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
        CancelTimers();
        m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
        QuitLeader(m_ip);
        PrintLeaderTable();
        m_stateTrace(m_state, VirtualLayerPlusHeader::NONLEADER, m_region);
        m_state = VirtualLayerPlusHeader::NONLEADER;
        m_timeInState = 0;
        m_promiscuousMode = false;
        m_leaderID = UINT32_MAX;

        m_timerRelinquishment.Cancel();
        //m_hostsTable.clear();
    }

    void VirtualLayerPlusNetDevice::TimerLifeLeaderEntryExpire(uint32_t id, Ipv4Address ip) {
        NS_LOG_INFO("Timer LifeLeaderEntry expirado en el nodo " << m_node->GetId() << ", eliminando líder " << id << " de la LeaderTable...");

        if (id == m_leaderID + 1) m_newLeaderActive = false;
        QuitLeader(ip);
        PrintLeaderTable();
    }

    void VirtualLayerPlusNetDevice::TimerLifeNeighborLeaderEntryExpire(uint32_t region, uint32_t id) {

        NS_LOG_INFO("Timer LifeNeighborLeaderEntry expirado en el nodo " << m_node->GetId() << ", eliminando líder " << id << " de la región " << region << " de la NeighborLeadersTable...");
        QuitNeighborLeader(region, id);
        PrintNeighborLeadersTable();
    }

    void VirtualLayerPlusNetDevice::TimerHelloExpire() {

        NS_LOG_INFO("Timer Hello expirado en el nodo " << m_node->GetId() << " enviando Hello...");
        SendPkt(VirtualLayerPlusHeader::Hello, VirtualLayerPlusHeader::hello, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
        m_timerHello.Schedule(TIME_HELLO_INTERVAL);
    }

    void VirtualLayerPlusNetDevice::CancelTimers(void) {

        if (m_timerRequestWait.IsRunning())
            m_timerRequestWait.Cancel();
        if (m_timerHeartbeat.IsRunning())
            m_timerHeartbeat.Cancel();
        if (m_timerLeaderCease.IsRunning())
            m_timerLeaderCease.Cancel();
        if (m_timerRetrySyncData.IsRunning())
            m_timerRetrySyncData.Cancel();

    }

    uint32_t VirtualLayerPlusNetDevice::GetLocation(void) {

        uint32_t xRegion;
        uint32_t yRegion;

        Vector3D pos = m_node->GetObject<MobilityModel>()->GetPosition();

        xRegion = static_cast<uint32_t> (std::floor(pos.x / (double (m_long) / double (m_columns))));
        yRegion = static_cast<uint32_t> (std::floor(pos.y / (double (m_width) / double (m_rows))));

        return (m_columns * yRegion + xRegion);

    }

    uint32_t VirtualLayerPlusNetDevice::GetRegionIdOf(Vector2D pos) {

        uint32_t xRegion;
        uint32_t yRegion;

        xRegion = static_cast<uint32_t> (std::floor(pos.x / (double (m_long) / double (m_columns))));
        yRegion = static_cast<uint32_t> (std::floor(pos.y / (double (m_width) / double (m_rows))));

        return (m_columns * yRegion + xRegion);

    }

   
    /*
     * Funcion: estima el tiempo de salida
     *  de un nodo de su actual region
     */

    Time VirtualLayerPlusNetDevice::GetExitTime(void) {

        // verificamos que no haya cambiado de region
        uint32_t region = GetLocation();
        if (region != m_region)
            return Seconds(0.0);

        Vector3D pos = m_node->GetObject<MobilityModel>()->GetPosition();
        Vector3D vel = m_node->GetObject<MobilityModel>()->GetVelocity();

        NS_ASSERT_MSG((pos.x >= 0 && pos.x <= m_long) && (pos.y >= 0 && pos.y <= m_width), "COSA MALA! Reg: " << region << "\t" << pos.x << " :: " << m_long << "\t" << pos.y << "::" << m_width);

        double speed = sqrt(pow(vel.x, 2) + pow(vel.y, 2));
        double dist = DBL_MAX;

        // Si la velocidad es cero, retorna por defecto un valor para volver a consultar region
        if (speed == 0)
            return Seconds(2.0);

        if (speed == sqrt(pow(vel.x, 2)) && vel.y != 0) vel.y = 0;
        else if (speed == sqrt(pow(vel.y, 2)) && vel.x != 0) vel.x = 0;

        double xR = int (pos.x / (double (m_long) / double (m_columns))) * (double (m_long) / double (m_columns));
        double yR = int (pos.y / (double (m_width) / double(m_rows))) * (double (m_width) / double(m_rows));

        // Cut points
        Vector2D cut1, cut2, cut3, cut4;
        // Vectors from the position to the cut points
        Vector2D v1, v2, v3, v4;

        // The node is moving along the vertical axis --> There are 2 cut points (Straight 1 and 3)
        //
        //            |                                  |
        //            |                 3   |            |
        //        ----|---------------------x------------|----
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //          2 |            Node --> O            | 4
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //            |                     |            |
        //        ----|---------------------x------------|----
        //            |                 1   |            |
        //            |                                  |
        //
        //

        if (vel.x == 0) {
            // Cut with the straight 1
            cut1.x = (vel.x * yR + (vel.y * pos.x - vel.x * pos.y)) / vel.y;
            cut1.y = yR;

            // Cut with the straight 3
            cut3.x = (vel.x * (yR + (double (m_width) / double (m_rows))) + (vel.y * pos.x - vel.x * pos.y)) / vel.y;
            cut3.y = yR + (double (m_width) / double (m_rows));

            v1.x = cut1.x - pos.x;
            v1.y = cut1.y - pos.y;

            v3.x = cut3.x - pos.x;
            v3.y = cut3.y - pos.y;

            // If the direction of the v1 vector and the velocity vector is the same --> The node is going to the cut1 point
            if ((v1.x * vel.x >= 0) && (v1.y * vel.y >= 0)) {
                dist = sqrt(pow(v1.x, 2) + pow(v1.y, 2));
                // retorna el tiempo + un delta t para asegurar la 
                // salida de la region actual
                return (Seconds(dist / speed) + dt);
            } else {
                dist = sqrt(pow(v3.x, 2) + pow(v3.y, 2));
                // retorna el tiempo + un delta t para asegurar la 
                // salida de la region actual
                return (Seconds(dist / speed) + dt);
            }
        }// The node is moving along the horizontal axis --> There are 2 cut points (Straight 2 and 4)
            //
            //            |                                  |
            //            |                 3                |
            //        ----|----------------------------------|----
            //            |          Node                    |
            //            |           |                      |
            //            |           v                      |
            //          --x-----------O----------------------x--
            //            |                                  |
            //            |                                  |
            //            |                                  |
            //          2 |                                  | 4
            //            |                                  |
            //            |                                  |
            //            |                                  |
            //            |                                  |
            //            |                                  |
            //            |                                  |
            //        ----|----------------------------------|----
            //            |                 1                |
            //            |                                  |
            //
            //
        else if (vel.y == 0) {
            // Cut with the straight 2
            cut2.x = xR;
            cut2.y = (vel.y * xR - (vel.y * pos.x - vel.x * pos.y)) / vel.x;

            // Cut with the straight 4
            cut4.x = xR + (double (m_long) / double (m_columns));
            cut4.y = (vel.y * (xR + (double (m_long) / double (m_columns))) - (vel.y * pos.x - vel.x * pos.y)) / vel.x;

            v2.x = cut2.x - pos.x;
            v2.y = cut2.y - pos.y;

            v4.x = cut4.x - pos.x;
            v4.y = cut4.y - pos.y;

            // If the direction of the v2 vector and the velocity vector is the same --> The node is going to the cut2 point
            if ((v2.x * vel.x >= 0) && (v2.y * vel.y >= 0)) {
                dist = sqrt(pow(v2.x, 2) + pow(v2.y, 2));
                // retorna el tiempo + un delta t para asegurar la 
                // salida de la region actual
                return (Seconds(dist / speed) + dt);
            } else {
                dist = sqrt(pow(v4.x, 2) + pow(v4.y, 2));
                // retorna el tiempo + un delta t para asegurar la 
                // salida de la region actual
                return (Seconds(dist / speed) + dt);
            }
        } else {
            //                                               |/
            //                                               x
            //                                              /|
            //                                             / |
            //                                            /  |
            //                                           /   |
            //                                          /    |
            //                                         /     |
            //                                        /      |
            //                                       /       |
            //                                      /        |
            //                                     /         |
            //            |                       /          |
            //            |                 3    /           |
            //        ----|---------------------x------------|----
            //            |                    /             |
            //            |                   /              |
            //            |                  /               |
            //            |                 /                |
            //            |                /                 |
            //            |               /                  |
            //            |              /                   |
            //          2 |    Node --> O                    | 4
            //            |            /                     |
            //            |           /                      |
            //            |          /                       |
            //            |         /                        |
            //            |        /                         |
            //            |       /                          |
            //        ----|------x---------------------------|----
            //            |     /           1                |
            //            |    /                             |
            //            |   /
            //            |  /
            //            | /
            //            |/
            //            x
            //           /|

            // Cut with the straight 1
            cut1.x = (vel.x * yR + (vel.y * pos.x - vel.x * pos.y)) / vel.y;
            cut1.y = yR;

            // Cut with the straight 2
            cut2.x = xR;
            cut2.y = (vel.y * xR - (vel.y * pos.x - vel.x * pos.y)) / vel.x;

            // Cut with the straight 3
            cut3.x = (vel.x * (yR + (double (m_width) / double (m_rows))) + (vel.y * pos.x - vel.x * pos.y)) / vel.y;
            cut3.y = yR + (double (m_width) / double (m_rows));

            // Cut with the straight 4
            cut4.x = xR + (double (m_long) / double (m_columns));
            cut4.y = (vel.y * (xR + (double (m_long) / double (m_columns))) - (vel.y * pos.x - vel.x * pos.y)) / vel.x;

            v1.x = cut1.x - pos.x;
            v1.y = cut1.y - pos.y;

            v2.x = cut2.x - pos.x;
            v2.y = cut2.y - pos.y;

            v3.x = cut3.x - pos.x;
            v3.y = cut3.y - pos.y;

            v4.x = cut4.x - pos.x;
            v4.y = cut4.y - pos.y;

            if ((v1.x * vel.x >= 0) && (v1.y * vel.y >= 0) && (sqrt(pow(v1.x, 2) + pow(v1.y, 2)) < dist)) {
                dist = sqrt(pow(v1.x, 2) + pow(v1.y, 2));
            }
            if ((v2.x * vel.x >= 0) && (v2.y * vel.y >= 0) && (sqrt(pow(v2.x, 2) + pow(v2.y, 2)) < dist)) {
                dist = sqrt(pow(v2.x, 2) + pow(v2.y, 2));
            }
            if ((v3.x * vel.x >= 0) && (v3.y * vel.y >= 0) && (sqrt(pow(v3.x, 2) + pow(v3.y, 2)) < dist)) {
                dist = sqrt(pow(v3.x, 2) + pow(v3.y, 2));
            }
            if ((v4.x * vel.x >= 0) && (v4.y * vel.y >= 0) && (sqrt(pow(v4.x, 2) + pow(v4.y, 2)) < dist)) {

                dist = sqrt(pow(v4.x, 2) + pow(v4.y, 2));
            }

            // retorna el tiempo + un delta t para asegurar la 
            // salida de la region actual
            return (Seconds(dist / speed) + dt);
        }

    }


    // Elimina BackUps no sincronizados

    void VirtualLayerPlusNetDevice::PurgeBackUp(bool updatePriorities) {

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (!i->isSynced) { // Borramos los no sincronizados
                uint32_t deletedPriority = i->priority;

                i = m_backUpTable.erase(i);
                i--;

                if (updatePriorities)
                    // Por cada uno que borramos actualizamos las prioridades de los sincronizados
                    for (std::list<BackUpTableHeader::BackUpEntry>::iterator j = m_backUpTable.begin(); j != m_backUpTable.end(); ++j) {
                        if (!j->isSynced) break;

                        if (j->priority > deletedPriority) j->priority = j->priority - 1;
                    }
            }
        }

    }

    // Inserta BackUp en la BackUpTable ordenado por prioridad

    void VirtualLayerPlusNetDevice::PutBackUp(Ipv4Address ipBackUp, Address macBackUp, uint32_t backUpPriority) {

        // Si conocíamos a uno con la misma prioridad lo eliminamos
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->priority == backUpPriority) {
                i = m_backUpTable.erase(i);
                i--;
            }
        }

        // Si ya lo tenemos en la tabla no lo volvemos a meter
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->ip.IsEqual(ipBackUp)) return;
        }

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->isSynced) continue; // Los sincronizados tienen prioridad sobre los no sincronizados
            if (i->priority <= backUpPriority) continue; // Los no sincronizados con más prioridad también tienen prioridad

            // Si llegamos hasta aquí es que estamos en la posición correspondiente a backUpPriority
            m_backUpTable.insert(i, BackUpTableHeader::BackUpEntry(ipBackUp, macBackUp, backUpPriority));

            return;
        }
        m_backUpTable.push_back(BackUpTableHeader::BackUpEntry(ipBackUp, macBackUp, backUpPriority));
    }

    // Inserta BackUp en la BackUpTable ordenado por prioridad rellenando huecos (saltos de prioridad)

    uint32_t VirtualLayerPlusNetDevice::PutBackUp(Ipv4Address ipBackUp, Address macBackUp) {

        uint32_t backUpPriority = GetInsertPriority();

        // Si ya lo tenemos en la tabla no lo volvemos a meter
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->ip.IsEqual(ipBackUp)) return i->priority;
        }

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->isSynced) continue; // Los sincronizados tienen prioridad sobre los no sincronizados
            if (i->priority < backUpPriority) continue; // Los no sincronizados con más prioridad también tienen prioridad

            // Si llegamos hasta aquí es que estamos en la posición correspondiente a backUpPriority
            m_backUpTable.insert(i, BackUpTableHeader::BackUpEntry(ipBackUp, macBackUp, backUpPriority));
            return backUpPriority;
        }
        m_backUpTable.push_back(BackUpTableHeader::BackUpEntry(ipBackUp, macBackUp, backUpPriority));

        return backUpPriority;
    }

    uint32_t VirtualLayerPlusNetDevice::GetPriorityOf(Ipv4Address ipBackUp) {
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {

            if (i->ip.IsEqual(ipBackUp)) return i->priority;
        }

        return static_cast<uint32_t> (UINT32_MAX);
    }

    uint32_t VirtualLayerPlusNetDevice::GetInsertPriority() {

        bool find;
        for (uint32_t c = 0; c < m_backUpTable.size(); c++) {
            find = false;
            for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
                if (i->priority == c) {
                    find = true;
                    break;
                }
            }

            if (find == false) return c;
        }

        return static_cast<uint32_t> (m_backUpTable.size());
    }

    void VirtualLayerPlusNetDevice::QuitBackUp(Ipv4Address ipBackUp, bool updatePriorities) {

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->ip.IsEqual(ipBackUp)) {
                uint32_t deletedPriority = i->priority;

                m_backUpTable.erase(i);

                if (updatePriorities)
                    // Por cada uno que borramos actualizamos las prioridades de los demás
                    for (std::list<BackUpTableHeader::BackUpEntry>::iterator j = m_backUpTable.begin(); j != m_backUpTable.end(); ++j) {

                        if (j->priority > deletedPriority) j->priority = j->priority - 1;
                    }
                return;
            }
        }

    }

    BackUpTableHeader::BackUpEntry VirtualLayerPlusNetDevice::GetPriorityBackUpOf(uint32_t leaderID) {

        if (!m_multipleLeaders) {
            if (m_backUpTable.size() > 0) return m_backUpTable.front(); //BackUpTableHeader::BackUpEntry(m_backUpTable.front().ip, m_backUpTable.front().mac, m_backUpTable.front().isSynced, m_backUpTable.front().priority);
            else return BackUpTableHeader::BackUpEntry(Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast(), UINT32_MAX);
        }

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {

            double backUpOfLeader = static_cast<double> (i->priority);
            while ((backUpOfLeader - static_cast<double> (m_leaderTable.size())) >= 0) backUpOfLeader -= static_cast<double> (m_leaderTable.size());

            if (static_cast<uint32_t> (backUpOfLeader) == leaderID) return BackUpTableHeader::BackUpEntry(i->ip, i->mac, i->isSynced, i->priority);

        }

        return BackUpTableHeader::BackUpEntry(Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast(), UINT32_MAX);
    }

    uint32_t VirtualLayerPlusNetDevice::GetSupportedLeaderIdOf(Ipv4Address backUpIP) {
        double leaderID = 0;
        double numLeaders = static_cast<double> (m_leaderTable.back().id + 1); //static_cast<double> (m_leaderTable.size())
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->ip.IsEqual(backUpIP)) {
                leaderID = static_cast<double> (i->priority);
                break;
            }
        }

        while ((leaderID - numLeaders) >= 0) leaderID -= numLeaders;

        return static_cast<uint32_t> (leaderID);
    }

    bool CompareBackUpEntries(const BackUpTableHeader::BackUpEntry& backUpEntry1, const BackUpTableHeader::BackUpEntry& backUpEntry2) {
        if (backUpEntry1.isSynced)
            if (!backUpEntry2.isSynced) return true;
            else if (backUpEntry1.priority < backUpEntry2.priority) return true;
            else return false;
        else if (backUpEntry2.isSynced) return false;
        else if (backUpEntry1.priority < backUpEntry2.priority) return true;

        else return false;
    }

    void VirtualLayerPlusNetDevice::SyncBackUp(Ipv4Address ipBackUp) {

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            if (i->ip.IsEqual(ipBackUp)) {
                i->isSynced = true;

                m_backUpTable.sort(CompareBackUpEntries);

                return;
            }

        }

    }

    void VirtualLayerPlusNetDevice::ClearBackUp(void) {

        m_backUpState = VirtualLayerPlusNetDevice::NONE;
        m_backUpPriority = UINT32_MAX;
        m_supportedLeaderID = UINT32_MAX;
        m_promiscuousMode = false;

        m_backUpTable.clear();
        for (std::list<PacketEntry>::iterator i = m_leaderSupportQueue.begin(); i != m_leaderSupportQueue.end(); ++i) {
            i->timerLifePacketEntry.Cancel();
            i = m_leaderSupportQueue.erase(i);
            i--;
        }
        m_leaderSupportQueue.clear();

        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            i->timerLifeHostEntry.Cancel();
            i = m_hostsTable.erase(i);
            i--;
        }
        m_hostsTable.clear();

    }

    void VirtualLayerPlusNetDevice::LoadBackUpTable(BackUpTableHeader backUpTableHdr) {

        m_backUpTable = backUpTableHdr.GetBackUpTable();

    }

    bool VirtualLayerPlusNetDevice::IsInBackUpTable(Ipv4Address backUpIP) {

        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {

            if (i->ip.IsEqual(backUpIP)) return true;
        }

        return false;
    }

    void VirtualLayerPlusNetDevice::PrintBackUpTable() {
        NS_LOG_INFO("      IP                 MAC               State        Priority \n");
        NS_LOG_INFO("-----------------------------------------------------------------\n");
        for (std::list<BackUpTableHeader::BackUpEntry>::iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {

            NS_LOG_INFO("|  " << i->ip << "    " << i->mac << "      " << ((i->isSynced) ? "sync    " : "non_sync") << "       " << i->priority << "    |\n");
            NS_LOG_INFO("-----------------------------------------------------------------\n");
        }

    }

    bool VirtualLayerPlusNetDevice::EnqueuePacket(Ptr<Packet> pkt, uint16_t protNum, bool sendFrom) {

        Ptr<Packet> copyPacket = pkt->Copy();

        VirtualLayerPlusHeader virtualLayerPlusHdr;
        copyPacket->RemoveHeader(virtualLayerPlusHdr);

        for (std::list<PacketEntry>::iterator i = m_leaderSupportQueue.begin(); i != m_leaderSupportQueue.end(); ++i) {

            if (i->pkt->GetUid() == pkt->GetUid()) {
                i->timerLifePacketEntry.Cancel();

                if (i->retransmitted) { // Si ya ha sido retransmitido mientras lo procesábamos --> lo descartamos
                    m_leaderSupportQueue.erase(i);
                    return false;
                } else {
                    i = m_leaderSupportQueue.erase(i);

                    i = m_leaderSupportQueue.insert(i, PacketEntry(pkt, protNum, sendFrom));

                    i->processed = true;
                    i->timerLifePacketEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifePacketEntryExpire, this);
                    i->timerLifePacketEntry.SetArguments(pkt->GetUid());
                    i->timerLifePacketEntry.SetDelay(TIME_LIFE_PACKET_ENTRY * (m_backUpPriority + 1));
                    return true;
                }

            }
            if (i->pkt->GetUid() > pkt->GetUid()) {
                i = m_leaderSupportQueue.insert(i, PacketEntry(pkt, protNum, sendFrom));

                i->timerLifePacketEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifePacketEntryExpire, this);
                i->timerLifePacketEntry.SetArguments(pkt->GetUid());
                i->timerLifePacketEntry.SetDelay(TIME_LIFE_PACKET_ENTRY * (m_backUpPriority + 1));
                return true;
            }
        }
        m_leaderSupportQueue.push_back(PacketEntry(pkt, protNum, sendFrom));
        std::list<PacketEntry>::iterator it = m_leaderSupportQueue.end();
        it--;
        it->timerLifePacketEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifePacketEntryExpire, this);
        it->timerLifePacketEntry.SetArguments(pkt->GetUid());
        it->timerLifePacketEntry.SetDelay(TIME_LIFE_PACKET_ENTRY * (m_backUpPriority + 1));

        return true;

    }

    bool VirtualLayerPlusNetDevice::DequeuePacketWithUid(uint64_t uid) {

        for (std::list<PacketEntry>::iterator i = m_leaderSupportQueue.begin(); i != m_leaderSupportQueue.end(); ++i) {
            if (i->pkt->GetUid() < uid) {
                if ((i->processed) && (!i->timerLifePacketEntry.IsRunning())) {
                    i->timerLifePacketEntry.Schedule();
                }
            } else if (i->pkt->GetUid() == uid) {
                i->timerLifePacketEntry.Cancel();
                m_leaderSupportQueue.erase(i);

                return true;
            }
        }
        return false;
    }

    bool VirtualLayerPlusNetDevice::ForwardPacketWithUid(uint64_t uid) {

        bool ret = false;

        for (std::list<PacketEntry>::iterator i = m_leaderSupportQueue.begin(); i != m_leaderSupportQueue.end(); ++i) {
            if (i->pkt->GetUid() == uid) {
                i->timerLifePacketEntry.Cancel();

                if (i->retransmitted) return ret;

                Ptr<Packet> copyPacket = i->pkt->Copy();
                VirtualLayerPlusHeader virtualLayerPlusHdr;
                copyPacket->RemoveHeader(virtualLayerPlusHdr);

                m_txTrace(i->pkt, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                if (i->doSendFrom) {
                    //NS_LOG_DEBUG("VirtualLayerPlusNetDevice::SendFrom " << m_node->GetId() << " " << *i->pkt);
                    ret = m_netDevice->SendFrom(i->pkt, virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetDstMAC(), i->protocolNumber);
                } else {
                    //NS_LOG_DEBUG("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *i->pkt);
                    ret = m_netDevice->Send(i->pkt, virtualLayerPlusHdr.GetDstMAC(), i->protocolNumber);
                }

                m_leaderSupportQueue.erase(i);

                return ret;
            }
        }

        return ret;

    }

    bool VirtualLayerPlusNetDevice::PutHost(Ipv4Address ip, Address mac) {

        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            if (i->ip.IsEqual(ip)) {
                NS_LOG_INFO(ip << " (" << mac << ")" << " ya estaba en la HostTable (hay " << m_hostsTable.size() << "), reiniciamos su timerLife...");
                i->timerLifeHostEntry.Cancel();
                NS_ASSERT_MSG(i->timerLifeHostEntry.GetDelay().IsPositive(), "Programando timer LifeHostEntry con tiempo negativo" << i->timerLifeHostEntry.GetDelay());
                i->timerLifeHostEntry.Schedule();

                return false;
            }
        }

        //m_hostsTable.push_back(HostEntry(ip, mac));
		HostEntry newHost(ip, mac);
		newHost.HostEntryRegion(ip, mac, GetIp(), GetMac(), GetVirtualIpOf(m_region), m_waysegment, m_region, m_bandwidth, m_rsat);
		
		m_hostsTable.push_back(newHost);        
		
		
		// removed to test ibr routing
		IbrRegionHostTable::HostEntry IbrNewHost(ip,mac);
		IbrNewHost.HostEntryRegion(ip, mac, GetIp(), GetMac(), GetVirtualIpOf(m_region), m_waysegment, m_region, m_bandwidth, m_rsat);
		VirtualLayerPlusNetDevice::ibr_hostsTable.push_back(IbrNewHost);        
		////NS_LOG_DEBUG(ip << " (" << mac << ")" << " añadido a la HostTable del nodo: " << m_node->GetId() << "\t; hay " << m_hostsTable.size());
        std::list<HostEntry>::iterator it = m_hostsTable.end();
        it--;
        it->timerLifeHostEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeHostEntryExpire, this);
        it->timerLifeHostEntry.SetArguments(ip);
        it->timerLifeHostEntry.SetDelay(TIME_LIFE_HOST_ENTRY);
        it->timerLifeHostEntry.Schedule();


        return true;
    }

    bool VirtualLayerPlusNetDevice::QuitHost(Ipv4Address ip) {
        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            if (i->ip.IsEqual(ip)) {
                i->timerLifeHostEntry.Cancel();
                m_hostsTable.erase(i);
				////NS_LOG_DEBUG(ip << " borrado de la HostTable, quedan " << m_hostsTable.size() << "\n");
			
				// remove from host list for ibr	
				if (VirtualLayerPlusNetDevice::ibr_hostsTable.size()>0) {
					VirtualLayerPlusNetDevice::removeHostFromTable(ip);
				}
			
                
				return true;
            }
        }
		
        return false;
    }

    bool VirtualLayerPlusNetDevice::ReschedTimerOfHost(Ipv4Address ip, Time t) {
        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            if (i->ip.IsEqual(ip)) {
                i->timerLifeHostEntry.Cancel();
                i->timerLifeHostEntry.SetDelay(t);
                i->timerLifeHostEntry.Schedule();
                NS_LOG_INFO("Timer de " << ip << " reprogramado con " << t.GetSeconds() << " segundos\n");

                return true;
            }
        }
        return false;
    }

    Ipv4Address VirtualLayerPlusNetDevice::GetIpOfHost(Address mac) {
        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            if (i->mac == mac) {

                return i->ip;
            }
        }
        return Ipv4Address::GetBroadcast();
    }

    Address VirtualLayerPlusNetDevice::GetMacOfHost(Ipv4Address ip) {
        for (std::list<HostEntry>::iterator i = m_hostsTable.begin(); i != m_hostsTable.end(); ++i) {
            if (i->ip.IsEqual(ip)) {

                return i->mac;
            }
        }
        return m_netDevice->GetBroadcast();
    }

    // Inserta Líder en la LeaderTable

    uint32_t VirtualLayerPlusNetDevice::PutLeader(Ipv4Address leaderIP, Address leaderMAC) {

        // Si ya lo tenemos en la tabla no lo volvemos a meter
        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->ip.IsEqual(leaderIP)) return i->id;
        }

        uint32_t c = 0;
        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->id > c) {
                i = m_leaderTable.insert(i, LeaderEntry(leaderIP, leaderMAC, c));
                i->timerLifeLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeLeaderEntryExpire, this);
                i->timerLifeLeaderEntry.SetArguments(i->id, i->ip);
                i->timerLifeLeaderEntry.SetDelay(TIME_WAIT_FOR_NEW_LEADER);
                return c;
            }
            c++;
        }
        m_leaderTable.push_back(LeaderEntry(leaderIP, leaderMAC, c));
        std::list<LeaderEntry>::iterator it = m_leaderTable.end();
        it--;
        it->timerLifeLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeLeaderEntryExpire, this);
        it->timerLifeLeaderEntry.SetArguments(it->id, it->ip);
        it->timerLifeLeaderEntry.SetDelay(TIME_WAIT_FOR_NEW_LEADER);

        return c;

    }

    // Inserta Líder en la LeaderTable

    void VirtualLayerPlusNetDevice::PutLeader(Ipv4Address leaderIP, Address leaderMAC, uint32_t leaderID) {

        // Si ya lo tenemos en la tabla no lo volvemos a meter
        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->ip.IsEqual(leaderIP)) return;
        }

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->id < leaderID) continue;
            if (i->id == leaderID) {
                i->timerLifeLeaderEntry.Cancel();
                i = m_leaderTable.erase(i);
            }

            // Si llegamos hasta aquí es que estamos en la posición correspondiente a leaderID
            i = m_leaderTable.insert(i, LeaderEntry(leaderIP, leaderMAC, leaderID));
            i->timerLifeLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeLeaderEntryExpire, this);
            i->timerLifeLeaderEntry.SetArguments(i->id, i->ip);
            i->timerLifeLeaderEntry.SetDelay(TIME_WAIT_FOR_NEW_LEADER);

            return;
        }
        m_leaderTable.push_back(LeaderEntry(leaderIP, leaderMAC, leaderID));
        std::list<LeaderEntry>::iterator it = m_leaderTable.end();
        it--;
        it->timerLifeLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeLeaderEntryExpire, this);
        it->timerLifeLeaderEntry.SetArguments(it->id, it->ip);
        it->timerLifeLeaderEntry.SetDelay(TIME_WAIT_FOR_NEW_LEADER);
    }

    void VirtualLayerPlusNetDevice::QuitLeader(Ipv4Address leaderIP) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->ip.IsEqual(leaderIP)) {
                if (i->timerLifeLeaderEntry.IsRunning()) i->timerLifeLeaderEntry.Cancel();
                m_leaderTable.erase(i);

                return;
            }
        }

    }

    // Inserta Líder en la LastLeadersTable

    void VirtualLayerPlusNetDevice::PushLastLeader(Ipv4Address leaderIP, Address leaderMAC, uint32_t leaderID) {

        // Si ya lo tenemos en la tabla lo borramos para meterlo dónde corresponda
        for (std::list<LeaderEntry>::iterator i = m_lastLeadersTable.begin(); i != m_lastLeadersTable.end(); ++i) {
            if (i->ip.IsEqual(leaderIP)) {
                m_lastLeadersTable.erase(i);
                break;
            }
        }

        bool inserted = false;
        for (std::list<LeaderEntry>::iterator i = m_lastLeadersTable.begin(); i != m_lastLeadersTable.end(); ++i) {
            if (i->id >= leaderID) {
                // Si llegamos hasta aquí es que estamos en la posición correspondiente a leaderID
                i = m_lastLeadersTable.insert(i, LeaderEntry(leaderIP, leaderMAC, leaderID));
                inserted = true;
                break;
            }
        }
        if (!inserted) m_lastLeadersTable.push_back(LeaderEntry(leaderIP, leaderMAC, leaderID));

        // Purgamos la tabla
        int count = 0;
        for (std::list<LeaderEntry>::iterator i = m_lastLeadersTable.begin(); i != m_lastLeadersTable.end(); ++i) {
            if (i->id == leaderID) {
                count++;
                if (count > MAX_LAST_LEADERS_PER_ID) i = m_lastLeadersTable.erase(i);
            }
        }

    }

    bool VirtualLayerPlusNetDevice::IsInLastLeadersTable(Address leaderMAC, uint32_t leaderID) {

        for (std::list<LeaderEntry>::iterator i = m_lastLeadersTable.begin(); i != m_lastLeadersTable.end(); ++i) {

            if ((i->mac == leaderMAC) && (i->id == leaderID)) return true;

        }

        return false;
    }

    long unsigned int VirtualLayerPlusNetDevice::GetNumOfLeaders() {
        return m_leaderTable.size();
    }

    void VirtualLayerPlusNetDevice::StartTimerLifeOf(uint32_t leaderID) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->id == leaderID)
                if (!i->timerLifeLeaderEntry.IsRunning()) {
                    i->timerLifeLeaderEntry.Schedule();
                }
        }

    }

    void VirtualLayerPlusNetDevice::StopTimerLifeOf(uint32_t leaderID) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->id == leaderID)

                if (i->timerLifeLeaderEntry.IsRunning()) i->timerLifeLeaderEntry.Cancel();
        }

    }

    bool VirtualLayerPlusNetDevice::IsInLeaderTable(Ipv4Address leaderIP) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {

            if (i->ip.IsEqual(leaderIP)) return true;
        }

        return false;
    }

    bool VirtualLayerPlusNetDevice::IsTheLatestLeader(uint32_t leaderID) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {

            if (i->id > leaderID) return false;
        }

        return true;

    }

    bool VirtualLayerPlusNetDevice::IsTheLessSupportedLeader(uint32_t leaderID) {

        uint32_t numBackUps = UINT32_MAX;
        uint32_t lessSupportedLeaderID = UINT32_MAX;

        uint32_t c;

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            c = 0;
            for (std::list<BackUpTableHeader::BackUpEntry>::iterator j = m_backUpTable.begin(); j != m_backUpTable.end(); ++j) {
                if (GetSupportedLeaderIdOf(j->ip) == i->id) c++;
            }
            if (c < numBackUps) {
                numBackUps = c;
                lessSupportedLeaderID = i->id;
            } else if ((c == numBackUps) && (i->id < lessSupportedLeaderID)) lessSupportedLeaderID = i->id;
        }

        if (leaderID == lessSupportedLeaderID) return true;

        else return false;

    }

    VirtualLayerPlusNetDevice::LeaderEntry VirtualLayerPlusNetDevice::GetLeader(uint32_t leaderID) {

        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {

            if (i->id == leaderID) return LeaderEntry(i->ip, i->mac, i->id);
        }

        return LeaderEntry(Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast(), UINT32_MAX);

    }

    void VirtualLayerPlusNetDevice::PrintLeaderTable() {
        NS_LOG_INFO("      IP                 MAC               ID \n");
        NS_LOG_INFO("--------------------------------------------------\n");
        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {

            NS_LOG_INFO("|  " << i->ip << "    " << i->mac << "      " << i->id << "    |\n");
            NS_LOG_INFO("--------------------------------------------------\n");
        }

    }

    bool VirtualLayerPlusNetDevice::NewLeaderActive() {
        for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
            if (i->id == m_leaderID + 1) {
                return true;
            }
        }
        return false;
    }

    // Inserta Líder en la NeighborLeadersTable

    void VirtualLayerPlusNetDevice::PutNeighborLeader(uint32_t leaderRegion, uint32_t leaderID, Ipv4Address leaderIP, Address leaderMAC) {

        for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i) {
            if (i->region < leaderRegion) continue;
            if (i->region == leaderRegion) {
                if (i->id < leaderID) continue;
                if (i->id == leaderID) {
                    i->timerLifeNeighborLeaderEntry.Cancel();
                    i->ip.Set(leaderIP.Get());
                    i->mac = leaderMAC;
                    i->timerLifeNeighborLeaderEntry.Schedule();

                    // Añadimos entrada con la IP virtual a nuestra caché ARP

                    //ArpCache::Entry *entry = m_arpCache->Lookup(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
                    //if (entry == NULL) entry = m_arpCache->Add(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
                    //entry->MarkWaitReply(0);
                    //entry->MarkAlive(GetVirtualAddressOf(leaderRegion));
                    ////entry->MarkAlive(leaderMAC);

                    //entry->ClearRetries();
                    //Ptr<Packet> pending = entry->DequeuePending();
                    //while (pending != 0) pending = entry->DequeuePending();

                    //NS_LOG_INFO("+++++++++++++ Añadido " << Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1) << " con la MAC " << leaderMAC << " a la ArpCache del nodo " << m_node->GetId());

                    return;
                }
            }

            // Si llegamos hasta aquí es que estamos en la posición correspondiente a leaderRegion y leaderID
            i = m_neighborLeadersTable.insert(i, NeighborLeaderEntry(leaderRegion, leaderID, leaderIP, leaderMAC));
            i->timerLifeNeighborLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeNeighborLeaderEntryExpire, this);
            i->timerLifeNeighborLeaderEntry.SetArguments(i->region, i->id);
            i->timerLifeNeighborLeaderEntry.SetDelay(TIME_LIFE_NEIGHBOR_LEADER_ENTRY);
            i->timerLifeNeighborLeaderEntry.Schedule();

            // Añadimos entrada con la IP virtual a nuestra caché ARP

            //ArpCache::Entry *entry = m_arpCache->Lookup(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
            //if (entry == NULL) entry = m_arpCache->Add(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
            //entry->MarkWaitReply(0);
            //entry->MarkAlive(GetVirtualAddressOf(leaderRegion));
            ////entry->MarkAlive(leaderMAC);

            //entry->ClearRetries();
            //Ptr<Packet> pending = entry->DequeuePending();
            //while (pending != 0) pending = entry->DequeuePending();

            //NS_LOG_INFO("+++++++++++++ Añadido " << Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1) << " con la MAC " << leaderMAC << " a la ArpCache del nodo " << m_node->GetId());

            return;
        }
        m_neighborLeadersTable.push_back(NeighborLeaderEntry(leaderRegion, leaderID, leaderIP, leaderMAC));
        std::list<NeighborLeaderEntry>::iterator it = m_neighborLeadersTable.end();
        it--;
        it->timerLifeNeighborLeaderEntry.SetFunction(&VirtualLayerPlusNetDevice::TimerLifeNeighborLeaderEntryExpire, this);
        it->timerLifeNeighborLeaderEntry.SetArguments(it->region, it->id);
        it->timerLifeNeighborLeaderEntry.SetDelay(TIME_LIFE_NEIGHBOR_LEADER_ENTRY);
        it->timerLifeNeighborLeaderEntry.Schedule();

        // Añadimos entrada con la IP virtual a nuestra caché ARP

        //ArpCache::Entry *entry = m_arpCache->Lookup(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
        //if (entry == NULL) entry = m_arpCache->Add(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
        //entry->MarkWaitReply(0);
        //entry->MarkAlive(GetVirtualAddressOf(leaderRegion));
        ////entry->MarkAlive(leaderMAC);

        //entry->ClearRetries();
        //Ptr<Packet> pending = entry->DequeuePending();
        //while (pending != 0) pending = entry->DequeuePending();

        //NS_LOG_INFO("+++++++++++++ Añadido " << Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1) << " con la MAC " << leaderMAC << " a la ArpCache del nodo " << m_node->GetId());

    }

    void VirtualLayerPlusNetDevice::QuitNeighborLeader(uint32_t leaderRegion, uint32_t leaderID) {

        for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i) {
            if ((i->region == leaderRegion) && (i->id == leaderID)) {
                if (i->timerLifeNeighborLeaderEntry.IsRunning()) i->timerLifeNeighborLeaderEntry.Cancel();
                m_neighborLeadersTable.erase(i);

                /*
                /
                // Si no queda ningún líder conocido en la región borramos la entrada virtual de la caché ARP
                if (GetNumOfNeighborLeadersIn(leaderRegion) == 0) {
                    ArpCache::Entry *entry = m_arpCache->Lookup(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
                    if (entry != NULL) {
                        entry->MarkDead();
                        entry->ClearRetries();

                        Ptr<Packet> pending = entry->DequeuePending();
                        if (pending != 0) m_dropTrace(VirtualLayerPlusNetDevice::DROP_NO_LEADER_RTX, pending, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                        while (pending != 0) {
                            pending = entry->DequeuePending();
                            if (pending != 0) m_dropTrace(VirtualLayerPlusNetDevice::DROP_NO_LEADER_RTX, pending, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                        }

                        NS_LOG_INFO("------------- Eliminado " << Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1) << " de la ArpCache del nodo " << m_node->GetId());
                    }
                }
                 //*/
                return;
            }
        }

    }

    uint32_t VirtualLayerPlusNetDevice::GetNumOfNeighborLeadersIn(uint32_t region) {
        uint32_t n = 0;
        for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i)
            if (i->region == region) n++;

        return n;
    }

    Address VirtualLayerPlusNetDevice::GetLeaderMacForRtx(uint32_t region, uint64_t uid) {

        uint32_t numberOfLeaders = 0;
        if (region == m_region)
            numberOfLeaders = GetNumOfLeaders();
        else
            numberOfLeaders = GetNumOfNeighborLeadersIn(region);

        double numOfLeader = 0;

        if (numberOfLeaders == 0) return m_netDevice->GetBroadcast();
        else if (numberOfLeaders != 1) {
            numOfLeader = static_cast<double> (uid);
            while ((numOfLeader - static_cast<double> (numberOfLeaders)) >= 0) numOfLeader -= static_cast<double> (numberOfLeaders);
        }

        if (region == m_region) {
            double c = 0;
            for (std::list<LeaderEntry>::iterator i = m_leaderTable.begin(); i != m_leaderTable.end(); ++i) {
                if (c == numOfLeader) return i->mac;
                else c++;
            }
        } else {
            double c = 0;
            for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i) {
                if (i->region == region) {
                    if (c == numOfLeader) return i->mac;
                    else c++;
                }
            }

        }
        return m_netDevice->GetBroadcast();
    }

    void VirtualLayerPlusNetDevice::UpdateNeighborLeadersTable() {

        //uint32_t leaderRegion = UINT32_MAX;

        for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i) {
            if (!IsNeighborRegion(i->region)) {

                if (i->timerLifeNeighborLeaderEntry.IsRunning()) i->timerLifeNeighborLeaderEntry.Cancel();
                //leaderRegion = i->region;
                i = m_neighborLeadersTable.erase(i);
                i--;

                /*
                /
                // Si no queda ningún líder conocido en la región borramos la entrada virtual de la caché ARP
                if (GetNumOfNeighborLeadersIn(leaderRegion) == 0) {
                    ArpCache::Entry *entry = m_arpCache->Lookup(Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1));
                    if (entry != NULL) {
                        entry->MarkDead();
                        entry->ClearRetries();

                        Ptr<Packet> pending = entry->DequeuePending();
                        if (pending != 0) m_dropTrace(VirtualLayerPlusNetDevice::DROP_NO_LEADER_RTX, pending, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                        while (pending != 0) {
                            pending = entry->DequeuePending();
                            if (pending != 0) m_dropTrace(VirtualLayerPlusNetDevice::DROP_NO_LEADER_RTX, pending, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());
                        }

                        NS_LOG_INFO("------------- Eliminado " << Ipv4Address(VIRTUAL_IP_BASE + leaderRegion + 1) << " de la ArpCache del nodo " << m_node->GetId());
                    }
                }
                //*/
            }
        }
    }

    void VirtualLayerPlusNetDevice::PrintNeighborLeadersTable() {
		NS_LOG_INFO("\nLista de lideres vecinos de la region: " << m_region);
        NS_LOG_INFO("Reg.ID" << "\t" << "IP             " << " \t" << "MAC");
        NS_LOG_INFO("-----------------------------------------------------------");
        for (std::list<NeighborLeaderEntry>::iterator i = m_neighborLeadersTable.begin(); i != m_neighborLeadersTable.end(); ++i) {

            NS_LOG_INFO(i->region << " " << i->id << "\t" << i->ip << "\t" << i->mac << "");
            
        }
        NS_LOG_INFO("-----------------------------------------------------------");

    }

    bool VirtualLayerPlusNetDevice::IsNeighborRegion(uint32_t region) {

        /*
        Vector3D pos = m_node->GetObject<MobilityModel>()->GetPosition();

        return ((region == GetRegionIdOf(Vector2D(pos.x + (double (m_long) / double (m_columns)), pos.y))) || (region == GetRegionIdOf(Vector2D(pos.x - (double (m_long) / double (m_columns)), pos.y))) || // Derecha e izquierda de nuestra región
                (region == GetRegionIdOf(Vector2D(pos.x, pos.y + (double (m_width) / double (m_rows))))) || (region == GetRegionIdOf(Vector2D(pos.x, pos.y - (double (m_width) / double (m_rows))))) || // Encima y debajo de nuestra región
                (region == GetRegionIdOf(Vector2D(pos.x + (double (m_long) / double (m_columns)), pos.y + (double (m_width) / double (m_rows))))) || (region == GetRegionIdOf(Vector2D(pos.x - (double (m_long) / double (m_columns)), pos.y + (double (m_width) / double (m_rows))))) || // Derecha e izquierda de la región de encima
                (region == GetRegionIdOf(Vector2D(pos.x + (double (m_long) / double (m_columns)), pos.y - (double (m_width) / double (m_rows))))) || (region == GetRegionIdOf(Vector2D(pos.x - (double (m_long) / double (m_columns)), pos.y - (double (m_width) / double (m_rows)))))); // Derecha e izquierda de la región de abajo
         */

        Vector2D coordThis = GetCoordinatesOfRegion(m_region);
        Vector2D coordNeigh = GetCoordinatesOfRegion(region);

        return (((std::abs(coordThis.x - coordNeigh.x) == 1) && (std::abs(coordThis.y - coordNeigh.y) <= 1)) || ((std::abs(coordThis.x - coordNeigh.x) <= 1) && (std::abs(coordThis.y - coordNeigh.y) == 1)));

    }

    bool VirtualLayerPlusNetDevice::IsVirtual(Address addr) {

        uint8_t mac[20];

        addr.CopyTo(mac);

        return ((mac[addr.GetLength() - 6] == 0x02) && (mac[addr.GetLength() - 5] == 0x00) && (GetRegionIdOf(addr) > 0));
    }

    uint32_t VirtualLayerPlusNetDevice::GetRegionIdOf(Address addr) {

        uint8_t mac[20];

        addr.CopyTo(mac);

        uint32_t region = ((static_cast<uint32_t> (mac[addr.GetLength() - 4]) << 24) | (static_cast<uint32_t> (mac[addr.GetLength() - 3]) << 16) | (static_cast<uint32_t> (mac[addr.GetLength() - 2]) << 8) | static_cast<uint32_t> (mac[addr.GetLength() - 1])) - 1;

        return region;
    }

    Address VirtualLayerPlusNetDevice::GetVirtualAddressOf(uint32_t region) {

        region++;

        uint8_t mac[20];

        m_mac.CopyTo(mac);

        mac[m_mac.GetLength() - 6] = 0x02;
        mac[m_mac.GetLength() - 5] = 0x00;
        mac[m_mac.GetLength() - 4] = static_cast<uint8_t> ((region & 0xFF000000) >> 24);
        mac[m_mac.GetLength() - 3] = static_cast<uint8_t> ((region & 0x00FF0000) >> 16);
        mac[m_mac.GetLength() - 2] = static_cast<uint8_t> ((region & 0x0000FF00) >> 8);
        mac[m_mac.GetLength() - 1] = static_cast<uint8_t> (region & 0x000000FF);

        Address addr;

        addr.CopyFrom(mac, m_mac.GetLength());

        return addr;
    }

    bool VirtualLayerPlusNetDevice::IsVirtual(Ipv4Address ip) {

        if ((!ip.IsBroadcast()) && (ip.Get() > VIRTUAL_IP_BASE)) return true;
        else return false;

    }

    uint32_t VirtualLayerPlusNetDevice::GetRegionIdOf(Ipv4Address ip) {

        if (!IsVirtual(ip)) return UINT32_MAX;
        else return (ip.Get() - VIRTUAL_IP_BASE - 1);

    }

    Ipv4Address VirtualLayerPlusNetDevice::GetVirtualIpOf(uint32_t region) {
        return Ipv4Address(VIRTUAL_IP_BASE + region + 1);
    }

    Vector2D VirtualLayerPlusNetDevice::GetCoordinateLimits() {
        return Vector2D(double(m_rows), double(m_columns));
    }

    Time VirtualLayerPlusNetDevice::GetHeartbeatPeriod() {
        return TIME_HEARTBEAT;
    }

    bool VirtualLayerPlusNetDevice::IsReplaced() {
        return m_replaced;
    }

    void VirtualLayerPlusNetDevice::RecvLeaderElection(Ptr<Packet> pkt) {

        
        
        ////NS_LOG_DEBUG("Recibido mensaje LeaderElection en el nodo " << m_node->GetId());

        VirtualLayerPlusHeader virtualLayerPlusHdr;
        pkt->RemoveHeader(virtualLayerPlusHdr);

        double Rand;

        if (virtualLayerPlusHdr.GetRegion() != m_region) {
            if (IsNeighborRegion(virtualLayerPlusHdr.GetRegion())) {
                switch (virtualLayerPlusHdr.GetSubType()) {
                        // Si recibimos un Heartbeat de una región vecina actualizamos la entrada de la tabla de líderes vecinos
                    case VirtualLayerPlusHeader::Heartbeat:
                        PutNeighborLeader(virtualLayerPlusHdr.GetRegion(), virtualLayerPlusHdr.GetLeaderID(), virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                        PrintNeighborLeadersTable();
                        break;
                    case VirtualLayerPlusHeader::CeaseLeaderAnnouncement:
                        // Debería esperar un tiempo antes de eliminarlo?
                        QuitNeighborLeader(virtualLayerPlusHdr.GetRegion(), virtualLayerPlusHdr.GetLeaderID());
                        PrintNeighborLeadersTable();
                        break;
                    default:;
                }
            }
            return;
        }

        switch (virtualLayerPlusHdr.GetSubType()) {
            case VirtualLayerPlusHeader::LeaderRequest:
                switch (m_state) {
                    case VirtualLayerPlusHeader::LEADER:
                        // Cada líder contesta y así sabe que líderes hay en la región
                        SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderReply, virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                        ////NS_LOG_DEBUG("Enviada respuesta a LeaderRequest desde lider ID: " << m_node->GetId());
                        break;
                    default:;
                }
                break;
            case VirtualLayerPlusHeader::LeaderReply:
                CancelTimers();
                m_expires = 0;
                m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                PrintLeaderTable();
                m_stateTrace(m_state, VirtualLayerPlusHeader::NONLEADER, m_region);
                m_state = VirtualLayerPlusHeader::NONLEADER;
                
                ////NS_LOG_DEBUG("Respuesta LiderReply recibida en nodo: " << m_node->GetId() << "\tdesde nodo lider: " << virtualLayerPlusHdr.GetLeaderID() << "\t" << virtualLayerPlusHdr.GetSrcAddress() << "\tRegion: " << m_region << ":" << virtualLayerPlusHdr.GetRegion());
                if (!m_timerGetLocation.IsRunning()) {
					m_timerGetLocation.Schedule(TIME_GET_NODE_POSITION);
				}
                break;
            case VirtualLayerPlusHeader::Heartbeat:
                switch (m_state) {
                    case VirtualLayerPlusHeader::LEADER:
                        if (virtualLayerPlusHdr.GetLeaderID() == m_leaderID) {
                            //ClearBackUp();
                            if (virtualLayerPlusHdr.GetTimeInState() < m_timeInState) { // No soy el lider
                                NS_LOG_INFO("Detección de líder duplicado en el nodo " << m_node->GetId() << " (" << m_ip << "). No soy el líder " << m_leaderID << ".");
                                CancelTimers();
                                m_timerRelinquishment.Cancel();
                                m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                                //QuitLeader(m_ip);
                                PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                                PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                                PrintLeaderTable();
                                if (IsInBackUpTable(virtualLayerPlusHdr.GetSrcAddress())) {
                                    if (m_multipleLeaders) QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), false);
                                    else QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), true);
                                    PrintBackUpTable();
                                }
                                m_stateTrace(m_state, VirtualLayerPlusHeader::NONLEADER, m_region);
                                m_state = VirtualLayerPlusHeader::NONLEADER;
                                m_timeInState = 0;
                                m_promiscuousMode = false;
                                m_leaderID = UINT32_MAX;
                            } else if (virtualLayerPlusHdr.GetTimeInState() == m_timeInState) { // Solicitud de lider nuevamente
                                NS_LOG_INFO("Detección de líder duplicado en el nodo " << m_node->GetId() << " (" << m_ip << "). Solicitud de líder " << m_leaderID << " nuevamente.");
                                CancelTimers();
                                m_timerRelinquishment.Cancel();
                                m_stateTrace(m_state, VirtualLayerPlusHeader::REQUEST, m_region);
                                m_state = VirtualLayerPlusHeader::REQUEST;
                                m_timeInState = 0;
                                m_promiscuousMode = false;
                                QuitLeader(m_ip);
                                PrintLeaderTable();
                                Rand = (rand() / (double) RAND_MAX)*0.2;
                                m_timerRequestWait.SetArguments(m_leaderID);
                                m_leaderID = UINT32_MAX;
                                m_timerRequestWait.Schedule(TIME_REQUEST_WAIT + Seconds(Rand));
                            } else { // Soy el lider
                                NS_LOG_INFO("Detección de líder duplicado en el nodo " << m_node->GetId() << " (" << m_ip << "). Soy el líder " << m_leaderID << ".");
                                CancelTimers();
                                SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::Heartbeat, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
                                m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                            }
                        } else {
                            if (!IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {

                                if (virtualLayerPlusHdr.GetLeaderID() == m_leaderID + 1) {
                                    m_newLeaderActive = true;
                                    StopTimerLifeOf(virtualLayerPlusHdr.GetLeaderID());
                                }
                                PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                                PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                                PrintLeaderTable();
                                if (IsInBackUpTable(virtualLayerPlusHdr.GetSrcAddress())) QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), false);
                            }
                        }
                        break;
                    case VirtualLayerPlusHeader::INTERIM:
                        if (virtualLayerPlusHdr.GetLeaderID() == m_leaderID) {
                            CancelTimers(); // Deja de enviar LeaderLefts hasta que abandone la región
                            m_replaced = true; // Deja de reenviar mensajes
                        }
                        break;
                    default:
                        // Un BackUp sincronizado sólo reestablece su timerHeartbeat si recibe mensajes de su líder
                        if ((m_multipleLeaders) && (m_backUpState == VirtualLayerPlusNetDevice::SYNC) && (m_supportedLeaderID != virtualLayerPlusHdr.GetLeaderID())) {
                            if (!IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {
                                PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                                PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                                PrintLeaderTable();
                                if (IsInBackUpTable(virtualLayerPlusHdr.GetSrcAddress())) QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), false);

                                if (m_supportedLeaderID != GetSupportedLeaderIdOf(m_ip)) { // Si paso a soportar a un nuevo líder tiro los paquetes que tenía del anterior
                                    m_supportedLeaderID = GetSupportedLeaderIdOf(m_ip);

                                    for (std::list<PacketEntry>::iterator i = m_leaderSupportQueue.begin(); i != m_leaderSupportQueue.end(); ++i) {
                                        i->timerLifePacketEntry.Cancel();
                                        i = m_leaderSupportQueue.erase(i);
                                        i--;
                                    }
                                    m_leaderSupportQueue.clear();

                                    if (m_supportedLeaderID == virtualLayerPlusHdr.GetLeaderID()) {
                                        CancelTimers();
                                        if (m_state == VirtualLayerPlusHeader::REQUEST) {
                                            m_stateTrace(m_state, VirtualLayerPlusHeader::NONLEADER, m_region);
                                            m_state = VirtualLayerPlusHeader::NONLEADER;
                                        }
                                        m_expires = 0;
                                        m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
                                    }
                                }
                            }
                            break;
                        }
                        CancelTimers();
                        if (m_state == VirtualLayerPlusHeader::REQUEST) {
                            m_stateTrace(m_state, VirtualLayerPlusHeader::NONLEADER, m_region);
                            m_state = VirtualLayerPlusHeader::NONLEADER;
                        }
                        m_expires = 0;
                        m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
						
                        if (!IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {
                            PutLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID());

                            PushLastLeader(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetLeaderID()); //******
                            PrintLeaderTable();
                            if (IsInBackUpTable(virtualLayerPlusHdr.GetSrcAddress())) {
                                if (m_multipleLeaders) QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), false);
                                else QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), true);
                            }
                        }

                        BackUpTableHeader::BackUpEntry priorityBackUp = GetPriorityBackUpOf(virtualLayerPlusHdr.GetLeaderID());

                        if ((virtualLayerPlusHdr.GetNumBackUp() == 0) && (m_backUpTable.size() > 0) && (priorityBackUp.isSynced)) {
                            if (priorityBackUp.ip.IsEqual(m_ip)) {
                                // Enviamos SyncData al líder con la BackUpTable + Información de estado de capas superiores (m_syncData)
                                
                                if (!SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::SyncData, virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC())) {
                                    m_timerRetrySyncData.Cancel();
                                    m_timerRetrySyncData.SetArguments(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                                    m_timerRetrySyncData.Schedule(TIME_RETRY_SYNC_DATA);
                                }
                                

                            }
                        } else if ((virtualLayerPlusHdr.GetNumBackUp() < virtualLayerPlusHdr.GetBackUpsNeeded())) {
                            if (m_backUpState != VirtualLayerPlusNetDevice::SYNC) {

                                m_backUpState = VirtualLayerPlusNetDevice::REQUESTING;
                                m_backUpPriority = virtualLayerPlusHdr.GetNumBackUp();

                                Rand = (rand() / (double) RAND_MAX) * 0.5;
                                m_timerBackUpRequest.Cancel();
                                m_timerBackUpRequest.Schedule(Seconds(Rand));
                            }
                        } else if (m_backUpState == VirtualLayerPlusNetDevice::REQUESTING) {
                            m_backUpState = VirtualLayerPlusNetDevice::NONE;
                            m_backUpPriority = UINT32_MAX;

                            m_supportedLeaderID = UINT32_MAX;

                            if (m_timerBackUpRequest.IsRunning()) m_timerBackUpRequest.Cancel();
                        }

                }
                break;
            case VirtualLayerPlusHeader::LeaderLeft:
                if (IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {
                    ReschedTimerOfHost(virtualLayerPlusHdr.GetSrcAddress(), Seconds(TIME_RELINQUISHMENT));
                    if ((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) {
                        if (virtualLayerPlusHdr.GetDstAddress().IsEqual(m_ip)) { // Soy el Priority Backup, me convierto en Lider
                            NS_LOG_INFO("Detección de abandono de líder en el nodo " << m_node->GetId() << " (" << m_ip << "). Soy el Priority Backup, me convierto en Lider " << virtualLayerPlusHdr.GetLeaderID() << ".");
                            CancelTimers();
                            QuitLeader(virtualLayerPlusHdr.GetSrcAddress());
                            PrintLeaderTable();
                            m_timerRequestWait.SetArguments(virtualLayerPlusHdr.GetLeaderID());
                            m_timerRequestWait.Schedule(Seconds(0.0));
                        } else {
                            CancelTimers();
                            m_stateTrace(m_state, VirtualLayerPlusHeader::REQUEST, m_region);
                            m_state = VirtualLayerPlusHeader::REQUEST;
                            QuitLeader(virtualLayerPlusHdr.GetSrcAddress());
                            PrintLeaderTable();

                            m_timerRequestWait.SetArguments(virtualLayerPlusHdr.GetLeaderID());

                            if (m_backUpState != VirtualLayerPlusNetDevice::NONE) { // Es BackUp
                                if (m_supportedLeaderID == virtualLayerPlusHdr.GetLeaderID()) { // Es BackUp del líder que abandona la región
                                    if (m_backUpState == VirtualLayerPlusNetDevice::SYNC) { // Es BackUp sincronizado del líder que abandona la región
                                        Time ti = Seconds(0.2 - double((((double) m_backUpTable.back().priority + 1) - (double) m_backUpPriority)*0.1 / ((double) m_backUpTable.back().priority + 1)));
                                        NS_ASSERT_MSG(ti.IsPositive(), "Programando timer RequestWait con tiempo negativo " << ti);
                                        m_timerRequestWait.Schedule(Seconds(0.2 - double((((double) m_backUpTable.back().priority + 1) - (double) m_backUpPriority)*0.1 / ((double) m_backUpTable.back().priority + 1))));
                                    } else { // Es BackUp no sincronizado del líder que abandona la región
                                        Time ti = Seconds(0.3 - double((((double) m_backUpTable.back().priority + 1) - (double) m_backUpPriority)*0.1 / ((double) m_backUpTable.back().priority + 1))) + dt;
                                        NS_ASSERT_MSG(ti.IsPositive(), "Programando timer RequestWait con tiempo negativo " << ti);
                                        m_timerRequestWait.Schedule(Seconds(0.3 - double((((double) m_backUpTable.back().priority + 1) - (double) m_backUpPriority)*0.1 / ((double) m_backUpTable.back().priority + 1))) + dt);
                                    }
                                }
                            } else { // No es BackUp
                                m_timerRequestWait.Schedule(Seconds(((double) rand() / (double) RAND_MAX)*0.1 + 0.4));
                            }

                        }
                    } else if ((m_state == VirtualLayerPlusHeader::LEADER) || (m_state == VirtualLayerPlusHeader::INTERIM)) {
                        StartTimerLifeOf(virtualLayerPlusHdr.GetLeaderID());
                    }
                }
                break;
            case VirtualLayerPlusHeader::LeaderBecomeRequest:
                NS_LOG_INFO("Recibido LeaderBecomeRequest en el nodo " << m_node->GetId());
                if (IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {
                    if ((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) {
                        CancelTimers();
                        NS_LOG_INFO("Programamos el TimerRequestWait con ID: " << virtualLayerPlusHdr.GetLeaderID() + 1);
                        m_timerRequestWait.SetArguments(virtualLayerPlusHdr.GetLeaderID() + 1);
                        m_timerRequestWait.Schedule(Seconds(0.0));
                    } else if (m_state == VirtualLayerPlusHeader::LEADER) { // El otro líder aún no sabe que ya soy líder
                        SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::Heartbeat, virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                    }
                }
                break;
            case VirtualLayerPlusHeader::CeaseLeaderAnnouncement:

                if (IsInLeaderTable(virtualLayerPlusHdr.GetSrcAddress())) {
                    if ((m_state == VirtualLayerPlusHeader::LEADER) || (m_state == VirtualLayerPlusHeader::INTERIM)) {
                        if (virtualLayerPlusHdr.GetLeaderID() == m_leaderID + 1) m_newLeaderActive = false;
                        QuitLeader(virtualLayerPlusHdr.GetSrcAddress());
                        PrintLeaderTable();
                    } else {
                        QuitLeader(virtualLayerPlusHdr.GetSrcAddress());
                        PrintLeaderTable();

                        if (m_backUpState == VirtualLayerPlusNetDevice::SYNC) m_supportedLeaderID = GetSupportedLeaderIdOf(m_ip);
                    }
                }

                break;
            default:;
        }

		
		 
    }

    void VirtualLayerPlusNetDevice::RecvSynchronization(Ptr<Packet> pkt) {

        NS_LOG_INFO("Recibido mensaje Synchronization en el nodo " << m_node->GetId());

        VirtualLayerPlusHeader virtualLayerPlusHdr;
        pkt->RemoveHeader(virtualLayerPlusHdr);

        if (virtualLayerPlusHdr.GetRegion() != m_region) {
            // Drop
            return;
        }
        switch (virtualLayerPlusHdr.GetSubType()) {
            case VirtualLayerPlusHeader::SyncRequest:
                if (m_state == VirtualLayerPlusHeader::LEADER) {
                    if (m_backUpTable.size() < MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE))) {
                        if (!m_multipleLeaders) PutBackUp(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetBackUpPriority());
                        else PutBackUp(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                        PrintBackUpTable();

                        if (!m_multipleLeaders || (m_multipleLeaders && IsTheLessSupportedLeader(m_leaderID)))
                            if (!SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::SyncData, virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC())) {
                                m_timerRetrySyncData.Cancel();
                                m_timerRetrySyncData.SetArguments(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                                m_timerRetrySyncData.Schedule(TIME_RETRY_SYNC_DATA);
                            }

                        m_timerBackUpRequest.Cancel();
                        m_timerBackUpRequest.Schedule(Seconds(1));
                    }
                } else if ((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) {
                    if ((m_backUpState == VirtualLayerPlusNetDevice::REQUESTING) && (m_timerBackUpRequest.IsRunning())) m_backUpPriority++;
                }
                break;
            case VirtualLayerPlusHeader::SyncData:
                if ((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) {

                    if (m_backUpState == VirtualLayerPlusNetDevice::REQUESTING) {
                        // Guardar los datos del SyncData
                        //m_syncData = new uint8_t[pkt->GetSize()];
                        //pkt->CopyData(m_syncData, pkt->GetSize());
                        // Aquí habría que enviar la m_syncData a las capas superiores

                        RoutingTableHeader routingTableHdr;

                        pkt->RemoveHeader(routingTableHdr);

                        Ipv4StaticRoutingHelper sr;

                        Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
                        Ptr<Ipv4StaticRouting> staticRouting = sr.GetStaticRouting(ipv4);

                        if (staticRouting != 0) {
                            std::list<RoutingTableHeader::RouteEntry> routes = routingTableHdr.GetRoutingTable();
                            for (std::list<RoutingTableHeader::RouteEntry>::iterator i = routes.begin(); i != routes.end(); ++i) {
                                staticRouting->AddHostRouteTo(i->dest, i->nextHop, i->interface, i->metric);
                                NS_LOG_INFO("Añadida ruta hacia " << i->dest << " pasando por " << i->nextHop);
                            }
                        }

                        m_backUpState = VirtualLayerPlusNetDevice::SYNC;

                        if (m_multipleLeaders) m_backUpPriority = virtualLayerPlusHdr.GetBackUpPriority();
                        // Se mete en su tabla de BackUps
                        PutBackUp(m_ip, m_mac, m_backUpPriority);
                        SyncBackUp(m_ip);
                        PrintBackUpTable();

                        m_supportedLeaderID = GetSupportedLeaderIdOf(m_ip);

                        NS_LOG_INFO(m_ip << " (" << m_backUpPriority << ")" << " apoya al líder " << m_supportedLeaderID);

                        if (m_supportiveBackups) m_promiscuousMode = true;

                        SendPkt(VirtualLayerPlusHeader::Synchronization, VirtualLayerPlusHeader::SyncAck, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
                    }
                } else if (m_state == VirtualLayerPlusHeader::LEADER) {
                    // Leer BackUpTable
                    BackUpTableHeader backUpTableHdr;
                    pkt->RemoveHeader(backUpTableHdr);
                    LoadBackUpTable(backUpTableHdr);
                    PrintBackUpTable();

                    // Leer SyncData y pasársela a capas superiores
                    //m_syncData = new uint8_t[pkt->GetSize()];
                    //pkt->CopyData(m_syncData, pkt->GetSize());

                    RoutingTableHeader routingTableHdr;

                    pkt->RemoveHeader(routingTableHdr);

                    Ipv4StaticRoutingHelper sr;

                    Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
                    Ptr<Ipv4StaticRouting> staticRouting = sr.GetStaticRouting(ipv4);

                    if (staticRouting != 0) {
                        std::list<RoutingTableHeader::RouteEntry> routes = routingTableHdr.GetRoutingTable();
                        for (std::list<RoutingTableHeader::RouteEntry>::iterator i = routes.begin(); i != routes.end(); ++i) {
                            staticRouting->AddHostRouteTo(i->dest, i->nextHop, i->interface, i->metric);
                        }
                    }

                }
                break;
            case VirtualLayerPlusHeader::SyncAck:
                PutBackUp(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC(), virtualLayerPlusHdr.GetBackUpPriority());
                SyncBackUp(virtualLayerPlusHdr.GetSrcAddress());
                PrintBackUpTable();
                break;
            case VirtualLayerPlusHeader::BackUpLeft:
                QuitHost(virtualLayerPlusHdr.GetSrcAddress());
                if (m_multipleLeaders) QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), false);
                else QuitBackUp(virtualLayerPlusHdr.GetSrcAddress(), true);
                PrintBackUpTable();
                if ((m_backUpTable.size() == 0) && (m_state == VirtualLayerPlusHeader::LEADER)) {
                    CancelTimers();
                    m_timerHeartbeat.Schedule(Seconds(0.0));
                }
                if ((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) {
                    if (m_backUpState != VirtualLayerPlusNetDevice::NONE) {
                        if (!m_multipleLeaders)

                            if (virtualLayerPlusHdr.GetBackUpPriority() < m_backUpPriority) m_backUpPriority--; // Si el BackUp que se va tenía más prioridad que tú, subes tu prioridad
                    }
                }
                break;
            default:;
        }

    }

    void VirtualLayerPlusNetDevice::RecvHello(Ptr<Packet> pkt) {

        NS_LOG_INFO("Recibido mensaje Hello en el nodo " << m_node->GetId());

        VirtualLayerPlusHeader virtualLayerPlusHdr;
        pkt->RemoveHeader(virtualLayerPlusHdr);

        if (virtualLayerPlusHdr.GetRegion() != m_region) {
			// buscamos y eliminamos el host de nuestra lista actual, si proviene de otra region
			QuitHost(virtualLayerPlusHdr.GetSrcAddress());
			//NS_LOG_DEBUG("Removiendo IP "<< virtualLayerPlusHdr.GetSrcAddress() << " de region " << m_region  );
			////NS_LOG_DEBUG("-\t(hello) Hosts en region " << m_region << ": " << m_hostsTable.size() );
            return;
        }
        switch (virtualLayerPlusHdr.GetSubType()) {
            case VirtualLayerPlusHeader::hello:
                //NS_LOG_DEBUG("+\t(hello) Insertamos " << virtualLayerPlusHdr.GetSrcAddress() << " en la HostTable\tRegion: " << m_region);
                // insertamos el host en la tabla de hosts del lider actual
                PutHost(virtualLayerPlusHdr.GetSrcAddress(), virtualLayerPlusHdr.GetSrcMAC());
                
                // obtenemos el numero de nodos en esa region
                //NS_LOG_DEBUG("+\t(hello) Hosts en region " << m_region << ": " << m_hostsTable.size() );
                break;
            default:;
        }
    }

    /*
     * Funcion: Envia mensajes de tipo Virtual hacia otros nodos
     * de forma Unicast y Broadcast
     */

    bool VirtualLayerPlusNetDevice::SendPkt(VirtualLayerPlusHeader::type msgType, VirtualLayerPlusHeader::subType msgSubType, Ipv4Address ipDst, Address macDst) {

        Ptr<Packet> pkt;

        if (msgSubType == VirtualLayerPlusHeader::SyncData) {

            if (m_state == VirtualLayerPlusHeader::LEADER) { // El líder sincroniza a un BackUp no sincronizado
                RoutingTableHeader routingTableHdr = RoutingTableHeader();
                // Aquí habría que pedir los datos de interés a capas superiores
                Ipv4StaticRoutingHelper sr;

                Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
                Ptr<Ipv4StaticRouting> staticRouting = sr.GetStaticRouting(ipv4);

                if (staticRouting != 0) {
                    for (uint32_t i = 0; i < staticRouting->GetNRoutes(); i++) {
                        Ipv4RoutingTableEntry route = staticRouting->GetRoute(i);

                        if (route.IsHost()) {
                            RoutingTableHeader::RouteEntry routeEntry = RoutingTableHeader::RouteEntry(route.GetDest(), route.GetGateway(), route.GetInterface());

                            routingTableHdr.AddRouteToTable(routeEntry);
                        }
                    }
                }
                //m_syncData = new uint8_t[1000];
                //pkt = Create<Packet>(m_syncData, 1000);

                pkt = Create<Packet>();
                pkt->AddHeader(routingTableHdr);
            } else if (((m_state == VirtualLayerPlusHeader::NONLEADER) || (m_state == VirtualLayerPlusHeader::REQUEST)) &&
                    (m_backUpState == VirtualLayerPlusNetDevice::SYNC) &&
                    (GetPriorityBackUpOf(m_supportedLeaderID).ip.IsEqual(m_ip))) { // El BackUp Priority sincronizado sincroniza al líder
                // Aquí hay que guardar la BackUpTable y la SyncData en un buffer y crear el paquete con él

                RoutingTableHeader routingTableHdr = RoutingTableHeader();

                Ipv4StaticRoutingHelper sr;

                Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
                Ptr<Ipv4StaticRouting> staticRouting = sr.GetStaticRouting(ipv4);

                if (staticRouting != 0) {
                    for (uint32_t i = 0; i < staticRouting->GetNRoutes(); i++) {
                        Ipv4RoutingTableEntry route = staticRouting->GetRoute(i);

                        if (route.IsHost()) {
                            RoutingTableHeader::RouteEntry routeEntry = RoutingTableHeader::RouteEntry(route.GetDest(), route.GetGateway(), route.GetInterface());

                            routingTableHdr.AddRouteToTable(routeEntry);
                        }
                    }
                }

                pkt = Create<Packet>();
                pkt->AddHeader(routingTableHdr);

                //pkt = Create<Packet>(m_syncData, 1000);

                BackUpTableHeader backUpTableHdr = BackUpTableHeader();

                backUpTableHdr.SetBackUpTable(m_backUpTable);

                pkt->AddHeader(backUpTableHdr);
            }
        } else pkt = Create<Packet>();

        NS_LOG_INFO("Generando paquete de la VNLayer en el nodo " << m_node->GetId() << " para " << macDst << ", enviándolo hacia el NetDevice...");

        VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();

        virtualLayerPlusHdr.SetType(msgType);
        virtualLayerPlusHdr.SetSubType(msgSubType);
        virtualLayerPlusHdr.SetRegion(m_region);
        virtualLayerPlusHdr.SetSrcAddress(m_ip);
        virtualLayerPlusHdr.SetSrcMAC(m_mac);
        virtualLayerPlusHdr.SetDstAddress(ipDst);
        virtualLayerPlusHdr.SetDstMAC(macDst);
        virtualLayerPlusHdr.SetRol(m_state);
        virtualLayerPlusHdr.SetLeaderID(m_leaderID);
        virtualLayerPlusHdr.SetTimeInState(m_timeInState);
        virtualLayerPlusHdr.SetNumBackUp(m_backUpTable.size());
        virtualLayerPlusHdr.SetBackUpsNeeded(MAX((MIN_NUM_BACKUPS_PER_LEADER * m_leaderTable.size()), std::ceil((float) m_hostsTable.size() * BACKUPS_PERCENTAGE)));
        if ((m_multipleLeaders) && (msgSubType == VirtualLayerPlusHeader::SyncData) && (m_state == VirtualLayerPlusHeader::LEADER))
            virtualLayerPlusHdr.SetBackUpPriority(GetPriorityOf(ipDst));
        else
            virtualLayerPlusHdr.SetBackUpPriority(m_backUpPriority);


        pkt->AddHeader(virtualLayerPlusHdr);

        m_txTrace(pkt, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

        NS_LOG_INFO("VirtualLayerPlusNetDevice::Send " << m_node->GetId() << " " << *pkt);
        return m_netDevice->Send(pkt, macDst, 0);

    }

	// *********************
    // pato

	Vector2D VirtualLayerPlusNetDevice::GetCoordinatesOfRegion(uint32_t region) {

        Vector2D v;
        v.x = 0;
        v.y = 0; //double(region);
		
		// calculamos la posicion de la region en funcion de el numero de intersecciones y las regiones del segmento
		
		int i=(int)(region/(numRegionsX));
		int j=(int)(i*numRegionsX);
		v.y=i;
		v.x=region-j;
				
        return v;
    }

    uint32_t VirtualLayerPlusNetDevice::GetRegionForCoordinates(Vector2D coord) {
        return static_cast<uint32_t> ((coord.x * m_columns) + coord.y);
    }

    uint32_t VirtualLayerPlusNetDevice::GetVirtualNodeLevel(uint32_t reg) {

        Vector2D coord = GetCoordinatesOfRegion(reg);
        
		// condicion inicial
		m_virtualNodeLevel=3;
		
		// verificamos si es nodo nivel 2
		// si estan uno antes o despues de las intersecciones, toma como nivel 2
		
		double difX=(double)coord.x/(double)(m_scnRegBetIntX+1);
		difX=difX-(int)difX;
		
		double difY=(double)coord.y/(double)(m_scnRegBetIntY+1);
		difY=difY-(int)difY;
		
		if (difX==0) {
			if (difY==(1.0/(m_scnRegBetIntY+1)) || difY==(1.0-(1.0/(m_scnRegBetIntY+1)))) {
				m_virtualNodeLevel=2;
			}
		}
	
		if (difY==0) {
			if (difX==(1.0/(m_scnRegBetIntX+1)) || difX==(1.0-(1.0/(m_scnRegBetIntX+1)))) {
				m_virtualNodeLevel=2;
			}
		}
		
		
		// si esta en las intersecciones es nivel 1
		if ((uint32_t)coord.x%(m_scnRegBetIntX+1)==0 && (uint32_t)coord.y%(m_scnRegBetIntY+1)==0) {
			m_virtualNodeLevel=1;
		}
		
		/*
		if (IsLeader()==false) {
			//NS_LOG_DEBUG( reg << "\t" << coord.x << "\t" << coord.y << "\t" << m_virtualNodeLevel << "\t" << difX << "\t" << difY << "\n");	
		}
		*/
		
        return m_virtualNodeLevel;
        
    }

	double VirtualLayerPlusNetDevice::GetNewBandwidth() {
		// if its a wifi assigned node sets the default bandwidth
		// else assigns a random one

		
		double bw=0.0;
		if (
			m_wsTable->isRegionInWaySegment((WIFI_CENTER_REGION-WIFI_OFFSET_REGION_2), m_waysegment) ||
			m_wsTable->isRegionInWaySegment((WIFI_CENTER_REGION-WIFI_OFFSET_REGION_1), m_waysegment) ||
			m_wsTable->isRegionInWaySegment((WIFI_CENTER_REGION), m_waysegment) ||
			m_wsTable->isRegionInWaySegment((WIFI_CENTER_REGION+WIFI_OFFSET_REGION_1), m_waysegment) ||
			m_wsTable->isRegionInWaySegment((WIFI_CENTER_REGION+WIFI_OFFSET_REGION_2), m_waysegment) 
		   )
				
			 {
			////NS_LOG_UNCOND("Wifi BW ->\tRegion: " << m_region << "\tws: " << m_wsTable->getWaySegment(m_region));	
			bw=(double)(WIFI_BW);
		
		} else {
			// ancho de banda entre 1 y 15 Mbps (3/4G)
			bw = (double) (rand() / (double) RAND_MAX) * 14.0 + 1.0;
			////NS_LOG_DEBUG("BW: " << bw);
		}
		return bw;
	}

	Time VirtualLayerPlusNetDevice::GetRSAT(double posX, double posY, double spdX, double spdY) {
		
		double tiempo=0.0;
		// calculamos la distancia pendiente antes de salir de la region
		double distX=fmod(posX, m_regionSize);
		double distY=fmod(posY, m_regionSize); //posY%m_regionSize;
		double dist;
		
		if (spdX==0.0 && spdY==0.0) {
			// detenido, devolvemos 0
			return Seconds(UINT32_MAX);
		}
		
		if (abs(spdX)>=abs(spdY)) {
			// desplazamiento con tendencia en X
			if (spdX>=0) {
				// desplazamiento positivo
				dist=m_regionSize-distX;
				
			} else {
				// desplazamiento negativo
				dist=m_regionSize-(m_regionSize-distX);
			}
			tiempo=dist/abs(spdX);
		} else {
			// desplazamiento con tendencia en Y
			if (spdY>=0) {
				// desplazamiento positivo
				dist=m_regionSize-distY;
				
			} else {
				// desplazamiento negativo
				dist=m_regionSize-(m_regionSize-distY);
			}
			tiempo=dist/abs(spdY);
		}
		////NS_LOG_DEBUG(posX << "\t" << posY << "\t" << spdX << "\t" << spdY << "\t" << dist << "\t" << tiempo ) ;
		return Seconds(tiempo);
	}
    
    void VirtualLayerPlusNetDevice::TimerGetLocationExpire() {
		uint32_t miRegion;
		uint8_t fil, col;
		double deltaX, deltaY;
		
		////NS_LOG_DEBUG("timer GetLocation en el nodo " << m_node->GetId() << ", actualizando región...");

		
		//obtenemos la posicion del nodo
		Vector3D pos = m_node->GetObject<MobilityModel>()->GetPosition();
		
		// obtenemos la region
		
		fil=floor(pos.y/m_regionSize);
		col=floor(pos.x/m_regionSize);
		
		miRegion=(uint32_t) (fil*numRegionsX + col);
		
		
		
		// verificamos el cambio de region
		if (miRegion!=m_region) {
			m_regionAnterior=m_region;
			m_regionActual=miRegion;
			m_region=miRegion;
			// obtenemos el nivel de la region
		
			m_virtualNodeLevel = GetVirtualNodeLevel(m_region);
			m_waysegment=GetActualWaySegment(m_region);
			// update the region and waysegment on the SCMA layer
			nodeSM.m_waysegment=m_waysegment;
			nodeSM.m_region=m_region;
			nodeSM.m_nodeLevel=m_virtualNodeLevel;

			////NS_LOG_DEBUG("setting SCMA region in region: " << miRegion << " ws: " << m_waysegment);
			// lauch the SCMA augmentation request from node 5
			// executed once
			/*
			if ( m_node->GetId()==5) {
				m_timerLaunchAugmentation.Schedule(TIME_LAUNCH_AUGMENTATION);
			}
			*/	
						// cambio de region
			
			////NS_LOG_DEBUG("Nodo: " << m_node->GetId() << " \t" << "Pos X: " << pos.x << "\t" << "Pos Y: " << pos.y );
			////NS_LOG_DEBUG("Cambio de nodo " << m_node->GetId() << " Region:  " << m_regionAnterior << "->" << m_region << "\tNivel: " << m_virtualNodeLevel << "\tEstado: " << m_state  );
					
			
			// asignamos el nivel de la region
			
			if (m_virtualNodeLevel==1) {
				nodeSM.SetIntersectionFlag(1);
			} else {
				if (nodeSM.IntersectionFlag==1) nodeSM.SetIntersectionFlag(0);
			}
			
			// removed to test ibr routing
			// pass the node state to the IBR layer
			// VirtualLayerPlusNetDevice::ibr.ibr_state=m_state;

			if (m_state == VirtualLayerPlusHeader::LEADER) {
				// colocamos el nodo el estado Lider
				m_state = VirtualLayerPlusHeader::LEADER;
				nodeSM.m_state=m_state;
				SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::Heartbeat, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
				m_timerHeartbeat.Cancel();
				m_expires=0;
				m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
				//hostListSyncDirection=true;
				// setup the IBR layer on the node
				// removed to test ibr routing
				/*
				VirtualLayerPlusNetDevice::ibr.ibr_netDevice=m_netDevice;
				VirtualLayerPlusNetDevice::ibr.m_node=m_node;
				VirtualLayerPlusNetDevice::ibr.m_region=m_region;
				VirtualLayerPlusNetDevice::ibr.ibr_waysegment=m_waysegment;
				VirtualLayerPlusNetDevice::ibr.ibr_virtualNodeLevel=m_virtualNodeLevel;
				VirtualLayerPlusNetDevice::ibr.waySegmentMembers = waySegmentMembers;
				VirtualLayerPlusNetDevice::ibr.hostListSyncDirection=false;
				VirtualLayerPlusNetDevice::ibr.ibr_ip=GetIp();
				*/

				// enable ibr pingpong if the node is L2 and the next region is an intersection

				//if (m_node->GetId()>=275 && m_node->GetId()<=356) {
					//m_timerSyncHostsRegion.Cancel();
					//m_timerSyncHostsRegion.Schedule(TIME_DISCOVERY);
					
					// start ibr pingpong sequence

						if (isL2Region(m_region, true)==true) {
							////NS_LOG_DEBUG("*** pingpong in node: " << m_node->GetId() << " Region: " << m_region);
							// removed to test ibr routing
							// VirtualLayerPlusNetDevice::ibr.PingPongStart();
						}
				//}
			

			} else {
			
				// mobile node setup
				//if (m_node->GetId()==3 || m_node->GetId()==4 || m_node->GetId()==2) {
					////NS_LOG_DEBUG("Cambio de nodo " << m_node->GetId() << " Region:  " << m_regionAnterior << "->" << m_region << "\tNivel: " << m_virtualNodeLevel << "\tEstado: " << m_state << "\tWS: " << GetActualWaySegment(m_region));
					// habilitamos el timer de envio de mensajes HELLO desde en nodo movil
										// enviamos un mensaje HELLO el momento de cambio de region
										// enviamos un leaderelection avisando el cambio de region
					////NS_LOG_DEBUG("*\t(NewRegion) Hosts en region " << m_region << ": " << m_hostsTable.size() );
					//
					if (m_region==(uint32_t)SCMA_DEBUG_REGION || m_region==(uint32_t)(SCMA_DEBUG_REGION)-1 || m_region==(uint32_t)(SCMA_DEBUG_REGION)-2) {
						if (!m_timerHello.IsRunning()) {
							double Rand = (rand() / (double) RAND_MAX)*0.5+1.0;
							m_timerHello.Schedule(Seconds(Rand));
						}

						SendPkt(VirtualLayerPlusHeader::Hello, VirtualLayerPlusHeader::hello, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());

						SendPkt(VirtualLayerPlusHeader::LeaderElection, VirtualLayerPlusHeader::LeaderRequest, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
											
						////NS_LOG_DEBUG("Enviado LeaderElection desde nodo: " << m_node->GetId() );
						m_timerHeartbeat.Cancel();
						m_expires=0;
						m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
					}
										
				//}
				
			}
			
			// calculamos el nuevo ancho de banda
			m_bandwidth=VirtualLayerPlusNetDevice::GetNewBandwidth();
		 	nodeSM.m_bandwidth=m_bandwidth;	
		
			// launch a scma if enters in the debug region	
			if (m_region==(uint32_t)SCMA_DEBUG_REGION && m_state != VirtualLayerPlusHeader::LEADER) {
				VirtualLayerPlusNetDevice::SendAugmentationRequest();
			}

		} 
		
		// calculamos RSAT
		if (m_state != VirtualLayerPlusHeader::LEADER) {
			// calculamos la velocidad actual
			
			deltaX=(pos.x-m_posXLast);
			deltaY=(pos.y-m_posYLast);
			
			m_spdX=deltaX/double(POSITION_TIMER_INTERVAL);
			m_spdY=deltaY/double(POSITION_TIMER_INTERVAL);
			
			// calculamos el tiempo estimado de salida de la region
			m_rsat = VirtualLayerPlusNetDevice::GetRSAT(pos.x, pos.y, m_spdX, m_spdY);
			////NS_LOG_DEBUG("Nodo " << m_node->GetId() << " Region:  " <<  m_region << "\tRSAT: " << m_rsat );
			
			m_posXLast=pos.x;
			m_posYLast=pos.y;
			
			// rehabilitamos el heartbeat si se detiene
			if (!m_timerHeartbeat.IsRunning()) {
				m_expires=0;
				m_timerHeartbeat.Schedule(TIME_HEARTBEAT);
			}
		}
		
		// habilitamos nuevamente el timer de posicion
		m_timerGetLocation.Schedule(TIME_GET_NODE_POSITION);
		
	}
	

		
	uint32_t VirtualLayerPlusNetDevice::GetActualWaySegment(uint32_t region) {
		// obtiene el id del segmento de via, con el ID de la region
		// si llega a una interseccion, devuelve el primer segmento de via encontrado
		
		uint32_t waySeg=m_wsTable->getWaySegment(region);
		/*
		for (std::list<IbrRoutingProtocol::wayMembers>::iterator ws = waySegmentMembers->begin(); ws != waySegmentMembers->end(); ws++) {
			if (ws->m_region==(int)region) {
				return ws->m_wayNumber;
			}
		}
		*/
		return waySeg;
	}
	
	bool VirtualLayerPlusNetDevice::IsRegionInWaySegment(uint32_t region, uint32_t waysegment) {

		// find the region of the incoming packet, from the region variable
		/*
		uint32_t WsPacket=GetActualWaySegment(region);
		// checks if a region is inside a waysegment (special case for intersections)
		for (std::list<IbrRoutingProtocol::wayMembers>::iterator ws = waySegmentMembers->begin(); ws != waySegmentMembers->end(); ws++) {
			////NS_LOG_DEBUG((int)region << "\t" << (int)waysegment << "\t" << ws->m_region << "\t" << ws->m_wayNumber);
			//if (ws->m_region==(int)region && ws->m_wayNumber==(int)waysegment) {
			if (ws->m_region==(int)m_region && ws->m_wayNumber==(int)WsPacket) {
				////NS_LOG_DEBUG("Encontrado!");

				return true;
			}
		}
		*/
		return false;
	}

	bool VirtualLayerPlusNetDevice::isL2Region(uint32_t m_region, bool dir) {
		/*/
		for (std::list<IbrRoutingProtocol::wayMembers>::iterator ws = waySegmentMembers->begin(); ws != waySegmentMembers->end(); ws++) {
			uint32_t neighbourRegion;

			if (ws->m_region==(int)m_region && ws->m_regionLevel==2) {
				if (dir==true) { 
					neighbourRegion=ws->m_neighbourNext;		
				} else {
					neighbourRegion=ws->m_neighbourPrev;		
				}
				if (VirtualLayerPlusNetDevice::getRegionLevel(neighbourRegion)==1) {
					return true;
				}
			}
		}
		*/
		return false;
	
	}

	uint32_t VirtualLayerPlusNetDevice::getRegionLevel(uint32_t m_region) {
		/*
		for (std::list<IbrRoutingProtocol::wayMembers>::iterator ws = waySegmentMembers->begin(); ws != waySegmentMembers->end(); ws++) {
			if (ws->m_region==(int)m_region ) {
				return 	ws->m_regionLevel;
			}
		}
		*/
		return UINT32_MAX;

	}

	// Augmentation launch routines
	
	void VirtualLayerPlusNetDevice::TimerLaunchAugmentation() {
			//NS_LOG_DEBUG("***\tLaunching Augmentation Process from node: " << m_node->GetId());
			VirtualLayerPlusNetDevice::SendAugmentationRequest();
			
			//m_timerLaunchAugmentation.Schedule(TIME_LAUNCH_AUGMENTATION);

	}

	bool VirtualLayerPlusNetDevice::SendAugmentationRequest() {
		// VNLayer
		Ptr<Packet> pkt = Create<Packet>();

		Ipv4Address bcastIp = Ipv4Address::GetBroadcast();
		Address bcastMac = m_netDevice->GetBroadcast();

		// create the vnLayer packet header
		VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();
	   	virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::VirtualToScma);
		// no subtype because its not required by the ibr layer
        virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::SCMA_Packet);
        virtualLayerPlusHdr.SetRegion(m_region);
        virtualLayerPlusHdr.SetSrcAddress(m_ip);
        virtualLayerPlusHdr.SetSrcMAC(m_mac);
        virtualLayerPlusHdr.SetDstAddress(bcastIp);
        virtualLayerPlusHdr.SetDstMAC(bcastMac);
        virtualLayerPlusHdr.SetRol(m_state);


		// create the ibr packet header
		//IbrHostListTrailer ibrHeader = IbrHostListTrailer();
		//ibrHeader.SetData(IbrHostListTrailer::IbrToScma, m_waysegment, m_region, false);

		// create the scma packet header
		ScmaMessageHeader scmaMessageHeader = ScmaMessageHeader();
		scmaMessageHeader.SetData(ScmaMessageHeader::T_SCMA, ScmaMessageHeader::M_AugmentationRequest, bcastIp, bcastMac, m_waysegment);
		
		// add the headers
		pkt->AddHeader(scmaMessageHeader);
		//pkt->AddHeader(ibrHeader);
		//pkt->AddHeader(virtualLayerPlusHdr);

        //m_txTrace(pkt, m_node->GetObject<VirtualLayerPlusNetDevice> (), GetIfIndex());

        
        //NS_LOG_DEBUG("AugmentationRequest from node: " << m_node->GetId() << " region: " << m_region << " ws: " << m_waysegment);
		//NS_LOG_DEBUG("SENDING SCMA packet " << (int)static_cast<uint8_t>(scmaMessageHeader.GetType()) << " : " <<  (int)static_cast<uint8_t>(scmaMessageHeader.GetMsg()) << ":" ) ;
		//m_netDevice->Send(pkt, m_netDevice->GetBroadcast(), 0);
		
		// set the node in ResourceRequest State
		nodeSM.RecvMsg=ScmaMessageHeader::M_AugmentationRequest;
		nodeSM.SetState(ScmaStateMachine::INITIAL);
		return true;		

		
		// SendPkt(VirtualLayerPlusHeader::SCMA, VirtualLayerPlusHeader::SCMA_AugmentationRequest, Ipv4Address::GetBroadcast(), m_netDevice->GetBroadcast());
		
	}

	bool VirtualLayerPlusNetDevice::removeHostFromTable(Ipv4Address ip) {
				// removes a host from the main host table
				//Ipv4Address ip = host->ip;
				for (std::list<IbrRegionHostTable::HostEntry>::iterator i = ibr_hostsTable.begin(); i != ibr_hostsTable.end(); ++i) {
				 if (i->ip.IsEqual(ip)) {				 	
						i=ibr_hostsTable.erase(i);
						return true;
					}
				}
				
				return false;
			
	}

	void VirtualLayerPlusNetDevice::SetWsTable (Ptr<IbrWaysegmentTable> wsT) {
	  NS_LOG_FUNCTION(wsT);
	  m_wsTable = wsT;
	}
	
	void VirtualLayerPlusNetDevice::ReceiveHostsTableIbr(std::list<IbrHostListTrailer::PayloadHostListEntry> hostsList) {
		////NS_LOG_DEBUG("Received IbrHostTable from Intersection IBR in node: " << m_node->GetId() << " size: " << hostsList.size());
		ibr_hostsTableDiscovery=hostsList;		
		// copy the table to the SCMA
		nodeSM.ibr_hostsTableDiscovery=hostsList;
	}

	uint32_t VirtualLayerPlusNetDevice::GetHostsTableSize() {
		return ibr_hostsTableDiscovery.size();
	}
}

// namespace ns3
