/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef SCMA_MESSAGE_HEADER_H_
#define SCMA_MESSAGE_HEADER_H_

#include <list>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/arp-header.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/mac48-address.h" 

namespace ns3 {
		
	class ScmaMessageHeader : public Header
	{

	public:
	 enum ScmaType {
	 			T_SCMA					= 0xF1		
	 };

	  enum ScmaMessages {
				M_AugmentationRequest	= 10,
				M_SearchRequest			= 11,
				M_WithoutCollaborators	= 12,
				M_UnavailableAugmentationService = 13,
				M_CloudRelease			= 14,
				M_InsufficientResources	= 15,
				M_SendTaskToCN			= 16,
				M_CompleteTask			= 17,
				M_CompleteTaskAck		= 18,
				M_IncompleteTask		= 19,
				M_IncompleteTaskAck		= 20,
				M_TaskInfo				= 21,
				M_AcceptedTaskFromCN	= 22,
				M_CloudResourceInfo		= 23,
				M_SendTaskToVN			= 24,
				M_ResourceInfoFromCN	= 25,
				M_ResourceRequest		= 26,
				M_AcceptedTaskFromVN	= 27,
				M_Nothing				= 00,
				M_NotAcceptedTaskFromCN = 28,	
			};
	  
	  
		
	  
	  
	  
	  // new methods
	  ScmaMessageHeader();
	  void SetData (ScmaMessageHeader::ScmaType type, ScmaMessageHeader::ScmaMessages msg, /* Ipv4Address srcIp, Address srcMac,*/ Ipv4Address dstIp, Address dstMac, uint32_t wsOrigen);
	  uint32_t GetData (void);
	  static TypeId GetTypeId (void);
	  // overridden from Header
	  virtual uint32_t GetSerializedSize (void) const;
	  virtual void Serialize (Buffer::Iterator start) const;
	  virtual uint32_t Deserialize (Buffer::Iterator start);
	  virtual void Print (std::ostream& os) const;
	  virtual TypeId GetInstanceTypeId (void) const;
	  
	  Ipv4Address GetSrcAddress () const;
	  ScmaMessages GetMsg();
	  uint32_t GetWaySegment();
	  ScmaType GetType();

	  void SetDownloadSize(uint32_t downSize);
	  void SetDownloadParts(uint32_t parts);
	
	  uint32_t GetDownloadSize();
	  uint32_t GetDownloadParts();

	  void SetBW(uint32_t bw) { nodeBW=bw; }
	  uint32_t GetBW() { return nodeBW;  }

	  void SetRSAT(uint32_t rsat) { nodeRSAT=rsat;	  }
	  uint32_t GetRSAT() { return nodeRSAT; }

	  void SetSrcIp(Ipv4Address ip) { m_srcAddress=ip; }
	  Ipv4Address GetSrcIp() { return m_srcAddress; }
	 
	  void SetSrcMAC(Address mac) { m_srcMAC=mac; }
	  Address GetSrcMAC() { return m_srcMAC; }

	  //std::ostream & operator<< (std::ostream & os, VirtualLayerPlusPayloadHeader const &header);
	  
	private:
		ScmaType m_type;		
		ScmaMessages m_msg;
		Ipv4Address m_srcAddress;
		Address m_srcMAC;
		Ipv4Address m_dstAddress;
		Address m_dstMAC;
		uint32_t m_wsOrigen;	
		
		uint32_t downloadSize;
		uint32_t downloadParts;	
		uint32_t nodeBW;
		uint32_t nodeRSAT;
       
	   	uint32_t packetSize;	
	};
}

#endif
