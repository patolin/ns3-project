/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef SCMA_STATEMACHINE_
#define SCMA_STATEMACHINE_

// Machine Objects State Machine

#include <list>
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/header.h"
#include "ns3/packet.h"
#include "src/network/utils/ipv4-address.h"
#include "scma-message-header.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/virtual-layer-plus-header.h"
//#include "ns3/ibr-routing-protocol.h"
#include "ns3/ibr-hostlist-trailer.h"
#include "ns3/ibr-waysegment-table.h"

namespace ns3 {
		
	class ScmaStateMachine
	{
		
		
		public: 
			
			
			enum ScmaStates {
				INITIAL 		= 0x00,
				RESOURCEREQUEST	= 0x01,
				TASKRECEPTION	= 0x02,
				TASKDISTRIBUTION= 0x03,
				COLLABORATION	= 0x04
					
			};
				
			ScmaStateMachine();
			
			uint8_t ResourceAvailable;
			uint8_t InsufficientResources;
			
			uint32_t expires;
			uint32_t Distribution;
			uint32_t DistributionFlag;
			uint32_t ExpireReception;
			uint32_t IntersectionFlag;
			uint32_t FinishFlag;

			
			
			uint32_t CompleteTaskCounter;
			uint32_t IncompleteTaskCounter;
			uint32_t AcceptedTaskCounter;
			uint32_t NoAcceptedTaskCounter;
			uint32_t NTasks;
			uint32_t m_region;
			uint32_t m_waysegment;
			uint32_t m_nodeLevel;	
			
			Ipv4Address m_ip;
			Address m_mac;
			uint32_t m_bandwidth;
			VirtualLayerPlusHeader::typeState m_state;

			bool AugmentationAgree;
			Timer T_ResourceRequest;
			Timer T_TaskDistribution;
			Timer T_CompleteTaskAck;
			Timer T_IncompleteTaskAck;
			Timer T_ExpireReception;
			Timer T_TaskReception;

			Ptr<NetDevice> m_netDeviceBase;
			Ptr<Node> m_nodeBase;
			Ptr<IbrWaysegmentTable> m_wsTable;	
			//Ptr<IbrRoutingProtocol> m_ibrRoutingProtocol;			
			
			void SetIntersectionFlag(uint8_t flag);
			void SetFinishFlag(uint8_t flag);
			void SetState(ScmaStates newState);
			ScmaStates GetState();
			void StateMachine();
			void StateDispatch();
			void StateInitial();
			void StateTaskReception();
			void StateResourceRequest();
			void StateTaskDistribution();
			void StateCollaboration();
			
			void RecvMess(ScmaMessageHeader::ScmaMessages message);
			void SendMess(ScmaMessageHeader::ScmaMessages message);
			
			void TimeoutResourceRequest();
			void TimeoutTaskDistribution();
			void TimeoutTaskReception();
			void TimeoutCompleteTaskAck();
			void TimeoutIncompleteTaskAck();
			void TimeoutExpireReception();
			void ClearRecvMessage();
			
			static TypeId GetTypeId (void);
			
			//void AugmentationRequest(Ptr<Packet> pkt);
			bool SendScmaPkt(ScmaMessageHeader::ScmaMessages msg, Ipv4Address ipDst, Address macDst, uint32_t payload);
			
			void RecvPkt(Ptr<Packet> packet);
			void SetMsg(ScmaMessageHeader::ScmaMessages msg);
			//std::list<IbrRoutingProtocol::wayMembers> *waySegmentMembers;

			ScmaMessageHeader::ScmaMessages RecvMsg;
			std::list<IbrHostListTrailer::PayloadHostListEntry> ibr_hostsTableDiscovery;

			
			double nodeUsePercent;

			
		private:
			ScmaStates nodeState;
			ScmaMessageHeader::ScmaMessages msg1;
			ScmaMessageHeader::ScmaType type1;
			
			//uint32_t GetWaySegment(uint32_t region);
			//bool IsRegionInWaySegment(uint32_t region, uint32_t waysegment);
			
			IbrHostListTrailer ibrHostListTrailer;
			// internal functions
			void SendCloudResources(uint32_t ws);
			void RecvCloudResourceInfo(uint32_t ws, Ptr<Packet> pkt);
			void SendTaskToCN(Ipv4Address ip, Address mac, uint32_t downSize, uint32_t totalNodes, uint32_t bw);
		
			Timer T_Download;
			float t_downloadTime;
			float downloadSize;
			float downloadTime;
			void TimeoutDownload();


			Ipv4Address responseIp;
			Address responseMac;

			Ipv4Address bcastIp;
			Address bcastMac;
			uint32_t nodeId;

			void CancelTimers();

			std::list<uint32_t> chunksList;
	};
	
	
}

#endif
