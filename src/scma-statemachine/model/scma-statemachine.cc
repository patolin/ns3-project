/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SCMA State Machine Module
 * Used with VNLayer to simulate collaborative downloads
 * (c) 2016 Patricio Reinoso M
 */

#include <iostream>
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "src/network/utils/ipv4-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/virtual-layer-plus-header.h"

#include "scma-statemachine.h"
#include "scma-message-header.h"

#include "ns3/ibr-hostlist-trailer.h"

#define TIME_ResourceRequest ns3::Seconds(2.0)
#define TIME_TaskDistribution ns3::Seconds(2.0)
#define TIME_CompleteTaskAck ns3::Seconds(3.0)
#define TIME_IncompleteTaskAck ns3::Seconds(2.0)
#define TIME_ExpireReception ns3::Seconds(05.0)
#define TIME_TaskReception ns3::Seconds(2.0)

#define MIN_HOSTS_AUGMENTATION	1
#define DOWNLOAD_SIZE 5				// download size in MB
#define DOWNLOAD_PART_SIZE 0.5		// chunk size in MB
#define DOWNLOAD_TOTAL_CHUNKS (DOWNLOAD_SIZE/DOWNLOAD_PART_SIZE)

//#define NODE_USE_PERCENT 50.0

NS_LOG_COMPONENT_DEFINE("ScmaStateMachine");

namespace ns3 {

    NS_OBJECT_ENSURE_REGISTERED(ScmaStateMachine);
    
    TypeId ScmaStateMachine::GetTypeId (void) {
	  static TypeId tid = TypeId ("ns3::ScmaStateMachine").SetParent<Header> ();
	  return tid;
	}
    

	ScmaStateMachine::ScmaStateMachine() {

		nodeState=INITIAL;
		IntersectionFlag=0;
		ResourceAvailable=0;
		NTasks=uint32_t(DOWNLOAD_SIZE/DOWNLOAD_PART_SIZE);
		m_region=-1;
		//NS_LOG_DEBUG ("StateMachine Constructor set in Node.\tIntersection: " << (double)IntersectionFlag << " .");
		
		// timers setup
		
		T_ResourceRequest.SetFunction(&ScmaStateMachine::TimeoutResourceRequest, this);	
		T_TaskDistribution.SetFunction(&ScmaStateMachine::TimeoutTaskDistribution, this);	
		T_CompleteTaskAck.SetFunction(&ScmaStateMachine::TimeoutCompleteTaskAck, this);
		T_IncompleteTaskAck.SetFunction(&ScmaStateMachine::TimeoutIncompleteTaskAck, this);
		T_ExpireReception.SetFunction(&ScmaStateMachine::TimeoutExpireReception, this);
		T_TaskReception.SetFunction(&ScmaStateMachine::TimeoutTaskReception , this);
		T_Download.SetFunction(&ScmaStateMachine::TimeoutDownload , this);
	
		//m_ibrRoutingProtocol = m_nodeBase->GetObject<IbrRoutingProtocol>();		
		
		ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_Nothing;
		msg1=ScmaMessageHeader::M_AugmentationRequest;

		//nodeId=m_nodeBase->GetId();
		//bcastIp=Ipv4Address::GetBroadcast();
	    //bcastMac=m_netDeviceBase->GetBroadcast(); 
	}
	
	
	void ScmaStateMachine::SetState(ScmaStates newState) {
		nodeState=newState;
		StateMachine();
	}

	ScmaStateMachine::ScmaStates ScmaStateMachine::GetState() {
		return nodeState;
	}
	
	
	void ScmaStateMachine::StateMachine() {
		// StateMachine main function		
		switch (nodeState) {
			case INITIAL:
				NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tState Initial") ;
				StateInitial();
				break;
			case RESOURCEREQUEST:
				NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tState Resourcerequest") ;
				StateResourceRequest();
				break;
			case TASKDISTRIBUTION:
				NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tState Taskdistribution") ;
				StateTaskDistribution();
				break;	
			case TASKRECEPTION:
				NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tState Taskreception" );
				StateTaskReception();
				break;	
			case COLLABORATION:
				NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tState Collaboration" );
				StateCollaboration();
				break;	
			default:
				break;
		}
				
	}


	void ScmaStateMachine::ClearRecvMessage() {
		ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_Nothing;	
	}

	void ScmaStateMachine::StateInitial() {
		// INITIAL to RESOURCE_REQUEST
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_AugmentationRequest && IntersectionFlag==0) {
			bcastIp=Ipv4Address::GetBroadcast();
	    	bcastMac=m_netDeviceBase->GetBroadcast(); 

			ScmaStateMachine::ClearRecvMessage();
			T_ResourceRequest.Cancel();
			T_ResourceRequest.Schedule(TIME_ResourceRequest);
			expires=0;
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tStateChange Initial\t->\tResourceRequest") ;
			ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_ResourceRequest, bcastIp, bcastMac,0);
			SetState(RESOURCEREQUEST);
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-AugmentationRequest\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
		}
		
		// INITIAL to COLLABORATION
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_SendTaskToCN && AugmentationAgree==true && IntersectionFlag==0 && ResourceAvailable==1) {
			SendMess(ScmaMessageHeader::M_AcceptedTaskFromCN);
			FinishFlag=0;
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tStateChange Initial\t->\tCollaboration") ;
			ClearRecvMessage();	

			SetState(COLLABORATION);
		}
	}



	void ScmaStateMachine::StateResourceRequest() {
		// RESOURCEREQUEST TO TASK DISTRIBUTION
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_CloudResourceInfo) {
			// cancel the resourcerequest timer
			T_ResourceRequest.Cancel();
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tStateChange ResourceRequest\t->\tTaskDistribution" );
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-RecvCloudResourceInfo\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);

			ScmaStateMachine::CancelTimers();
			ScmaStateMachine::ClearRecvMessage();
			SetState(TASKDISTRIBUTION);
		}



		// RESOURCE REQUEST to RESOURCE REQUEST
		if (expires==1) {
			NS_LOG_DEBUG("** Sending resource request to intersection leaders from node: " << m_nodeBase->GetId() <<  " ws: " << m_wsTable->getWaySegment(m_region) << ":" << m_waysegment << "\tIP: " << m_ip);
			// send the ResourceRequest message to the intersection node.
			// must receive back the CloudResourceInfo message, and the host list
			ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_ResourceRequest, Ipv4Address::GetBroadcast(), m_netDeviceBase->GetBroadcast() ,0 );
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-ResourceRequest\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
			
		}
		
		// RESOURCEREQUEST to INITIAL
		if (expires==2) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tStateChange EXP=" << expires << " ResourceRequest\t->\tInitial" );
			SendMess(ScmaMessageHeader::M_UnavailableAugmentationService);
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-UnavailableAugmentationService\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);

			SetState(INITIAL);
		}

		// RESOURCEREQUEST to INITIAL
		if (IntersectionFlag==1) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() <<"\tStateChange INF=1 ResourceRequest\t->\tInitial" );
			SendMess(ScmaMessageHeader::M_CloudRelease);
			SetState(INITIAL);
		}
	}
	
	void ScmaStateMachine::StateTaskDistribution() {
		// TASKDISTRIBUTION TO INITIAL
		if (InsufficientResources==1) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\tInsufficient Resources -> State Initial");
			//SendMess(ScmaMessageHeader::M_CloudRelease);
			//SendMess(ScmaMessageHeader::M_InsufficientResources);
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-InsufficientResources\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
			// launch a new augmentation request
			ScmaStateMachine::CancelTimers();
			ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_AugmentationRequest;
			SetState(INITIAL);
		}
		
		// TASKDISTRIBUTION TO INITIAL
		if (IntersectionFlag==1) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\tIntersection flag -> State Initial");
			SendMess(ScmaMessageHeader::M_CloudRelease);
			ScmaStateMachine::CancelTimers();
			// stop all processes
			SetState(INITIAL);

		}

		// TASKDISTRIBUTION to TASKDISTRIBUTION
		if (Distribution<3) {
			// Does nothing until Distribution=3
			//SendMess(ScmaMessageHeader::M_SendTaskToVN);
			//SetState(TASKDISTRIBUTION);
		}
		
		// TASKDISTRIBUTION TO RESOURCEREQUEST
		if (Distribution==3) {
			SendMess(ScmaMessageHeader::M_ResourceRequest);
			expires=0;
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\tTaskDistribution timer = 3 -> restarting resourcerequest");
			ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_Nothing;
			T_ResourceRequest.Schedule();
			SetState(RESOURCEREQUEST);
		}
		
		// TASKDISTRIBUTION TO TASKRECEPTION
		if (DistributionFlag==1) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\tTaskDistribution\t->\tTaskReception");
			SendMess(ScmaMessageHeader::M_SendTaskToVN);
			
			CompleteTaskCounter=0;
			IncompleteTaskCounter=0;
			ExpireReception=0;
			
			if (T_TaskDistribution.IsRunning()) {
				T_TaskDistribution.Cancel();
			}
			ScmaStateMachine::nodeState=TASKRECEPTION;
			T_TaskReception.Schedule();
			SetState(TASKRECEPTION);
		}

		// Send tasks to nodes
		if (DistributionFlag==0) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\tTask Delivery to nodes");
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-TaskDistribution\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);

			DistributionFlag=1;

			// get host list and send to the specific nodes the task
			ScmaStateMachine::ibr_hostsTableDiscovery=ScmaStateMachine::ibrHostListTrailer.GetHostsList();
			ScmaStateMachine::ibr_hostsTableDiscovery.unique();
			uint32_t availableNodes=ScmaStateMachine::ibr_hostsTableDiscovery.size()-1;
			//uint32_t usableNodes=(int)(availableNodes*NODE_USE_PERCENT/100.0);
			uint32_t usableNodes=(int)(availableNodes*nodeUsePercent/100.0);
			NTasks=usableNodes;
			
			uint32_t nodeInUseCounter=0;
			if (usableNodes>=1) {
					for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = ScmaStateMachine::ibr_hostsTableDiscovery.begin(); i != ScmaStateMachine::ibr_hostsTableDiscovery.end(); ++i) {
						//if (nodeInUseCounter<usableNodes) {
								if (i->hostIp!=m_ip) {
									ScmaStateMachine::SendTaskToCN(i->hostIp, i->hostMac, DOWNLOAD_SIZE, usableNodes, i->BW);
									nodeInUseCounter++;
									NS_LOG_DEBUG("-- SCMA Node: " << m_nodeBase->GetId() << "\tSending task to IP: "<<i->hostIp << "\t" << nodeInUseCounter << "/" << usableNodes);
								}
						//}
					}
					
					NTasks=usableNodes;
					// enable T_ExpireRequest timer, to wait for unfinished downloads
					// set double of the estimated download time alone
					ScmaStateMachine::downloadTime = 3.0*(DOWNLOAD_SIZE/(ScmaStateMachine::m_bandwidth/8.0));
					NS_LOG_DEBUG("++ SCMA Node: " << m_nodeBase->GetId() << " T_ExpireReception enabled to " << downloadTime << " s");
					if (T_ExpireReception.IsRunning()) {
						T_ExpireReception.Cancel();
					} 
					T_ExpireReception.Schedule(ns3::Seconds(downloadTime));
					// change state to task reception
					ScmaStateMachine::SetState(TASKRECEPTION);
			} else {
				NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-InsufficientResources\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
			}
		}
	}
	
	
	void ScmaStateMachine::StateTaskReception() {
		// TASKRECEPTION to TASKRECEPTION
		if (ExpireReception==1) {
			SendMess(ScmaMessageHeader::M_CloudRelease);
			//SetState(TASKRECEPTION);
		}
		// TASKRECEPTION to TASKRECEPTION
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_IncompleteTask) {
			SendMess(ScmaMessageHeader::M_IncompleteTaskAck);
			SendMess(ScmaMessageHeader::M_TaskInfo);
			IncompleteTaskCounter++;
			//SetState(TASKRECEPTION);
		}
		// TASKRECEPTION to TASKRECEPTION
		if (IntersectionFlag==1) {
			SendMess(ScmaMessageHeader::M_CloudRelease);
			if (T_TaskReception.IsRunning()) {
				T_TaskReception.Cancel();	
			}
			T_TaskReception.Schedule();
			///SetState(TASKRECEPTION);
		}
		// TASKRECEPTION to TASKRECEPTION
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_AcceptedTaskFromVN) {
			// update TaskAck
			//SetState(TASKRECEPTION);
		}
		// TASKRECEPTION to TASKRECEPTION
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_CompleteTask) {
			SendMess(ScmaMessageHeader::M_CompleteTaskAck);
			SendMess(ScmaMessageHeader::M_TaskInfo);
			CompleteTaskCounter++;
			//SetState(TASKRECEPTION);
		}
		// TASKRECEPTION to INITIAL

		if (CompleteTaskCounter>=NTasks /*|| (CompleteTaskCounter+IncompleteTaskCounter+NoAcceptedTaskCounter)==NTasks */) {
			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " Task Finished.\t"<<CompleteTaskCounter<< "+"<< IncompleteTaskCounter << "+" << NoAcceptedTaskCounter << "=" << NTasks << "\tGoing back to INITIAL state ------------------------------");
			// reset counters
			CompleteTaskCounter=0;
			IncompleteTaskCounter=0;
			NoAcceptedTaskCounter=0;
			ExpireReception=0;

			// cancel all timers
			ScmaStateMachine::CancelTimers();
			ScmaStateMachine::ClearRecvMessage();
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-TaskFinished\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-TaskNew\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
			// launch a new augmentation request
			ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_AugmentationRequest;
			SetState(INITIAL);
		}
		if (ExpireReception==3) {
			SetState(INITIAL);
		}
		
	}
	
	void ScmaStateMachine::StateCollaboration() {
		//COLLABORATION to COLLABORATION
		if (FinishFlag==1) {
			SendMess(ScmaMessageHeader::M_CompleteTask);
			FinishFlag=0;
			T_CompleteTaskAck.Schedule(TIME_CompleteTaskAck);
			SetState(COLLABORATION);
		}
		//COLLABORATION to COLLABORATION
		if (IntersectionFlag==1 || ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_CloudRelease || AugmentationAgree==false) {
			NS_LOG_DEBUG("* Collaborator node: " << m_nodeBase->GetId() << " Intersection flag=1 / CloudRelease / AugmentationAgree=false" );
			// send incomplete task msg
			ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_IncompleteTask, responseIp, responseMac,0);
			T_IncompleteTaskAck.Schedule();
			//SetState(COLLABORATION);
		}
		//COLLABORATION TO INITIAL
		if (ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_CompleteTaskAck || ScmaStateMachine::RecvMsg==ScmaMessageHeader::M_IncompleteTaskAck ) {
			
			SendMess(ScmaMessageHeader::M_ResourceInfoFromCN);
			//SetState(INITIAL);
		}
	}
	
	void ScmaStateMachine::StateDispatch() {
		
	}
	
	void ScmaStateMachine::SendMess(ScmaMessageHeader::ScmaMessages message) {
		//StateMachine();
	}
	
	void ScmaStateMachine::RecvMess(ScmaMessageHeader::ScmaMessages message) {
		ScmaStateMachine::RecvMsg=message;
		StateMachine();
	}
	
	
	void ScmaStateMachine::SetIntersectionFlag(uint8_t flag) {
		if (flag!=IntersectionFlag) {
			IntersectionFlag=flag;		
			NS_LOG_DEBUG ("SCMA Node: " << m_nodeBase->GetId() << "\tIntersection: " << (uint32_t)IntersectionFlag << " t: " << Simulator::Now().GetSeconds());
			StateMachine();

		}
	}
	
	void ScmaStateMachine::SetFinishFlag(uint8_t flag) {
		FinishFlag=flag;
		StateMachine();
	}
	
	void ScmaStateMachine::TimeoutResourceRequest() {
		if (expires<2) {
			expires++;

			NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << "\t Timeout T_ResourceRequest\texpires: " << (uint32_t)expires);
			T_ResourceRequest.Schedule(TIME_ResourceRequest);
			StateMachine();
		} else {
			// go back to initial state
			ScmaStateMachine::CancelTimers();
			ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_AugmentationRequest;
			ScmaStateMachine::SetState(INITIAL);
		}

	}

	void ScmaStateMachine::TimeoutTaskDistribution() {
		if (Distribution<3) {
			NS_LOG_DEBUG("- node: " << m_nodeBase->GetId() << " Timer TaskDistribution timeout");	
			Distribution++;
			T_TaskDistribution.Schedule();
			StateMachine();
		}
	}
	
	void ScmaStateMachine::TimeoutTaskReception() {
		if (ExpireReception<=2) {
			
			ExpireReception++;
			T_TaskReception.Schedule();
			StateMachine();
		}
	
	}

	void ScmaStateMachine::TimeoutCompleteTaskAck() {
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " T_CompleteTaskAck timeout");
		// send the information to the main node
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " Sending M_CompleteTask & M_ResourceInfoFromCN from ip: " << m_ip  << " to ip: " << ScmaStateMachine::responseIp);		
		ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_CompleteTask, responseIp, responseMac,0);
		ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_ResourceInfoFromCN, responseIp, responseMac,0);
		ScmaStateMachine::SetState(INITIAL);
	}

	void ScmaStateMachine::TimeoutExpireReception() {
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " T_ExpireReception timeout: Canceling all requests. Going back to INITIAL and launching cloud for pending parts");	
		// cancel all timers
		T_ResourceRequest.Cancel();
		T_TaskDistribution.Cancel();
		T_CompleteTaskAck.Cancel();
		T_IncompleteTaskAck.Cancel();
		T_ExpireReception.Cancel();
		T_TaskReception.Cancel();
		ScmaStateMachine::ClearRecvMessage();
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-TaskExpireReception\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region);
		ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_AugmentationRequest;
		SetState(INITIAL);
	}
	
	void ScmaStateMachine::TimeoutIncompleteTaskAck() {
		// disable all timers and go back to initial state
		T_ResourceRequest.Cancel();
		T_TaskDistribution.Cancel();
		T_CompleteTaskAck.Cancel();
		T_IncompleteTaskAck.Cancel();
		T_ExpireReception.Cancel();
		T_TaskReception.Cancel();
		ScmaStateMachine::ClearRecvMessage();

		SetState(INITIAL);

	}

	void ScmaStateMachine::CancelTimers() {
		T_ResourceRequest.Cancel();
		T_TaskDistribution.Cancel();
		T_CompleteTaskAck.Cancel();
		T_IncompleteTaskAck.Cancel();
		T_ExpireReception.Cancel();
		T_TaskReception.Cancel();
	}

		
	bool ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::ScmaMessages msg, Ipv4Address ipDst, Address macDst, uint32_t payload=0) {
		Ptr<Packet> pkt = Create<Packet>(payload);
		ScmaMessageHeader messageHdr = ScmaMessageHeader();
		NS_LOG_DEBUG("***\tIniciando SendPkt en SM");

		NS_LOG_DEBUG("***\tmessageHdr OK");
		

		// add the virtual layer plus header to the packet (prepend header)
		VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();
        virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::VirtualToScma);
        virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::SCMA_Packet);
        virtualLayerPlusHdr.SetRegion(m_region);
        virtualLayerPlusHdr.SetSrcAddress(m_ip);
        virtualLayerPlusHdr.SetSrcMAC(m_mac);
        virtualLayerPlusHdr.SetDstAddress(ipDst);
        virtualLayerPlusHdr.SetDstMAC(macDst);
        virtualLayerPlusHdr.SetRol(m_state);

		/*
		// create the ibr packet header
		IbrHostListTrailer ibrHeader = IbrHostListTrailer();
		ibrHeader.SetData(ibrType, m_waysegment, m_region, false);
		*/

		// create the scma packet header
		ScmaMessageHeader scmaMessageHeader = ScmaMessageHeader();
		scmaMessageHeader.SetData(ScmaMessageHeader::T_SCMA, msg, ipDst, macDst, m_waysegment);
		scmaMessageHeader.SetSrcIp(m_ip);
		scmaMessageHeader.SetSrcMAC(m_mac);
		// add the headers
		
		pkt->AddHeader(scmaMessageHeader);
		//pkt->AddHeader(ibrHeader);
		pkt->AddHeader(virtualLayerPlusHdr);

		// send the packet
		ScmaStateMachine::m_netDeviceBase->Send(pkt, macDst, 0);
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-PktTx\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<pkt->GetSize() << "\tsrcIp: " << m_ip << "\tmsg: " << (int32_t)msg );

		return true;
	}

	void ScmaStateMachine::RecvPkt(Ptr<Packet> pkt1) {
		// receive the packet for the SCMA only. the other headers are removed on previous layers
		//ClearRecvMessage();
		
		Ptr<Packet> pkt = pkt1->Copy();
		
		ScmaMessageHeader scmaMessageHeader = ScmaMessageHeader();
		pkt->RemoveHeader(scmaMessageHeader);
		

		uint32_t wsPacket=scmaMessageHeader.GetWaySegment();
		// check the node waysegment and discard packets from another way segments
		//
		if (ScmaStateMachine::m_wsTable->isRegionInWaySegment(m_region, wsPacket)) {
				if (pkt->GetSize()>0) NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-PktRx\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<pkt->GetSize() << "\tsrcIp: "<< responseIp << "\tMsg: " << scmaMessageHeader);

				if (scmaMessageHeader.GetType()==ScmaMessageHeader::T_SCMA) {
					ScmaStateMachine::RecvMsg=scmaMessageHeader.GetMsg(); //ScmaMessageHeader::M_AugmentationRequest;

					switch (scmaMessageHeader.GetMsg()) {
						// receive an AugmentationRequest Packet
						case ScmaMessageHeader::M_AugmentationRequest:
								NS_LOG_DEBUG("Received on SCMA packet IbrToScma " << (int)static_cast<uint8_t>(scmaMessageHeader.GetType()) << " : " <<  (int)static_cast<uint8_t>(scmaMessageHeader.GetMsg()) << " on Region: " << m_region ) ;
								SetState(INITIAL);
								break;
						// receive a ResourceRequest Packet
						// sends the information from the intersection
						// first on tx is accepted
						
						case ScmaMessageHeader::M_ResourceRequest:
								// receive only if its an intersection node
								if (ScmaStateMachine::m_nodeLevel==1 && ScmaStateMachine::m_wsTable->isRegionInWaySegment(m_region, wsPacket) && m_state==VirtualLayerPlusHeader::LEADER ) {
									NS_LOG_DEBUG("* Received M_ResourceRequest on node: " << m_nodeBase->GetId() << " region: " << m_region << " wsPacket: " << wsPacket);
									// transmit the information of the hosts table to the requiring node
									NS_LOG_DEBUG("* Sending M_CloudResourceInfo from node: " << m_nodeBase->GetId() << "\tnodes in host: " << ScmaStateMachine::ibr_hostsTableDiscovery.size()  );
									ScmaStateMachine::SendCloudResources(wsPacket);
									
								}
								break;
						case ScmaMessageHeader::M_CloudResourceInfo: 
								// receive only if its on resourcerequest state


								if (nodeState==RESOURCEREQUEST) {
									NS_LOG_DEBUG("* Received M_CloudResourceInfo on node: " << m_nodeBase->GetId() << " region: " << m_region << " wsPacket: " << wsPacket);

									ScmaStateMachine::RecvCloudResourceInfo(wsPacket, pkt->Copy());
								}
								break;
						case ScmaMessageHeader::M_SendTaskToCN:
								// receive task from main node
								// start a download timer to simulate
								
								// get the response address
								responseIp=scmaMessageHeader.GetSrcIp();
								responseMac=scmaMessageHeader.GetSrcMAC();

								if (nodeState==INITIAL) {
									AugmentationAgree=true;
									ResourceAvailable=1;
									float downloadParts=scmaMessageHeader.GetDownloadParts();
									ScmaStateMachine::downloadSize=float(scmaMessageHeader.GetDownloadSize()/downloadParts);
									ScmaStateMachine::downloadTime=(float)(downloadSize)/((float)scmaMessageHeader.GetBW()/8); 
									// 1000x timer to simulate unfinished downloads
									// float downloadTime=1000.0*(float)(downloadSize/downloadParts)/((float)scmaMessageHeader.GetBW()/8); 
									NS_LOG_DEBUG("* Received M_SendTaskToCN in node: " << m_nodeBase->GetId() << "[" << m_ip << "]\tfrom IP: " << scmaMessageHeader.GetSrcIp()  << "\tActual BW: " << scmaMessageHeader.GetBW()  << " Mbps\t download time: " << downloadTime);
									
									// enable simulated download timer
									if (T_Download.IsRunning()) {
										T_Download.Cancel();
									}
									



									//T_Download.Schedule(ns3::Seconds(downloadTime));

									// 2017-01-28
									// check if requested chunks are in memory
									if (chunksList.size()>=DOWNLOAD_TOTAL_CHUNKS) {
										T_Download.Schedule(ns3::Seconds(0.1));
										NS_LOG_DEBUG("* All chunk in memory in node: " << m_nodeBase->GetId() << "[" << m_ip << "]\tNum. Chunks: " << chunksList.size() << "/" << DOWNLOAD_TOTAL_CHUNKS	<< "/" << downloadParts);
									} else {
										T_Download.Schedule(ns3::Seconds(downloadTime));
										NS_LOG_DEBUG("* New chunk in memory in node: " << m_nodeBase->GetId() << "[" << m_ip << "]\tNum. Chunks: " << chunksList.size() << "/" << DOWNLOAD_TOTAL_CHUNKS	<< "/" << downloadParts);
										//for (int partsTemp=0; partsTemp<downloadParts; partsTemp++) {
											if (chunksList.size()<DOWNLOAD_TOTAL_CHUNKS) {
												chunksList.push_back(downloadParts);
											}
										//}
										
									}									
									
	

									ScmaStateMachine::StateMachine();
								} else {
									// send task not accepted message
									ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_NotAcceptedTaskFromCN, responseIp, responseMac);

								}
								break;
						case ScmaMessageHeader::M_NotAcceptedTaskFromCN:
								NoAcceptedTaskCounter++;
								ScmaStateMachine::StateMachine();
								break;
						case ScmaMessageHeader::M_CompleteTask:
								// completed task from CN
								//CompleteTaskCounter++;	
								ScmaStateMachine::StateMachine();
								break;
						case ScmaMessageHeader::M_CompleteTaskAck:
								NS_LOG_DEBUG("- Received M_CompleteTaskAck in node: " << m_ip );
								T_Download.Cancel();	
								T_ResourceRequest.Cancel();
								T_TaskDistribution.Cancel();
								T_CompleteTaskAck.Cancel();
								T_IncompleteTaskAck.Cancel();
								T_ExpireReception.Cancel();
								T_TaskReception.Cancel();
								ScmaStateMachine::ClearRecvMessage();

								FinishFlag=0;
								ScmaStateMachine::SetState(INITIAL);
								break;
						case ScmaMessageHeader::M_IncompleteTask:
								responseIp=scmaMessageHeader.GetSrcIp();
								responseMac=scmaMessageHeader.GetSrcMAC();

								IncompleteTaskCounter++;

								ScmaStateMachine::StateMachine();

								break;
						case ScmaMessageHeader::M_ResourceInfoFromCN:
								// get the download from the CN 
								CompleteTaskCounter++;	
								if (ScmaStateMachine::nodeState==ScmaStateMachine::TASKRECEPTION) {
									NS_LOG_DEBUG("- Received task from node: " << scmaMessageHeader.GetSrcIp() << "\tTask: " << CompleteTaskCounter << "/" << NTasks);
									// send the M_CompleteTaskAck message to the node
									responseIp=scmaMessageHeader.GetSrcIp();
									responseMac=scmaMessageHeader.GetSrcMAC();
									NS_LOG_DEBUG("- Sending M_CompleteTaskAck to node: " << responseIp);

									ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_CompleteTaskAck, responseIp, responseMac);
									ScmaStateMachine::StateMachine();

								}
								
								break;

						default:
								break;
					
					}
				
				
				}	
		}
		
	}	

	void ScmaStateMachine::TimeoutDownload() {
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " download finished ");
		FinishFlag=1;
		// send the download information to the app host
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " Sending M_CompleteTask & M_ResourceInfoFromCN to node ip: " << ScmaStateMachine::responseIp);		
		
		ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_ResourceInfoFromCN, responseIp, responseMac,0);
		ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_CompleteTask, responseIp, responseMac, (uint32_t)(downloadSize*1024*50));
		NS_LOG_DEBUG("SCMA Node: " << m_nodeBase->GetId() << " Waiting confirmation from app host");		
		ScmaStateMachine::StateMachine();
	}

	void ScmaStateMachine::SendCloudResources(uint32_t ws) {
		// takes the hostsTable information from the intersection node and sends to the node that request the info
		Ptr<Packet> pkt = Create<Packet>();
		Ipv4Address ipDst=Ipv4Address::GetBroadcast();
	    Address macDst=m_netDeviceBase->GetBroadcast(); 	
		// create the scma packet header
		ScmaMessageHeader scmaMessageHeader = ScmaMessageHeader();
		scmaMessageHeader.SetData(ScmaMessageHeader::T_SCMA, ScmaMessageHeader::M_CloudResourceInfo, ipDst, macDst, ws);

		// add the virtual layer plus header to the packet (prepend header)
		VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();
        virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::VirtualToScma);
        virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::SCMA_Packet);
        virtualLayerPlusHdr.SetRegion(m_region);
        virtualLayerPlusHdr.SetSrcAddress(m_ip);
        virtualLayerPlusHdr.SetSrcMAC(m_mac);
        virtualLayerPlusHdr.SetDstAddress(ipDst);
        virtualLayerPlusHdr.SetDstMAC(macDst);
        virtualLayerPlusHdr.SetRol(m_state);

		// add the hostsTable packet, sending the packets of the corresponding waysegment
		//NS_LOG_DEBUG("\tnode: " << m_nodeBase->GetId() << "\tHosts in table:\t" << ScmaStateMachine::ibr_hostsTableDiscovery.size() );
		
		// remove the unwanted hosts from other waysegments
		// and add the hosts to the packet
		IbrHostListTrailer ibrHostListTrailer = IbrHostListTrailer();
		ibrHostListTrailer.ClearHostList();
		ibrHostListTrailer.SetData(IbrHostListTrailer::IbrDiscovery, ws, m_region, true);
	


		for (std::list<IbrHostListTrailer::PayloadHostListEntry>::iterator i = ScmaStateMachine::ibr_hostsTableDiscovery.begin(); i != ScmaStateMachine::ibr_hostsTableDiscovery.end(); ++i) {
			if (i->waySegment!=ws) {
				i=ScmaStateMachine::ibr_hostsTableDiscovery.erase(i);
			} else {
				ibrHostListTrailer.AddHost(i->waySegment, i->m_region, i->leaderIp, i->leaderMac, i->hostIp, i->hostMac, i->virtualIp, i->BW, i->RSAT);

			}				
		}
		//NS_LOG_DEBUG("\tnode: " << m_nodeBase->GetId() << "\tHosts in packet:\t" << ibrHostListTrailer.GetNumHosts() );
			

		// add the headers
		pkt->AddHeader(ibrHostListTrailer);
		pkt->AddHeader(scmaMessageHeader);
		pkt->AddHeader(virtualLayerPlusHdr);
		
		// send the packet
		ScmaStateMachine::m_netDeviceBase->Send(pkt, macDst, 0);
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-PktTx\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<pkt->GetSize() << "\tMsg: " << (scmaMessageHeader));

	
	}

	void ScmaStateMachine::RecvCloudResourceInfo(uint32_t ws, Ptr<Packet> pkt) {
		// process the CloudResourceInformation packet
		
		// remove the ibrHostListTrailer header	
		pkt->RemoveHeader(ScmaStateMachine::ibrHostListTrailer);

		// check if there are enought resources (numHosts > 1)
		uint32_t hostsInWs=ibrHostListTrailer.GetNumHosts();

		//NS_LOG_DEBUG("* Nodes on waysegment (from ibr packet): " << ibrHostListTrailer.GetNumHosts());
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-RecvCloudResourceInfo\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region<<"\tNumHosts:"<< hostsInWs );
		if (hostsInWs>=MIN_HOSTS_AUGMENTATION) {
			
			T_ResourceRequest.Cancel();
			T_TaskDistribution.Schedule();
			DistributionFlag=0;
			InsufficientResources=0;
			//ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_Nothing;

			ScmaStateMachine::SetState(TASKDISTRIBUTION);

		} else {
			// Insufficient Resources
			// go back to initial state
			NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-InsufficientResources\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region );
			InsufficientResources=1;
			T_ResourceRequest.Cancel();
			ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_CloudRelease, Ipv4Address::GetBroadcast(), m_netDeviceBase->GetBroadcast(),0 );
			ScmaStateMachine::SendScmaPkt(ScmaMessageHeader::M_InsufficientResources, Ipv4Address::GetBroadcast(), m_netDeviceBase->GetBroadcast() ,0);

			ScmaStateMachine::RecvMsg=ScmaMessageHeader::M_Nothing;	
			ScmaStateMachine::SetState(INITIAL);
		}



	}


	void ScmaStateMachine::SetMsg(ScmaMessageHeader::ScmaMessages msg) {
		NS_LOG_DEBUG("MSG:: " << (int)static_cast<uint8_t>(msg));
		msg1=ScmaMessageHeader::M_Nothing;
	}
	
	void ScmaStateMachine::SendTaskToCN(Ipv4Address ip, Address mac, uint32_t downSize, uint32_t totalNodes, uint32_t bw) {
		// scma header
		ScmaMessageHeader scmaMessageHeader = ScmaMessageHeader();
		scmaMessageHeader.SetData(ScmaMessageHeader::T_SCMA, ScmaMessageHeader::M_SendTaskToCN, ip, mac, m_waysegment);
		scmaMessageHeader.SetDownloadSize(downSize);
		scmaMessageHeader.SetDownloadParts(totalNodes);
		scmaMessageHeader.SetBW(bw);
		scmaMessageHeader.SetSrcIp(m_ip);
		scmaMessageHeader.SetSrcMAC(m_mac);

		//vnlayer header
		VirtualLayerPlusHeader virtualLayerPlusHdr = VirtualLayerPlusHeader();
        virtualLayerPlusHdr.SetType(VirtualLayerPlusHeader::VirtualToScma);
        virtualLayerPlusHdr.SetSubType(VirtualLayerPlusHeader::SCMA_Packet);
        virtualLayerPlusHdr.SetRegion(m_region);
        virtualLayerPlusHdr.SetSrcAddress(m_ip);
        virtualLayerPlusHdr.SetSrcMAC(m_mac);
        virtualLayerPlusHdr.SetDstAddress(ip);
        virtualLayerPlusHdr.SetDstMAC(mac);

		Ptr<Packet> pkt = Create<Packet>();
		pkt->AddHeader(scmaMessageHeader);
		pkt->AddHeader(virtualLayerPlusHdr);
		// send the packet
		ScmaStateMachine::m_netDeviceBase->Send(pkt, mac, 0);
		NS_LOG_UNCOND("Time:" << Simulator::Now().GetSeconds() << "\tEvt:ScmaStateMachine-PktTx\tNode:"<<m_nodeBase->GetId()<<"\tRegion:"<<m_region<<"\tPktSize:"<<pkt->GetSize() << "\tMsg: " << scmaMessageHeader);
		//NS_LOG_DEBUG("- Pkt SendTaskToCN to "<< ip << " - " << mac << "\t" << downSize << " MB in " << totalNodes << " parts from IP: " << m_ip);	
	}
		
	
}
