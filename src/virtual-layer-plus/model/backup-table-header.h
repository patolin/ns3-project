/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef BACKUP_TABLE_HEADER_H_
#define BACKUP_TABLE_HEADER_H_

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/mac48-address.h"

namespace ns3 {

    class BackUpTableHeader : public Header {
    public:

        struct BackUpEntry {
            Ipv4Address ip;
            Address mac;
            bool isSynced;
            uint32_t priority;
            
            BackUpEntry()
            {
            }
            
            BackUpEntry(Ipv4Address backUpIP, Address backUpMAC, uint32_t backUpPriority):
            ip(backUpIP),
            mac(backUpMAC),
            isSynced(false),
            priority(backUpPriority)
            {
            }
            
            BackUpEntry(Ipv4Address backUpIP, Address backUpMAC, bool backUpState, uint32_t backUpPriority):
            ip(backUpIP),
            mac(backUpMAC),
            isSynced(backUpState),
            priority(backUpPriority)
            {
            }
            
        };
        
        BackUpTableHeader(void);
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        /**
         * \brief Return the instance type identifier.
         * \return instance type ID
         */
        virtual TypeId GetInstanceTypeId(void) const;

        virtual void Print(std::ostream& os) const;

        /**
         * \brief Get the serialized size of the packet.
         * \return size
         */
        virtual uint32_t GetSerializedSize(void) const;

        /**
         * \brief Serialize the packet.
         * \param start Buffer iterator
         */
        virtual void Serialize(Buffer::Iterator start) const;

        /**
         * \brief Deserialize the packet.
         * \param start Buffer iterator
         * \return size of the packet
         */
        virtual uint32_t Deserialize(Buffer::Iterator start);

        std::list<BackUpEntry> GetBackUpTable ();
        void SetBackUpTable(std::list<BackUpEntry> backUpTable);

    private:
        std::list<BackUpEntry> m_backUpTable;

    };

    /**
     * \brief Stream insertion operator.
     *
     * \param os the reference to the output stream
     * \param header the VirtualLayerPlusHeader
     * \returns the reference to the output stream
     */
    std::ostream & operator<<(std::ostream & os, BackUpTableHeader const &header);

}

#endif /* BACKUP_TABLE_HEADER_H_ */
