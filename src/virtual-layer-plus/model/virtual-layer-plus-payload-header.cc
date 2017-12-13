/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <list>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "virtual-layer-plus-payload-header.h"

NS_LOG_COMPONENT_DEFINE("VirtualLayerPlusPayloadHeader");

namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED(VirtualLayerPlusPayloadHeader);

    /*
     * VirtualLayerPlusPayloadHeader
     */
    

    VirtualLayerPlusPayloadHeader::VirtualLayerPlusPayloadHeader() {
    }
    
    
    
    //void VirtualLayerPlusPayloadHeader::SetData(VirtualLayerPlusPayloadHeader::type pkt_type, VirtualLayerPlusPayloadHeader::subType pkt_subType,  uint32_t ws, uint32_t region, bool direction)
    void VirtualLayerPlusPayloadHeader::SetData(VirtualLayerPlusPayloadHeader::type pkt_type, VirtualLayerPlusPayloadHeader::subType pkt_subType,  uint32_t ws, uint32_t region, bool direction, Ipv4Address srcIp, Address srcMac, Ipv4Address dstIp, Address dstMac)
	{
	  m_type=pkt_type;
	  m_subType=pkt_subType;
	  m_waysegment=ws;
	  m_region=region;
	  m_txDirection=direction;
	  m_srcAddress=srcIp;
	  m_dstAddress=dstIp;
	  m_srcMAC=srcMac;
	  m_dstMAC=dstMac;
	  
	  //NS_LOG_DEBUG("Cargando heder con: " << m_data.size() << " datos" );
	}
	
	uint32_t VirtualLayerPlusPayloadHeader::GetData (void)
	{
	  return 0; //m_data.size();
	}
	
	uint32_t VirtualLayerPlusPayloadHeader::GetSerializedSize (void) const
	{
	  // two bytes of data to store
	  return 1000;
	}
	
	
	
	void VirtualLayerPlusPayloadHeader::Serialize (Buffer::Iterator start) const
	{
		
		uint8_t mac[20];
		
		Buffer::Iterator buf = start;
		
		// serialize the header of the packet
		buf.WriteU8(static_cast<uint8_t> (m_type));
		buf.WriteU8(static_cast<uint8_t> (m_subType));
		buf.WriteHtonU32(m_waysegment);
		buf.WriteHtonU32(m_region);
		
		buf.WriteHtonU32(m_srcAddress.Get());
        m_srcMAC.CopyTo(mac);
        buf.WriteU8(m_srcMAC.GetLength());
        buf.Write(mac, m_srcMAC.GetLength());
        buf.WriteHtonU32(m_dstAddress.Get());
        m_dstMAC.CopyTo(mac);
        buf.WriteU8(m_dstMAC.GetLength());
        buf.Write(mac, m_dstMAC.GetLength());
		
		
		
		uint8_t dirTemp=0x00;
		if (m_txDirection==true) {
			dirTemp=0x01;
		}
		buf.WriteU8(static_cast<uint8_t> (dirTemp));
		buf.WriteHtonU32(m_numHosts);
		
		
		// send the node table to the next leader
		if (m_numHosts>0) {
			NS_LOG_DEBUG("**\tSerialize Region: " << m_region << "\tNodes on discovery list: " << m_data.size() << ":"<< m_numHosts);
			// loop over list, to serialize the hosts information		
			for (std::list<PayloadHostListEntry>::const_iterator i = m_data.begin(); i != m_data.end(); ++i) {
				buf.WriteHtonU32(i->waySegment);
				buf.WriteHtonU32(i->m_region);
				buf.WriteHtonU32(i->leaderIp.Get());
				i->leaderMac.CopyTo(mac);
				buf.WriteU8(i->leaderMac.GetLength());
				buf.Write(mac, i->leaderMac.GetLength());
				
				buf.WriteHtonU32(i->hostIp.Get());
				i->hostMac.CopyTo(mac);
				buf.WriteU8(i->hostMac.GetLength());
				buf.Write(mac, i->hostMac.GetLength());
				
				buf.WriteHtonU32(i->virtualIp.Get());
				buf.WriteHtonU32(i->BW);
				buf.WriteHtonU32(i->RSAT.GetSeconds());
			}
		}
		
		
	  
	}
	
	uint32_t  VirtualLayerPlusPayloadHeader::Deserialize (Buffer::Iterator start)
	{
		uint8_t mac[20];
		
		Buffer::Iterator i = start;
		
		m_type = static_cast<type> (i.ReadU8());
        m_subType = static_cast<subType> (i.ReadU8());
        m_waysegment = i.ReadNtohU32();
        m_region = i.ReadNtohU32();
        
        m_srcAddress.Set(i.ReadNtohU32());
        uint8_t len = i.ReadU8();
        i.Read(mac, len);
        m_srcMAC.CopyFrom(mac, len);
        m_dstAddress.Set(i.ReadNtohU32());
        len = i.ReadU8();
        i.Read(mac, len);
        m_dstMAC.CopyFrom(mac, len);
        
		uint8_t dir = static_cast<type> (i.ReadU8());
		m_txDirection=false;
		if (dir==0x01) {
			m_txDirection=true;
		}
		m_numHosts = i.ReadNtohU32();
		if (m_numHosts>0) {
			m_data.clear();
			NS_LOG_INFO("**\tDeserialize Region: " << m_region << "\tNodes on discovery list: " << m_numHosts);
			uint32_t j;
			for (j=0;j<=(m_numHosts-1);j++) {
				uint8_t len ;
				uint32_t ws = i.ReadNtohU32();
				uint32_t reg = i.ReadNtohU32();
				
				Ipv4Address lIp;
				lIp.Set(i.ReadNtohU32());
				len = i.ReadU8();
				i.Read(mac, len);
				Address lMac;
				lMac.CopyFrom(mac,len);
				
				Ipv4Address hIp;
				hIp.Set(i.ReadNtohU32());
				len = i.ReadU8();
				i.Read(mac, len);
				Address hMac;
				hMac.CopyFrom(mac,len);
				
				Ipv4Address vIp;
				vIp.Set(i.ReadNtohU32());
				
				uint32_t bw = i.ReadNtohU32();
				Time rsat = Seconds(i.ReadNtohU32());
				
				PayloadHostListEntry host = PayloadHostListEntry(ws, reg, lIp, lMac, hIp, hMac, vIp, bw, rsat);
				m_data.push_back(host);
				
			}
		}
		
		
		return m_numHosts;
	}
	
	uint32_t  VirtualLayerPlusPayloadHeader::GetRegion() {
		return m_region;
	}
	
	uint32_t  VirtualLayerPlusPayloadHeader::GetWaysegment() {
		return m_waysegment;
	}
	
	bool VirtualLayerPlusPayloadHeader::GetDirection() {
		return m_txDirection;
	}
	
	TypeId VirtualLayerPlusPayloadHeader::GetTypeId (void)
	{
	  static TypeId tid = TypeId ("ns3::VirtualLayerPlusPayloadHeader").SetParent<Header> ();
	  return tid;
	}
	
	TypeId VirtualLayerPlusPayloadHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }
    
    /*
    
    std::ostream & operator<<(std::ostream & os, const VirtualLayerPlusPayloadHeader & h) {
        h.Print(os);
        return os;
    }
    */
    
    void VirtualLayerPlusPayloadHeader::Print(std::ostream & os) const {
		os << "Packete Payload";
		/*
        switch (m_type) {
            case Application:
                os << "Message Type: Application";
                break;
            case LeaderElection:
                os << "Message Type: LeaderElection";
                break;
            case Synchronization:
                os << "Message Type: Synchronization";
                break;
            case Hello:
                os << "Message Type: Hello";
                break;
            case virtual_to_aodv:
                os << "Message Type: virtual_to_aodv";
                break;
            case aodv_hello:
                os << "Message Type: aodv_hello";
                break;
            case topology:
                os << "Message Type: topology";
                break;
            case Empty:
                os << "Message Type: Empty";
                break;
            case RtRepair:
                os << "Message Type: RtRepair";
                break;
            case LeaderSync:
                os << "Message Type: LeaderSync";
                break;
            default:
                os << "Message Type: UNDEFINED";
                break;
        }

        switch (m_subType) {
            case Client:
                os << ", Message SubType: Client";
                break;
            case Server:
                os << ", Message SubType: Server";
                break;
            case ForwardedByLeader:
                os << ", Message SubType: ForwardedByLeader";
                break;
            case ForwardedByBackUp:
                os << ", Message SubType: ForwardedByBackUp";
                break;
            case LeaderRequest:
                os << ", Message SubType: LeaderRequest";
                break;
            case LeaderReply:
                os << ", Message SubType: LeaderReply";
                break;
            case Heartbeat:
                os << ", Message SubType: Heartbeat";
                break;
            case LeaderLeft:
                os << ", Message SubType: LeaderLeft";
                break;
            case BackUpLeft:
                os << ", Message SubType: BackUpLeft";
                break;
            case SyncRequest:
                os << ", Message SubType: SyncRequest";
                break;
            case SyncData:
                os << ", Message SubType: SyncData";
                break;
            case SyncAck:
                os << ", Message SubType: SyncAck";
                break;
            case hello:
                os << ", Message SubType: hello";
                break;
            case aodv:
                os << ", Message SubType: aodv";
                break;
            case AckRepair:
                os << ", Message SubType: AckRepair";
                break;
            case ChangeRoutes:
                os << ", Message SubType: ChangeRoutes";
                break;
            case CorrectionRouteRequest:
                os << ", Message SubType: CorrectionRouteRequest";
                break;
            case CorrectionRouteReply:
                os << ", Message SubType: CorrectionRouteReply";
                break;
            case LeaderBecomeRequest:
                os << ", Message SubType: LeaderBecomeRequest";
                break;
            case CeaseLeaderAnnouncement:
                os << ", Message SubType: CeaseLeaderAnnouncement";
                break;
            case ArpRequest:
                os << ", Message SubType: ArpRequest";
                break;
            case ArpReply:
                os << ", Message SubType: ArpReply";
                break;
            default:
                os << ", Message SubType: UNDEFINED";
                break;
        }
		*/
    }
    
    void VirtualLayerPlusPayloadHeader::ClearHostList() {
		m_data.clear();
		m_numHosts=0;
	}
	
	void VirtualLayerPlusPayloadHeader::AddHost(uint32_t ws,
									uint32_t reg,
									Ipv4Address lIp,
									Address lMac,
									Ipv4Address hIp,
									Address hMac,
									Ipv4Address vIp,
									double bw,
									Time rsat) {
		 PayloadHostListEntry host = PayloadHostListEntry(ws, reg, lIp, lMac, hIp, hMac, vIp, bw, rsat);
		 m_data.push_back(host);
		 m_numHosts++;
	 }
		
	Ipv4Address VirtualLayerPlusPayloadHeader::GetSrcAddress() const {
        return m_srcAddress;
    }
    
    uint32_t VirtualLayerPlusPayloadHeader::GetNumHosts() {
        return m_numHosts;
    }
    
    std::list<VirtualLayerPlusPayloadHeader::PayloadHostListEntry> VirtualLayerPlusPayloadHeader::GetHostsList() {
        return m_data;
    }
	
}
