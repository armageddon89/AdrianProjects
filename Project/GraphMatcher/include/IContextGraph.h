#pragma once

class IContextGraph
{
public:
	typedef std::unordered_multimap<CNode, const CEdge *, std::function<size_t(const CNode &)>> T;
	typedef std::unordered_multiset<CEdge, std::function<size_t(const CEdge &)>> TE;
	typedef std::unordered_set<CNode, std::function<size_t(const CNode &)>> TN;

	typedef std::vector<const CEdge *> AdjacentEdges;
	typedef std::unordered_map<CNode, AdjacentEdges, std::function<size_t(const CNode &)>> Row;
	typedef std::unordered_map<CNode, Row, std::function<size_t(const CNode &)>> AdjacentMatrix;
	typedef std::unordered_multimap<std::wstring, std::tuple<const CNode *, const CNode *, std::vector<const CEdge *>>> RegexCache;

public:

	//ordered by size of the path
	typedef std::multiset<std::vector<CNode const *>, std::function<bool(const std::vector<CNode const *> &, const std::vector<CNode const *> &)>> Paths;
	typedef std::set<std::vector<const CEdge *>, std::function<bool(const std::vector<const CEdge *> &, const std::vector<const CEdge *> &)>> PointerEdgePaths;
	typedef std::unordered_map<CNode, Paths, std::function<size_t(const CNode &)>> PathsRow;
	typedef std::unordered_map<CNode, PathsRow, std::function<size_t(const CNode &)>> AccessibilityMatrix;
};