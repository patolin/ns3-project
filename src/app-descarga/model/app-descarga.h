/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *	Modulo app-descarga
 *	Permite la simulacion de una aplicacion de descarga en modo colaborativo
 *	hereda de la clase Application de ns3
 *	(c) 2017 Patricio Reinoso M.
 */

#ifndef APP_DESCARGA_H
#define APP_DESCARGA_H

#include <vector>
#include "src/network/utils/ipv4-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"

#include <list>
#include <utility>
#include <stdint.h>
#include "ns3/ipv4.h"
#include "ns3/socket.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/timer.h"
#include "ns3/application.h"
#include "ns3/app-message-header.h"

#include <map>

namespace ns3 {

	class AppDescarga : public Application {
		// definimos los estados disponibles para la State Machine
		enum AppStates {
			INITIAL			= 0x00,
			DOWNLOADSTART	= 0x01,
			RESOURCEREQUEST	= 0x02,
			SENDRESOURCES	= 0x03,
			TASKDISTRIBUTION= 0x04,
			TASKRECEPTION	= 0x05,
			COLLABORATION	= 0x06,
			TASKCOMPLETE	= 0x07,
			TASKWAIT 		= 0x08,
			EXECUTINGTASK	= 0x09
		};

		// definimos la estructura de los colaboradores disponibles
		struct AvailableCollaborators {
			Ipv4Address m_ip;
			Address m_mac;
			uint32_t bw;
			Time RSAT;
		};

		public:
			// funciones publicas necesarias para operar la clase
			AppDescarga();
			virtual ~AppDescarga();
			static TypeId GetTypeId();
			void Initialize();


		private:
			// variables y funciones privadas
			AppStates AppState;
			bool isAppNode;
			bool isLeader;
			uint32_t nodeId;
			std::vector<bool> myChunks;
		    std::vector<AvailableCollaborators> myCollabs;	
			Timer T_ResRequest;
			Timer T_TaskTimeout;
			Timer T_DownloadStart;
			Timer T_DownloadSimulationTimer;
			Timer T_CompleteTaskAck;
			Timer T_StopCollabotarion;

			uint32_t downloadSize;
			uint32_t collabsPercent;
			uint32_t m_region;
			void StateMachine();

			// Timers usados
			void TimeoutResourceRequest();
			void TimeoutTaskTimeout();
			void TimeoutDownloadStart();
			void TimeoutDownloadSimulatorTimer();
			void TimeoutCompleteTaskAck();
			void TimeoutStopCollaboration();

			// Funciones para envio y recepcion de paquetes, mediante sockets
			void Accept(Ptr<Socket> socket,const ns3::Address& from) ;
			void RecvPkt (Ptr<Socket> socket);
			void SendPkt(Ptr<Packet>, Ipv4Address ipDest1);
			std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses; 
			void SendTaskAck(Ipv4Address ipDest);
			
			// funciones virtuales, se heredan de la clase aplicacion (requeridas)
			virtual void StartApplication();
			virtual void StopApplication();


			Ptr<Ipv4> m_ipv4;
			uint32_t m_ifaceId; 
			Ptr<NetDevice> app_netDevice;
			Ipv4Address m_mainAddress;
		    	
			Ipv4Address m_broadcast;
			Address m_broadcastMac;

						void SendResources(Ipv4Address dstIp);
			void StartDownloadTask(uint32_t idPart);
			uint32_t downloadPartId;
			Ipv4Address responseIp;

			uint32_t lastChunkRequested;
			bool isDownloadComplete();
			bool isChunkDownloaded(uint32_t chunkId);

			uint32_t countSendCompleteTask;	
			uint32_t countTaskDistribution;	

			uint32_t m_percentCollabs;
			
			void RecordLog(uint8_t TxRx, AppMessageHeader msg, uint32_t pkts, uint32_t byteSize);

			

	};

}

#endif /* APP_DESCARGA_H */

