/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef IBR_HOST_LIST_TRAILER_H_
#define IBR_HOST_LIST_TRAILER_H_

#include <list>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/arp-header.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/mac48-address.h" 
#include "ns3/timer.h"
#include "ns3/nstime.h"

namespace ns3 {
		
	class IbrHostListTrailer : public Header
	{

	public:
	  enum ibrtype {
			
			IbrDiscovery	=0x01,
			IbrToScma		=0x02,
			IbrToIntersection = 0x03,
			IbrHostToLeader	= 0x04,
			IbrHostToIntersection = 0x05,
			IbrLeaderToLeader = 0x06,
			IbrResourceRequest = 0x07,
		};


	  struct PayloadHostListEntry {
			uint32_t waySegment;
			uint32_t m_region;
			Ipv4Address leaderIp;
            Address leaderMac;
            Ipv4Address hostIp;
            Address hostMac;
            Ipv4Address virtualIp;
            double BW;
            Time RSAT;
            
            
            PayloadHostListEntry(uint32_t ws, uint32_t reg, Ipv4Address lip, Address lmac, Ipv4Address hip, Address hmac, Ipv4Address vIp, double bw, Time rsat) :
            waySegment(ws),
            m_region(reg),
            leaderIp(lip),
            leaderMac(lmac),
            hostIp(hip),
            hostMac(hmac),
            virtualIp(vIp),
            BW(bw),
            RSAT(rsat)
             {
            }

			inline bool operator==(const PayloadHostListEntry& rhs){
    			return hostIp.IsEqual(rhs.hostIp);
			}
        };
		
	  
	  
	  
	  // new methods
	  //void SetPayload(std::list<VirtualLayerPlusNetDevice::HostListEntry> data)
	  IbrHostListTrailer();
	  //void SetData (IbrHostListTrailer::type pkt_type, IbrHostListTrailer::subType pkt_subType, uint32_t ws, uint32_t region, bool direction);
	  //void SetData (IbrHostListTrailer::type pkt_type, IbrHostListTrailer::subType pkt_subType, uint32_t ws, uint32_t region, bool direction, Ipv4Address srcIp, Address srcMac, Ipv4Address dstIp, Address dstMac);
	  void SetData (ibrtype type, uint32_t ws, uint32_t region, bool direction);
	  void ClearHostList();
	  void AddHost (uint32_t ws, uint32_t reg, Ipv4Address lIp, Address lMac, Ipv4Address hIp, Address hMac, Ipv4Address vIp, double bw, Time rsat);
	  uint32_t GetData (void);
	  static TypeId GetTypeId (void);
	  // overridden from Header
	  virtual uint32_t GetSerializedSize (void) const;
	  virtual void Serialize (Buffer::Iterator start) const;
	  virtual uint32_t Deserialize (Buffer::Iterator start);
	  virtual void Print (std::ostream& os) const;
	  virtual TypeId GetInstanceTypeId (void) const;
	  
	  uint32_t GetRegion();
	  uint32_t GetWaysegment();
	  uint32_t GetNumHosts();
	  std::list<PayloadHostListEntry> GetHostsList();
	  
	  bool GetDirection();
  	  ibrtype GetType() const;
	  
	  //std::ostream & operator<< (std::ostream & os, IbrHostListTrailer const &header);
	  
	private:
	  /*
	  struct HostListEntry {
			uint32_t waySegment;
			uint32_t m_region;
			Ipv4Address leaderIp;
            Address leaderMac;
            Ipv4Address hostIp;
            Address hostMac;
            Ipv4Address virtualIp;
            double BW;
            Time RSAT;
            
            Timer timerLifeHostEntry;

            HostListEntry(uint32_t ws, uint32_t reg, Ipv4Address lip, Address lmac, Ipv4Address hip, Address hmac, Ipv4Address vIp, double bw, Time rsat) :
            waySegment(ws),
            m_region(reg),
            leaderIp(lip),
            leaderMac(lmac),
            hostIp(hip),
            hostMac(hmac),
            virtualIp(vIp),
            BW(bw),
            RSAT(rsat)
             {
            }
        };
		*/
		uint32_t m_waysegment;
	    ibrtype ibr_type;
		uint32_t m_region;
		
		bool m_txDirection;
		uint32_t m_numHosts;
		
		std::list<PayloadHostListEntry> m_data;
		
		


        
	};
}

#endif
