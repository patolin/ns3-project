/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *	Modulo app-descarga
 *  Construcción de paquetes enviados y recibidos por el módulo de aplicacion
 *	hereda de la clase Header de ns3
 *	(c) 2017 Patricio Reinoso M.
 */

#include <list>
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/address-utils.h"
#include "app-message-header.h"

NS_LOG_COMPONENT_DEFINE("AppMessageHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(AppMessageHeader);

    AppMessageHeader::AppMessageHeader() {
		// configuramos los valores por defecto del mensaje
    	downloadSize=0;
		downloadParts=0;
		packetSize=1+1+4+4+4+4+4+1+(20*1);

	}
    
    
    void AppMessageHeader::SetData(AppMessageHeader::AppType type, AppMessageHeader::AppMessages msg, /* Ipv4Address srcIp, Address srcMac,*/ Ipv4Address dstIp)
	{
	  // configuramos los valores para el mensaje, según los parametros pasados en la funcion
	  m_type=type;
	  m_msg=msg;
	  m_dstAddress=dstIp;
	}

	uint32_t AppMessageHeader::GetSerializedSize (void) const
	{
	  // funcion que devuelve el tamano del mensaje en bytes
	  return packetSize;
	}
	
	
	
	void AppMessageHeader::Serialize (Buffer::Iterator start) const
	{
		// Serialización del mensaje
		// Arma la cabecera del paquete segun los valores indicados en los parametros
		
		uint8_t mac[20];
		Buffer::Iterator buf = start;
		
		// generamos la cabecera del paquete 
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
	
	uint32_t  AppMessageHeader::Deserialize (Buffer::Iterator start)
	{
		// Deserializacion del mensaje
		// Obtiene la cabecera del paquete recibido
		uint8_t mac[20];
		
		Buffer::Iterator i = start;
		m_type = static_cast<AppType> (i.ReadU8());
		m_msg = static_cast<AppMessages> (i.ReadU8());
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
	
	
	TypeId AppMessageHeader::GetTypeId (void)
	{
	  static TypeId tid = TypeId ("ns3::AppMessageHeader").SetParent<Header> ();
	  return tid;
	}
	
	TypeId AppMessageHeader::GetInstanceTypeId(void) const {
        return GetTypeId();
    }
    
    
    
    std::ostream & operator<<(std::ostream & os, const AppMessageHeader & h) {
        h.Print(os);
        return os;
    }
    
    
    void AppMessageHeader::Print(std::ostream & os) const {
		switch (m_type) {
			case T_APPMSG:
				os << "APP - ";
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
			case M_DownloadComplete:
				os << "M_DownloadComplete";
				break;
			case M_NewDownload:
				os << "M_NewDownload";
				break;
			default:
				os << "M_Unknown";
				break;
		}
		//os << " - ";
		//os << " Part# " << downloadParts;
    }
    
    
		
	Ipv4Address AppMessageHeader::GetSrcAddress() const {
        return m_srcAddress;
    }
    
    
    AppMessageHeader::AppMessages AppMessageHeader::GetMsg() {
		return m_msg;
	}

	AppMessageHeader::AppType AppMessageHeader::GetType() {
		return m_type;
	}
    
	uint32_t AppMessageHeader::GetWaySegment() {
		return m_wsOrigen;
	}
   
   void AppMessageHeader::SetDownloadSize(uint32_t downSize) {
  		downloadSize=downSize; 
   }	

   uint32_t AppMessageHeader::GetDownloadSize() {
   		return downloadSize;
   }

	void AppMessageHeader::SetDownloadParts(uint32_t parts) {
  		downloadParts=parts; 
   }	

   uint32_t AppMessageHeader::GetDownloadParts() {
   		return downloadParts;
   }

		
}
