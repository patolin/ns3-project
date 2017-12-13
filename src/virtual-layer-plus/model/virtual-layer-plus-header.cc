/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "virtual-layer-plus-header.h"


namespace ns3 {


    /*
     * VirtualLayerPlusHeader
     */
    NS_OBJECT_ENSURE_REGISTERED(VirtualLayerPlusHeader);

    VirtualLayerPlusHeader::VirtualLayerPlusHeader() {
    }

    TypeId VirtualLayerPlusHeader::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::VirtualLayerPlusHeader").SetParent<Header> ().AddConstructor<VirtualLayerPlusHeader> ();
        return tid;
    }

    TypeId VirtualLayerPlusHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    void VirtualLayerPlusHeader::Print(std::ostream & os) const {

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

    }

    uint32_t VirtualLayerPlusHeader::GetSerializedSize() const {
        uint32_t serializedSize = 42;

        serializedSize += m_srcMAC.GetLength() + m_dstMAC.GetLength();
        return serializedSize;
    }

    void VirtualLayerPlusHeader::Serialize(Buffer::Iterator start) const {
        Buffer::Iterator i = start;

        uint8_t mac[20];

        i.WriteU8(static_cast<uint8_t> (m_type));
        i.WriteU8(static_cast<uint8_t> (m_subType));
        i.WriteHtonU32(m_region);
        i.WriteHtonU32(m_srcAddress.Get());
        m_srcMAC.CopyTo(mac);
        i.WriteU8(m_srcMAC.GetLength());
        i.Write(mac, m_srcMAC.GetLength());
        i.WriteHtonU32(m_dstAddress.Get());
        m_dstMAC.CopyTo(mac);
        i.WriteU8(m_dstMAC.GetLength());
        i.Write(mac, m_dstMAC.GetLength());
        i.WriteHtonU64(m_timeInState);
        i.WriteHtonU32(m_numBackUp);
        i.WriteHtonU32(m_backUpsNeeded);
        i.WriteHtonU32(m_backUpPriority);
        i.WriteU8(static_cast<uint8_t> (m_rol));
        i.WriteHtonU32(m_leaderID);

    }

    uint32_t VirtualLayerPlusHeader::Deserialize(Buffer::Iterator start) {
        Buffer::Iterator i = start;

        uint8_t mac[20];

        m_type = static_cast<type> (i.ReadU8());
        m_subType = static_cast<subType> (i.ReadU8());
        m_region = i.ReadNtohU32();
        m_srcAddress.Set(i.ReadNtohU32());
        uint8_t len = i.ReadU8();
        i.Read(mac, len);
        m_srcMAC.CopyFrom(mac, len);
        m_dstAddress.Set(i.ReadNtohU32());
        len = i.ReadU8();
        i.Read(mac, len);
        m_dstMAC.CopyFrom(mac, len);
        m_timeInState = i.ReadNtohU64();
        m_numBackUp = i.ReadNtohU32();
        m_backUpsNeeded = i.ReadNtohU32();
        m_backUpPriority = i.ReadNtohU32();
        m_rol = static_cast<typeState> (i.ReadU8());
        m_leaderID = i.ReadNtohU32();

        return GetSerializedSize();
    }

    void VirtualLayerPlusHeader::SetSrcAddress(Ipv4Address srcAddress) {
        m_srcAddress = srcAddress;
    }

    Ipv4Address VirtualLayerPlusHeader::GetSrcAddress() const {
        return m_srcAddress;
    }

    void VirtualLayerPlusHeader::SetDstAddress(Ipv4Address dstAddress) {
        m_dstAddress = dstAddress;
    }

    Ipv4Address VirtualLayerPlusHeader::GetDstAddress() const {
        return m_dstAddress;
    }

    VirtualLayerPlusHeader::type VirtualLayerPlusHeader::GetType() const {
        return m_type;
    }

    void VirtualLayerPlusHeader::SetType(VirtualLayerPlusHeader::type msgType) {
        m_type = msgType;
    }

    VirtualLayerPlusHeader::subType VirtualLayerPlusHeader::GetSubType() const {
        return m_subType;
    }

    void VirtualLayerPlusHeader::SetSubType(VirtualLayerPlusHeader::subType msgSubType) {
        m_subType = msgSubType;
    }

    uint32_t VirtualLayerPlusHeader::GetRegion() const {
        return m_region;
    }

    void VirtualLayerPlusHeader::SetRegion(uint32_t region) {
        m_region = region;
    }

    uint64_t VirtualLayerPlusHeader::GetTimeInState() const {
        return m_timeInState;
    }

    void VirtualLayerPlusHeader::SetTimeInState(uint64_t timeInState) {
        m_timeInState = timeInState;
    }

    uint32_t VirtualLayerPlusHeader::GetNumBackUp() const {
        return m_numBackUp;
    }

    void VirtualLayerPlusHeader::SetNumBackUp(uint32_t numBackUp) {
        m_numBackUp = numBackUp;
    }

    uint32_t VirtualLayerPlusHeader::GetBackUpsNeeded() const {
        return m_backUpsNeeded;
    }

    void VirtualLayerPlusHeader::SetBackUpsNeeded(uint32_t backUpsNeeded) {
        m_backUpsNeeded = backUpsNeeded;
    }

    uint32_t VirtualLayerPlusHeader::GetBackUpPriority() const {
        return m_backUpPriority;
    }

    void VirtualLayerPlusHeader::SetBackUpPriority(uint32_t backUpPriority) {
        m_backUpPriority = backUpPriority;
    }

    VirtualLayerPlusHeader::typeState VirtualLayerPlusHeader::GetRol() const {
        return m_rol;
    }

    void VirtualLayerPlusHeader::SetRol(VirtualLayerPlusHeader::typeState rol) {
        m_rol = rol;
    }

    Address VirtualLayerPlusHeader::GetSrcMAC() const {
        return m_srcMAC;
    }

    void VirtualLayerPlusHeader::SetSrcMAC(Address srcMAC) {
        m_srcMAC = srcMAC;
    }

    Address VirtualLayerPlusHeader::GetDstMAC() const {
        return m_dstMAC;
    }

    void VirtualLayerPlusHeader::SetDstMAC(Address dstMAC) {
        m_dstMAC = dstMAC;
    }

    uint32_t VirtualLayerPlusHeader::GetLeaderID() const {
        return m_leaderID;
    }

    void VirtualLayerPlusHeader::SetLeaderID(uint32_t leaderID) {
        m_leaderID = leaderID;
    }

    std::ostream & operator<<(std::ostream & os, const VirtualLayerPlusHeader & h) {
        h.Print(os);
        return os;
    }

}

