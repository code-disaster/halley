#pragma once

#include "navmesh.h"
#include "navigation_query.h"
#include "navigation_path.h"

namespace Halley {
	class NavmeshSet {
	public:
		NavmeshSet();
		NavmeshSet(const ConfigNode& nodeData);

		ConfigNode toConfigNode() const;

		void add(Navmesh navmesh);
		void addChunk(NavmeshSet navmeshSet, Vector2f origin, Vector2i gridPosition);
		void addRaw(NavmeshSet navmeshSet);
		void clear();
		void clearSubWorld(int subWorld);

		void linkNavmeshes();
		void reportUnlinkedPortals(std::function<String(Vector2i)> getChunkName) const;

		std::optional<NavigationPath> pathfind(const NavigationQuery& query) const;
		std::optional<NavigationPath> pathfindInRegion(const NavigationQuery& query, uint16_t regionId) const;
		std::optional<NavigationPath> pathfindBetweenRegions(const NavigationQuery& queryStart, const NavigationQuery& queryEnd, uint16_t startRegionId, uint16_t endRegionId, const Navmesh::Portal& portal, NavigationQuery::PostProcessingType postProcessing) const;

		gsl::span<const Navmesh> getNavmeshes() const { return navmeshes; }
		const Navmesh* getNavMeshAt(Vector2f pos, int subWorld) const;
		size_t getNavMeshIdxAt(Vector2f pos, int subWorld) const;

		std::pair<uint16_t, uint16_t> getPortalDestination(uint16_t region, uint16_t edge) const;

	private:
		struct PortalConnection {
			uint16_t portalId;
			uint16_t regionId;
			float cost;

			PortalConnection() = default;
			PortalConnection(uint16_t portalId, uint16_t regionId, float cost)
				: portalId(portalId)
				, regionId(regionId)
				, cost(cost)
			{}
		};
		
		struct PortalNode {
			Vector2f pos;
			Vector<PortalConnection> connections;
			uint16_t fromRegion;
			uint16_t fromPortal;
			uint16_t toRegion;
			uint16_t toPortal;

			PortalNode() = default;
			PortalNode(Vector2f pos, uint16_t fromRegion, uint16_t fromPortal, uint16_t toRegion, uint16_t toPortal)
				: pos(pos), fromRegion(fromRegion), fromPortal(fromPortal), toRegion(toRegion), toPortal(toPortal)
			{}
		};

		struct RegionNode {
			Vector<uint16_t> portals;
		};

		using NodeId = uint16_t;
		using NodeAndConn = NavigationPath::RegionNode;

		struct State {
			float gScore = std::numeric_limits<float>::infinity();
			float fScore = std::numeric_limits<float>::infinity();
			NodeId cameFrom;
			bool inOpenSet = false;
			bool inClosedSet = false;
		};

		class NodeComparator {
		public:
			NodeComparator(const Vector<State>& state) : state(state) {}
			
			bool operator()(NodeId a, NodeId b) const
			{
				return state[a].fScore > state[b].fScore;
			}

		private:
			const Vector<State>& state;
		};

		Vector<Navmesh> navmeshes;
		Vector<PortalNode> portalNodes;
		Vector<RegionNode> regionNodes;

		void tryLinkNavMeshes(uint16_t idxA, uint16_t idxB);

		Vector<NavigationPath::RegionNode> findRegionPath(Vector2f startPos, Vector2f endPos, uint16_t fromRegionId, uint16_t toRegionId) const;

		Vector<Vector2f> postProcessPathBetweenRegions(
			const NavigationQuery& queryStart, const NavigationQuery& queryEnd, 
			uint16_t startRegionId, uint16_t endRegionId, const Navmesh::Portal& portal,
			Vector<Navmesh::NodeAndConn> startLeg, Vector<Navmesh::NodeAndConn> endLeg, 
			NavigationQuery::PostProcessingType type) const;
	};
}
