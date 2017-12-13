/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <list>
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "scma-message-header.h"

NS_LOG_COMPONENT_DEFINE("ScmaMessageHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(ScmaMessageHeader);

    

    ScmaMessageHeader::ScmaMessageHeader() {
    	downloadSize=0;
		downloadParts=0;
		packetSize=1+1+4+4+4+4+4+1+(20*1);

	}
    
    
    
    void ScmaMessageHeader::SetData(ScmaMessageHeader::ScmaType type, ScmaMessageHeader::ScmaMessages msg, /* Ipv4Address srcIp, Address srcMac,*/ Ipv4Address dstIp, Address dstMac, uint32_t wsOrigen)
	{
	  m_type=type;
	  m_msg=msg;
	  //m_srcAddress=srcIp;
	  m_dstAddress=dstIp;
	  //m_srcMAC=srcMac;
	  m_dstMAC=dstMac;
	  m_wsOrigen=wsOrigen; 
	}
/*	
	uint32_t ScmaMessageHeader::GetData (void)
	{
	  return 2; //m_data.size();
	}
*/

	uint32_t ScmaMessageHeader::GetSerializedSize (void) const
	{
	  // two bytes of data to store
	  //NS_LOG_UNCOND( "getSerializedSize");		
	  	//u_int32_t size=1000;
		return packetSize;
	}
	
	
	
	void ScmaMessageHeader::Serialize (Buffer::Iterator start) const
	{
		
		uint8_t mac[20];
		//NS_LOG_UNCOND("serialize1");		
		Buffer::Iterator buf = start;
		
		// serialize the header of the packet
		buf.WriteU8(static_cast<uint8_t> (m_type));
	
		buf.WriteU8(static_cast<uint8_t> (m_msg));
		
		buf.WriteHtonU32(m_wsOrigen);

		buf.WriteHtonU32(downloadSize);
		buf.WriteHtonU32(downloadParts);

		buf.WriteHtonU32(nodeBW);

		buf.WriteHtonU32(m_srcAddress.Get());
		m_srcMAC.CopyTo(mac);
		buf.WriteU8(m_srcMAC.GetLength());
        buf.Write(mac, m_srcMAC.GetLength());

	}
	
	uint32_t  ScmaMessageHeader::Deserialize (Buffer::Iterator start)
	{
		uint8_t mac[20];
		
		Buffer::Iterator i = start;
		
		m_type = static_cast<ScmaType> (i.ReadU8());
		
		m_msg = static_cast<ScmaMessages> (i.ReadU8());
   		m_wsOrigen =  i.ReadNtohU32(); 
		downloadSize =  i.ReadNtohU32(); 
		downloadParts =  i.ReadNtohU32();

		nodeBW=i.ReadNtohU32();

		m_srcAddress.Set(i.ReadNtohU32());
        uint8_t len = i.ReadU8();
        i.Read(mac, len);
		m_srcMAC.CopyFrom(mac, len);
		return packetSize;
	}
	
	
	TypeId ScmaMessageHeader::GetTypeId (void)
	{
	  static TypeId tid = TypeId ("ns3::ScmaMessageHeader").SetParent<Header> ();
	  return tid;
	}
	
	TypeId ScmaMessageHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }
    
    
    
    std::ostream & operator<<(std::ostream & os, const ScmaMessageHeader & h) {
        h.Print(os);
        return os;
    }
    
    
    void ScmaMessageHeader::Print(std::ostream & os) const {
		switch (m_type) {
			case T_SCMA:
				os << "SCMA - ";
				break;
		}
		switch (m_msg) {
			case M_AugmentationRequest:
				os << "Augmentation Request";
				break;
			case M_SearchRequest:
				os << "Search Request";
				break;
			case M_WithoutCollaborators:
				os << "Without Collaborators";
				break;
			case M_UnavailableAugmentationService:
				os << "M_UnavailableAugmentationService";
				break;
			case M_CloudRelease:
				os << "M_CloudRelease";
				break;
			case M_InsufficientResources:
				os << "M_InsufficientResources";
				break;
			case M_SendTaskToCN:
				os << "M_SendTaskToCN";
				break;
			case M_CompleteTask:
				os << "M_CompleteTask";
				break;
			case M_CompleteTaskAck:
				os << "M_CompleteTaskAck";
				break;
			case M_IncompleteTask:
				os << "M_IncompleteTask";
				break;
			case M_IncompleteTaskAck:
				os << "M_IncompleteTaskAck";
				break;
			case M_TaskInfo:
				os << "M_TaskInfo";
				break;
			case M_AcceptedTaskFromCN:
				os << "M_AcceptedTaskFromCN";
				break;
			case M_CloudResourceInfo:
				os << "M_CloudResourceInfo";
				break;
			case M_SendTaskToVN:
				os << "M_SendTaskToVN";
				break;
			case M_ResourceInfoFromCN:
				os << "M_ResourceInfoFromCN";
				break;
			case M_ResourceRequest:
				os << "M_ResourceRequest";
				break;
			case M_AcceptedTaskFromVN:
				os << "M_AcceptedTaskFromVN";
				break;
			case M_NotAcceptedTaskFromCN:
				os << "M_NotAcceptedTaskFromCN";
				break;
			case M_Nothing:
				os << "M_Nothing";
				break;
		}
    }
    
    
		
	Ipv4Address ScmaMessageHeader::GetSrcAddress() const {
        return m_srcAddress;
    }
    
    
    ScmaMessageHeader::ScmaMessages ScmaMessageHeader::GetMsg() {
		return m_msg;
	}

	ScmaMessageHeader::ScmaType ScmaMessageHeader::GetType() {
		return m_type;
	}
    
	uint32_t ScmaMessageHeader::GetWaySegment() {
		return m_wsOrigen;
	}
   
   void ScmaMessageHeader::SetDownloadSize(uint32_t downSize) {
  		downloadSize=downSize; 
   }	

   uint32_t ScmaMessageHeader::GetDownloadSize() {
   		return downloadSize;
   }

	void ScmaMessageHeader::SetDownloadParts(uint32_t parts) {
  		downloadParts=parts; 
   }	

   uint32_t ScmaMessageHeader::GetDownloadParts() {
   		return downloadParts;
   }

		
}
