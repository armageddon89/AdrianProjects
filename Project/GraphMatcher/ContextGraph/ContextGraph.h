#pragma once

#include "Node.h"
#include "Edge.h"
#include <boost/regex.hpp>
#include "IContextGraph.h"

class ContextGraph : public IContextGraph
{
public:
	ContextGraph() : 
		m_bAllowDuplicateEdges(true),
		m_bFixedExpireTime(false),
		m_bFixedValidityInterval(false),
		m_bQuickMatch(false),
		m_graph(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
		m_tGraph(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
                m_edges(10, [] (const CEdge &edge) { return std::hash<std::wstring>()(edge.GetLabel()); }),
		m_nodes(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
		m_oldEdges(10, [] (const CEdge &edge) { return std::hash<std::wstring>()(edge.GetLabel()); }),
		m_oldNodes(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
		m_matrix(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
                m_pathMatrix(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); }),
                m_valability(NEVER_EXPIRE),
		m_validityInterval(PERMANENT_DURATION),
		fakeNodeDeleter([] (const CNode *) { }),
		fakeEdgeDeleter([] (const CEdge *) { })
	{ }

	ContextGraph operator =(const ContextGraph &other)
	{
		ContextGraph cg;
		for (auto edge : other.m_edges)
			cg.AddEdge(edge.GetLabel(), edge.GetSource(), edge.GetDestination());

		return cg;
	}

	ContextGraph(const ContextGraph &other) : ContextGraph()
	{
		for (auto edge : other.m_edges)
			AddEdge(edge.GetLabel(), edge.GetSource(), edge.GetDestination());
	}

	void AddEdge(/* in */ const CEdge &e);
	void AddEdge(/* in */ const std::wstring &strLabel,
				 /* inout */ const CNode &source,
				 /* inout */ const CNode &destination,
				 /* in */ std::chrono::system_clock::time_point expireTime = NEVER_EXPIRE, 
				 /* in */ Duration duration = PERMANENT_DURATION);
	void AddEdge(/* in */ const std::wstring &strLabel,
				 /* in */ const std::wstring &strNode1,
				 /* in */  const std::wstring &strNode2,
				 /* in */ std::chrono::system_clock::time_point expireTime = NEVER_EXPIRE, 
				 /* in */ Duration duration = PERMANENT_DURATION);
	bool GetPathBetweenNodes(/* in */ const CNode &n1, /* in */ const CNode &n2, Paths &solutions) const;
	void ConvertNodesToUnknown (/* in */ unsigned int percentOfNodes);

	inline const TE & GetEdges(void) const
	{ return m_edges; }
	inline const TN & GetNodes(void) const
	{ return m_nodes; }
	inline void AllowDuplicateEdges(bool bAllow = true) { m_bAllowDuplicateEdges = bAllow; }
	inline void SetForcedExpireTime(/* in */ std::chrono::system_clock::time_point expireTime = NEVER_EXPIRE, /* in */ bool bIsFixedTime = false) 
	{ 
		m_valability = expireTime;
		m_bFixedExpireTime = bIsFixedTime; 
	}
	inline void SetForcedValidityInterval(/* in */ Duration interval = PERMANENT_DURATION, /* in */ bool bIsFixedInterval = false)
	{ 
		m_validityInterval = interval;
		m_bFixedValidityInterval = bIsFixedInterval; 
	}

	//it is sure that the node exists
	inline const TN::key_type & GetNodeByName(/* in */ std::wstring name) const
	{ return *m_nodes.find(name); }

	inline bool FindNodeByName(/* in */ std::wstring name, /* out */ const CNode * &foundNode) const
	{
		auto found = m_nodes.find(name);
		if (found != m_nodes.cend())
		{
			foundNode = const_cast<CNode *>(&*found);
			return true;
		}
		
		foundNode = nullptr;
		return false;
	}
	std::vector<const CNode *> FindNodesMatchingNodeName(/* in */ const CNode &node) const;
	std::wstring SerializeGraph(void) const;
	bool IsIncludedIn(/* in */ const ContextGraph &bigGraph) const;

	inline auto GetEdgesByName(/* in */ const std::wstring &strLabel) const -> std::pair<TE::const_iterator, TE::const_iterator>
	{ return m_edges.equal_range(CEdge(strLabel)); }
	inline auto GetChildren(/* in */ const CNode &node) const -> std::pair<T::const_iterator, T::const_iterator> 
	{ return m_graph.equal_range(node);	}
	inline auto GetParents(/* in */ const CNode &node) const -> std::pair<T::const_iterator, T::const_iterator>
	{ return m_tGraph.equal_range(node); }
	inline const T & GetInstanceGraph(void) const { return m_graph; }
	inline const T & GetInstanceGraphTransposed(void) const { return m_tGraph; }
	inline bool IsQuickMatch(void) const { return m_bQuickMatch; }
	inline void SetQuickMatch(bool bQuickMatch) { m_bQuickMatch = bQuickMatch; }

	std::vector<std::vector<const CEdge *>> ComputeConnexComponents(void) const;
	const CEdge * FindEdge(/* in */ const std::wstring &strLabel, /* in */ const CNode &source, /* in */ const CNode &destiation) const;

	inline bool IsAdjacent(/* in */ const CNode &n1, /* in */ const CNode &n2) const
	{
		const auto it1 = m_matrix.find(n1.GetLabel());
		if (it1 == m_matrix.cend())
			return false;
		const auto it2 = it1->second.find(n2.GetLabel());
		if (it2 == it1->second.cend())
			return false;
		return !it2->second.empty();
	}

	template <typename Granularity>
	inline long long GetTimeLeft(void) const { return std::chrono::duration_cast<Granularity>(m_valability - std::chrono::system_clock::now()).count();	}
	inline std::chrono::system_clock::time_point GetExpireTime(void) const 	{ return m_valability; }
	inline Duration GetValidityInterval(void) const { return m_validityInterval; }

	void RefreshGraphConsistency(void);

	std::vector<const CEdge *> FindMaxOriginalPathMatchedByRegex(/* in */ const std::wstring &regex, /* in */ const CNode &source, /* in */ const CNode &destination);
	PointerEdgePaths FindAllOriginalPathsMatchedByRegex(/* in */ const std::wstring &regex, /* in */ const CNode &source, /* in */ const CNode &destination);
	std::vector<const CEdge *> GetCorrespondingConcreteEdges(/* in */ const CEdge &edge, /* in */ const TN &unkNodes) const;

	std::set<const CNode *> GetLabeledNodes(void) const;
	std::set<std::set<const CEdge *>> GetMaximumMatch(/* in */ const ContextGraph &patternGraph, bool bRealTime = false, bool bMatchInThePast = false);
	std::vector<ContextGraph> GetMaximumMatchGraphs(/* in */ const ContextGraph &patternGraph, bool bRealTime = false, bool bMatchInThePast = false);
	bool BuildFromDotFile(/* in */ const std::wstring &fileName, /* in_opt */ bool bClearPrecedent = false);
	bool WriteGraphToDotFile(/* in */ const std::wstring &fileName, /* in */ const std::wstring &graphName) const;
	void Clear(void);
	void PrecomputeRoadsBetweenPairOfNodes(void);
	std::wstring GetEdgeTextRepresentation(void) const;

	template <typename ContainerMember, typename ContainerExcl>
	std::set<const CNode *> GetNeighbours(/* in */ const CNode &node, /* in_opt */ const ContainerMember &memberNodes, /* in_opt */ const ContainerExcl &excludedNodes) const;
	template <typename ContainerMember = std::vector<const CNode *>, typename ContainerExcl = std::vector<const CNode *>>
	inline std::set<const CNode *> GetNeighbours(/* in */ const CNode &node) const
	{
		return GetNeighbours(node, ContainerMember(), ContainerExcl());
	}

	void RemoveNode(/* in */ const std::wstring &node);
	void ReplaceNode(/* in */ const std::wstring &oldNode, /* in */ const std::wstring newNode);

	ContextGraph GenerateRandomSubgraph(/* in_opt */ unsigned int nodesPercent = 50,
										/* in_opt */ unsigned int edgesPercentOnSubsetOfNodes = 50,
										/* in_opt */ unsigned int percentOfUnknownNodes = 50,
										/* in_opt */ std::set<std::wstring> mandatoryNodes = std::set<std::wstring>());

	inline static bool IntervalIntersection(/* in */ Duration firstInterval, /* in */ Duration secondInterval)
	{
		return ! (firstInterval.first > secondInterval.second || secondInterval.first > firstInterval.second);
	}

	virtual void PrintAdjacencyMatrix(void) const
	{
		for (auto &&it : m_matrix)
		{
			for (auto &&it2 : it.second)
			{
				std::wcout << "[" << it.first << "->" << it2.first << "] --> ";
				for(auto &&it3 : it2.second)
					std::wcout << it3->GetLabel() << ", ";
				std::wcout << std::endl;
			}

			std::wcout << std::endl;
		}
	}

	friend std::wostream& operator<<(/* in */ std::wostream &stream, /* in */ const ContextGraph &cg)
	{
		for(auto &&edge : cg.GetEdges())
		{
			stream << edge << L"\n";
		}

		return stream;
	}

public:
        bool m_bAllowDuplicateEdges;
	bool m_bFixedExpireTime;
	bool m_bFixedValidityInterval;
	bool m_bQuickMatch;
    
	T m_graph;
	T m_tGraph;
	TE m_edges;
	TN m_nodes;
        TE m_oldEdges;
	TN m_oldNodes;
	AdjacentMatrix m_matrix;
	AccessibilityMatrix m_pathMatrix;
	RegexCache m_regexCache;
	std::chrono::system_clock::time_point m_valability;
	Duration m_validityInterval;

	std::function<void(const CNode *)> fakeNodeDeleter;
	std::function<void(const CEdge *)> fakeEdgeDeleter; 
	typedef std::unique_ptr<const CNode, decltype(fakeNodeDeleter)> CNodePtr;
	typedef std::unique_ptr<const CEdge, decltype(fakeEdgeDeleter)> CEdgePtr;
	#define CRefNodePtr(x) CNodePtr(&(x), fakeNodeDeleter)
	#define CRefEdgePtr(x) CEdgePtr(&(x), fakeEdgeDeleter)

#ifdef TESTING
public:
#else 
private:
#endif
	bool _IsRegexMatch(/* in */ const std::wstring &regex, /* in */ const std::wstring &string) const;
	bool _IsRegexMatch(/* in */ const boost::wregex &regex, /* in */ const std::wstring &string) const;
	TN _GetPossibleUnknownNodes(/* in */ const ContextGraph patternGraph) const;
	void _AddEdgeToAdjacencyMatrix(/* in */ const CEdge &e);
	void _AddCurrentPathsToCache(/* in */ const CNode &source, /* in */ const CNode &dest, /* in */ const Paths &roads);
	void _AddSingleUniquePathToCache(/* in */ const CNode &source, /* in */ const CNode &dest, /* in */ const Paths::key_type &road);
	void _ReadGraphFromDotFormat(/* in */ const std::wstring &dot);
	void _ExtractEdge(/* in */ const std::wstring &edge);
	void _ReadGraphFromDotFormatNoRegex(const std::wstring &dot);
	void _DeleteEdge(/* in */ const CEdge &edge);
	std::vector<const CEdge *> _FindRandomSpanningTree(/* in */ const std::vector<const CNode *> &nodes) const;
	void _BeforeEdgeDeletion(/* in */ const CEdge &edge);
	HAS_MEM_FUNC(find, m_hasFind)

	template <typename T, typename ToFind> 
	typename std::enable_if<m_hasFind<T>::result, bool>::type
	inline static _CallFind(/* in */ const T &container, /* in */ const ToFind &element)
	{
		return container.find(element) != container.cend();
	}

	template <typename T, typename ToFind> 
	typename std::enable_if<!m_hasFind<T>::result, bool>::type
	inline static _CallFind(/* in */ const T &container, /* in */ const ToFind &element)
	{
		return std::find(container.cbegin(), container.cend(), element) != container.cend();
	}

	template <typename Container>
	inline static void _DeleteFromContainer(/* out */ Container &container, /* in */ std::function<bool(typename Container::const_iterator)> condition)
	{
		for (auto it = container.cbegin(); it != container.cend();)
		{
			if (condition(it))
			{
				it = container.erase(it);
			}
			else
				it++;
		}
	}
};
