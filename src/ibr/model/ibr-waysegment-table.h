// ibr waysegment table

#ifndef IBR_WAYSEGMENT_TABLE_H
#define IBR_WAYSEGMENT_TABLE_H

#include "ns3/node.h"
#include "ns3/ipv4-route.h"
#include "ns3/output-stream-wrapper.h"
#include <list>

namespace ns3 {

	class IbrWaysegmentTable : public Object {
		private:

		public:

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
		
			std::list<wayMembers> waySegmentMembers;

			IbrWaysegmentTable();
			//virtual ~IbrWaysegmentTable();
			
			static TypeId GetTypeId ();
			
			void clearTable();
			
			void addRegion(uint32_t region, uint32_t region_level, uint32_t waysegment, uint32_t neigPrev, uint32_t neigNext);

			uint32_t getTableSize();
		    bool isL2Region(uint32_t region, bool dir);
			uint32_t getRegionLevel(uint32_t region);	
			uint32_t GetRxSourceRegion(bool dir, uint32_t ibr_region);	
			bool IsIntersectionNeighbour(uint32_t regionLocal, uint32_t region, bool dir);	
			uint32_t getWaySegment(uint32_t region);
			bool isRegionInWaySegment(uint32_t region, uint32_t waysegment);
			//bool IsPacketFromIntersectionNeighbour(uint32_t regionLocal, uint32_t region, bool dir);
	};
			}



#endif
