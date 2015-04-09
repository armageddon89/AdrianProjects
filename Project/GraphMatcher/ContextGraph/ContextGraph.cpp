#include "CommonTypes.h"
#include "ContextGraph.h"

void ContextGraph::AddEdge(/* in */ const std::wstring &strLabel, 
						   /* in */ const std::wstring &strNode1,
						   /* in */ const std::wstring &strNode2,
						   /* in */ std::chrono::system_clock::time_point expireTime,
						   /* in */ Duration duration)
{
	CNode n1(strNode1);
	CNode n2(strNode2);

	AddEdge(strLabel, n1, n2, expireTime, duration);
}

void ContextGraph::AddEdge(/* in */ const std::wstring &strLabel,
                           /* in */ const CNode &source,
						   /* in */ const CNode &destination,
						   /* in */ std::chrono::system_clock::time_point expireTime,
						   /* in */ Duration duration)
{
	const auto its = m_nodes.emplace(source);
	const auto itd = m_nodes.emplace(destination);

	CEdge e(strLabel, *its.first, *itd.first, expireTime, duration);
	AddEdge(e);
}

void ContextGraph::_AddEdgeToAdjacencyMatrix(/* in */ const CEdge &e)
{
	const auto it = m_matrix.find(e.GetSource());
	if (it != m_matrix.cend())
	{
		it->second[e.GetDestination()].emplace_back(&e);
	}
	else
	{
		AdjacentEdges edges;
		edges.emplace_back(&e);
		Row rowAdjacent(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); });
		rowAdjacent[e.GetDestination()] = std::move(edges);
		m_matrix[e.GetSource()] = std::move(rowAdjacent);
	}
}

void ContextGraph::AddEdge(/* in */ const CEdge &e)
{
	if (!m_bFixedExpireTime)
	{
		auto expireTime = e.GetLastExpirationTime();
		if (expireTime < m_valability)
			m_valability = expireTime;
	}

	if (!m_bFixedValidityInterval)
	{
		auto edgeInterval = e.GetDuration();
		if (edgeInterval.first > m_validityInterval.first)
			m_validityInterval.first = edgeInterval.first;
		if (edgeInterval.second < m_validityInterval.second)
			m_validityInterval.second = edgeInterval.second;
	}

	if (!m_bAllowDuplicateEdges)
	{
		auto found = m_edges.find(e);
		if (found != m_edges.cend())
		{
			const_cast<CEdge &>(*found).AddExpirationTime(e.GetLastExpirationTime());
			return;
		}
	}

	const auto edgeLocation = m_edges.insert(e);

	m_graph.emplace(e.GetSource(), &*edgeLocation);
	m_tGraph.emplace(e.GetDestination(), &*edgeLocation);

	_AddEdgeToAdjacencyMatrix(*edgeLocation);
}

bool ContextGraph::GetPathBetweenNodes(/* in */ const CNode &n1, /* in */ const CNode &n2, Paths &solutions) const
{
	std::unordered_set<CEdgePtr> visitedEdges;

	typedef std::vector<CNodePtr> History;
	typedef std::function<void(const CNode &, const CNode &, /* inout */ History &)> RecurringFn;

	History path;

	RecurringFn findPath = [&] (const CNode &n1, const CNode &n2, /* inout */ History &previous)
	{
		std::unordered_set<CNodePtr> unique_destination;

		const auto foundSource = m_pathMatrix.find(n1);
		if (foundSource != m_pathMatrix.cend())
		{
			const auto foundDest = foundSource->second.find(n2);
			if (foundDest != foundSource->second.cend())
			{
				for (const auto &path : foundDest->second)
				{
					Paths::key_type solution;
					for (const auto &node : previous)
					{
						solution.emplace_back(node.get());
					}
					solution.insert(solution.end(), path.cbegin() + 1, path.cend());
					unique_destination.emplace(CRefNodePtr(**(path.cbegin() + 1)));

					solutions.emplace(solution);
				}
			}
		}

		auto &&children = GetChildren(n1);

		for (auto &&it = children.first; it != children.second; it++)
		{
			const CNode &destination = it->second->GetDestination();

			if (unique_destination.cend() != unique_destination.find(CRefNodePtr(destination)))
				continue;

			unique_destination.emplace(CRefNodePtr(destination));

			if (visitedEdges.cend() == visitedEdges.find(CRefEdgePtr(*it->second)))
			{
				if (&destination == &n2)
				{
					Paths::key_type solution;
					for (const auto &it : previous)
					{
						solution.emplace_back(it.get());
					}
					solution.emplace_back(&n2);

					solutions.emplace(solution);
				}

				visitedEdges.emplace(CRefEdgePtr(*it->second));

				previous.emplace_back(CRefNodePtr(destination));
				findPath(destination, n2, previous);
				previous.pop_back();

				visitedEdges.erase(CRefEdgePtr(*it->second));
			}
		}
	};

	auto it = m_nodes.find(n1);
	if (it != m_nodes.cend())
	{
		const CNode &firstNode = *m_nodes.find(n1);
		path.emplace_back(CRefNodePtr(firstNode));
		findPath(n1, n2, path);
	}

	if (solutions.empty())
		return false;

	return true;
}

bool ContextGraph::_IsRegexMatch(/* in */ const std::wstring &regex, /* in */ const std::wstring &string) const
{
	return boost::regex_match(string, boost::wregex(regex));
}

bool ContextGraph::_IsRegexMatch(/* in */ const boost::wregex &regex, /* in */ const std::wstring &string) const
{
	return boost::regex_match(string, regex);
}

void ContextGraph::_AddCurrentPathsToCache(/* in */ const CNode &source, /* in */ const CNode &dest, /* in */ const Paths &roads)
{
	const auto it = m_pathMatrix.find(source);
	if (it != m_pathMatrix.cend())
	{
		it->second[dest] = std::move(roads);
	}
	else
	{
		PathsRow rowAdjacent(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); });
		rowAdjacent[dest] = roads;
		m_pathMatrix[source] = std::move(rowAdjacent);
	}
}

void ContextGraph::_AddSingleUniquePathToCache(/* in */ const CNode &source, /* in */ const CNode &dest, /* in */ const Paths::key_type &road)
{
	static auto pathComparator = [] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); };

	const auto foundSource = m_pathMatrix.find(source);
	if (foundSource != m_pathMatrix.cend())
	{
		const auto foundDest = foundSource->second.find(dest);
		if (foundDest != foundSource->second.cend())
			foundDest->second.emplace(road);
		else
		{
			Paths paths(pathComparator);
			paths.emplace(road);
			foundSource->second[dest] = paths;
		}
	}
	else
	{
		PathsRow rowAdjacent(10, [] (const CNode &node) { return std::hash<std::wstring>()(node.GetLabel()); });
		Paths paths(pathComparator);
		paths.emplace(road);
		rowAdjacent[dest] = paths;
		m_pathMatrix[source] = std::move(rowAdjacent);
	}
}

std::vector<const CEdge *> ContextGraph::FindMaxOriginalPathMatchedByRegex(/* in */ const std::wstring &regex, /* in */ const CNode &source, /* in */ const CNode &destination)
{
	std::vector<const CEdge *> solution;

	Paths roads([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2)
	{ return l1.size() > l2.size(); 
	});
	GetPathBetweenNodes(source, destination, roads);

	boost::wregex regexpr(regex);

	std::function<bool(const Paths::value_type &, std::wstring, std::vector<const CEdge *> &, int)> stepperFn = 
		[&] (const Paths::value_type &path, std::wstring partialPath, std::vector<const CEdge *> &stackList, int i) -> bool
	{
		if (static_cast<size_t>(i) == path.size())
		{
			bool bRet = _IsRegexMatch(regexpr, partialPath);
			if (bRet)
			{
				solution.assign(stackList.cbegin(), stackList.cend());
			}
			return bRet;
		}

		const auto &edges = m_matrix[*path[i - 1]][*path[i]];
		bool bRet = false;
		for (auto &&it : edges)
		{
			std::wstring newPath;
			newPath.assign(partialPath);
			newPath.append(it->GetLabel());
			stackList.emplace_back(it);
			bRet = stepperFn(path, newPath, stackList, i + 1);
			stackList.pop_back();
			if (bRet)
				break;
		}

		return bRet;
	};

	for (auto &&path : roads)
	{
		std::vector<const CEdge *> stackList;
		if (stepperFn(path, L"", stackList, 1))
		{
			break;
		}
	}

	_AddCurrentPathsToCache(source, destination, roads);

	return solution;
}

IContextGraph::PointerEdgePaths ContextGraph::FindAllOriginalPathsMatchedByRegex(const std::wstring &regex, const CNode &source, const CNode &destination)
{
	PointerEdgePaths solutions([] (const std::vector<const CEdge *> &l1, const std::vector<const CEdge *> &l2) { return &l1 < &l2; });
	auto hint = solutions.cend();

	Paths roads([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	GetPathBetweenNodes(source, destination, roads);
	boost::wregex regexpr(regex);

	std::function<void(const Paths::value_type &, std::wstring, std::vector<const CEdge *> &, int)> stepperFn = 
		[&] (const Paths::value_type &path, std::wstring partialPath, std::vector<const CEdge *> &stackList, int i)
	{
		if (static_cast<size_t>(i) == path.size())
		{
			bool bRet = _IsRegexMatch(regexpr, partialPath);
			if (bRet)
			{
				std::vector<const CEdge *> solution;
				for (auto &&it : stackList)
				{
					solution.emplace_back(it);
				}
				hint = solutions.emplace_hint(hint, solution);
			}

			return;
		}

		const auto &edges = m_matrix[*path[i - 1]][*path[i]];
		for (auto &&it : edges)
		{
			std::wstring newPath;
			newPath.assign(partialPath);
			newPath.append(it->GetLabel());
			stackList.emplace_back(it);
			stepperFn(path, newPath, stackList, i + 1);
			stackList.pop_back();
		}
	};

	for (auto &&path : roads)
	{
		std::vector<const CEdge *> stackList;
		stepperFn(path, L"", stackList, 1);
	}

	_AddCurrentPathsToCache(source, destination, roads);

	return solutions;
}

std::set<const CNode *> ContextGraph::GetLabeledNodes(void) const
{
	std::set<const CNode *> labeledNodes;
	auto hint = labeledNodes.cend();
	for (auto &&it : m_nodes)
	{
		if (!it.IsUnknown())
			hint = labeledNodes.emplace_hint(hint, &it);
	}

	return labeledNodes;
}

IContextGraph::TN ContextGraph::_GetPossibleUnknownNodes(/* in */ const ContextGraph patternGraph) const
{
	std::set<const CNode *> patternLabeledNodes = patternGraph.GetLabeledNodes();
	
	auto nodes = m_nodes;
	for (auto &&it : patternLabeledNodes)
	{
		nodes.erase(*it);
	}

	return nodes;
}

std::vector<const CEdge *> ContextGraph::GetCorrespondingConcreteEdges(const CEdge &edge, const TN &unkNodes) const
{
	std::vector<const CEdge *> correspondingEdges;

	const auto edges = GetEdgesByName(edge.GetLabel());
	const auto &source = edge.GetSource();
	const auto &destination = edge.GetDestination();
	bool sourceUnknown = source.IsUnknown();
	bool destinationUnknown = destination.IsUnknown();

	if (sourceUnknown && destinationUnknown)
	{
		for (auto edge = edges.first; edge != edges.second; edge++)
		{
			auto &otherSource = edge->GetSource();
			auto &otherDest = edge->GetDestination();
			if (unkNodes.find(otherSource) == unkNodes.cend() || unkNodes.find(otherDest) == unkNodes.cend())
				continue;

			correspondingEdges.emplace_back(&*edge);
		}
	}
	else
	{
		using namespace std::placeholders;
		
		boost::wregex sourceRegex;
		if (source.IsRegex())
			sourceRegex.assign(source.GetLabel());
		boost::wregex destinationRegex(L"", std::regex_constants::extended);
		if (destination.IsRegex())
			destinationRegex.assign(destination.GetLabel());
		auto sourceMatchRegex = std::bind(static_cast<bool(ContextGraph::*)(const boost::wregex &, const std::wstring &) const>(&ContextGraph::_IsRegexMatch),
										  this, std::cref(sourceRegex), _1);
		auto destinationMatchRegex = std::bind(static_cast<bool(ContextGraph::*)(const boost::wregex &, const std::wstring &) const>(&ContextGraph::_IsRegexMatch),
											   this, std::cref(destinationRegex), _1);

		auto copyConditionFn = [&] (const CEdge &e) -> bool
		{
			const auto &otherSource = e.GetSource();
			const auto &otherDestination = e.GetDestination();

			if ((sourceUnknown && unkNodes.find(otherSource) == unkNodes.cend()) || (destinationUnknown && unkNodes.find(otherDestination) == unkNodes.cend()))
				return false;

			bool bSourceMatch = false;
			bool bDestinationMatch = false;

			bSourceMatch = !source.IsRegex()? source == otherSource : sourceMatchRegex(otherSource.GetLabel());
			bDestinationMatch = !destination.IsRegex()? destination == otherDestination : destinationMatchRegex(otherDestination.GetLabel());

			return (sourceUnknown && bDestinationMatch) ||
				   (destinationUnknown && bSourceMatch) ||
				   (bSourceMatch && bDestinationMatch);
		};

		for (auto edge = edges.first; edge != edges.second; edge++)
		{
			if (copyConditionFn(*edge))
			{
				correspondingEdges.emplace_back(&*edge);
			}
		}
	}

	return correspondingEdges;
}

std::set<std::set<const CEdge *>> ContextGraph::GetMaximumMatch(/* in */ const ContextGraph &patternGraph, bool bRealTime, bool bMatchInThePast)
{
	std::set<std::set<const CEdge *>> bestSolutions;

	if (bRealTime)
	{
		ContextGraph cg;
		RefreshGraphConsistency();
		auto patternTime = patternGraph.GetExpireTime();
		auto validityInterval = patternGraph.GetValidityInterval();

		if (patternGraph.GetTimeLeft<std::chrono::microseconds>() <= 0)
		{
			if (bMatchInThePast)
			{
				auto fnAddValidEdgesToTheNewGraph = [&cg, validityInterval](const TE &container) -> void 
				{
					for (auto &edge : container)
					{
						if (ContextGraph::IntervalIntersection(validityInterval, edge.GetDuration()))
						{
							cg.AddEdge(edge.GetLabel(), edge.GetSource().GetLabel(), edge.GetDestination().GetLabel());
						}
					}
				};

				fnAddValidEdgesToTheNewGraph(m_edges);
				fnAddValidEdgesToTheNewGraph(m_oldEdges);
			}
			else 
				return bestSolutions; //no match performed
		}
		else
		{
			for (auto &edge : m_edges)
			{
				if (patternTime <= edge.GetLastExpirationTime() && IntervalIntersection(validityInterval, edge.GetDuration()))
				{
					cg.AddEdge(edge.GetLabel(), edge.GetSource().GetLabel(), edge.GetDestination().GetLabel());
				}
			}
		}

		return cg.GetMaximumMatch(patternGraph);
	}

	auto unkNodes = _GetPossibleUnknownNodes(patternGraph);

	auto &patternEdges = patternGraph.GetEdges();

	std::vector<const CEdge * > solution;
	size_t maxSize = 0;
	std::vector<CEdge> edges;
	std::vector<std::pair<CEdge, int>> unsortedEdges;

	for (auto &edge : patternEdges) 
	{
		auto &src = edge.GetSource();
		auto &dst = edge.GetDestination();

		if (edge.IsRegex() || src.IsRegex() || dst.IsRegex() || src.IsUnknown() || dst.IsUnknown())
		{
			unsortedEdges.emplace_back(edge, 0);
			continue;
		}
		auto possibleMatches = GetCorrespondingConcreteEdges(edge, unkNodes);

		auto size = possibleMatches.size();
		if (size == 1)
		{
			auto match = possibleMatches[0];
			auto &src = match->GetSource();
			auto &dest = match->GetDestination();
			auto &patternSrc = edge.GetSource();
			auto &patternDest = edge.GetDestination();
			const_cast<CNode &>(src).SetAssignment(true);
			const_cast<CNode &>(dest).SetAssignment(true);
			const_cast<CNode &>(patternSrc).SetCorespondent(true, &src);
			const_cast<CNode &>(patternDest).SetCorespondent(true, &dest);
			solution.emplace_back(possibleMatches[0]);
		}
		else
		{
			unsortedEdges.emplace_back(edge, static_cast<int>(size));
		}
	}

	std::sort(unsortedEdges.begin(), unsortedEdges.end(), [&] (/* in */ const std::pair<const CEdge, int> &edge1, /* in */ const std::pair<const CEdge, int> &edge2)
	{
		return edge1.second < edge2.second;
	});
	for (auto &e : unsortedEdges)
	{
		edges.emplace_back(e.first);
	}

	typedef std::unordered_map<const CEdge *, std::vector<const CEdge *>, std::function<size_t(const CEdge *)>> CachedEdges;
	CachedEdges edgeMatchSugestions(10, [](const CEdge *e) { return std::hash<int>() (reinterpret_cast<int>(e)); });
	for (auto &edge : edges)
	{
		if (!edge.IsRegex())
		{
			edgeMatchSugestions[&edge] = GetCorrespondingConcreteEdges(edge, unkNodes);
		}
	}

	auto firstEdge = edges.begin();

	auto fnFindPossibleNodesThatMatchNode = [&] (/* in */ const CNode &node) -> std::vector<const CNode *>
	{
		std::vector<const CNode*> matchedNodes;
		if (node.IsUnknown())
			for (auto &node : unkNodes)
				matchedNodes.emplace_back(&*m_nodes.find(node));
		else
			matchedNodes = FindNodesMatchingNodeName(node);

		return matchedNodes;
	};

	typedef std::unordered_map<const CNode *, std::vector<const CNode *>, std::function<size_t(const CNode *)>> CachedNodes;
	CachedNodes nodeMatchSugestions(10, [](const CNode *n) { return std::hash<int>() (reinterpret_cast<int>(n)); });
	for (auto &node : patternGraph.GetNodes())
	{
		nodeMatchSugestions.emplace(&node, fnFindPossibleNodesThatMatchNode(node));
	}

	auto filterOnChosenPairOfNodes = [&] (const CNode &patternSource, const CNode &patternDest, const CNode &labeledSource, const CNode &labeledDest) -> bool
	{
		bool bSourceAssignment = labeledSource.HasAssignment();
		bool bDestAssignment = labeledDest.HasAssignment();
		bool bSourceCorrespondent = patternSource.HasCorrespondent();
		bool bDestCorrespondent = patternDest.HasCorrespondent();
		
		if (bSourceCorrespondent && &patternSource.GetCorrespondent() != &labeledSource)
			return false;
		if (bDestCorrespondent && &patternDest.GetCorrespondent() != &labeledDest)
			return false;
		if ((bSourceAssignment && !bSourceCorrespondent) || (bDestAssignment && !bDestCorrespondent))
			return false;

		return true;
	};

	auto filterOnChosenEdgesFn = [&] (const CEdge &patternEdge, const CEdge &labeledEdge)
	{
		return !labeledEdge.IsAlreadyUsed() &&
			   filterOnChosenPairOfNodes(patternEdge.GetSource(), patternEdge.GetDestination(), labeledEdge.GetSource(), labeledEdge.GetDestination());		
	};

	auto findTheMatchedPathByRegexInCacheFn = [&](/* in */ const std::wstring &regex, 
                /* in */ const CNode *source,
                /* in */ const CNode *dest,
                /* out */ std::vector<const CEdge *> &maxPath) -> bool
	{
		auto found = m_regexCache.equal_range(regex);

		for (auto it = found.first; it != found.second; it++)
		{
			auto matchedTuple = it->second;
			if (std::get<0>(matchedTuple) == source && std::get<1>(matchedTuple) == dest)
			{
				maxPath = std::get<2>(matchedTuple);
				return true;
			}
		}

		maxPath = std::vector<const CEdge *>();
		return false;
	};

	std::function<void(/* in */ decltype(firstEdge) &currentEdge)> fnMatchFind = [&](/* inout */ decltype(firstEdge) &currentEdge) -> void
	{
		if (currentEdge == edges.cend())
		{
			auto size = solution.size();
			if (size == maxSize)
			{
				bestSolutions.emplace(std::set<const CEdge *>(solution.cbegin(), solution.cend()));
			}
			else
				if (size > maxSize)
				{
					maxSize = size;
					bestSolutions.clear();
					bestSolutions.emplace(std::set<const CEdge *>(solution.cbegin(), solution.cend()));
				}

			return;
		}

		if (!currentEdge->IsRegex())
		{
			const auto &possibleEdges = edgeMatchSugestions[&*currentEdge];
			for(auto &edge : possibleEdges)
			{
				if (!filterOnChosenEdgesFn(*currentEdge, *edge))
					continue;

				solution.emplace_back(edge);
				auto source = &edge->GetSource();
				auto dest = &edge->GetDestination();
				
				const_cast<CEdge *>(edge)->SetAlreayUsed(true);
				bool bPreviousSourceAssignment = source->HasAssignment();
				bool bPreviousDestAssignment = dest->HasAssignment();
				bool bPreviousSourceCorrespondent = currentEdge->GetSource().HasCorrespondent();
				bool bPreviousDestCorrespondent = currentEdge->GetDestination().HasCorrespondent();
				if (!bPreviousSourceAssignment)
					const_cast<CNode *>(source)->SetAssignment(true);
				if (!bPreviousDestAssignment)
					const_cast<CNode *>(dest)->SetAssignment(true);
				if (!bPreviousSourceCorrespondent)
					const_cast<CNode *>(&currentEdge->GetSource())->SetCorespondent(true, source);
				if (!bPreviousDestCorrespondent)
					const_cast<CNode *>(&currentEdge->GetDestination())->SetCorespondent(true, dest);

				fnMatchFind(++currentEdge);

				currentEdge--;
				solution.pop_back();
				
				if (!bPreviousSourceAssignment)
					const_cast<CNode *>(source)->SetAssignment(false);
				if (!bPreviousDestAssignment)
					const_cast<CNode *>(dest)->SetAssignment(false);
				if (!bPreviousSourceCorrespondent)
					const_cast<CNode *>(&currentEdge->GetSource())->SetCorespondent(false);
				if (!bPreviousDestCorrespondent)
					const_cast<CNode *>(&currentEdge->GetDestination())->SetCorespondent(false);
				const_cast<CEdge *>(edge)->SetAlreayUsed(false);
			}
		}
		else
		{
			auto &source = currentEdge->GetSource();
			auto &dest = currentEdge->GetDestination();

			std::vector<const CNode*> matchedSources, matchedDestinations;
			if (source.HasCorrespondent())
				matchedSources.emplace_back(&source.GetCorrespondent());
			else 
				matchedSources = nodeMatchSugestions[&source];

			if (dest.HasCorrespondent())
				matchedDestinations.emplace_back(&dest.GetCorrespondent());
			else 
				matchedDestinations = nodeMatchSugestions[&dest];

			for (auto matchSource : matchedSources)
			{
				bool bPreviousSourceAssignment = matchSource->HasAssignment();
				bool bPreviousSourceCorrespondent = currentEdge->GetSource().HasCorrespondent();
				if (!bPreviousSourceAssignment)
					const_cast<CNode *>(matchSource)->SetAssignment(true);
				if (!bPreviousSourceCorrespondent)
					const_cast<CNode *>(&source)->SetCorespondent(true, matchSource);
				
				for (auto matchDest : matchedDestinations)
				{
					if (!filterOnChosenPairOfNodes(source, dest, *matchSource, *matchDest))
						continue;

					if (&*matchSource != &*matchDest || source == dest)
					{
						std::vector<const CEdge *> maxPath;
						const auto &regexLabel = currentEdge->GetLabel();
						bool bRet = findTheMatchedPathByRegexInCacheFn(regexLabel, matchSource, matchDest, maxPath);
						if (!bRet)
						{
							maxPath = FindMaxOriginalPathMatchedByRegex(regexLabel, *matchSource, *matchDest);
							m_regexCache.emplace(regexLabel, std::make_tuple(matchSource, matchDest, maxPath));
						}
						if (!maxPath.empty())
						{
							for (const auto e : maxPath)
								solution.emplace_back(e);
							bool bPreviousDestAssignment = matchDest->HasAssignment();
							bool bPreviousDestCorrespondent = currentEdge->GetDestination().HasAssignment();
							if (!bPreviousDestAssignment)
								const_cast<CNode *>(matchDest)->SetAssignment(true);
							if (!bPreviousDestCorrespondent)
							const_cast<CNode *>(&dest)->SetCorespondent(true, matchDest);

							fnMatchFind(++currentEdge);

							currentEdge--;
							if (!bPreviousDestAssignment)
								const_cast<CNode *>(matchDest)->SetAssignment(false);
							if (!bPreviousDestCorrespondent)
								const_cast<CNode *>(&dest)->SetCorespondent(false);

							solution.erase(solution.end() - maxPath.size(), solution.end());
						}
					}
				}
				if (!bPreviousSourceAssignment)
					const_cast<CNode *>(matchSource)->SetAssignment(false);
				if (!bPreviousSourceCorrespondent)
					const_cast<CNode *>(&source)->SetCorespondent(false);
			}
		}

		if (std::distance(currentEdge, edges.end()) + solution.size() > maxSize)
		{
			fnMatchFind(++currentEdge);
			currentEdge--;
		}
	};

	fnMatchFind(firstEdge);

	for (auto &edge : solution)
	{
		auto &src = edge->GetSource();
		auto &dest = edge->GetDestination();
		const_cast<CNode &>(src).SetAssignment(false);
		const_cast<CNode &>(dest).SetAssignment(false);
	}

	return bestSolutions;
}

std::vector<std::vector<const CEdge *>> ContextGraph::ComputeConnexComponents(void) const
{
	std::unordered_set<CNodePtr> visitedNodes;
	std::vector<std::vector<const CEdge *>> connexComponents;

	typedef std::pair<T::const_iterator, T::const_iterator> children_type;

	for (auto &&it : m_nodes)
	{
		if (visitedNodes.find(CRefNodePtr(it)) != visitedNodes.cend())
		{
			continue;
		}

		std::vector<const CEdge *> connexComponent;
		std::unordered_set<CEdgePtr> visitedEdges;
		std::queue<CNodePtr> queue;
		queue.emplace(CRefNodePtr(it));
		visitedNodes.emplace(CRefNodePtr(it));
		
		std::function<void(children_type &, bool)> iterateFn = [&] (/* in */ children_type &nodeList, /* in */ bool areParents) -> void
		{
			typedef const CNode& (CEdge::*FnDestination) () const;
			FnDestination fnDestination = areParents? &CEdge::GetSource : &CEdge::GetDestination;

			for (auto &child = nodeList.first; child != nodeList.second; child++)
			{
				const auto &destination = (*(child->second).*fnDestination)();
				if (visitedEdges.find(CRefEdgePtr(*child->second)) == visitedEdges.cend())
				{
					visitedEdges.emplace(CRefEdgePtr(*child->second));
					connexComponent.emplace_back(child->second);
					if (visitedNodes.find(CRefNodePtr(destination)) == visitedNodes.cend())
					{
						queue.emplace(CRefNodePtr(destination));
						visitedNodes.emplace(CRefNodePtr(destination));
					}
				}
			}
		};

		while (!queue.empty())
		{
			auto currentNode = *queue.front();
			auto children = GetChildren(currentNode);
			auto parents = GetParents(currentNode);

			iterateFn(children, false);
			iterateFn(parents, true);

			queue.pop();
		}

		if (!connexComponent.empty())
			connexComponents.emplace_back(connexComponent);
	}

	return connexComponents;
}

void ContextGraph::_ExtractEdge(/* in */ const std::wstring &edge)
{
	size_t size = edge.size();
	std::locale loc;

	size_t i = 0;
	for (; i < size && std::isspace((char)edge[i], loc); i++) { }

	boost::wregex regex(LR"r((".*?[^\\]"|.*?)\s*->\s*(".*?[^\\]"|.*?)\s*(?=\[|$))r");

	boost::wsmatch matches;
	bool bRet = boost::regex_search(edge.begin() + i, edge.end(), matches, regex);
	if (!bRet)
		return;
	
	boost::wregex edgeRegex(LR"r(\s*(\w+)\s*=\s*(".*?[^\\]"|.*?)\s*)r");
	std::wstring edgeName(L"");
	time_t expireTime = 0;
	bool bExpireTimeAvailable = false;
	Duration withinInterval = PERMANENT_DURATION;

	boost::wsmatch edgeMatches;
	if (matches.suffix().length() > 0)
	{
		std::wstring edgeInfo = matches.suffix().str();
		while (boost::regex_search(edgeInfo, edgeMatches, edgeRegex))
		{
			std::wstring s1 = edgeMatches[0];
			std::wstring s2 = edgeMatches[1];
			std::wstring s3 = edgeMatches[2];
			if (edgeMatches.size() == 3)
			{
				if (edgeMatches[1] == L"label")
					edgeName = edgeMatches[2];
				else if (edgeMatches[1] == L"expire_time")
				{
					//skipping quotes if present
					std::wstring expireTimeStr = edgeMatches[2].str();
					std::wstringstream stream(expireTimeStr.front() == L'\"'? std::wstring(expireTimeStr.begin() + 1, expireTimeStr.end() - 1) : expireTimeStr);

					stream >> expireTime;
					bExpireTimeAvailable = true;
				}
				else if (edgeMatches[1] == L"within")
				{
					std::wstring intervalStr = edgeMatches[2].str();
					std::wstringstream stream(std::wstring(intervalStr.begin() + 1, intervalStr.end() - 1));
					time_t t1, t2;
					std::wstring dummy;
					stream >> t1 >> dummy >> t2;
					withinInterval.first = std::chrono::system_clock::from_time_t(t1);
					withinInterval.second = std::chrono::system_clock::from_time_t(t2);
				}
			}
			edgeInfo = edgeMatches.suffix().str();
		}
	}

	const std::wstring &source = matches[1].str();
	const std::wstring &dest = matches[2].str();

	AddEdge(edgeName.front() == L'\"' ? std::wstring(edgeName.cbegin() + 1, edgeName.cend() - 1) : edgeName,
			source.front() == L'\"' ? std::wstring(source.cbegin() + 1, source.cend() - 1) : source,
			dest.front() == L'\"' ? std::wstring(dest.cbegin() + 1, dest.cend() - 1) : dest,
			bExpireTimeAvailable? std::chrono::system_clock::from_time_t(expireTime): NEVER_EXPIRE,
			withinInterval);
}

void ContextGraph::_ReadGraphFromDotFormat(const std::wstring &dot)
{
	auto ffind = dot.find_first_of(L'{');
	auto lfind = dot.find_last_of(L"}");

	boost::wregex regex(LR"r(\s*;*\s*$)r");

	boost::wsmatch matches;
	const auto & substr = dot.substr(ffind + 1, lfind - ffind - 1);

	boost::wsregex_token_iterator endOfSeq;
	boost::wsregex_token_iterator iterator(substr.cbegin(), substr.cend(), regex);

	while (iterator != endOfSeq)
	{
		_ExtractEdge(iterator->str());
		iterator++;
	}
}

void ContextGraph::_ReadGraphFromDotFormatNoRegex(const std::wstring &dot)
{
	auto ffind = dot.find_first_of(L'{');
	while (std::isspace(dot[ffind + 1]))
		ffind++;
	auto lfind = dot.find_last_of(L"}");

	const auto & substr = dot.substr(ffind + 1, lfind - ffind - 1);

	auto substrSize = substr.size();
	std::wstring edge;
	for (unsigned int i = 0; i < substrSize; i++)
	{
		edge.push_back(substr[i]);
		if (substr[i] == L'\n')
		{
			while (std::isspace(edge.back()))
				edge.pop_back();
			if (edge.back() == L';')
				edge.pop_back();
			_ExtractEdge(edge);
			edge.clear();	
			
			bool advancedOnSpaces = false;
			while (std::isspace(substr[i]))
			{
				i++;
				advancedOnSpaces = true;
			}

			if (advancedOnSpaces)
				i--;
		}
	}	
}

bool ContextGraph::WriteGraphToDotFile(/* in */ const std::wstring &fileName, /* in */ const std::wstring &graphName) const
{
	std::wofstream file(std::string(fileName.cbegin(), fileName.cend()));
	if (!file)
		return false;

	std::wstring header = L"digraph ";
	header.append(graphName);
	header.push_back(L'{');

	file << header << std::endl; 
	for (auto &&edge : m_edges)
	{
		file << L"\"" << edge.GetSource().GetLabel() << L"\" -> \"" << edge.GetDestination().GetLabel() << L"\"";
		file << L" [label = \"" + edge.GetLabel() + L"\"];" << std::endl;
	}
	file << L"}";

	return true;
}

bool ContextGraph::BuildFromDotFile(/* in */ const std::wstring &fileName, /* in_opt */ bool bClearPrecedent)
{
	if (bClearPrecedent)
	{
		Clear();
	}

	std::wifstream file(std::string(fileName.cbegin(), fileName.cend()));

    if (!file)
		return false;

    std::wstringstream buffer;
    buffer << file.rdbuf();
	file.close();

	_ReadGraphFromDotFormatNoRegex(buffer.str());

	return true;
}

void ContextGraph::Clear()
{
	m_matrix.clear();
	m_nodes.clear();
	m_edges.clear();
	m_oldNodes.clear();
	m_oldEdges.clear();
	m_graph.clear();
	m_tGraph.clear();
	m_pathMatrix.clear();
}

const CEdge * ContextGraph::FindEdge(/* in */ const std::wstring &strLabel, /* in */ const CNode &source, /* in */ const CNode &destiation) const
{
	auto edges = GetEdgesByName(strLabel);
	for (auto &&it = edges.first; it != edges.second; it++)
	{
		if (it->FullEqual(strLabel, source, destiation))
		{
			return &*it;
		}
	}

	return nullptr;
}

std::vector<const CNode *> ContextGraph::FindNodesMatchingNodeName(/* in */ const CNode &node) const
{
	const auto label = node.GetLabel();
	std::vector<const CNode *> foundNodes;
	if (!node.IsRegex())
	{
		const CNode *foundNode = nullptr;
		bool bFound = FindNodeByName(label, foundNode);
		if (bFound)
		{
			foundNodes.emplace_back(foundNode);
		}
	}
	else
	{
		boost::wregex regex(label);
		auto found = std::find_if(m_nodes.cbegin(), m_nodes.cend(), [&] (const CNode &n) { return _IsRegexMatch(regex, n.GetLabel()); });
		while (found != m_nodes.cend())
		{
			foundNodes.emplace_back(&*found);
			found = std::find_if(++found, m_nodes.cend(), [&] (const CNode &n) { return _IsRegexMatch(regex, n.GetLabel()); });
		}
	}

	return foundNodes;
}

template <typename CM, typename CE>
std::set<const CNode *> ContextGraph::GetNeighbours(/* in */ const CNode &node, /* in_opt */ const CM &memberNodes, /* in_opt */ const CE &excludedNodes) const
{
	std::set<const CNode *> neighs;
	auto children = GetChildren(node);
	auto parents = GetParents(node);

	auto hint = neighs.cbegin();
	for (auto child = children.first; child != children.second; child++)
	{
		hint = neighs.emplace_hint(hint, &child->second->GetDestination());
	}

	for (auto parent = parents.first; parent != parents.second; parent++)
	{
		hint = neighs.emplace_hint(hint, &parent->second->GetSource());
	}

	auto sameNodeNeighbour = std::find(neighs.cbegin(), neighs.cend(), &node);
	if (sameNodeNeighbour != neighs.cend())
		neighs.erase(sameNodeNeighbour);

	std::set<const CNode *> filteredNeighs;
	std::remove_copy_if(neighs.cbegin(), neighs.cend(), std::inserter(filteredNeighs, filteredNeighs.end()),
						[&] (const CNode * node) 
						{ 
							return _CallFind(excludedNodes, node) || (!memberNodes.empty() && !_CallFind(memberNodes, node)); 
						});

	return filteredNeighs;
}

std::vector<const CEdge *> ContextGraph::_FindRandomSpanningTree(/* in */ const std::vector<const CNode *> &nodes) const
{
	std::vector<const CEdge *> spanningTree;
	std::set<const CNode *> visited;
	std::random_device device;
	std::uniform_int_distribution<> dist;

	std::function<void(const CNode *)> dfsFn = [&] (const CNode * currentNode) -> void
	{
		auto neighs = GetNeighbours(*currentNode, nodes, visited);
		
		while (!neighs.empty())
		{
			dist.param(std::uniform_int_distribution<>::param_type(0, static_cast<int>(neighs.size()) - 1));
			auto pickedIt = neighs.cbegin();
			std::advance(pickedIt, dist(device));
			auto newNode = *pickedIt;
			neighs.erase(pickedIt);

			if (visited.find(newNode) != visited.cend())
				continue;

			IContextGraph::AdjacentEdges edgesDirected, edgesReverted;
			if (IsAdjacent(*currentNode, *newNode))
				edgesDirected = m_matrix.at(*currentNode).at(*newNode);
			if (IsAdjacent(*newNode, *currentNode))
				edgesReverted = m_matrix.at(*newNode).at(*currentNode);

			edgesDirected.resize(edgesDirected.size() + edgesReverted.size());
			std::move_backward(edgesReverted.cbegin(), edgesReverted.cend(), edgesDirected.end());
			dist.param(std::uniform_int_distribution<>::param_type(0, static_cast<int>(edgesDirected.size()) - 1));
			const auto chosenEdge = edgesDirected[dist(device)];

			spanningTree.emplace_back(chosenEdge);

			visited.emplace(newNode);
			dfsFn(newNode);
		}
	};

	dist.param(std::uniform_int_distribution<>::param_type(0, static_cast<int>(nodes.size()) - 1));
	const auto rootTree = nodes[dist(device)];
	visited.emplace(rootTree);
	dfsFn(rootTree);

	return spanningTree;
}

ContextGraph ContextGraph::GenerateRandomSubgraph(/* in_opt */ unsigned int nodesPercent,
												  /* in_opt */ unsigned int edgesPercentOnSubsetOfNodes,
												  /* in_opt */ unsigned int percentOfUnknownNodes,
												  /* in_opt */ std::set<std::wstring> mandatoryNodes)
{
	auto totalNodes = m_nodes.size();

	auto expectedNodes = nodesPercent * totalNodes / 100;
	std::vector<const CNode *> generatedNodes;

	std::random_device device;
	std::uniform_int_distribution<> dist(0, static_cast<int>(totalNodes) - 1);
	if (mandatoryNodes.empty())
	{
		unsigned int firstNodeIndex = dist(device);
		auto beginIt = m_nodes.cbegin();
		std::advance(beginIt, firstNodeIndex);
		generatedNodes.emplace_back(&*beginIt);
	}
	else
	{
		auto &node = GetNodeByName(*mandatoryNodes.cbegin());
		generatedNodes.emplace_back(&node);
		mandatoryNodes.erase(mandatoryNodes.cbegin());
	}

	auto nodePool = GetNeighbours(*generatedNodes[0]);

	for (unsigned int i = 1; i < expectedNodes; i++)
	{
		if (mandatoryNodes.empty())
		{
			dist.param(std::uniform_int_distribution<>::param_type(0, static_cast<int>(nodePool.size()) - 1));
			auto pickedIndex = dist(device);

			auto pickedIt = nodePool.cbegin(); 
			std::advance(pickedIt, pickedIndex);
			const auto &newNode = **pickedIt;
			nodePool.erase(pickedIt);
			generatedNodes.emplace_back(&newNode);
		}
		else
		{
			const auto &newNode = GetNodeByName(*mandatoryNodes.cbegin());
			generatedNodes.emplace_back(&newNode);
			mandatoryNodes.erase(mandatoryNodes.cbegin());
		}

		auto newNeighs = GetNeighbours(*generatedNodes.back(), decltype(generatedNodes)(), generatedNodes);

		nodePool.insert(newNeighs.cbegin(), newNeighs.cend());
	}

	std::vector<const CEdge *> totalSubgraphEdges;
	std::vector<const CEdge *> chosenEdges;

	auto n = generatedNodes.size();
	for (unsigned int i = 0; i < n; i++)
	{
		register const auto n1 = generatedNodes[i];
		for (unsigned int j = 0; j < n; j++)
		{
			const auto n2 = generatedNodes[j];
			if (IsAdjacent(*n1, *n2))
			{
				auto &edges = m_matrix[*n1][*generatedNodes[j]];
				totalSubgraphEdges.insert(totalSubgraphEdges.end(), edges.cbegin(), edges.cend());
			}
		}
	}

	auto totalEdgeNumber = totalSubgraphEdges.size();
	auto expectedEdgeNumber = edgesPercentOnSubsetOfNodes * totalEdgeNumber / 100;

	auto spanningTree = _FindRandomSpanningTree(generatedNodes);
	totalSubgraphEdges.erase(std::remove_if(totalSubgraphEdges.begin(),
											totalSubgraphEdges.end(),
											[&](const CEdge * edge) { return _CallFind(spanningTree, edge); }),
							 totalSubgraphEdges.end());
	for (size_t i = spanningTree.size(); i < expectedEdgeNumber; i++)
	{
		if (totalSubgraphEdges.empty())
			break;

		dist.param(std::uniform_int_distribution<>::param_type(0, static_cast<int>(totalSubgraphEdges.size()) - 1));
		int pickedIndex = dist(device);
		spanningTree.emplace_back(totalSubgraphEdges[pickedIndex]);
		totalSubgraphEdges.erase(totalSubgraphEdges.begin() + pickedIndex);
	}

	ContextGraph cg;
	for (unsigned int i = 0; i < spanningTree.size(); i++)
	{
		auto edge = spanningTree[i];
		cg.AddEdge(edge->GetLabel(), edge->GetSource().GetLabel(), edge->GetDestination().GetLabel());
	}

	cg.ConvertNodesToUnknown(percentOfUnknownNodes);

	//drop move constructor to keep pointers integrity
	return cg;
}

void ContextGraph::ConvertNodesToUnknown(/* in */ unsigned int percentOfNodes)
{
	auto totalNodes = m_nodes.size();
	auto unknownNodes = percentOfNodes * totalNodes / 100;

	std::vector<CNode *> nodes;
	for (auto &&node : m_nodes)
	{
		nodes.emplace_back(const_cast<CNode *>(&node));
	}
	std::random_device device;
	std::mt19937 generator(device());
	std::shuffle(nodes.begin(), nodes.end(), generator);

	for (unsigned int i = 0; i < unknownNodes; i++)
	{
		std::wstring label = L"?";
		label += std::to_wstring(i);
		ReplaceNode(nodes[i]->GetLabel(), label);
	}
}

void ContextGraph::RemoveNode(/* in */ const std::wstring &node)
{
	m_graph.erase(node);
	m_tGraph.erase(node);

	std::function<bool(T::const_iterator)> deleteConditionFn = [&node] (T::const_iterator it) { return it->second->GetSource() == node || it->second->GetDestination() == node; };
	_DeleteFromContainer(m_graph, deleteConditionFn);
	_DeleteFromContainer(m_tGraph, deleteConditionFn);

	m_matrix.erase(node);
	for (auto &&it : m_matrix)
	{
		auto row = it.second;
		row.erase(node);
	}

	m_pathMatrix.clear();

        std::function<bool(TE::const_iterator)> fnDeleteCondition = [&node] (TE::const_iterator it) { return it->GetDestination() == node || it->GetSource() == node; };
	_DeleteFromContainer(m_edges, fnDeleteCondition);
	
	m_nodes.erase(node);
}

void ContextGraph::ReplaceNode(/* in */ const std::wstring &oldNode, /* in */ const std::wstring newNode)
{
	auto found = m_nodes.find(oldNode);
	if (found == m_nodes.end())
		return;

	auto oldAddress = &*found;
	auto newAddress = &*m_nodes.emplace(newNode).first;

	std::function<bool(T::const_iterator)> deleteConditionFn = [&oldNode] (T::const_iterator it) { return it->second->GetSource() == oldNode || it->second->GetDestination() == oldNode; };
	_DeleteFromContainer(m_graph, deleteConditionFn);
	_DeleteFromContainer(m_tGraph, deleteConditionFn);

	for (auto &it : m_edges)
	{
		bool bDelete = false;
		auto source = &it.GetSource();
		auto dest = &it.GetDestination();
		
		if (source == oldAddress)
		{
			source = newAddress;
			bDelete = true;
		}

		if (dest == oldAddress)
		{
			dest = newAddress;
			bDelete = true;
		}

		if (bDelete)
		{
			const_cast<CEdge &>(it).SetNodes(*source, *dest);
			m_graph.emplace(*source, &it);
			m_tGraph.emplace(*dest, &it);
		}
	}

	m_nodes.erase(oldNode);
}

void ContextGraph::_BeforeEdgeDeletion(/* in */ const CEdge &edge)
{
	const auto newSource = m_oldNodes.emplace(edge.GetSource());
	const auto newDest = m_oldNodes.emplace(edge.GetDestination());
	CEdge e(edge.GetLabel(), *newSource.first, *newDest.first, NEVER_EXPIRE, edge.GetDuration());
	m_oldEdges.emplace(e);

	for (auto it = m_graph.cbegin(); it != m_graph.cend(); it++)
		if (it->second == &edge)
		{
			m_graph.erase(it);
			break;
		}

	for (auto it = m_tGraph.cbegin(); it != m_tGraph.cend(); it++)
		if (it->second == &edge)
		{
			m_tGraph.erase(it);
			break;
		}

	auto &source = edge.GetSource();
	auto &dest = edge.GetDestination();

	auto &adjacentEdges = m_matrix[source][dest];
	auto found = std::find(adjacentEdges.begin(), adjacentEdges.end(), &edge);
	if (found != adjacentEdges.cend())
		adjacentEdges.erase(found);

	auto removeIfEmptyFn = [this](const CNode &node)
	{
		auto neigh = GetNeighbours(node);
		if (neigh.empty())
		{
			m_pathMatrix.erase(node);
			for (auto &it : m_pathMatrix)
				it.second.erase(node);
			m_nodes.erase(node);
		}
	};

	removeIfEmptyFn(source);
	removeIfEmptyFn(dest);
}

void ContextGraph::_DeleteEdge(/* in */ const CEdge &edge)
{
	_BeforeEdgeDeletion(edge);

	for (auto e = m_edges.cbegin(); e != m_edges.cend(); e++)
		if (&*e == &edge)
		{
			m_edges.erase(e);
			break;
		}
}

void ContextGraph::RefreshGraphConsistency(void)
{
        std::function<bool(TE::const_iterator)> fnDeleter = [this](TE::const_iterator it) 
	{
		const_cast<CEdge *>(&*it)->RefreshExpiration();
		if (it->IsExpired())
		{
			_BeforeEdgeDeletion(*it);
			return true;
		}
		
		return false;
	};
	_DeleteFromContainer(m_edges, fnDeleter);
}

std::vector<ContextGraph> ContextGraph::GetMaximumMatchGraphs(/* in */ const ContextGraph &patternGraph, bool bRealTime, bool bMatchInThePast)
{
	std::vector<ContextGraph> solutions;
	auto match = GetMaximumMatch(patternGraph, bRealTime, bMatchInThePast);

	for (auto &solution : match)
	{
		ContextGraph graph;
		for (const auto &edge : solution)
		{
			graph.AddEdge(edge->GetLabel(), edge->GetSource().GetLabel(), edge->GetDestination().GetLabel(), edge->GetFirstExpirationTime(), edge->GetDuration());
		}

		solutions.emplace_back(graph);
	}

	return solutions;
}

void ContextGraph::PrecomputeRoadsBetweenPairOfNodes(void)
{
	for (auto row : m_matrix)
	{
		for (auto cell : row.second)
		{
			auto etalonEdge = cell.second.front();
			auto &source = etalonEdge->GetSource();
			auto &destination = etalonEdge->GetDestination();
			Paths::key_type path;
			path.emplace_back(&source);
			path.emplace_back(&destination);
			_AddSingleUniquePathToCache(source, destination, path);
		}
	}

	auto fnDetectSameCiclicRoad = [&](Paths::key_type v) -> bool
	{
		for (size_t i = 2; i < v.size() - 1; i++)
			if (v[i] == v[0] && v[i + 1] == v[1])
				return true;

		return false;
	};

	bool bAddedNew = true;
	unsigned int currentSize = 2;
	while (bAddedNew)
	{
		bAddedNew = false;

		for (auto row : m_matrix)
		{
			for (auto cell : row.second)
			{
				auto etalonEdge = cell.second.front();
				auto &source = etalonEdge->GetSource();
				auto &destination = etalonEdge->GetDestination();

				auto rowFound = m_pathMatrix.find(destination);
				if (rowFound != m_pathMatrix.cend())
				{
					for (auto end : rowFound->second)
					{
						auto &paths = end.second;
						for (auto path : paths)
						{
							if (path.size() == currentSize && (source != destination || destination != *path[1]))
							{
								Paths::key_type newPath;
								newPath.emplace_back(&source);
								std::copy(path.cbegin(), path.cend(), std::back_inserter(newPath));

								if (!fnDetectSameCiclicRoad(newPath))
								{
									_AddSingleUniquePathToCache(source, *newPath.back(), newPath);
									bAddedNew = true;
								}
							}
						}
					}
				}
			}
		}

		currentSize++;
	}
}

std::wstring ContextGraph::SerializeGraph(void) const
{
	std::wstring strGraph;
	for (auto &edge : m_edges)
	{
		strGraph.append(edge.GetLabel() + L"-" + edge.GetSource().GetLabel() + L"-" + edge.GetDestination().GetLabel());
		strGraph.push_back(L'|');
	}

	return strGraph;
}

bool ContextGraph::IsIncludedIn(/* in */ const ContextGraph &bigGraph) const
{
	for (auto &edge : m_edges)
	{
		const auto &sameNameEdges = bigGraph.GetEdgesByName(edge.GetLabel());
		bool bFound = false;
		for (auto it = sameNameEdges.first; it != sameNameEdges.second; it++)
		{
			if (edge.FullEqual(*it))
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
			return false;
	}

	return true;
}

std::wstring ContextGraph::GetEdgeTextRepresentation(void) const
{
	std::wstring hash;
	for (auto &&edge : m_edges)
	{
		hash.append(edge.GetLabel());
		hash.push_back(L'~');
	}

	return hash;
}
