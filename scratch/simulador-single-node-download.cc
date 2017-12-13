// Simulation for individual download on each node 
// to get comparison data

#include "ns3/core-module.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/buildings-module.h"
#include "ns3/mobility-module.h"
#include "ns3/animation-interface.h"
#include "ns3/virtual-layer-plus-module.h"
#include "src/virtual-layer-plus/model/virtual-layer-plus-header.h"
#include "ns3/internet-module.h"
#include "ns3/ibr-waysegment-table.h"

#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"

#include "ns3/app-descarga.h"

#include <list>

#include "ns3/aodv-module.h" 
using namespace ns3;

struct rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};



struct rgb colors [] = {
                        { 255, 0, 0 }, // Red
                        { 0, 255, 0 }, // Blue
                        { 0, 0, 255 }  // Green
                        };


struct infoRegion {
	uint32_t num;
	uint32_t x;
	uint32_t y;
	uint32_t level;
	
	void PrintInfo()
        {

			std::cout << "Num: " << (int)num << "\tX: " << x << "\tY: " << y << "\tLevel: " << (int)level << "\n";
        }
};


//IbrWaysegmentTable ibrWsTable;
Ptr<IbrWaysegmentTable> ibrWsTable=CreateObject<IbrWaysegmentTable> ();

//IbrHostsTable for VnLayer
Ptr<IbrHostsTable> ibrHostsTable=CreateObject<IbrHostsTable>();


AnimationInterface *pAnim = 0;

std::ofstream m_os;

NS_LOG_COMPONENT_DEFINE("SimuladorUPS");


class SimuladorUPS {
public:
	SimuladorUPS();
	void Run();
	void CommandSetup(int argc, char** argv);
	void ConfiguraMedioFisico();
	void ConfiguraNodos();
	void ConfiguraNodosLider();
	void ConfiguraObstaculos();
	void IniciaVirtualLayer(Ptr<VirtualLayerPlusNetDevice> vlPlusNetDev, bool leader);
	void ConfiguraLogs();
    void AsignaNodosLider();
	void ConfiguraTraceSinks();
	void ConfiguraVirtualLayerUps();
	void GeneraListaRegiones();
	void ConfiguraAplicacion();

	void ConfigNodos();

private:
	bool IsIntersection(uint32_t regX, uint32_t regY);
	bool IsIntersectionY(uint32_t regY);
	bool IsIntersectionX(uint32_t regX);
	static void CambioRegion(std::ostream *os, std::string cntxt, uint32_t oldValue, uint32_t newValue);
	
	
	
    std::string m_outTrace;
	
	
	uint32_t m_bytesRx;
	
    uint32_t m_packetsRx;
    uint32_t m_bytesTx;
    uint32_t m_packetsTx;

    uint32_t m_vnBytesTx;
    uint32_t m_vnPacketsTx;

    std::string m_phyMode;
    char m_powerClass; // Class A (0 dBm), Class B (10 dBm), Class C (20 dBm), Class D (28.8 dBm)

    uint32_t m_packetSize; // bytes
    uint32_t m_numPackets;
    double m_secondsInterval; // seconds

    bool m_verbosity;

    std::string m_traceFile;

    //AvailabilityComputer m_avComp;
    double m_availability;

    double m_scnLong; // The long of the simulation scenario
    double m_scnWidth; // The width of the simulation scenario
    uint32_t m_scnRows; // The number of rows of the simulation scenario
    uint32_t m_scnColumns; // The number of columns of the simulation scenario
    uint32_t m_scnGridX; // Manhatann X grid value
    uint32_t m_scnGridY; // Manhatann Y grid value
    uint32_t m_scnRegBetIntX; // Manhatann regions between intersections X value
    uint32_t m_scnRegBetIntY; // Manhatann regions between intersections Y value
    uint32_t m_lanesNum; // Number of lanes in each direction per road
    double m_pavementWidth; // Width of the pavement
    double m_regionSize; // Size of each virtual region
    double m_deltaBuildings; // Gap between buildings
    double m_shoulderWidth; // Width of the hard shoulder
    double m_laneWidth; // Width of lane
    double m_deltaLanes; // Gap between lanes
    double m_buildingsLength; // Length of buildings
    double m_buildingsWidth; // Width of buildings
    double m_buildingsOrigin; // Xo (=Yo) of the first building
    double m_roadWidth; // Total width of the road
	

    int m_nodeNum;
	
	uint32_t m_streamNum;
    uint32_t m_hotspotNum;
	uint32_t m_regionsNumX; // numero de regiones por segmento de via
	uint32_t m_regionsNumY;
    std::list<uint32_t> m_regionsHotspots;

    uint32_t m_secondsOfDuration;
	uint32_t m_percentCollabs;	// percentage of collaborator nodes for SCMA (0/100)	
    Time m_duration;
    Time m_interval;
public:
	NodeContainer m_nodes;
	NetDeviceContainer m_netDevices;
	NetDeviceContainer m_virtualLayerNetDevices;
	
	Ipv4InterfaceContainer m_ifCont;
	Ipv4InterfaceContainer m_ifContVn;
	int numLideresTotal;

	infoRegion regiones[1024];
	
};


SimuladorUPS::SimuladorUPS()
	: m_outTrace("~/simulador-ups-aodv/ns-allinone-3.26/ns-3.26/scratch/traces.tr"),
	  m_bytesRx(0),
	  m_packetsRx(0),
	  m_bytesTx(0),
	  m_packetsTx(0),
	  m_vnBytesTx(0),
	  m_vnPacketsTx(0),
	  m_phyMode("OfdmRate6MbpsBW10MHz"),
	  m_powerClass('A'),
	  m_packetSize(1000),
	  m_numPackets(100000),
	  m_secondsInterval(0.01),
	  m_verbosity(true),
	  m_traceFile("~/simulador-ups-aodv/ns-allinone-3.26/ns-3.26/scratch/ns2mobility_process_0.tcl"),
	  m_scnLong(715),
	  m_scnWidth(715),
	  m_scnRows(13),
	  m_scnColumns(13),
	  m_scnGridX(8),
	  m_scnGridY(8),
	  m_scnRegBetIntX(1),
	  m_scnRegBetIntY(1),
	  m_lanesNum(2),
	  m_pavementWidth(10),
	  m_shoulderWidth(10),
	  m_laneWidth(3),
	  m_deltaLanes(0.1),
	  m_nodeNum(300),
	  m_streamNum(10),
	  m_hotspotNum(6),
	  m_regionsNumX(3),
      m_regionsNumY(3),	  
	  m_secondsOfDuration(300),
	  m_percentCollabs(75) {

	}

void SimuladorUPS::CommandSetup(int argc, char **argv) {

    CommandLine cmd;

    cmd.AddValue("phyMode", "Wifi Phy mode", m_phyMode);
    cmd.AddValue("powerClass", "802.11p Power class (A, B, C or D)", m_powerClass);
    cmd.AddValue("streamNum", "Number of streams", m_streamNum);
    cmd.AddValue("hotspotNum", "Number of Wifi Hotspots access points", m_hotspotNum);
    cmd.AddValue("packetSize", "Size of application packet sent by the streams", m_packetSize);
    cmd.AddValue("numPackets", "Number of packets generated by each stream", m_numPackets);
    cmd.AddValue("interval", "Interval between packets sent by each stream [s]", m_secondsInterval);
    cmd.AddValue("verbose", "Turn on log messages", m_verbosity);
    cmd.AddValue("traceFile", "Ns2 movement trace file", m_traceFile);
    cmd.AddValue("nodeNum", "Number of nodes", m_nodeNum);
    cmd.AddValue("duration", "Duration of Simulation [s]", m_secondsOfDuration);
    cmd.AddValue("scnGridX", "X grid value of the Manhatann simulation scenario", m_scnGridX);
    cmd.AddValue("scnGridY", "Y grid value of the Manhatann simulation scenario", m_scnGridY);
    cmd.AddValue("scnRegBetIntX", "Regions between intersections X value of the Manhatann simulation scenario", m_scnRegBetIntX);
    cmd.AddValue("scnRegBetIntY", "Regions between intersections Y value of the Manhatann simulation scenario", m_scnRegBetIntY);
    cmd.AddValue("lanesNum", "Number of lanes in each direction", m_lanesNum);
    cmd.AddValue("pavementWidth", "Width of the pavement [m]", m_pavementWidth);
    cmd.AddValue("shoulderWidth", "Width of the hard shoulder [m]", m_shoulderWidth);
    
    cmd.AddValue("deltaLanes", "Gap between lanes [m]", m_deltaLanes);
	cmd.AddValue("regionsX", "Regions X", m_regionsNumX);
	cmd.AddValue("regionsY", "Regions Y", m_regionsNumY);
	cmd.AddValue("collabs", "APP Collaborators Percentage", m_percentCollabs);

	
    cmd.Parse(argc, argv);
    m_interval = Seconds(m_secondsInterval);
    m_duration = Seconds(m_secondsOfDuration);
	m_roadWidth=(m_lanesNum*m_pavementWidth)+(m_deltaLanes*(m_lanesNum-1));
	m_regionSize=m_roadWidth+(m_shoulderWidth*2.0);
	m_scnLong =(m_scnGridX+1)*m_regionSize + (m_scnGridX*m_regionsNumX*m_regionSize);
	m_scnWidth=(m_scnGridY+1)*m_regionSize + (m_scnGridY*m_regionsNumY*m_regionSize);
	
	m_deltaBuildings = m_regionSize; //m_roadWidth + 2 * m_shoulderWidth;
    m_buildingsLength = (m_regionsNumX) * m_regionSize;
    m_buildingsWidth = (m_regionsNumY) * m_regionSize;
    m_buildingsOrigin = (m_regionSize);

    
}

void SimuladorUPS::ConfiguraMedioFisico() {

    NS_LOG_DEBUG("Instalando capa física y mac en los nodos...");

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel;
	
    // 802.11p 5.9 GHz, ITU R-1441 Loss Model
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::HybridBuildingsPropagationLossModel", "Frequency", DoubleValue(5.9e9), "CitySize", EnumValue(ns3::SmallCity), "RooftopLevel", DoubleValue(30.0), "ShadowSigmaOutdoor", DoubleValue(3.0), "ShadowSigmaIndoor", DoubleValue(4.0), "ShadowSigmaExtWalls", DoubleValue(3.0), "InternalWallLoss", DoubleValue(1.0), "Los2NlosThr", DoubleValue(110.0));

    Ptr<YansWifiChannel> channel = wifiChannel.Create();
    wifiPhy.SetChannel(channel);

    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);

    wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-96.0));//96
	wifiPhy.Set ("TxGain", DoubleValue (1.0) ); 
    wifiPhy.Set ("RxGain", DoubleValue (1.0) ); 
    switch (m_powerClass) {

        case 'A':
            wifiPhy.Set("TxPowerStart", DoubleValue(0.0));
            wifiPhy.Set("TxPowerEnd", DoubleValue(0.0));
            break;
        case 'B':
            wifiPhy.Set("TxPowerStart", DoubleValue(10.0));
            wifiPhy.Set("TxPowerEnd", DoubleValue(10.0));
            break;
        case 'C':
            wifiPhy.Set("TxPowerStart", DoubleValue(20.0));
            wifiPhy.Set("TxPowerEnd", DoubleValue(20.0));
            break;
        case 'D':
            wifiPhy.Set("TxPowerStart", DoubleValue(28.8));
            wifiPhy.Set("TxPowerEnd", DoubleValue(28.8));

            break;

    }
	
    NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default();
    Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default();
	
    wifi80211p.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(m_phyMode),
            "ControlMode", StringValue(m_phyMode));
    m_netDevices = wifi80211p.Install(wifiPhy, wifi80211pMac, m_nodes);

	NS_LOG_DEBUG("ConfiguiraMedioFisico OK.");

}


void SimuladorUPS::ConfiguraNodos() {

	NS_LOG_DEBUG("Creando contenedor de nodos moviles: " << m_nodeNum);
	m_nodes.Create(m_nodeNum);

	NS_LOG_DEBUG("Instalando modelo de mobilidad de trazas NS-2...");
	Ns2MobilityHelper ns2= Ns2MobilityHelper(m_traceFile);
	ns2.Install();
	
	InternetStackHelper internet;
    Ipv4ListRoutingHelper list;
 	Ipv4StaticRoutingHelper staticRouting;	

	AodvHelper aodv;
	aodv.Set("IbrHostsTable", PointerValue(ibrHostsTable));
	aodv.Set("EnableHello", BooleanValue(false));
	aodv.Set("ActiveRouteTimeout", TimeValue(Seconds(5.0)));
	aodv.Set("TtlStart", UintegerValue(2));
   	list.Add(aodv, 0);	
	list.Add(staticRouting, -1);


    internet.SetRoutingHelper(list);
    internet.Install(m_nodes);
	NS_LOG_DEBUG("Instalado protocolo de enrutado estatico en nodos moviles");
	
}

void SimuladorUPS::ConfigNodos() {
	int numNodosLideres=0;
	int contadorNodos=0;
	
	MobilityHelper mobility;
	NodeContainer nodosLider;
	NodeContainer nodosMoviles;

    BuildingsHelper::Install(m_nodes);
	BuildingsHelper::MakeMobilityModelConsistent();
	NS_LOG_DEBUG("BuildingHelper agregado al contenedor de nodos principal");	

	
	ConfiguraMedioFisico();
	
	VirtualLayerPlusHelper vnPlusLayer;
	vnPlusLayer.SetDeviceAttribute("WaysegmentTable", PointerValue(ibrWsTable));
	vnPlusLayer.SetDeviceAttribute("IbrHostsTable", PointerValue(ibrHostsTable));
	
    NS_LOG_DEBUG("Instalando capa virtual en los nodos...");
    m_virtualLayerNetDevices = vnPlusLayer.Install(m_netDevices);
	
	NS_LOG_DEBUG("Instalando capa virtual en los nodos moviles: " << m_nodeNum);
    for (int i=0; i<(m_nodeNum); i++ ) {
        IniciaVirtualLayer(dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_virtualLayerNetDevices.Get(i))), false);
    }

	
	NS_LOG_DEBUG("Instalando capa virtual en los nodos lider: " << numLideresTotal);
    for (int i=m_nodeNum; i<(numNodosLideres+m_nodeNum); i++ ) {
        IniciaVirtualLayer(dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_virtualLayerNetDevices.Get(i))), true);
    }
	

    Ipv4AddressHelper ipv4;
    NS_LOG_DEBUG("Asignando direcciones IP Lider");
    ipv4.SetBase("10.0.0.0", "255.0.0.0");
	m_ifCont = ipv4.Assign(m_netDevices);     
	
	ipv4.SetBase("20.0.0.0", "255.0.0.0");
	m_ifCont.Add(ipv4.Assign(m_virtualLayerNetDevices));
  	
	ibrHostsTable->CreateTable(m_nodeNum+contadorNodos);

}



bool SimuladorUPS::IsIntersection(uint32_t regX, uint32_t regY) {
	if ((uint32_t)regX%(m_regionsNumX+1)==0 && (uint32_t)regY%(m_regionsNumY+1)==0) {
		return true;
	} else {
		return false;
	}
}

bool SimuladorUPS::IsIntersectionY(uint32_t regY) {
	if ( (uint32_t)regY%(m_regionsNumY+1)==0) {
		return true;
	} else {
		return false;
	}
}

bool SimuladorUPS::IsIntersectionX(uint32_t regX) {
	if ( (uint32_t)regX%(m_regionsNumX+1)==0) {
		return true;
	} else {
		return false;
	}
}

void SimuladorUPS::GeneraListaRegiones() {
	int numNodosLiderX=m_scnGridX*(m_regionsNumX+1);
	int numNodosLiderY=m_scnGridY*(m_regionsNumY+1);
	
	NS_LOG_DEBUG("iniciando lista de regiones: " << numNodosLiderX << ":" << numNodosLiderY);
	NS_LOG_DEBUG("Generada la lista de regiones");
	
}


void SimuladorUPS::ConfiguraObstaculos() {

    NS_LOG_DEBUG("Creando y posicionando los obstáculos...");

    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
    gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(m_scnGridX ));
    gridBuildingAllocator->SetAttribute("MinX", DoubleValue(m_buildingsOrigin));
    gridBuildingAllocator->SetAttribute("MinY", DoubleValue(m_buildingsOrigin));
    gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(m_buildingsLength));
    gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(m_buildingsWidth));
    gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(m_deltaBuildings));
    gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(m_deltaBuildings));
    gridBuildingAllocator->SetAttribute("Height", DoubleValue(50));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(7));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(7));
    gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(5));
    gridBuildingAllocator->SetBuildingAttribute("Type", EnumValue(Building::Office));
    gridBuildingAllocator->SetBuildingAttribute("ExternalWallsType", EnumValue(Building::ConcreteWithWindows));
    gridBuildingAllocator->Create((m_scnGridX)*(m_scnGridY));

}

void SimuladorUPS::IniciaVirtualLayer(Ptr<VirtualLayerPlusNetDevice> vlPlusNetDev, bool leader) {

    vlPlusNetDev->SetAttribute("Long", DoubleValue(m_scnLong));
    vlPlusNetDev->SetAttribute("Width", DoubleValue(m_scnWidth));
    vlPlusNetDev->SetAttribute("Rows", UintegerValue(m_scnRows));
    vlPlusNetDev->SetAttribute("scnGridX", UintegerValue(m_scnGridX));
    vlPlusNetDev->SetAttribute("scnGridY", UintegerValue(m_scnGridY));
    vlPlusNetDev->SetAttribute("scnRegBetIntX", UintegerValue(m_regionsNumX));
    vlPlusNetDev->SetAttribute("scnRegBetIntY", UintegerValue(m_regionsNumY));
	vlPlusNetDev->SetAttribute("PercentageCollabs", UintegerValue(m_percentCollabs));

	vlPlusNetDev->SetLeader(leader);
	vlPlusNetDev->m_regionSize=m_regionSize;
	vlPlusNetDev->numRegionsX=m_scnGridX*(m_regionsNumX+1)+1;
	vlPlusNetDev->numRegionsY=m_scnGridY*(m_regionsNumY+1)+1;
}



void SimuladorUPS::CambioRegion(std::ostream *os, std::string cntxt, uint32_t oldValue, uint32_t newValue) {

    std::string nodeId;

    std::size_t first = cntxt.find('/', 1);
    first++;
    std::size_t second = cntxt.find('/', first);

    nodeId = cntxt.substr(first, second - first);

}

void SimuladorUPS::ConfiguraTraceSinks() {

    NS_LOG_DEBUG("Configurando sumideros de eventos de interés...");

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::VirtualLayerPlusNetDevice/Region",
            MakeBoundCallback(&SimuladorUPS::CambioRegion, &m_os));
    
}


void SimuladorUPS::ConfiguraLogs() {

    NS_LOG_DEBUG("Configurando nivel de los mensajes LOG...");

    if (m_verbosity) {
		LogComponentEnable("SimuladorUPS", LOG_LEVEL_INFO);	
		LogComponentEnable("AppDescarga", LOG_LEVEL_DEBUG);	
       	//LogComponentEnable("VirtualLayerPlusNetDevice", LOG_LEVEL_DEBUG);
		LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_WARN);
		LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_DEBUG);
		//LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_FUNCTION);
		//LogComponentEnable("Packet", LOG_LEVEL_FUNCTION);
		//LogComponentEnable("Buffer", LOG_LEVEL_DEBUG);
        //LogComponentEnable("Address", LOG_LEVEL_FUNCTION);
        //LogComponentEnable("VirtualNetworkRoutingProtocol", LOG_LEVEL_INFO);
        //LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_INFO);
    }

	NS_LOG_DEBUG("Abriendo fichero de salida de trazas...");
    // open trace file for output
    m_os.open(m_outTrace.c_str());
}

void SimuladorUPS::Run() {
	std::cout << "Duracion: " << m_duration << "\n";
	Simulator::Stop(m_duration);
    Simulator::Run();
    m_duration = Simulator::Now();
    Simulator::Destroy();
}

void SimuladorUPS::ConfiguraAplicacion() {
	NS_LOG_DEBUG("Instalando aplicacion en nodos, con un porcentaje de colaboradores del " << m_percentCollabs << "%");

	for (int i=0;i<m_nodeNum;i++) {	
		Ptr<AppDescarga> clientApp = CreateObject<AppDescarga>();
		clientApp->SetAttribute("PercentageCollabs", UintegerValue(m_percentCollabs));
		m_nodes.Get(i)->AddApplication( clientApp );
		m_nodes.Get(i)->GetApplication(0)->SetStartTime(Seconds(1.0));
		m_nodes.Get(i)->GetApplication(0)->SetStopTime(m_duration);
	}
	


}

int main(int argc, char *argv[]) {
		
	std::cout << "Iniciando configuracion\n";

    SimuladorUPS experimento;
	
    experimento.CommandSetup(argc, argv);
	experimento.ConfiguraLogs();
	experimento.GeneraListaRegiones();
	
	experimento.ConfiguraNodos();	
		
	Time::SetResolution(Time::NS);

	experimento.ConfigNodos();

	experimento.ConfiguraAplicacion();
	std::cout << "Iniciando simulacion!\n";
    experimento.Run();

	std::cout << "Finalizando la simulacion!\n";
	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");
	m_os.close();
	std::cout << "Cerrado el log de salida!\n";
}
