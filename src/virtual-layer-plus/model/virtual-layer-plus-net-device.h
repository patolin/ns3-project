/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef VIRTUAL_LAYER_PLUS_NET_DEVICE_H
#define VIRTUAL_LAYER_PLUS_NET_DEVICE_H

#include <stdint.h>
#include <string>
#include <sstream>
#include <list>
#include <math.h>
#include <map>
#include "ns3/traced-callback.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/scma-statemachine.h"
#include "virtual-layer-plus-header.h"
#include "virtual-layer-plus-payload-header.h"

#include "backup-table-header.h"
#include "routing-table-header.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/node.h"
#include "ns3/vector.h"
#include "ns3/wifi-module.h"
#include <ns3/pointer.h>
#include <ns3/ocb-wifi-mac.h>
#include "ns3/arp-cache.h"
#include "ns3/internet-module.h"
#include "ns3/config.h"
#include <ns3/mobility-model.h>
#include "ns3/ibr-region-host-table.h"
#include "ns3/ibr-hostlist-trailer.h"
#include "ns3/ibr-waysegment-table.h"


#define sync              true
#define non_sync          false
#define empty             0
#define dt                Seconds(0.01)
#define none              0
#define unknown           (0xFFFFFFFF)

#ifndef UINT32_MAX
#define UINT32_MAX        (0xFFFFFFFF)
#endif
#ifndef VIRTUAL_IP_BASE
#define VIRTUAL_IP_BASE   (0xF0000000) // Rango IP experimental: 240.0.0.0 (0xF0000000) ~ 255.255.255.255 (0xFFFFFFFF)
#endif


namespace ns3 {

    /**
     * \ingroup virtual-layer-plus
     *
     * \brief Shim performing Virtual Layer Network Protocol
     *
     * This class implements the shim between IPv4 and a generic NetDevice,
     * performing the protocol operations of a Virtual Layer Network Protocol in a transparent way.
     * To this end, the class pretend to be a normal NetDevice, masquerading some functions
     * of the underlying NetDevice.
     */
    class VirtualLayerPlusNetDevice : public NetDevice {
    public:

        /**
         * Enumeration of the dropping reasons in VNLayer.
         */
        enum DropReason {
            DROP_REGION_ABROAD = 1, /**< Packet from another region */
            DROP_NO_LEADER_RTX = 2
        };

        enum BackUpState// : uint8_t
        {
            NONE = 0x00,
            REQUESTING = 0x01,
            SYNC = 0x02,
        };

        struct PacketEntry {
            Ptr<Packet> pkt;
            uint16_t protocolNumber;
            bool doSendFrom;
            bool processed;
            bool retransmitted;
            Timer timerLifePacketEntry;

            PacketEntry(Ptr<Packet> p, uint16_t protNum, bool sendFrom) :
            pkt(p),
            protocolNumber(protNum),
            doSendFrom(sendFrom),
            processed(false),
            retransmitted(false),
            timerLifePacketEntry(Timer::CANCEL_ON_DESTROY) {
            }
        };
        
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
	
		struct wayMembers{
			int m_wayNumber;
			int m_region;
			int m_regionLevel;
			int m_neighbourNext;
			int m_neighbourPrev;
			
			
			
			wayMembers():
				m_wayNumber(-1),
				m_region(-1),
				m_regionLevel(-1),
				m_neighbourNext(-1),
				m_neighbourPrev(-1)
				{ }
				
			void PrintInfo() {
				std::cout << "WN: " << m_wayNumber << "\tRegion: " << m_region << "\tRegionLevel: " << m_regionLevel << "\tNeig.Prev: " << m_neighbourPrev << "\tNeig.Next: " << m_neighbourNext << "\n"; 
			}
		};

/*
        struct HostEntry {
            Ipv4Address ip;
            Address mac;
            Timer timerLifeHostEntry;

            HostEntry(Ipv4Address ipAddr, Address macAddr) :
            ip(ipAddr),
            mac(macAddr),
            timerLifeHostEntry(Timer::CANCEL_ON_DESTROY) {
            }
        };
-*/
	
		

        struct LeaderEntry {
            Ipv4Address ip;
            Address mac;
            uint32_t id;
            Timer timerLifeLeaderEntry;

            LeaderEntry(Ipv4Address leaderIP, Address leaderMAC, uint32_t leaderID) :
            ip(leaderIP),
            mac(leaderMAC),
            id(leaderID),
            timerLifeLeaderEntry(Timer::CANCEL_ON_DESTROY) {
            }
        };

        struct NeighborLeaderEntry {
            uint32_t region;
            uint32_t id;
            Ipv4Address ip;
            Address mac;
            Timer timerLifeNeighborLeaderEntry;

            NeighborLeaderEntry(int leaderRegion, uint32_t leaderID, Ipv4Address leaderIP, Address leaderMAC) :
            region(leaderRegion),
            id(leaderID),
            ip(leaderIP),
            mac(leaderMAC),
            timerLifeNeighborLeaderEntry(Timer::CANCEL_ON_DESTROY) {
            }
        };

        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        /**
         * Constructor for the VirtualLayerPlusNetDevice.
         */
        VirtualLayerPlusNetDevice();

        // inherited from NetDevice base class
        virtual void SetIfIndex(const uint32_t index);
        virtual uint32_t GetIfIndex(void) const;
        virtual Ptr<Channel> GetChannel(void) const;
        virtual void SetAddress(Address address);
        virtual Address GetAddress(void) const;
        virtual bool SetMtu(const uint16_t mtu);

        /**
         * \brief Returns the link-layer MTU for this interface.
         * If the link-layer MTU is smaller than IPv4's minimum MTU,
         * 576 will be returned.
         *
         * \return the link-level MTU in bytes for this interface.
         */
        virtual uint16_t GetMtu(void) const;
        virtual bool IsLinkUp(void) const;
        virtual void AddLinkChangeCallback(Callback<void> callback);
        virtual bool IsBroadcast(void) const;
        virtual Address GetBroadcast(void) const;
        virtual bool IsMulticast(void) const;
        virtual Address GetMulticast(Ipv4Address multicastGroup) const;
        virtual bool IsPointToPoint(void) const;
        virtual bool IsBridge(void) const;
        virtual bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
        virtual bool SendFrom(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
        virtual Ptr<Node> GetNode(void) const;
        virtual void SetNode(Ptr<Node> node);
        virtual bool NeedsArp(void) const;
        virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);
        virtual void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb);
        virtual bool SupportsSendFrom() const;
        virtual Address GetMulticast(Ipv6Address addr) const;

        /**
         * \brief Returns a smart pointer to the underlying NetDevice.
         *
         * \return a smart pointer to the underlying NetDevice.
         */
        Ptr<NetDevice> GetNetDevice() const;

        /**
         * \brief Setup VNLayer to be a proxy for the specified NetDevice.
         * All the packets incoming and outgoing from the NetDevice will be
         * processed by VirtualLayerPlusNetDevice.
         *
         * \param device a smart pointer to the NetDevice to be proxied.
         */
        void SetNetDevice(Ptr<NetDevice> device);

        /**
         * Assign a fixed random variable stream number to the random variables
         * used by this model.  Return the number of streams (possibly zero) that
         * have been assigned.
         *
         * \param stream first stream index to use
         * \return the number of stream indices assigned by this model
         */
        int64_t AssignStreams(int64_t stream);

        /**
         * TracedCallback signature for packet send/receive events.
         *
         * \param [in] packet The packet.
         * \param [in] vnNetDevice The VirtualLayerPlusNetDevice
         * \param [in] ifindex The ifindex of the device.
         */
        typedef void (* RxTxTracedCallback)
        (const Ptr<const Packet> packet,
                const Ptr<const VirtualLayerPlusNetDevice> vnNetDevice,
                const uint32_t ifindex);

        /**
         * TracedCallback signature for
         *
         * \param [in] reason The reason for the drop.
         * \param [in] packet The packet.
         * \param [in] vnNetDevice The VirtualLayerPlusNetDevice
         * \param [in] ifindex The ifindex of the device.
         */
        typedef void (* DropTracedCallback)
        (const DropReason reason, const Ptr<const Packet> packet,
                const Ptr<const VirtualLayerPlusNetDevice> vnNetDevice,
                const uint32_t ifindex);

        typedef void (* StateTracedCallback)
        (const VirtualLayerPlusHeader::typeState oldValue,
                const VirtualLayerPlusHeader::typeState newValue);

        VirtualLayerPlusHeader::typeState GetState();
        uint32_t GetRegion();

        Ipv4Address GetIp();
        Address GetMac();

		bool IsLeader();
        void SetLeader(bool leader);

        /*
         * Arranca el protocolo 
         */
        void Start();


        
      
    private:
        /*
         * Variables de capa Virtual
         */

        VirtualLayerPlusHeader::typeState m_state;
        TracedValue<uint32_t> m_region;
        uint32_t m_leaderID;
        uint32_t m_virtualNodeLevel;
        std::list<LeaderEntry> m_leaderTable;
        double m_timeInState;

        std::list<LeaderEntry> m_lastLeadersTable;

        double m_long;
        double m_width;
        uint32_t m_rows;
        uint32_t m_columns;
        uint32_t m_scnGridX; // Manhatann X grid value
        uint32_t m_scnGridY; // Manhatann Y grid value
        uint32_t m_scnRegBetIntX; // Manhatann regions between intersections X value
        uint32_t m_scnRegBetIntY; // Manhatann regions between intersections Y value
        
        Ipv4Address m_ip;
        Address m_mac;
        Ptr<ArpCache> m_arpCache;
        int m_expires;
        uint8_t m_emptyMAC[20];

        bool m_supportiveBackups;
        bool m_promiscuousMode;

        bool m_multipleLeaders;
        bool m_newLeaderActive;

        bool m_replaced;

        /*
         * Variables para el control de Backups
         */

        BackUpState m_backUpState;
        uint32_t m_backUpPriority;
        uint32_t m_supportedLeaderID;
        std::list<BackUpTableHeader::BackUpEntry> m_backUpTable;

        // Cuando se sepa el formato de la tabla de enrutado esto se cambiará
        // por un nuevo tipo de cabecera que lleve esa información
        uint8_t *m_syncData;

        std::list<HostEntry> m_hostsTable;

        std::list<PacketEntry> m_leaderSupportQueue;

        std::list<NeighborLeaderEntry> m_neighborLeadersTable;

        /*
         * Timers
         */

        Timer m_timerRequestWait;
        Timer m_timerHeartbeat;
        Timer m_timerGetLocation;
        Timer m_timerBackUpRequest;
        Timer m_timerRelinquishment;
        Timer m_timerRetrySyncData;
        Timer m_timerLeaderCease;
        Timer m_timerHello;


		
        
        
        
        // ****************************

    protected:
        virtual void DoDispose(void);

    private:
        /**
         * \brief Copy constructor
         *
         * Defined and unimplemented to avoid misuse
         */
        VirtualLayerPlusNetDevice(VirtualLayerPlusNetDevice const &);
        /**
         * \brief Copy constructor
         *
         * Defined and unimplemented to avoid misuse
         * \returns
         */
        VirtualLayerPlusNetDevice& operator=(VirtualLayerPlusNetDevice const &);
        /**
         * \brief receives all the packets from a NetDevice for further processing.
         * \param device the NetDevice the packet was received from
         * \param packet the received packet
         * \param protocol the protocol (if known)
         * \param source the source address
         * \param destination the destination address
         * \param packetType the packet kind (e.g., HOST, BROADCAST, etc.)
         */
        void ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                Address const &source, Address const &destination, PacketType packetType);


        void PromiscReceiveFromDevice(Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                Address const &src, Address const &dst, PacketType packetType);


        /**
         * \param packet packet sent from above down to Network Device
         * \param source source mac address (so called "MAC spoofing")
         * \param dest mac address of the destination (already resolved)
         * \param protocolNumber identifies the type of payload contained in
         *        this packet. Used to call the right L3Protocol when the packet
         *        is received.
         * \param doSendFrom perform a SendFrom instead of a Send
         *
         *  Called from higher layer to send packet into Network Device
         *  with the specified source and destination Addresses.
         *
         * \return whether the Send operation succeeded
         */
        bool DoSend(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber, bool doSendFrom);

        /**
         * The callback used to notify higher layers that a packet has been received.
         */
        NetDevice::ReceiveCallback m_rxCallback;

        /**
         * The callback used to notify higher layers that a packet has been received in promiscuous mode.
         */
        NetDevice::PromiscReceiveCallback m_promiscRxCallback;

        /**
         * \brief Callback to trace TX (transmission) packets.
         *
         * Data passed:
         * \li Packet received (including VNLayer header)
         * \li Ptr to VirtualLayerPlusNetDevice
         * \li interface index
         */
        TracedCallback<Ptr<const Packet>, Ptr<VirtualLayerPlusNetDevice>, uint32_t> m_txTrace;

        /**
         * \brief Callback to trace RX (reception) packets.
         *
         * Data passed:
         * \li Packet received (including VNLayer header)
         * \li Ptr to VirtualLayerPlusNetDevice
         * \li interface index
         */
        TracedCallback<Ptr<const Packet>, Ptr<VirtualLayerPlusNetDevice>, uint32_t> m_rxTrace;

        /**
         * \brief Callback to trace drop packets.
         *
         * Data passed:
         * \li DropReason
         * \li Packet dropped (including VNLayer header)
         * \li Ptr to VirtualLayerPlusNetDevice
         * \li interface index
         */
        TracedCallback<DropReason, Ptr<const Packet>, Ptr<VirtualLayerPlusNetDevice>, uint32_t> m_dropTrace;

        TracedCallback<VirtualLayerPlusHeader::typeState, VirtualLayerPlusHeader::typeState, uint32_t> m_stateTrace;


        Ptr<Node> m_node; /**< Smart pointer to the Node */
        Ptr<NetDevice> m_netDevice; /**< Smart pointer to the underlying NetDevice */
        uint32_t m_ifIndex; /**< Interface index */
        uint32_t m_ipIfIndex; /**< Ipv4 Interface index */


        /*
         * Funciones asociadas a los timers
         */

        void TimerRequestWaitExpire(uint32_t leaderID);
        void TimerHeartbeatExpire();
        void TimerGetLocationExpire();
        void TimerBackUpRequestExpire();
        void TimerRelinquishmentExpire();
        void TimerRetrySyncDataExpire(Ipv4Address ip, Address mac);
        void TimerLifePacketEntryExpire(uint64_t uid);
        void TimerLifeHostEntryExpire(Ipv4Address ip);
        void TimerLifeLeaderEntryExpire(uint32_t id, Ipv4Address ip);
        void TimerLifeNeighborLeaderEntryExpire(uint32_t region, uint32_t id);
        void TimerLeaderCeaseExpire();
        void TimerHelloExpire();

        void CancelTimers(void);

    public:
        /*
         * Funciones de localizacion
         */

        uint32_t GetLocation(void);
        uint32_t GetRegionIdOf(Vector2D pos);
        Time GetExitTime(void);
        uint32_t GetVirtualNodeLevel(uint32_t reg);

        void CourseChange(std::string context, Ptr<const MobilityModel> mobility);
        
		uint32_t m_actualRegion;
		
    private:

        /*
         * Funciones para el envio y recepcion de paquetes
         */

        void RecvLeaderElection(Ptr<Packet> pkt);
        void RecvSynchronization(Ptr<Packet> pkt);
        void RecvHello(Ptr<Packet> pkt);
        bool SendPkt(VirtualLayerPlusHeader::type msgType, VirtualLayerPlusHeader::subType msgSubType, Ipv4Address ipDst, Address macDst);


        /*
         * Backup Table Management
         */

        void PurgeBackUp(bool updatePriorities);
        void PutBackUp(Ipv4Address ipBackUp, Address macBackUp, uint32_t backUpPriority);
        uint32_t PutBackUp(Ipv4Address ipBackUp, Address macBackUp);
        void QuitBackUp(Ipv4Address ipBackUp, bool updatePriorities);
        void SyncBackUp(Ipv4Address ipBackUp);
        void ClearBackUp(void);
        void LoadBackUpTable(BackUpTableHeader backUpTableHdr);
        void PrintBackUpTable(void);
        uint32_t GetInsertPriority();
        uint32_t GetPriorityOf(Ipv4Address ipBackUp);
        BackUpTableHeader::BackUpEntry GetPriorityBackUpOf(uint32_t leaderID);
        uint32_t GetSupportedLeaderIdOf(Ipv4Address backUpIP);
        bool IsInBackUpTable(Ipv4Address backUpIP);

        /*
         * Hosts Table Management
         */

        /// Put host in the table, if the host is already in the table restart its timerLife.
        bool PutHost(Ipv4Address ip, Address mac);
        /// Quit host from the table with given IP
        bool QuitHost(Ipv4Address ip);
        /// Reschedule the host's timer with time t with given IP
        bool ReschedTimerOfHost(Ipv4Address ip, Time t);
        /// Return the IP Address of a host identified the MAC mac
        Ipv4Address GetIpOfHost(Address mac);
        /// Return the MAC Address of a host identified the IP ip
        Address GetMacOfHost(Ipv4Address ip);


        /*
         * Leader Support Queue Management
         */

        /// Push pkt in queue, if there is no the same packet in queue.
        bool EnqueuePacket(Ptr<Packet> pkt, uint16_t protNum, bool sendFrom);
        /// Pop packet from the queue with given UID without forward it (Overheard)
        bool DequeuePacketWithUid(uint64_t uid);
        /// Forward and erase packet with UID uid
        bool ForwardPacketWithUid(uint64_t uid);


        /*
         * Leader Table Management
         */

        uint32_t PutLeader(Ipv4Address leaderIP, Address leaderMAC);
        void PutLeader(Ipv4Address leaderIP, Address leaderMAC, uint32_t leaderID);
        void QuitLeader(Ipv4Address leaderIP);
        bool IsInLeaderTable(Ipv4Address leaderIP);
        bool IsTheLatestLeader(uint32_t leaderID);
        bool IsTheLessSupportedLeader(uint32_t leaderID);
        void StartTimerLifeOf(uint32_t leaderID);
        void StopTimerLifeOf(uint32_t leaderID);
        LeaderEntry GetLeader(uint32_t leaderID);
        void PrintLeaderTable();
        bool NewLeaderActive();

        /*
         * Last Leaders Table Management
         * 
         */

        void PushLastLeader(Ipv4Address leaderIP, Address leaderMAC, uint32_t leaderID);
        bool IsInLastLeadersTable(Address leaderMAC, uint32_t leaderID);


        /*
         * Neighbor Leaders Table Management
         */

        void PutNeighborLeader(uint32_t leaderRegion, uint32_t leaderID, Ipv4Address leaderIP, Address leaderMAC);
        void QuitNeighborLeader(uint32_t leaderRegion, uint32_t leaderID);
        Address GetLeaderMacForRtx(uint32_t region, uint64_t uid);
        void UpdateNeighborLeadersTable();

    public:
        void PrintNeighborLeadersTable();
        bool IsNeighborRegion(uint32_t region);
        uint32_t GetNumOfNeighborLeadersIn(uint32_t region);

        long unsigned int GetNumOfLeaders();


        bool IsVirtual(Address addr);
        uint32_t GetRegionIdOf(Address addr);
        Address GetVirtualAddressOf(uint32_t region);

        bool IsVirtual(Ipv4Address ip);
        uint32_t GetRegionIdOf(Ipv4Address ip);
        Ipv4Address GetVirtualIpOf(uint32_t region);

        Vector2D GetCoordinatesOfRegion(uint32_t region);
        uint32_t GetRegionForCoordinates(Vector2D coord);

        Vector2D GetCoordinateLimits();

        Time GetHeartbeatPeriod();

        bool IsReplaced();

        


    private:
        Ptr<UniformRandomVariable> m_rng;
        
    
    
    // pato
    public:
        uint32_t m_regionActual;
        uint32_t m_regionAnterior;
    	double m_regionSize;
    	uint32_t numRegionsX;
    	uint32_t numRegionsY;
    	uint8_t m_regionLevel;
    /*	
    	struct wayMembers{
			int m_wayNumber;
			int m_region;
			int m_regionLevel;
			int m_neighbourNext;
			int m_neighbourPrev;
			
			
			
			wayMembers():
				m_wayNumber(-1),
				m_region(-1),
				m_regionLevel(-1),
				m_neighbourNext(-1),
				m_neighbourPrev(-1)
				{ }
				
			void PrintInfo() {
				std::cout << "WN: " << m_wayNumber << "\tRegion: " << m_region << "\tRegionLevel: " << m_regionLevel << "\tNeig.Prev: " << m_neighbourPrev << "\tNeig.Next: " << m_neighbourNext << "\n"; 
			}
		};
		std::list<wayMembers> *waySegmentMembers;
	*/
		std::list<wayMembers> *waySegmentMembers;

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
    	
		// Access point flags
		bool m_isAccessPoint;
    
		// host table for IBR discovery process	
		std::list<IbrRegionHostTable::HostEntry> ibr_hostsTable;
    	
    	void ReceiveHostsTableIbr(std::list<IbrHostListTrailer::PayloadHostListEntry> hostsList);
		uint32_t GetHostsTableSize();
    


	private:
		Time GetRSAT(double posX, double posY, double spdX, double spdY);
		double GetNewBandwidth();
		bool m_isLeader;
		double m_bandwidth;
		double m_posXLast, m_posYLast;
		double m_spdX, m_spdY;
		Time m_rsat;
		uint32_t m_waysegment;
		bool hostListSyncDirection;
		
		uint32_t GetActualWaySegment(uint32_t region);
		uint32_t GetActualWaySegment(uint32_t regionAct, uint32_t regionPrev);
		bool IsRegionInWaySegment(uint32_t region, uint32_t waysegment);
		
		//bool SendSyncHostTablePkt(VirtualLayerPlusPayloadHeader::type msgType, VirtualLayerPlusPayloadHeader::subType msgSubType, Ipv4Address ipDst, Address macDst);
		
		//void RecvSyncHostTable(Ptr<Packet> pkt);
		bool SendAugmentationRequest();
		//uint32_t GetRxSourceRegion(bool dir);
		bool IsIntersectionNeighbour(uint32_t regionLocal, uint32_t region, bool dir);
		
		
        std::list<HostListEntry> m_hostsTableDiscovery;
        std::list<HostListEntry> m_hostsTableRegion;
		// IBR Setup
		// IbrRoutingProtocol ibr;

		ScmaStateMachine nodeSM;


		// augmentation launch timer
		Timer m_timerLaunchAugmentation;
		void TimerLaunchAugmentation();
		bool isL2Region(uint32_t region, bool dir);
		uint32_t getRegionLevel(uint32_t m_region);
		bool removeHostFromTable(Ipv4Address ip);
		
		// waysegment table pointer
		Ptr<IbrWaysegmentTable> m_wsTable;
		void SetWsTable (Ptr<IbrWaysegmentTable> wsT);
	
		// intersection hosts table
		std::list<IbrHostListTrailer::PayloadHostListEntry> ibr_hostsTableDiscovery;


    };
    

    
} // namespace ns3

#endif /* VIRTUAL_LAYER_PLUS_NET_DEVICE_H */
