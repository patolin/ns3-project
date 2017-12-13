/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <list>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "ibr-hostlist-trailer.h"

NS_LOG_COMPONENT_DEFINE("IbrHostListTrailer");

namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED(IbrHostListTrailer);

    /*
     * IbrHostListTrailer
     */
    

    IbrHostListTrailer::IbrHostListTrailer() {
		m_numHosts=0;
	}
    
    
    
    //void IbrHostListTrailer::SetData(IbrHostListTrailer::type pkt_type, IbrHostListTrailer::subType pkt_subType,  uint32_t ws, uint32_t region, bool direction)
    void IbrHostListTrailer::SetData(IbrHostListTrailer::ibrtype type, uint32_t ws, uint32_t region, bool direction)
	{
	  ibr_type=type;
	  m_waysegment=ws;
	  m_region=region;
	  m_txDirection=direction;
	  
	}
	
	uint32_t IbrHostListTrailer::GetData (void)
	{
	  return (14+70*m_numHosts);//1024; //m_data.size();
	}
	
	uint32_t IbrHostListTrailer::GetSerializedSize (void) const
	{
		uint32_t numBytes=(14+70*m_numHosts);
	  	return numBytes;
	}
	
	
	
	void IbrHostListTrailer::Serialize (Buffer::Iterator start) const
	{
	
		/*
			IBR: bytes
			1+1+4+4+4+n*(4+4+4+1+20+1+20+4+4+4)
			14+n*(66)

		 
		 
		 */
		uint8_t mac[20];
		
		Buffer::Iterator buf = start;
		
		buf.WriteU8(static_cast<uint8_t> (ibr_type));	
		uint8_t dirTemp=0x00;
		if (m_txDirection==true) {
			dirTemp=0x01;
		}
		buf.WriteU8(static_cast<uint8_t> (dirTemp));

		buf.WriteHtonU32(m_waysegment);
		buf.WriteHtonU32(m_region);
		buf.WriteHtonU32(m_numHosts);
		
		
		// NS_LOG_DEBUG("**\tSerialize Region: " << m_region << "\tNodes on discovery list: " << m_data.size() << ":"<< m_numHosts);
		// send the node table to the next leader
		if (m_numHosts>0) {
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
	
	uint32_t  IbrHostListTrailer::Deserialize (Buffer::Iterator start)
	{
		uint8_t mac[20];
		
		Buffer::Iterator i = start;
	
	 	ibr_type = static_cast<IbrHostListTrailer::ibrtype> (i.ReadU8());

		uint8_t dir = static_cast<uint8_t> (i.ReadU8());
		m_txDirection=false;
		if (dir==0x01) {
			m_txDirection=true;
		}
        
		m_waysegment = i.ReadNtohU32();
        m_region = i.ReadNtohU32();
		m_numHosts = i.ReadNtohU32();

		if (m_numHosts>0) {
			m_data.clear();
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
		
		
		return (14+70*m_numHosts);
	}
	
	IbrHostListTrailer::ibrtype IbrHostListTrailer::GetType() const {
		return ibr_type;
	}
	
	uint32_t  IbrHostListTrailer::GetRegion() {
		return m_region;
	}
	
	uint32_t  IbrHostListTrailer::GetWaysegment() {
		return m_waysegment;
	}
	
	bool IbrHostListTrailer::GetDirection() {
		return m_txDirection;
	}
	
	TypeId IbrHostListTrailer::GetTypeId (void)
	{
	  static TypeId tid = TypeId ("ns3::IbrHostListTrailer").SetParent<Header> ();
	  return tid;
	}
	
	TypeId IbrHostListTrailer::GetInstanceTypeId(void) const {
        return GetTypeId();
    }
    
    /*
    
    std::ostream & operator<<(std::ostream & os, const IbrHostListTrailer & h) {
        h.Print(os);
        return os;
    }
    */
    
    void IbrHostListTrailer::Print(std::ostream & os) const {
		os << "Packet Trailer Host List Sync";
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
    
    void IbrHostListTrailer::ClearHostList() {
		m_data.clear();
		m_numHosts=0;
	}
	
	void IbrHostListTrailer::AddHost(uint32_t ws,
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
		
    
    uint32_t IbrHostListTrailer::GetNumHosts() {
        return m_numHosts;
    }
    
    std::list<IbrHostListTrailer::PayloadHostListEntry> IbrHostListTrailer::GetHostsList() {
        return m_data;
    }
	
}
