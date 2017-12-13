/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "backup-table-header.h"


namespace ns3 {


    /*
     * BackUpTableHeader
     */
    NS_OBJECT_ENSURE_REGISTERED(BackUpTableHeader);

    BackUpTableHeader::BackUpTableHeader() {
    }

    TypeId BackUpTableHeader::GetTypeId(void) {
        static TypeId tid = TypeId("ns3::BackUpTableHeader").SetParent<Header> ().AddConstructor<BackUpTableHeader> ();
        return tid;
    }

    TypeId BackUpTableHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }

    void BackUpTableHeader::Print(std::ostream & os) const {
        os << "NumBackUp: " << m_backUpTable.size();
    }

    uint32_t BackUpTableHeader::GetSerializedSize() const {
        uint32_t serializedSize = 4 + static_cast<uint32_t> (m_backUpTable.size()) * 10;

        for (std::list<BackUpEntry>::const_iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            serializedSize += i->mac.GetLength();
        }
        return serializedSize;
    }

    void BackUpTableHeader::Serialize(Buffer::Iterator start) const {

        Buffer::Iterator iter = start;

        uint8_t mac[20];

        iter.WriteHtonU32(static_cast<uint32_t> (m_backUpTable.size()));

        for (std::list<BackUpEntry>::const_iterator i = m_backUpTable.begin(); i != m_backUpTable.end(); ++i) {
            iter.WriteHtonU32(i->ip.Get()); // IP
            iter.WriteU8(((i->isSynced) ? 1 : 0)); // State
            iter.WriteHtonU32(i->priority); // Priority

            // MAC
            i->mac.CopyTo(mac);
            iter.WriteU8(i->mac.GetLength());
            iter.Write(mac, i->mac.GetLength());
        }

    }

    uint32_t BackUpTableHeader::Deserialize(Buffer::Iterator start) {

        Buffer::Iterator iter = start;



        uint32_t numBackUp = iter.ReadNtohU32();

        for (int i = 0; i < (int)numBackUp; i++) {
            BackUpEntry backUpEntry;

            backUpEntry.ip.Set(iter.ReadNtohU32()); // IP
            backUpEntry.isSynced = ((iter.ReadU8() == 1) ? true : false); // State
            backUpEntry.priority = iter.ReadNtohU32(); // Priority

            // MAC
            uint8_t len;
            uint8_t mac[20];

            len = iter.ReadU8();
            iter.Read(mac, len);
            backUpEntry.mac.CopyFrom(mac, len);

            m_backUpTable.push_back(backUpEntry);
        }

        return GetSerializedSize();
    }

    std::list<BackUpTableHeader::BackUpEntry> BackUpTableHeader::GetBackUpTable() {
        return m_backUpTable;
    }

    void BackUpTableHeader::SetBackUpTable(std::list<BackUpTableHeader::BackUpEntry> backUpTable) {
        m_backUpTable = backUpTable;
    }


    std::ostream & operator<<(std::ostream & os, const BackUpTableHeader & h) {
        h.Print(os);
        return os;
    }

}

