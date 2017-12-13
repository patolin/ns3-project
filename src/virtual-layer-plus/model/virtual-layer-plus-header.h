/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef VIRTUAL_LAYER_PLUS_HEADER_H_
#define VIRTUAL_LAYER_PLUS_HEADER_H_

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/arp-header.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/mac48-address.h"

namespace ns3 {


class VirtualLayerPlusHeader : public Header
{
public:
    
    enum type// : uint8_t
    {
        Application = 0x00,
        LeaderElection = 0x01,
        Synchronization = 0x02,
        Hello = 0x03,
		virtual_to_aodv = 0x04,
        aodv_hello = 0x05,
        topology = 0x06,
        Empty = 0x07,
        RtRepair = 0x08,
        LeaderSync = 0x09,
        // for leader host table sync        
        VirtualToIbr = 0xF0,
        /* SCMA Start */
        VirtualToScma= 0xF1
    };
    
    enum subType// : uint8_t
    {
        Client = 0x00,
        Server = 0x01,
        ForwardedByLeader = 0x02,
        ForwardedByBackUp = 0x03,
	LeaderRequest = 0x04,
        LeaderReply = 0x05,
        Heartbeat = 0x06,
        LeaderLeft = 0x07,
        BackUpLeft = 0x08,
	SyncRequest = 0x09,
        SyncData = 0x0A,
        SyncAck = 0x0B,
        hello = 0x0C,
        aodv = 0x0D,
        AckRepair = 0x0E,
        ChangeRoutes = 0x0F,
        CorrectionRouteRequest = 0x10,
        CorrectionRouteReply = 0x11,
        LeaderBecomeRequest = 0x12,
        CeaseLeaderAnnouncement = 0x13,
        ArpRequest = 0x14,
        ArpReply = 0x15,
        LeaderHostTableSync = 0x16,
        IbrDiscovery = 0x20,
		SCMA_AugmentationRequest = 0x30,
		SCMA_Packet = 0x31
    };

    enum typeState// : uint8_t
    {
        INITIAL = 0x00,
        REQUEST = 0x01,
        LEADER = 0x02,
        NONLEADER = 0x03,
        INTERIM = 0x04
    };

  VirtualLayerPlusHeader (void);
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);


    type GetType() const;
    void SetType(type msgType);
    
    subType GetSubType() const;
    void SetSubType(subType msgSubType);
    
    u_int32_t GetRegion() const;
    void SetRegion(u_int32_t region);
    
    uint64_t GetTimeInState() const;
    void SetTimeInState(uint64_t timeInState);
    
    uint32_t GetNumBackUp() const;
    void SetNumBackUp(uint32_t numBackUp);
    
    uint32_t GetBackUpsNeeded() const;
    void SetBackUpsNeeded(uint32_t backUpsNeeded);
    
    uint32_t GetBackUpPriority() const;
    void SetBackUpPriority(uint32_t backUpPriority);
    
    typeState GetRol() const;
    void SetRol(typeState rol);
    
    Address GetSrcMAC() const;
    void SetSrcMAC(Address srcMAC);
    
    Address GetDstMAC() const;
    void SetDstMAC(Address dstMAC);
    
    uint32_t GetLeaderID() const;
    void SetLeaderID(uint32_t leaderID);
    
  /**
   * \brief Set the Source Address.
   * \param srcAddress the Source Address.
   */
  void SetSrcAddress (Ipv4Address srcAddress);

  /**
   * \brief Get the Source Address.
   * \return the Source Address.
   */
  Ipv4Address GetSrcAddress () const;

  /**
   * \brief Set the Destination Address.
   * \param dstAddress the Destination Address.
   */
  void SetDstAddress (Ipv4Address dstAddress);

  /**
   * \brief Get the Destination Address.
   * \return the Destination Address.
   */
  Ipv4Address GetDstAddress () const;

private:
    type m_type;
    subType m_subType;
    uint32_t m_region;
    Ipv4Address m_srcAddress;
    Address m_srcMAC;
    Ipv4Address m_dstAddress;
    Address m_dstMAC;
    uint64_t m_timeInState;
    uint32_t m_numBackUp;
    uint32_t m_backUpsNeeded;
    uint32_t m_backUpPriority;
    uint32_t m_leaderID;
	
    typeState m_rol;
	

};

/**
 * \brief Stream insertion operator.
 *
 * \param os the reference to the output stream
 * \param header the LayerPlusHeader
 * \returns the reference to the output stream
 */
std::ostream & operator<< (std::ostream & os, VirtualLayerPlusHeader const &header);

}

#endif /* VIRTUAL_LAYER_PLUS_HEADER_H_ */
