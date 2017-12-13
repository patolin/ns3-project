// ibr host table from vnlayer


#ifndef IBR_REGION_HOST_TABLE_H
#define IBR_REGION_HOST_TABLE_H

namespace ns3 {
	class IbrRegionHostTable {
		public:
			struct HostEntry {
					Ipv4Address ip;
					Address mac;
					Timer timerLifeHostEntry;
					
					Ipv4Address leaderIp;
					Address leaderMac;
					
					Ipv4Address virtualIp;
					
					uint32_t waySegment;
					uint32_t m_region;
				
					double BW;
					Time RSAT;

					

					HostEntry(Ipv4Address ipAddr, Address macAddr) :
					ip(ipAddr),
					mac(macAddr),
					timerLifeHostEntry(Timer::CANCEL_ON_DESTROY) {
					}
					void HostEntryRegion(Ipv4Address ipAddr, Address macAddr, Ipv4Address leadIpAddr, Address leadMacAddr, Ipv4Address virtualIpAddr, uint32_t ws, uint32_t reg, double bw, Time rsat) {
						ip=(ipAddr);
						mac=(macAddr);
						timerLifeHostEntry=(Timer::CANCEL_ON_DESTROY);
						leaderIp=(leadIpAddr);
						leaderMac=(leadMacAddr);
						virtualIp=(virtualIpAddr);
						waySegment=(ws);
						m_region=(reg);
						BW=bw;
						RSAT=rsat;
					}
					
			};
	};
}

#endif
