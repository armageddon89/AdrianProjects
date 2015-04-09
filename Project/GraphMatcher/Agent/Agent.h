#pragma once
#include "ContextGraph.h"

class Agent
{
public:

	Agent() : m_cachedMatches(10,
		[](const ContextGraph &cg) { return std::hash<std::wstring>()(cg.GetEdgeTextRepresentation()); },
		[](const ContextGraph &g1, const ContextGraph &g2) { return g1.GetEdges() == g2.GetEdges(); })
	{}

	template <typename First>
	inline void AddPatterns(/* in */ First&& pattern) 
	{ 
		m_patterns.emplace_back(pattern); 
	}
	template <typename First, typename ...T>
	inline void AddPatterns(/* in */ First&& pattern, /* in */ T&& ...patterns) 
	{ 
		m_patterns.emplace_back(pattern);
		AddPatterns(patterns...); 
	}

	bool HasPreviousMatch(/* in */ ContextGraph cg, /* out */ ContextGraph matchFound) const;

private:

	std::vector<ContextGraph> m_context;
	std::vector<ContextGraph> m_patterns;
	std::unordered_map<ContextGraph, ContextGraph, std::function<size_t(const ContextGraph &)>,  std::function<bool(const ContextGraph &g1, const ContextGraph &g2)>> m_cachedMatches;
};
