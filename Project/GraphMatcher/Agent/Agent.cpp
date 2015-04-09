#include "stdafx.h"
#include "Agent.h"

bool Agent::HasPreviousMatch(/* in */ ContextGraph cg, /* out */ ContextGraph matchFound) const
{
	for (auto match : m_cachedMatches)
	{
		if (match.first.IsIncludedIn(cg))
		{
			matchFound = match.second;
			return true;
		}
	}

	return false;
}

//to test
int main()
{
	Agent a;
	ContextGraph g1;
	g1.BuildFromDotFile(L"g1.dot");
	ContextGraph g2;
	g2.BuildFromDotFile(L"g2.dot");
	ContextGraph g3;
	g3.BuildFromDotFile(L"g3.dot");
	a.AddPatterns(g1, g2, g3);

	ContextGraph g;
	g.BuildFromDotFile(L"gcontext.dot");

	return 0;
}
