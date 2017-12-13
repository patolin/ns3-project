#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/ibr-waysegment-table.h"
#include "ns3/mobility-model.h"
#include <vector>
#include <boost/lexical_cast.hpp>

using namespace std;

NS_LOG_COMPONENT_DEFINE ("IbrWaysegmentTable");

namespace ns3 {
	NS_OBJECT_ENSURE_REGISTERED (IbrWaysegmentTable);
	
	IbrWaysegmentTable::IbrWaysegmentTable() {
		clearTable();
	}

	

	TypeId IbrWaysegmentTable::GetTypeId () {
  		static TypeId tid = TypeId ("ns3::IbrWaysegmentTable")
    		.SetParent<Object> ()
    		.AddConstructor<IbrWaysegmentTable> ()
    	;
  		return tid;
	}


	void IbrWaysegmentTable::clearTable() {
		waySegmentMembers.clear();
	}

	void IbrWaysegmentTable::addRegion(uint32_t region, uint32_t region_level, uint32_t waysegment, uint32_t neigPrev, uint32_t neigNext) {
		wayMembers wm;
		wm.m_region = region;
		wm.m_regionLevel = region_level;
		wm.m_wayNumber = waysegment;
		wm.m_neighbourPrev = neigPrev;
		wm.m_neighbourNext = neigNext;	
		waySegmentMembers.push_back(wm);
		wm.PrintInfo();
	}

	uint32_t IbrWaysegmentTable::getTableSize() {
		return waySegmentMembers.size();
	}

	bool IbrWaysegmentTable::isL2Region(uint32_t region, bool dir) {
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
			uint32_t neighbourRegion;

			if (ws->m_region==(int)region && ws->m_regionLevel==2) {
				if (dir==true) { 
					neighbourRegion=ws->m_neighbourNext;		
				} else {
					neighbourRegion=ws->m_neighbourPrev;		
				}
				if (IbrWaysegmentTable::getRegionLevel(neighbourRegion)==1) {
					return true;
				}
			}
		}
		
		return false;
	}

	uint32_t IbrWaysegmentTable::getRegionLevel(uint32_t region) {
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
			if (ws->m_region==(int)region ) {
				return 	ws->m_regionLevel;
			}
		}
		return UINT32_MAX;

	}

	uint32_t IbrWaysegmentTable::GetRxSourceRegion(bool dir, uint32_t ibr_region) {
		
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
			if (ws->m_region==(int)ibr_region) {
				if (dir==true) {
					return ws->m_neighbourNext;
				} else {
					return ws->m_neighbourPrev;
				}	
			}
		}
		return UINT32_MAX;
	}

	bool IbrWaysegmentTable::IsIntersectionNeighbour(uint32_t regionLocal, uint32_t region, bool dir) {
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
			if (ws->m_region == (int)regionLocal && ws->m_neighbourNext==(int)region && ws->m_neighbourPrev==-1 && dir==false) {
				return true;
			}
			if (ws->m_region == (int)regionLocal && ws->m_neighbourPrev==(int)region && ws->m_neighbourNext==-1 && dir==true) {
				return true;
			}
		}
		return false;
	}


	uint32_t IbrWaysegmentTable::getWaySegment(uint32_t region) {
		// obtiene el id del segmento de via, con el ID de la region
		// si llega a una interseccion, devuelve el primer segmento de via encontrado
		uint32_t waySeg=UINT32_MAX;
		
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
					if (ws->m_region==(int)region) {
				return ws->m_wayNumber;
			}
		}
		return waySeg;
	}
	
	bool IbrWaysegmentTable::isRegionInWaySegment(uint32_t region, uint32_t waysegment) {
		// checks if a region is inside a waysegment (special case for intersections)
		
		for (std::list<IbrWaysegmentTable::wayMembers>::iterator ws = IbrWaysegmentTable::waySegmentMembers.begin(); ws != IbrWaysegmentTable::waySegmentMembers.end(); ws++) {
			if (ws->m_region==(int)region && ws->m_wayNumber==(int)waysegment) {
				return true;
			}
		}
		return false;
	}

}


