/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *	Module app-descarga
 *	Allows collaborative download in nodes
 *	Inherited from the Application class
 *	(c) 2017 Patricio Reinoso M.
 */


#include "app-descarga.h"
#include "ns3/log.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/socket.h"
#include "ns3/app-message-header.h"
#include "ns3/uinteger.h"


// constants used in the simulation
#define DOWNLOAD_SIZE 5000000
#define T_ResourceRequest ns3::Seconds(10.0)
#define T_StartDownload ns3::Seconds(3.0)
#define T_CompleteTaskAckTime ns3::Seconds(0.5)
#define T_StopCollabotarionTime ns3::Seconds(30.0);

#define MIN_COLLABORATOR_HOSTS 1
#define DOWNLOAD_CHUNKS 100
#define DOWNLOAD_CHUNK_SIZE (DOWNLOAD_SIZE/DOWNLOAD_CHUNKS)
#define APP_PORT_NUMBER 8080
// #define COLLABORATORS_PERCENT 0.75	// used in initial tests



NS_LOG_COMPONENT_DEFINE("AppDescarga");
namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED(AppDescarga);
    
    TypeId AppDescarga::GetTypeId (void) {
	  static TypeId tid = TypeId ("ns3::AppDescarga")
			  .SetParent<Application> ()
			  .AddConstructor<AppDescarga> ()
			  .AddAttribute("PercentageCollabs",
                	"% of collaborators used in App cloud",
                	UintegerValue(75),
                	MakeUintegerAccessor(&AppDescarga::m_percentCollabs),
                	MakeUintegerChecker<uint32_t> ());


	  return tid;
	}
    

	AppDescarga::AppDescarga() {
		// Setup required variables and timers, in class constructor
		myChunks.reserve(DOWNLOAD_CHUNKS);
		AppState=INITIAL;
		T_ResRequest.SetFunction(&AppDescarga::TimeoutResourceRequest, this);	
		T_TaskTimeout.SetFunction(&AppDescarga::TimeoutTaskTimeout, this);	
		T_DownloadStart.SetFunction(&AppDescarga::TimeoutDownloadStart, this);	
		T_DownloadSimulationTimer.SetFunction(&AppDescarga::TimeoutDownloadSimulatorTimer, this);
		T_CompleteTaskAck.SetFunction(&AppDescarga::TimeoutCompleteTaskAck, this);
		T_StopCollabotarion.SetFunction(&AppDescarga::TimeoutStopCollaboration, this);

		isLeader=false;
		isAppNode=false;
		lastChunkRequested=0;
		countSendCompleteTask=0;
		countTaskDistribution=0;
		

	}

	AppDescarga::~AppDescarga() {
		// Stop timers on class destructor
		if (T_ResRequest.IsRunning()) {
			T_ResRequest.Cancel();
		}	
		if (T_TaskTimeout.IsRunning()) {
			T_TaskTimeout.Cancel();
		}
		if (T_DownloadStart.IsRunning()) {
			T_DownloadStart.Cancel();
		}
		if (T_DownloadSimulationTimer.IsRunning()) {
			T_DownloadSimulationTimer.Cancel();
		}
		if (T_CompleteTaskAck.IsRunning()) {
			T_CompleteTaskAck.Cancel();
		}
		if (T_StopCollabotarion.IsRunning()) {
			T_StopCollabotarion.Cancel();
		}

	}

	void AppDescarga::StartApplication() {
		// Setup and start application on each node
		AppDescarga::m_node=this->GetNode();
		AppDescarga::nodeId=AppDescarga::m_node->GetId();
		AppDescarga::Initialize();

	   	// NS_LOG_INFO("Iniciando App en nodo: " << AppDescarga::nodeId); 
		
		// Setup the download application on 10% of the nodes
		if (nodeId%10==0) {
			isAppNode=true;
			NS_LOG_DEBUG("Application Node: " << AppDescarga::nodeId);
		}	

		if (isAppNode) {
			T_DownloadStart.Schedule(T_StartDownload);
		} 

	}

	void AppDescarga::StopApplication() {
	
	}


	void AppDescarga::TimeoutResourceRequest() {
		// At the ResourceRequest event timeout, send the tasks to the collaborators, and waits for the finished task
		NS_LOG_INFO("TASKDISTRIBUTION on node " << m_node->GetId() << "\t# nodes: " << myCollabs.size());
		uint32_t collabNodes=0;
		// apply the collaborators percentage defined in the command line arguments
		if ((myCollabs.size()*((float)m_percentCollabs/100.0))>=1) {
			collabNodes=(myCollabs.size()*((float)m_percentCollabs/100.0));
		}
		
		if (collabNodes>0) {
			// if exists collabotators, start the application
			AppState=TASKDISTRIBUTION;
			for (uint8_t i=0;i<collabNodes;i++) {
				NS_LOG_INFO("Collab. " << (i+1) << " -> " << myCollabs[i].m_ip);
			
			}
			// Send tasks to CNs
			if (!isDownloadComplete()) {
	
			for (uint8_t i=0;i<collabNodes;i++) {
				if (lastChunkRequested>=DOWNLOAD_CHUNKS) {
					lastChunkRequested=0;
				}
				while (isChunkDownloaded(lastChunkRequested)) {
					lastChunkRequested++;
				}
			
			
				AppMessageHeader appMsg=AppMessageHeader();
				appMsg.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_TaskInfo, myCollabs[i].m_ip );
				appMsg.SetSrcIp(m_mainAddress);
				appMsg.SetDownloadParts(lastChunkRequested);
				Ptr<Packet> pkt1;
				pkt1=Create<Packet>();
				pkt1->AddHeader(appMsg);
				AppDescarga::SendPkt(pkt1, myCollabs[i].m_ip);
				NS_LOG_INFO("Sent M_TASKINFO packet from node: " << m_node->GetId() << " to IP: " <<  myCollabs[i].m_ip << " Part: " << appMsg);
				AppDescarga::SendPkt(pkt1, myCollabs[i].m_ip);
				RecordLog(0, appMsg, 1, appMsg.GetSerializedSize());
				
				lastChunkRequested++;	
				AppState=TASKRECEPTION;
				countTaskDistribution++;
	
				if (T_ResRequest.IsRunning()) {
					T_ResRequest.Cancel();
				}
				T_ResRequest.Schedule(Seconds(0.5));

			}
	
			} else {
				NS_LOG_INFO("* Task completed in node: " << m_node->GetId());
				AppMessageHeader appMsgTmp=AppMessageHeader();
				appMsgTmp.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_DownloadComplete, m_mainAddress );
				RecordLog(0, appMsgTmp, 0, 0);

				// restart the application, to get a new download		
				isLeader=false;
				isAppNode=false;
				lastChunkRequested=0;
				countSendCompleteTask=0;
				countTaskDistribution=0;
				AppState=INITIAL;	
				if (T_DownloadStart.IsRunning()) {
					T_DownloadStart.Cancel();
				}
				AppDescarga::TimeoutDownloadStart();
				//T_DownloadStart.Schedule(T_StartDownload);
	
			}

		} else {
				// if there is no CNs, restart the application
				NS_LOG_INFO("* No Collabs in node : " << m_node->GetId() << " .... restarting");	
				isLeader=false;
				isAppNode=false;
				lastChunkRequested=0;
				countSendCompleteTask=0;
				countTaskDistribution=0;
				AppState=INITIAL;	
				if (T_DownloadStart.IsRunning()) {
					T_DownloadStart.Cancel();
				}
				AppDescarga::TimeoutDownloadStart();
				AppMessageHeader appMsgTmp=AppMessageHeader();
				appMsgTmp.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_InsufficientResources, m_mainAddress );
				RecordLog(0, appMsgTmp, 0, 0);

		}
		

	}
	
	bool AppDescarga::isDownloadComplete(){ 
		// verifies if the download is complete, checking if all the chunks are downloaded
		bool complete=true;
		std::string str("|");
		for (uint8_t i=0; i<DOWNLOAD_CHUNKS;i++) {
			if (myChunks[i]==false) {
				complete=false;
				str.push_back(' ');
			} else {
				str.push_back('*');
			}
		}
		str.push_back('|');
		NS_LOG_DEBUG("*Node: " << m_node->GetId() << " t: " << Simulator::Now().GetSeconds() << " " << str);
		
		return complete;
	}

	bool AppDescarga::isChunkDownloaded(uint32_t chunkId) {
		// Checks if the required chunk is already downloaded
		if (myChunks[chunkId]==true) {
			return true;
		} 
		return false;
	}


	void AppDescarga::TimeoutTaskTimeout() {
		// Timer TimeoutTask (not used)
	}

	void AppDescarga::TimeoutDownloadStart() {
		// Download start Timer
		AppMessageHeader appMsgTmp=AppMessageHeader();
		appMsgTmp.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_NewDownload, m_mainAddress );
		RecordLog(0, appMsgTmp, 0, 0);

		NS_LOG_INFO("Start download in node: " << m_node->GetId());
		AppState=DOWNLOADSTART;
		AppDescarga::StateMachine();
		// sends a ResourceRequest message by broadcast, to detect nearest nodes
		AppMessageHeader appMsg=AppMessageHeader();
		appMsg.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_ResourceRequest, m_broadcast );
		appMsg.SetSrcIp(m_mainAddress);
		Ptr<Packet> pkt1;
		pkt1=Create<Packet>();
		pkt1->AddHeader(appMsg);
		AppDescarga::SendPkt(pkt1, m_broadcast);
		NS_LOG_INFO("Sent DOWNLOADSTART packet from node: " << m_node->GetId() << " to IP: " << m_broadcast);
		RecordLog(0, appMsg, 1, appMsg.GetSerializedSize());
		// start the resource request timer, to count the available CNs 
		AppState=RESOURCEREQUEST;
		if (T_ResRequest.IsRunning()) {
			T_ResRequest.Cancel();
		}
		T_ResRequest.Schedule(T_ResourceRequest);


	}

	void AppDescarga::TimeoutDownloadSimulatorTimer() {
		// download finished in the CN
		// create the download payload	
		Ptr<Packet> pkt1;
		pkt1=Create<Packet>(DOWNLOAD_CHUNK_SIZE);
		//pkt1=Create<Packet>();
		// creamos la cabecera del paquete 
		AppMessageHeader appMsg=AppMessageHeader();
		appMsg.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_CompleteTask, responseIp);
		appMsg.SetSrcIp(m_mainAddress);
		appMsg.SetDownloadParts(downloadPartId);
		pkt1->AddHeader(appMsg);
		// send the data packet
		AppDescarga::SendPkt(pkt1, responseIp);
		NS_LOG_INFO("Sent M_CompleteTask packet from node: " << m_mainAddress << " to IP: " << responseIp << " Part: " << downloadPartId);
		RecordLog(0, appMsg, 1, appMsg.GetSerializedSize());
		AppState=TASKWAIT;
		if (T_CompleteTaskAck.IsRunning()) {
			T_CompleteTaskAck.Cancel();
		}
		T_CompleteTaskAck.Schedule(T_CompleteTaskAckTime);
	}

	void AppDescarga::TimeoutCompleteTaskAck() {
		// Timer to wait ACK from the app node
		// if not received at timeout, resends the download data
		
		if (countSendCompleteTask<3) {
			// resend the data packet
			NS_LOG_INFO("Resending (" << countSendCompleteTask << ") complete task in " << m_mainAddress);
			AppDescarga::TimeoutDownloadSimulatorTimer();			
			countSendCompleteTask++;
		} else {
			AppState=INITIAL;
			NS_LOG_INFO("Resending (" << countSendCompleteTask << ") complete finished in " << m_mainAddress);
			if (T_CompleteTaskAck.IsRunning()) {
				T_CompleteTaskAck.Cancel();
			}

			countSendCompleteTask=0;
		}
	}

	void AppDescarga::TimeoutStopCollaboration() {
		// finishes the collaboration and goes back to INITIAL state
		countSendCompleteTask=0;
		countTaskDistribution=0;
		if (T_CompleteTaskAck.IsRunning()) {
			T_CompleteTaskAck.Cancel();
		}
		AppState=INITIAL;
	}

	void AppDescarga::StateMachine() {
		// basic state machine for the application
		if (AppState==DOWNLOADSTART) {
			// reset the download chunks table
			for (int i=0; i<DOWNLOAD_CHUNKS;i++) {
				myChunks[i]=false;
			}	
			if (T_DownloadStart.IsRunning()) {
				T_DownloadStart.Cancel();
			}
			// reset the collaborations table
			myCollabs.clear();

		}
	}

	void AppDescarga::Initialize() {
	  // Object initialization and configuration
	  m_ipv4=m_node->GetObject<Ipv4>();
  	  m_broadcast=Ipv4Address::GetBroadcast();
	  app_netDevice = m_ipv4->GetObject<NetDevice>();
	  //m_broadcastMac=app_netDevice->GetBroadcast();

	  // setup the communication socket
	  // get the available net device for Packets Rx/Tx
	  app_netDevice = 0;
        for (uint32_t i = 0; i < m_node->GetNDevices(); i++) {
                if (dynamic_cast<NetDevice*> (PeekPointer(m_node->GetDevice(i)))) {
                    //if (IBR_DEBUG) //NS_LOG_INFO("NetDevice found in the node " << ibr_node->GetId());
                    app_netDevice=dynamic_cast<NetDevice*> (PeekPointer(m_node->GetDevice(i)));
                    break;
                }
        }

	// enable the socket
	//if (IBR_DEBUG) //NS_LOG_INFO("Setting up IBR Socket in node: " << ibr_node->GetId() );
	  Ipv4Address loopback ("127.0.0.1");
	  if (m_mainAddress == Ipv4Address ())  {
			  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
				{
				  // use the main address if multiple interfaces are available 
				  Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
				  if (addr != loopback)
					{
					  m_mainAddress = addr;
					  break;
					}
				}

			  NS_ASSERT (m_mainAddress != Ipv4Address ());
 	  }

	  for (uint32_t i = 1; i < m_ipv4->GetNInterfaces (); i++) {
			  Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
			  if (addr == loopback)
				{
				  continue;
				}
			  // create a socket only in the selected interface
			  Ptr<Socket> socket = Socket::CreateSocket (m_node, UdpSocketFactory::GetTypeId ());
			  socket->SetAllowBroadcast (true);
			  InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal ()/* Ipv4Address::GetAny() */, APP_PORT_NUMBER);
			  // setup Rx & Tx callbacks
			  socket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>,const Address &> (),MakeCallback(&AppDescarga::Accept, this));
			  socket->SetRecvCallback ( MakeCallback (&AppDescarga::RecvPkt,  this));
			  if (socket->Bind (inetAddr))
				{
				  NS_FATAL_ERROR ("Failed to bind() Ibr socket");
				}
			  socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
			  m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);
			  // NS_LOG_INFO("Binding APP to " << m_ipv4->GetAddress(i,0) << " PORT " << APP_PORT_NUMBER);
			  break;
      }
	  	  	
	}

	void AppDescarga::Accept(Ptr<Socket> socket,const ns3::Address& from) {
		// Packet accept function for socket	
		NS_LOG_INFO("Accept in node " << m_node->GetId());
		socket->SetRecvCallback (MakeCallback (&AppDescarga::RecvPkt,  this));

	}


	void AppDescarga::RecvPkt (Ptr<Socket> socket) {
		  // Socket packet reception  
		  Ptr<Packet> receivedPacket;
		  Address sourceAddress;
		  receivedPacket = socket->RecvFrom (sourceAddress);

		  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
		  Ipv4Address receiverIfaceAddr = m_socketAddresses[socket].GetLocal ();
		  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
		  // verify socket port number
		  NS_ASSERT (inetSourceAddr.GetPort () == APP_PORT_NUMBER);
		  // copy the packet to remove headers
		  Ptr<Packet> copyPkt = receivedPacket->Copy();
		  AppMessageHeader appMsg;
		  copyPkt->RemoveHeader(appMsg);
		   
		  RecordLog(1, appMsg, 1, appMsg.GetSerializedSize());

		  // Packet processing, defined by application state
		  switch (appMsg.m_msg) {
				case AppMessageHeader::M_ResourceRequest:
					if (AppState==INITIAL) {
						NS_LOG_INFO("Sending resources to " << appMsg.GetSrcIp());
						AppDescarga::SendResources(appMsg.GetSrcIp());				
						AppState=TASKWAIT; 
					}
					break;
				case AppMessageHeader::M_ResourceInfoFromCN:
					if (AppState==RESOURCEREQUEST) {
						NS_LOG_INFO("Received Resources on IP: " << m_mainAddress << " from IP: " << appMsg.GetSrcIp());
						AvailableCollaborators colab;
						colab.m_ip=appMsg.GetSrcIp();
						myCollabs.push_back(colab);
					}
					break;
				case AppMessageHeader::M_TaskInfo:
					if (AppState==TASKWAIT) {
						uint32_t idPart=appMsg.GetDownloadParts();
						NS_LOG_INFO("Received M_TAskInfo on IP: " << m_mainAddress << " from IP: " << appMsg.GetSrcIp() << appMsg);					
						AppState=EXECUTINGTASK;
						responseIp=appMsg.GetSrcIp();
						AppDescarga::StartDownloadTask(idPart);	
					}


					break;
				case AppMessageHeader::M_CompleteTask:
					if (AppState==TASKRECEPTION) {
						NS_LOG_INFO("Received M_CompleteTask on IP: " << m_mainAddress << " from IP: " << appMsg.GetSrcIp() << " PartId: " << appMsg.GetDownloadParts());					
						AppDescarga::SendTaskAck(appMsg.GetSrcIp());
						myChunks[appMsg.GetDownloadParts()]=true;


					}
					break;
				case AppMessageHeader::M_CompleteTaskAck:
					NS_LOG_INFO("Received M_CompleteTaskAck on IP: " << m_mainAddress << " Going Back to TASKWAIT");
					if (T_CompleteTaskAck.IsRunning()) {
						T_CompleteTaskAck.Cancel();
					}
					countSendCompleteTask=0;
					AppState=TASKWAIT;

					break;
				default:
					break;
		  
		  }		

  	}

	void AppDescarga::StartDownloadTask(uint32_t idPart) {
		// Starts the download task
		// Simulates a download time, with a timer and available BW
		if (T_DownloadSimulationTimer.IsRunning()) {
			T_DownloadSimulationTimer.Cancel();
		}
		downloadPartId=idPart;
		// BW between  1 & 15 Mbps (3/4G)
		uint32_t bw = ((double) (rand() / (double) RAND_MAX) * 14.0 + 1.0)*1000000;
		float downSize=float(DOWNLOAD_CHUNK_SIZE);
		float downTime=(float)(downSize)/((float)bw/8); 

		T_DownloadSimulationTimer.Schedule(ns3::Seconds(downTime));
		NS_LOG_INFO("Setting download time to " << downTime << " s in node: " << m_node->GetId() << " BW: " << bw);
		

	}


	void AppDescarga::SendResources(Ipv4Address dstIp) {
		// Send available resources
		AppMessageHeader appMsg=AppMessageHeader();
		appMsg.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_ResourceInfoFromCN, m_broadcast );
		appMsg.SetSrcIp(m_mainAddress);
		Ptr<Packet> pkt1;
		pkt1=Create<Packet>();
		pkt1->AddHeader(appMsg);
		AppDescarga::SendPkt(pkt1, m_broadcast);
		countSendCompleteTask=0;
		RecordLog(0, appMsg, 1, appMsg.GetSerializedSize());

	}

	void AppDescarga::SendTaskAck(Ipv4Address dstIp) {
		// send finished task ACK
		AppMessageHeader appMsg=AppMessageHeader();
		appMsg.SetData(AppMessageHeader::T_APPMSG, AppMessageHeader::M_CompleteTaskAck, dstIp );
		appMsg.SetSrcIp(m_mainAddress);
		Ptr<Packet> pkt1;
		pkt1=Create<Packet>();
		pkt1->AddHeader(appMsg);
		AppDescarga::SendPkt(pkt1, dstIp);
		RecordLog(0, appMsg, 1, appMsg.GetSerializedSize());
	}

	void AppDescarga::SendPkt (Ptr<Packet> packet, Ipv4Address ipDest1) {
		  // send a packet in the open socket
		  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
				 m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
			{
			  	i->first->SendTo(packet, 0, InetSocketAddress(ipDest1, APP_PORT_NUMBER));
			}
	}

	void AppDescarga::RecordLog(uint8_t TxRx, AppMessageHeader msg, uint32_t pkts, uint32_t byteSize){
		// record event log
		std::string dir;

		if (TxRx==0) {
			dir="Tx";
		} else {
			dir="Rx";
		}

		NS_LOG_DEBUG( "[node " << m_node->GetId() << "] Time:" << Simulator::Now().GetSeconds() << "\t" << "Pkt:App-" << dir << "\t" << "Msg:" << msg << "\t" <<  "Pkts:" << pkts << "\t" << "Size:" << byteSize );
		

	}



}

