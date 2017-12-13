/*
 * Main code for the VNLayer / IBR / SCMA Simulation
 */

#include "ns3/core-module.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/buildings-module.h"
#include "ns3/mobility-module.h"
#include "ns3/animation-interface.h"
#include "ns3/vnr-module.h"
#include "ns3/virtual-layer-plus-module.h"
#include "src/virtual-layer-plus/model/virtual-layer-plus-header.h"
#include "ns3/internet-module.h"
#include "ns3/scma-statemachine.h"
#include "ns3/ibr-routing-protocol.h"
#include "ns3/ibr-helper.h"
#include "ns3/ibr-waysegment-table.h"

#include <list>


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

// Road Segment global list
std::list<IbrWaysegmentTable::wayMembers> waySegment;
Ptr<IbrWaysegmentTable> ibrWsTable=CreateObject<IbrWaysegmentTable> ();

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
	void ConfiguraRouting();
	void AsignaDireccionesIP();
	void IniciaVirtualLayer(Ptr<VirtualLayerPlusNetDevice> vlPlusNetDev, bool leader);
    void ConfiguraVirtualLayer();
	void ConfiguraLogs();
    void AsignaNodosLider();
	void ConfiguraTraceSinks();
	void ConfiguraIPs();
	void ConfiguraVirtualLayerUps();
	void GeneraListaRegiones();
	
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
    Time m_duration;
    Time m_interval;

	// Definition of node containers 
	NodeContainer m_nodes;
	NetDeviceContainer m_netDevices;
	NetDeviceContainer m_virtualLayerNetDevices;
	
	Ipv4InterfaceContainer m_ifCont;

	int numLideresTotal;

	infoRegion regiones[1024];
		
};

// Default values in the class constructor
SimuladorUPS::SimuladorUPS()
	: m_outTrace("~/proyecto-ups/ns-allinone-3.25/ns-3.25/scratch/traces.tr"),
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
	  m_traceFile("~/proyecto-ups/ns-allinone-3.25/ns-3.25/scratch/ns2mobility_process.tcl"),
	  m_scnLong(715),
	  m_scnWidth(715),
	  m_scnRows(13),
	  m_scnColumns(13),
	  m_scnGridX(7),
	  m_scnGridY(7),
	  m_scnRegBetIntX(1),
	  m_scnRegBetIntY(1),
	  m_lanesNum(2),
	  m_pavementWidth(10),
	  m_shoulderWidth(10),
	  m_laneWidth(3),
	  m_deltaLanes(0.1),
	  m_nodeNum(100),
	  m_streamNum(10),
	  m_hotspotNum(6),
	  m_regionsNumX(3),
      m_regionsNumY(3),	  
	  m_secondsOfDuration(300) {

	}

void SimuladorUPS::CommandSetup(int argc, char **argv) {
	// Command line parameters parser
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
	
	
    cmd.Parse(argc, argv);
    // Convert to time object
    m_interval = Seconds(m_secondsInterval);
    m_duration = Seconds(m_secondsOfDuration);

	// Building variables definition
	m_roadWidth=(m_lanesNum*m_pavementWidth)+(m_deltaLanes*(m_lanesNum-1));
	m_regionSize=m_roadWidth+(m_shoulderWidth*2.0);
	
	// Road variables definition
	m_scnLong =(m_scnGridX+1)*m_regionSize + (m_scnGridX*m_regionsNumX*m_regionSize);
	m_scnWidth=(m_scnGridY+1)*m_regionSize + (m_scnGridY*m_regionsNumY*m_regionSize);

	
	m_deltaBuildings = m_regionSize; //m_roadWidth + 2 * m_shoulderWidth;
    m_buildingsLength = (m_regionsNumX) * m_regionSize;
    m_buildingsWidth = (m_regionsNumY) * m_regionSize;
    m_buildingsOrigin = (m_regionSize);

    
}

void SimuladorUPS::ConfiguraMedioFisico() {
	// Physical layer setup
    NS_LOG_DEBUG("Instalando capa física y mac en los nodos...");

    // Physical channel configuration
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel;	
	
    // 802.11p 5.9 GHz, ITU R-1441 Loss Model
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::HybridBuildingsPropagationLossModel", "Frequency", DoubleValue(5.9e9), "CitySize", EnumValue(ns3::SmallCity), "RooftopLevel", DoubleValue(30.0), "ShadowSigmaOutdoor", DoubleValue(3.0), "ShadowSigmaIndoor", DoubleValue(4.0), "ShadowSigmaExtWalls", DoubleValue(3.0), "InternalWallLoss", DoubleValue(1.0), "Los2NlosThr", DoubleValue(110.0));

    Ptr<YansWifiChannel> channel = wifiChannel.Create();
    wifiPhy.SetChannel(channel);

    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);

    // Tx/Rx power setup
    wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-100.0));//96
	wifiPhy.Set ("TxGain", DoubleValue (5.0) ); 
    wifiPhy.Set ("RxGain", DoubleValue (5.0) ); 
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
	// Mobile nodes container creation
	NS_LOG_DEBUG("Creando contenedor de nodos moviles");
	m_nodes.Create(m_nodeNum);

	// Node mobility setup
	NS_LOG_DEBUG("Instalando modelo de mobilidad de trazas NS-2...");
	Ns2MobilityHelper ns2= Ns2MobilityHelper(m_traceFile);
	ns2.Install();
		
}

void SimuladorUPS::ConfiguraNodosLider() {
	// Leader nodes setup
	// Fixed position in this scenary
	int numNodosLiderX=m_scnGridX*(m_regionsNumX+1);
	int numNodosLiderY=m_scnGridY*(m_regionsNumY+1);
	int x;
	int y;
	int numNodosLideres=0;
	int contadorNodos=0;
	
	double streetWidth;
	double regionSize;
	double alturaNodo=1.5;

	MobilityHelper mobility;
	NodeContainer nodosLider;
	
	NS_LOG_DEBUG("Configurando nodos lideres estaticos");

	streetWidth=(m_lanesNum*m_pavementWidth)+m_deltaLanes*(m_lanesNum-1);
	regionSize=streetWidth+(m_shoulderWidth*2);

	// Calculation for the total of leader nodes

	NS_LOG_DEBUG("Creando contenedor de nodos lider");
	numNodosLideres=((numNodosLiderX+1)*(numNodosLiderY+1))-((m_regionsNumX*m_regionsNumY)*(m_scnGridY*m_scnGridX));
	nodosLider.Create(numNodosLideres);

	
	NS_LOG_DEBUG("Configurando posicion de nodos lider");

		for (y=0;y<=numNodosLiderY;y++) {
			if (y%(m_regionsNumY+1)==0) {
				// If is a street (horizontal) sets leaders in every region and intersection
				for (x=0;x<=numNodosLiderX;x++) {
					std::cout << "*" << " ";
					Ptr<ListPositionAllocator> leaderPosition = CreateObject<ListPositionAllocator>();
					leaderPosition->Add(Vector((x*regionSize+(regionSize/2)),(y*regionSize+(regionSize/2)), alturaNodo));
					mobility.SetPositionAllocator(leaderPosition);
					mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
					mobility.Install(nodosLider.Get(contadorNodos));
					
					contadorNodos++;
				}
				
			} else {
				// If not, sets leader on each street
				for (x=0;x<=numNodosLiderX;x++) {
					if (x%(m_regionsNumX+1)==0) {
						std::cout << "*" << " ";
						Ptr<ListPositionAllocator> leaderPosition = CreateObject<ListPositionAllocator>();
						//leaderPosition->Add(Vector((x*regionSize),(y*regionSize), alturaNodo));
						leaderPosition->Add(Vector((x*regionSize+(regionSize/2)),(y*regionSize+(regionSize/2)), alturaNodo));
						mobility.SetPositionAllocator(leaderPosition);
						mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
						mobility.Install(nodosLider.Get(contadorNodos));
						contadorNodos++;
					} else {
						std::cout << "  ";
					}
				}
			}
			std::cout << "\n";
		}

		std::cout << "Total de nodos lider: " << numNodosLideres << "\n" ;
		std::cout << "Nodos con posicion configurada: " << contadorNodos << "\n" ;

		numLideresTotal=numNodosLideres;
		
		// add nodes to the node container
		m_nodes.Add(nodosLider);	
		NS_LOG_DEBUG("Nodos agregados al contenedor de nodos principal");

	    BuildingsHelper::Install(m_nodes);
		BuildingsHelper::MakeMobilityModelConsistent();
		NS_LOG_DEBUG("BuildingHelper agregado al contenedor de nodos principal");
		
		
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
	// Generates a list of regions
	int numNodosLiderX=m_scnGridX*(m_regionsNumX+1);
	int numNodosLiderY=m_scnGridY*(m_regionsNumY+1);
	int x,y;
	int posRegion=0;
	
	NS_LOG_DEBUG("iniciando lista de regiones: " << numNodosLiderX << ":" << numNodosLiderY);
		
	// Road segment generation 
	int segmentNum=0;
	int segInt=0;
		posRegion=0;

	// X road segments
	for (y=0;y<=(numNodosLiderY);y++) {
		for (x=0;x<=(numNodosLiderX);x++) {
			IbrWaysegmentTable::wayMembers miembro;
					if (IsIntersection(x,y)) {
						NS_LOG_DEBUG("Int: x,y -> " << x << "," << y);
						// intersection
						miembro.m_wayNumber=segmentNum;
						miembro.m_region=posRegion;
						miembro.m_regionLevel=1;
						if (x<numNodosLiderX) {
							miembro.m_neighbourNext=-1; //posRegion+1;
						} 
						if (x==0) {
							miembro.m_neighbourNext=posRegion+1;
						}
						if (x>0) {
							miembro.m_neighbourPrev=posRegion-1;
						}
						
						waySegment.push_back(miembro);
						// add to the IbrWaysegmentTable
						ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );

						if (x>0) {
							segmentNum++;
							// if is not in the end of the road, add it to the new region
							if (x<numNodosLiderX) {
								miembro.m_wayNumber=segmentNum;
								miembro.m_neighbourNext=posRegion+1;
								miembro.m_neighbourPrev=-1;
								waySegment.push_back(miembro);
								
								// add to the IbrWaysegmentTable
								ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );


							}
						}
						
						
					
						
					} else if (IsIntersectionY(y)) {
						// horizontal road

						miembro.m_wayNumber=segmentNum;
						miembro.m_region=posRegion;
						if (segInt==0 || (m_regionsNumX-segInt)==1) {
							miembro.m_regionLevel=2;
						} else {
							miembro.m_regionLevel=3;
						}
						segInt++;
						if (segInt==3) {
							segInt=0;
						}
						if (x<numNodosLiderX) {
							miembro.m_neighbourNext=posRegion+1;
						}
						if (x>0) {
							miembro.m_neighbourPrev=posRegion-1;
						}
						
						
						waySegment.push_back(miembro);
						// add to the IbrWaysegmentTable
						ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );



						
					}

			posRegion++;
			
		}
	}
	posRegion=0;
	// Y Road Segments (same process as in X road segments) 
	for (x=0;x<=(numNodosLiderX);x++) {
		posRegion=x;
		segInt=0;
		for (y=0;y<=(numNodosLiderY);y++) {

			IbrWaysegmentTable::wayMembers miembro;
			if (IsIntersection(x,y)) {
				miembro.m_wayNumber=segmentNum;
				miembro.m_region=posRegion;
				miembro.m_regionLevel=1;
				if (y<numNodosLiderY) {
					miembro.m_neighbourNext=-1;
				}
				if (y==0) {
					miembro.m_neighbourNext=posRegion+(1+numNodosLiderX);
				}
				
				if (y>0) {
					miembro.m_neighbourPrev=posRegion-(1+numNodosLiderX);
				}
				waySegment.push_back(miembro);
				// add to the IbrWaysegmentTable
				ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );

	
				if (y>0) {
					segmentNum++;
					if (y<numNodosLiderY) {
						miembro.m_wayNumber=segmentNum;
						miembro.m_neighbourNext=posRegion+(1+numNodosLiderX);
						miembro.m_neighbourPrev=-1;
						waySegment.push_back(miembro);
						// add to the IbrWaysegmentTable
						ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );


					}
				}
	
			} else if (IsIntersectionX(x)) {
				miembro.m_wayNumber=segmentNum;
				miembro.m_region=posRegion;
				if (segInt==0 || (m_regionsNumY-segInt)==1) {
					miembro.m_regionLevel=2;
				} else {
					miembro.m_regionLevel=3;
				}
				segInt++;
				if (segInt==3) {
					segInt=0;
				}
				if (y<numNodosLiderY) {
					miembro.m_neighbourNext=posRegion+(1+numNodosLiderX);
				}
				if (y>0) {
					miembro.m_neighbourPrev=posRegion-(1+numNodosLiderX);
				}
				
				waySegment.push_back(miembro);
				// add to the IbrWaysegmentTable
				ibrWsTable->addRegion(miembro.m_region, miembro.m_regionLevel, miembro.m_wayNumber, miembro.m_neighbourPrev, miembro.m_neighbourNext );


				
			}
			
			
			posRegion+=(numNodosLiderX+1);
			
		}
		
	}
	// Print the Road Segment table	
	/*
	for (std::list<IbrRoutingProtocol::wayMembers>::iterator ws = waySegment.begin(); ws != waySegment.end(); ws++) {
		ws->PrintInfo();
	}
	*/
	
	// Print the regions grid, for debugging
	/*
	if (m_verbosity==true) {
		posRegion=0;
		for (y=0;y<=numNodosLiderY;y++) {
			for (x=0;x<=numNodosLiderX;x++) {
				if ((int)regiones[posRegion].level>0) {
					std::cout << (int)regiones[posRegion].level << " ";
				} else {
					std::cout << "-" << " ";
				}
				posRegion++;
			}
			std::cout << "\n";
			
		}
	}
	*/
	NS_LOG_DEBUG("Generada la lista de regiones");
	
}


void SimuladorUPS::ConfiguraObstaculos() {
	// Setup building obstacles
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

void SimuladorUPS::ConfiguraRouting() {
	// Setup Routing Protocol for each node
	NS_LOG_DEBUG("Instalando protocolo de enrutado IBR");
    InternetStackHelper internet;
    Ipv4ListRoutingHelper list;

  	IbrRoutingHelper ir;
	Ptr<IbrRoutingTable> ibrrt=CreateObject<IbrRoutingTable> ();
	ir.Set("RoutingTable", PointerValue (ibrrt));
	ir.Set("WaysegmentTable", PointerValue(ibrWsTable));
	
	// Initially tested with VNR Routing
	//VnrHelper vr;
	//list.Add(vr, 100);
 	// Static routing on each node
	Ipv4StaticRoutingHelper staticRouting;	
    //list.Add(staticRouting,10);
 	list.Add(ir, 100);
	
    internet.SetRoutingHelper(list);
    internet.Install(m_nodes);
	NS_LOG_DEBUG("Instalado protocolo de enrutado");
}

void SimuladorUPS::AsignaDireccionesIP() {
	// Configure IP Addresses
    Ipv4AddressHelper ipv4;
    NS_LOG_DEBUG("Asignando direcciones IP...");
    ipv4.SetBase("10.0.0.0", "255.255.248.0");
	m_ifCont = ipv4.Assign(m_netDevices);
}

void SimuladorUPS::IniciaVirtualLayer(Ptr<VirtualLayerPlusNetDevice> vlPlusNetDev, bool leader) {

    // Setup VNLayer on the nodes 

    vlPlusNetDev->SetAttribute("Long", DoubleValue(m_scnLong));
    vlPlusNetDev->SetAttribute("Width", DoubleValue(m_scnWidth));
    vlPlusNetDev->SetAttribute("Rows", UintegerValue(m_scnRows));
    vlPlusNetDev->SetAttribute("scnGridX", UintegerValue(m_scnGridX));
    vlPlusNetDev->SetAttribute("scnGridY", UintegerValue(m_scnGridY));
    vlPlusNetDev->SetAttribute("scnRegBetIntX", UintegerValue(m_regionsNumX));
    vlPlusNetDev->SetAttribute("scnRegBetIntY", UintegerValue(m_regionsNumY));

	// Setup a fixed leader node
	vlPlusNetDev->SetLeader(leader);
	// Pass the region size
	vlPlusNetDev->m_regionSize=m_regionSize;
	vlPlusNetDev->numRegionsX=m_scnGridX*(m_regionsNumX+1)+1;
	vlPlusNetDev->numRegionsY=m_scnGridY*(m_regionsNumY+1)+1;
}

void SimuladorUPS::ConfiguraVirtualLayer() {

    // Configure Virtual Layer in the containers
    VirtualLayerPlusHelper vnPlusLayer;
	vnPlusLayer.SetDeviceAttribute("WaysegmentTable", PointerValue(ibrWsTable));


    NS_LOG_DEBUG("Instalando capa virtual en los nodos...");
    m_virtualLayerNetDevices = vnPlusLayer.Install(m_netDevices);

    // mobile nodes
	NS_LOG_DEBUG("Instalando capa virtual en los nodos moviles: " << m_nodeNum);
    for (int i = 0; i < m_nodeNum; i++) {
        IniciaVirtualLayer(dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_virtualLayerNetDevices.Get(i))), false);
    }

    // fixed leader nodes
	NS_LOG_DEBUG("Instalando capa virtual en los nodos lider: " << numLideresTotal);
    for (int i=m_nodeNum; i<(m_nodeNum+numLideresTotal); i++ ) {
        IniciaVirtualLayer(dynamic_cast<VirtualLayerPlusNetDevice*> (PeekPointer(m_virtualLayerNetDevices.Get(i))), true);
    }


}


void SimuladorUPS::CambioRegion(std::ostream *os, std::string cntxt, uint32_t oldValue, uint32_t newValue) {
	// Region change detection (unused, defined on VNLayer module)
    std::string nodeId;
    std::size_t first = cntxt.find('/', 1);
    first++;
    std::size_t second = cntxt.find('/', first);
    nodeId = cntxt.substr(first, second - first);
}

void SimuladorUPS::ConfiguraTraceSinks() {
	// Configuration of trace sinks (unused)
    NS_LOG_DEBUG("Configurando sumideros de eventos de interés...");
    // Configure callback for logging
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::VirtualLayerPlusNetDevice/Region",
            MakeBoundCallback(&SimuladorUPS::CambioRegion, &m_os));
    
}


void SimuladorUPS::ConfiguraLogs() {
	// Configure Logging
    NS_LOG_DEBUG("Configurando nivel de los mensajes LOG...");

    if (m_verbosity) {
		//LogComponentEnable("SimuladorUPS", LOG_LEVEL_DEBUG);	
        LogComponentEnable("VirtualLayerPlusNetDevice", LOG_LEVEL_DEBUG);
		LogComponentEnable("ScmaStateMachine", LOG_LEVEL_DEBUG);
		LogComponentEnable("IbrRoutingProtocol", LOG_LEVEL_INFO);
		//LogComponentEnable("IbrWaysegmentTable", LOG_LEVEL_DEBUG);
		//LogComponentEnable("IbrHostListTrailer", LOG_LEVEL_DEBUG);
		//LogComponentEnable("Packet", LOG_LEVEL_FUNCTION);
		//LogComponentEnable("Buffer", LOG_LEVEL_DEBUG);
        //LogComponentEnable("VirtualLayerPlusPayloadHeader", LOG_LEVEL_DEBUG);
        //LogComponentEnable("Address", LOG_LEVEL_FUNCTION);
		//LogComponentEnable("VirtualLayerPlusNetDevice", LOG_LEVEL_INFO);
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

void SimuladorUPS::ConfiguraIPs() {

    Ipv4AddressHelper ipv4;
    NS_LOG_DEBUG("Asignando direcciones IP...");
    ipv4.SetBase("10.0.0.0", "255.255.248.0");
    m_ifCont = ipv4.Assign(m_virtualLayerNetDevices);    // para los 
   
}

int main(int argc, char *argv[]) {
	// Main simulation routine		
	std::cout << "Iniciando configuracion\n";
	// Simulation class definition
    SimuladorUPS experimento;
	// Command line parameters
    experimento.CommandSetup(argc, argv);
	// Logging Setup
	experimento.ConfiguraLogs();
	// Region List generation
	experimento.GeneraListaRegiones();
	// Obstacles (buildings) configuration
	experimento.ConfiguraObstaculos();
	// Mobile Nodes configuration
	experimento.ConfiguraNodos();
	// Fixed leader nodes configuration
	experimento.ConfiguraNodosLider();
	// Physical channel configuration
	experimento.ConfiguraMedioFisico();
	// Virtual layer installation on nodes
	experimento.ConfiguraVirtualLayer();
	// Routing setup in nodes	
	experimento.ConfiguraRouting();
	// IP Addresses setup
	experimento.ConfiguraIPs();
	// Trace Sinks	
    experimento.ConfiguraTraceSinks();
	
	// Set resolution of simulation to ns
	Time::SetResolution(Time::NS);
	
	// Simulation start
	std::cout << "Iniciando simulacion!\n";
    experimento.Run();
    
	// Simulation end
	std::cout << "Finalizando la simulacion!\n";
	m_os.close();
	std::cout << "Cerrado el log de salida!\n";
}
