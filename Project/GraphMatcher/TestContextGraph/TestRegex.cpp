// TestRegex.cpp : Defines the entry point for the console application.
//

#include "CommonTypes.h"
#include "ContextGraph.h"

bool Test_AddStringEdge()
{
	ContextGraph cg;

	cg.AddEdge(L"2", L"3", L"4");
	auto edges = cg.GetEdges();

	auto edge = edges.begin();

	return edges.size() == 1 && 
		   edge->GetLabel() == L"2" &&
		   edge->GetSource().GetLabel() == L"3" &&
		   edge->GetDestination().GetLabel() == L"4";
}

bool Test_AddDistinctEdges()
{
	ContextGraph cg;

	cg.AddEdge(L"2", L"3", L"4");
	cg.AddEdge(L"3", L"5", L"6");
	auto edges = cg.GetEdges();

	auto edge1 = edges.begin();
	auto it = edges.begin();
	auto edge2 = ++it;

	auto testFn = [&](decltype(edge1) &edge1, decltype(edge1) &edge2)
	{
		return
		edge1->GetLabel() == L"2" &&
		edge1->GetSource().GetLabel() == L"3" &&
		edge1->GetDestination().GetLabel() == L"4" &&
		edge2->GetLabel() == L"3" &&
		edge2->GetSource().GetLabel() == L"5" &&
		edge2->GetDestination().GetLabel() == L"6";
	};

	return edges.size() == 2 && (testFn(edge1, edge2) || testFn(edge2, edge1));
}

bool Test_AddDifferentEdgeBetweenSameNodes()
{
	ContextGraph cg;

	cg.AddEdge(L"2", L"3", L"4");
	cg.AddEdge(L"3", L"3", L"4");
	auto edges = cg.GetEdges();

	auto edge1 = edges.begin();
	auto it = edges.begin();
	auto edge2 = ++it;

	auto testFn = [&](decltype(edge1) &edge1, decltype(edge1) &edge2)
	{
		return
		edge1->GetLabel() == L"2" &&
		edge1->GetSource().GetLabel() == L"3" &&
		edge1->GetDestination().GetLabel() == L"4" &&
		edge2->GetLabel() == L"3" &&
		edge2->GetSource().GetLabel() == L"3" &&
		edge2->GetDestination().GetLabel() == L"4" &&
		(void *)&edge1->GetSource() == &edge2->GetSource() &&
		(void *)&edge1->GetDestination() == &edge2->GetDestination();
	};

	return edges.size() == 2 && (testFn(edge1, edge2) || testFn(edge2, edge1));
		   
}

bool Test_AddSameEdgeBetweenSameNodes()
{
	ContextGraph cg;

	cg.AddEdge(L"2", L"3", L"4");
	cg.AddEdge(L"2", L"3", L"4");
	auto edges = cg.GetEdges();

	auto edge1 = edges.begin();
	auto it = edges.begin();
	auto edge2 = ++it;

	auto testFn = [&](decltype(edge1) &edge1, decltype(edge1) &edge2)
	{
		return
		edge1->GetLabel() == L"2" &&
		   edge1->GetSource().GetLabel() == L"3" &&
		   edge1->GetDestination().GetLabel() == L"4" &&
		   edge2->GetLabel() == L"2" &&
		   edge2->GetSource().GetLabel() == L"3" &&
		   edge2->GetDestination().GetLabel() == L"4" &&
		   (void *)&edge1->GetSource() == &edge2->GetSource() &&
		   (void *)&edge1->GetDestination() == &edge2->GetDestination();
		
	};

	return edges.size() == 2 && (testFn(edge1, edge2) || testFn(edge2, edge1));
}

bool Test_AddUnknownNode()
{
	ContextGraph cg;

	cg.AddEdge(L"2", L"3", L"?");
	auto edges = cg.GetEdges();

	auto edge = edges.begin();

	return edges.size() == 1 && 
		   edge->GetLabel() == L"2" &&
		   !edge->GetSource().IsUnknown() &&
		   edge->GetDestination().IsUnknown();
}

bool Test_GetPathBetweenNodes_NoPath()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");
	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n3, n4);

	IContextGraph::Paths solutions;
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n4.GetLabel()), cg.GetNodeByName(n1.GetLabel()), solutions);

	return bRet == false;
}

bool Test_GetPathBetweenNodes_DirectEdge()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");

	cg.AddEdge(L"e", n1, n2);

	IContextGraph::Paths solutions;
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 1) return false;

	auto path = solutions.begin();
	auto src = path->begin();
	auto dest = src + 1;
	
	return (*src)->GetLabel() == L"1" && 
		   (*dest)->GetLabel() == L"2";
}

bool Test_GetPathBetweenNodes_DirectMultipleEdges()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	
	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n1, n2);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 1) return false;

	auto path = solutions.begin();
	auto src = path->begin();
	auto dest = src + 1;
	
	return (*src)->GetLabel() == L"1" && 
		   (*dest)->GetLabel() == L"2";
}

bool Test_GetPathBetweenNodes_TransitivePath()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n2, n3);
	cg.AddEdge(L"e2", n3, n4);

	IContextGraph::Paths solutions;
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n4.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 1) return false;

	auto path = solutions.begin();
	auto nod1 = path->begin();
	auto nod2 = nod1 + 1;
	auto nod3 = nod2 + 1;
	auto nod4 = nod3 + 1;
	
	return (*nod1)->GetLabel() == L"1" && 
		   (*nod2)->GetLabel() == L"2" &&
		   (*nod3)->GetLabel() == L"3" &&
		   (*nod4)->GetLabel() == L"4";
}

bool Test_GetPathBetweenNodes_MultipleTransitivePathsSameSize()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n2, n4);
	cg.AddEdge(L"e2", n1, n3);
	cg.AddEdge(L"e3", n3, n4);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n4.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 2) return false;

	auto path = solutions.begin();
	auto nod1 = path->begin();
	auto nod2 = nod1 + 1;
	auto nod3 = nod2 + 1;
	
	bool predPath1 = (*nod1)->GetLabel() == L"1" && 
			 ((*nod2)->GetLabel() == L"2" || (*nod2)->GetLabel() == L"3") &&
			 (*nod3)->GetLabel() == L"4";

	path++;
	nod1 = path->begin();
	nod2 = nod1 + 1;
	nod3 = nod2 + 1;
	bool predPath2 = (*nod1)->GetLabel() == L"1" &&
			 ((*nod2)->GetLabel() == L"3" || (*nod2)->GetLabel() == L"2") &&
			 (*nod3)->GetLabel() == L"4";

	return predPath1 && predPath2;
}

bool Test_GetPathBetweenNodes_MultipleTransitivePathsDifferentSize()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");
	CNode n5(L"5");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n2, n4);
	cg.AddEdge(L"e2", n1, n3);
	cg.AddEdge(L"e3", n3, n5);
	cg.AddEdge(L"e4", n5, n4);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n4.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 2) return false;

	auto path = solutions.begin();
	auto nod1 = path->begin();
	auto nod2 = nod1 + 1;
	auto nod3 = nod2 + 1;
	auto nod4 = nod3 + 1;
	
	//biggest path first
	bool predPath1 = (*nod1)->GetLabel() == L"1" &&
					 (*nod2)->GetLabel() == L"3" &&
					 (*nod3)->GetLabel() == L"5" &&
					 (*nod4)->GetLabel() == L"4";

	path++;
	nod1 = path->begin();
	nod2 = nod1 + 1;
	nod3 = nod2 + 1;

	bool predPath2 = (*nod1)->GetLabel() == L"1" && 
					 (*nod2)->GetLabel() == L"2" &&
					 (*nod3)->GetLabel() == L"4";	

	return predPath1 && predPath2;
}

bool Test_GetPathBetweenNodes_CombinationWithSameNode()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");

	cg.AddEdge(L"e", n1, n1);
	cg.AddEdge(L"e1", n1, n2);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 2) return false;

	auto path = solutions.begin();
	auto nod1 = path->begin();
	auto nod2 = nod1 + 1;
	auto nod3 = nod2 + 1;

	bool predPath1 = (*nod1)->GetLabel() == L"1" &&
					 (*nod2)->GetLabel() == L"1" &&
					 (*nod3)->GetLabel() == L"2";

	path++;
	nod1 = path->begin();
	nod2 = nod1 + 1;

	bool predPath2 = (*nod1)->GetLabel() == L"1" && 
					 (*nod2)->GetLabel() == L"2";	

	return predPath1 && predPath2;
}

bool Test_GetPathBetweenNodes_Cicle()
{
	ContextGraph cg;

	CNode n1(L"1");
	
	cg.AddEdge(L"e", n1, n1);
	cg.AddEdge(L"e1", n1, n1);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n1.GetLabel()), solutions);

	if (!bRet) return false;
	if (solutions.size() != 1) return false;

	auto path = solutions.begin();
	auto src = path->begin();
	auto dest = src + 1;
	
	return (*src)->GetLabel() == L"1" && 
		   (*dest)->GetLabel() == L"1";
}

bool Test_GetPathBetweenNodes_TransitiveCicles()
{
	ContextGraph cg;

	CNode n0(L"0");
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n4(L"4");
	
	cg.AddEdge(L"e", n1, n1);
	cg.AddEdge(L"e'", n1, n1);
	cg.AddEdge(L"e1", n1, n2);
	cg.AddEdge(L"e2", n2, n1);
	cg.AddEdge(L"e5", n0, n1);
	cg.AddEdge(L"e6", n1, n4);

	IContextGraph::Paths solutions([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2) { return l1.size() > l2.size(); });
	bool bRet = cg.GetPathBetweenNodes(cg.GetNodeByName(n0.GetLabel()), cg.GetNodeByName(n4.GetLabel()), solutions);

	if (!bRet) return false;

	return solutions.size() == 5;
}

bool Test_FindMaxOriginalPathMatchedByRegex_SingleEdge()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	cg.AddEdge(L"e", n1, n2);

	auto found = cg.FindMaxOriginalPathMatchedByRegex(L"e", cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()));

	return found.size() == 1 && (*found.begin())->GetLabel() == L"e";
}

bool Test_FindMaxOriginalPathMatchedByRegex_NoEdge()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	cg.AddEdge(L"e", n1, n2);

	auto found = cg.FindMaxOriginalPathMatchedByRegex(L"!e", cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()));

	return found.empty();
}

bool Test_FindMaxOriginalPathMatchedByRegex_ChooseMax()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e", n1, n3);
	cg.AddEdge(L"e", n3, n2);

	auto found = cg.FindMaxOriginalPathMatchedByRegex(L"e*", cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()));

	auto it = found.begin();
	return found.size() == 2 &&
		(*it)->GetSource() == n1 &&
		(*it)->GetDestination() == n3 &&
		(*++it)->GetSource() == n3 &&
		(*it)->GetDestination() == n2;
}

bool Test_FindAllOriginalPathsMatchedByRegex()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e", n1, n3);
	cg.AddEdge(L"e", n3, n2);

	auto found = cg.FindAllOriginalPathsMatchedByRegex(L"e*", cg.GetNodeByName(n1.GetLabel()), cg.GetNodeByName(n2.GetLabel()));

	auto fPath = found.begin();
	auto it = fPath->begin();
	bool bFirstPath = (1 == fPath->size() &&
					   (*it)->GetSource() == n1 &&
					   (*it)->GetDestination() ==n2) ||
					  (2 == fPath->size() &&
					   (*it)->GetSource() == n1 &&
					   (*it)->GetDestination() == n3 &&
					   (*++it)->GetSource() == n3 &&
					   (*it)->GetDestination() == n2);
		
	fPath++;
	it = fPath->begin();
	bool bSecondPath = (2 == fPath->size() &&
					    (*it)->GetSource() == n1 &&
					    (*it)->GetDestination() == n3 &&
					    (*++it)->GetSource() == n3 &&
					    (*it)->GetDestination() == n2) ||
					    (1 == fPath->size() &&
					    (*it)->GetSource() == n1 &&
					    (*it)->GetDestination() ==n2);

	return bFirstPath && bSecondPath;
}

bool Test_GetLabeledNodes_NoLabeled()
{
	ContextGraph cg;

	cg.AddEdge(L"e", L"?", L"?");

	auto nodes = cg.GetLabeledNodes();

	return nodes.size() == 0;
}

bool Test_GetLabeledNodes_BothLabeledAndUnlabeled()
{
	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"?");
	CNode n3(L"3");
	CNode n4(L"?1");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n2, n3);
	cg.AddEdge(L"e2", n3, n4);

	auto nodes = cg.GetLabeledNodes();
	auto it = nodes.begin();
	bool bFirst = (*it)->GetLabel() == n1.GetLabel() || (*it)->GetLabel() == n3.GetLabel();
	it++;
	bool bSecond = (*it)->GetLabel() == n1.GetLabel() || (*it)->GetLabel() == n3.GetLabel();

	return nodes.size() == 2 && bFirst && bSecond;
}

bool Test_GetPossibleUnknownNodes()
{
	ContextGraph pg;

	CNode np1(L"1");
	CNode np2(L"?");
	CNode np3(L"3");

	ContextGraph cg;

	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n2, n3);
	cg.AddEdge(L"e2", n3, n4);

	pg.AddEdge(L"e", np1, np2);
	pg.AddEdge(L"e1", np2, np3);

	auto nodes = cg._GetPossibleUnknownNodes(pg);

	auto it = nodes.begin();
	auto label1 = it->GetLabel();
	it++;
	auto label2 = it->GetLabel();
	return nodes.size() == 2 &&
		   ((label1 == n2.GetLabel() && label2 == n4.GetLabel()) ||
		   (label1 == n4.GetLabel() && label2 == n2.GetLabel()));
}

bool Test_GetCorrespondingConcreteEdges_ExactEdge()
{
	CNode n1(L"?1");
	CNode n2(L"2");
	CNode n3(L"?3");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n1, n3);
	cg.AddEdge(L"e", n2, n4);
	cg.AddEdge(L"e1", n2, n4);

	CEdge edge(L"e", n2, n4);
	ContextGraph ppg;
	ppg.AddEdge(edge);

	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));
	auto e = edges.begin();

	return edges.size() == 1 &&
		   (*e)->GetLabel() == edge.GetLabel() &&
		   (*e)->GetSource().GetLabel() == n2.GetLabel() &&
		   (*e)->GetDestination().GetLabel() == n4.GetLabel();
}

bool Test_GetCorrespondingConcreteEdges_UnknownSource()
{
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n1, n4);
	cg.AddEdge(L"e", n2, n4);
	cg.AddEdge(L"e1", n3, n4);

	CNode unknown(L"??");
	CEdge edge(L"e", unknown, n4);
	ContextGraph ppg;
	ppg.AddEdge(edge);

	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 2)
		return false;

	auto e1 = edges[0];
	auto e2 = edges[1];

	return     (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n1.GetLabel() &&
		   e1->GetDestination().GetLabel() == n4.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n4.GetLabel()) ||

		   (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n4.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n1.GetLabel() &&
		   e2->GetDestination().GetLabel() == n4.GetLabel());
}

bool Test_GetCorrespondingConcreteEdges_UnknownDestination()
{
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n2, n3);
	cg.AddEdge(L"e", n2, n4);
	cg.AddEdge(L"e", n3, n4);

	CNode unknown(L"??");
	CEdge edge(L"e", n2, unknown);
	ContextGraph ppg;
	ppg.AddEdge(edge);
	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 2)
		return false;

	auto e1 = edges[0];
	auto e2 = edges[1];

	return (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n3.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n4.GetLabel()) ||

		   (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n4.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n3.GetLabel());
}

bool Test_GetCorrespondingConcreteEdges_RegexSource()
{
	CNode n1(L"1");
	CNode n2(L"1 mare");
	CNode n3(L"mic 1");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n2, n3);
	cg.AddEdge(L"e", n3, n4);
	cg.AddEdge(L"e", n4, n1);

	CNode nn(L"1");
	CNode unknown(L"??");
	nn.SetRegex(L"^[\\w\\s]*1[\\w\\s]*$");;
	CEdge edge(L"e", nn, unknown);

	ContextGraph ppg;
	ppg.AddEdge(edge);

	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 2)
		return false;

	auto e1 = edges[0];
	auto e2 = edges[1];

	return (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n3.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n3.GetLabel() &&
		   e2->GetDestination().GetLabel() == n4.GetLabel()) ||

		   (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n3.GetLabel() &&
		   e1->GetDestination().GetLabel() == n4.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n3.GetLabel());
}

bool Test_GetCorrespondingConcreteEdges_RegexDestination()
{
	CNode n1(L"1");
	CNode n2(L"1 mare");
	CNode n3(L"mic 1");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n2, n2);
	cg.AddEdge(L"e", n4, n1);
	cg.AddEdge(L"e", n3, n4);

	CNode nn(L"1");
	CNode unknown(L"??");
	nn.SetRegex(L"^[\\w\\s]*1[\\w\\s]*$");
	CEdge edge(L"e", unknown, nn);
	ContextGraph ppg;
	ppg.AddEdge(edge);

	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 2)
		return false;

	auto e1 = edges[0];
	auto e2 = edges[1];

	return (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n2.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n4.GetLabel() &&
		   e2->GetDestination().GetLabel() == n1.GetLabel()) ||

		   (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n4.GetLabel() &&
		   e1->GetDestination().GetLabel() == n1.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n2.GetLabel());
}

bool Test_GetCorrespondingConcreteEdges_RegexSourceAndDestination()
{
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"a1");
	CNode n4(L"b2");

	ContextGraph cg;

	cg.AddEdge(L"e", n3, n4);
	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e", n1, n4);

	CNode nn1(L"1");
	CNode nn2(L"2");
	nn1.SetRegex(L"^[\\w]+1$");;
	nn2.SetRegex(L"^[\\w]+2$");;
	CEdge edge(L"e", nn1, nn2);
	ContextGraph ppg;
	ppg.AddEdge(edge);
	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 1)
		return false;

	auto e = edges[0];

	return e->GetLabel() == edge.GetLabel() && 
		   e->GetSource().GetLabel() == n3.GetLabel() &&
		   e->GetDestination().GetLabel() == n4.GetLabel();
}

bool Test_GetCorrespondingConcreteEdges_UnknownSourceAndDestination()
{
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");

	ContextGraph cg;

	cg.AddEdge(L"e", n2, n3);
	cg.AddEdge(L"e", n1, n4);

	CNode un1(L"??1"), un2(L"??2");
	CEdge edge(L"e", un1, un2);

	ContextGraph ppg;
	ppg.AddEdge(edge);
	auto edges = cg.GetCorrespondingConcreteEdges(edge, cg._GetPossibleUnknownNodes(ppg));

	if (edges.size() != 2)
		return false;

	auto e1 = edges[0];
	auto e2 = edges[1];

	return (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n2.GetLabel() &&
		   e1->GetDestination().GetLabel() == n3.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n1.GetLabel() &&
		   e2->GetDestination().GetLabel() == n4.GetLabel()) ||

		   (e1->GetLabel() == edge.GetLabel() &&
		   e1->GetSource().GetLabel() == n1.GetLabel() &&
		   e1->GetDestination().GetLabel() == n4.GetLabel() &&
		   e2->GetLabel() == edge.GetLabel() &&
		   e2->GetSource().GetLabel() == n2.GetLabel() &&
		   e2->GetDestination().GetLabel() == n3.GetLabel());
}

bool Test_ComputeConnexComponents_Two()
{
	ContextGraph cg;
	cg.AddEdge(L"e", L"2", L"4");
	cg.AddEdge(L"e1", L"4", L"1");
	cg.AddEdge(L"e2", L"1", L"2");
	cg.AddEdge(L"e3", L"3", L"2");
	cg.AddEdge(L"e4", L"5", L"6");
	cg.AddEdge(L"e5", L"4", L"7");
	cg.AddEdge(L"e6", L"2", L"2");
	cg.AddEdge(L"e6", L"4", L"4");

	auto connexComponents = cg.ComputeConnexComponents();
	
	auto c1 = connexComponents[0];
	auto c2 = connexComponents[1];
	return connexComponents.size() == 2 &&
		   (c1.size() == 7 || c1.size() == 1) &&
		   (c2.size() == 7 || c2.size() == 1) &&
		   c1.size() != c2.size();
}

bool Test_BuildFromDotFile()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test1.dot");

	return cg.m_graph.size() == 12;
}

bool Test_GetMaximumMatch()
{
	ContextGraph cg;
	cg.AddEdge(L"e", L"1", L"2");
	cg.AddEdge(L"e1", L"3", L"2");
	cg.AddEdge(L"e2", L"1", L"4");
	cg.AddEdge(L"e3", L"2", L"5");
	cg.AddEdge(L"e4", L"4", L"6");

	ContextGraph pg;
	pg.AddEdge(L"e", L"1", L"?1");
	pg.AddEdge(L"e1", L"?2", L"?3");
	pg.AddEdge(L"e3", L"?1", L"5");
	pg.AddEdge(L"e4", L"fake", L"stake");

	auto match = cg.GetMaximumMatch(pg);
	return match.size() == 1 && match.begin()->size() == 2;
}

bool Test_FindNodeMatchingNodeName()
{
	ContextGraph cg;
	cg.AddEdge(L"e", L"abc", L"abcd");
	cg.AddEdge(L"e", L"cba", L"abcd");
	cg.AddEdge(L"e", L"abcd", L"abcdef");

	CNode node(L"");
	node.SetRegex(L"ab.*");
	auto matches = cg.FindNodesMatchingNodeName(node);

	return matches.size() == 3;
}

bool Test_MatchGraphWithQuestionMarks()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test2G.dot");
	ContextGraph pg;
	pg.BuildFromDotFile(L"test2P.dot");

	auto match = cg.GetMaximumMatch(pg);

	return match.size() == 2;
}

bool Test_GenerateRandomSubgraph()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test2G.dot");
	cg.GenerateRandomSubgraph(80, 80);

	return true;
}

bool Test_GenerateRandomSubgraph_MandatoryNodes()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test2G.dot");
	std::set<std::wstring> nodes;
	nodes.emplace(L"AIConf");
	nodes.emplace(L"conftime");
	cg.GenerateRandomSubgraph(80, 80, 0, nodes);

	return true;
}

bool Test_RemoveNode()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test_biggraph.dot");
	cg.RemoveNode(L"Schedule");

	return true;
}

bool Test_ReplaceNode()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test_biggraph.dot");
	cg.ReplaceNode(L"Schedule", L"?1");

	return true;
}

bool Test_GetMaximumMatchSameSolution()
{
	ContextGraph cg;
	cg.AddEdge(L"is", L"John", L"Doctor");
	cg.AddEdge(L"is", L"Vasile", L"Medic");

	ContextGraph pg;
	pg.AddEdge(L"is", L"?1", L"?2");
	pg.AddEdge(L"is", L"?3", L"?4");

	auto edges = cg.GetMaximumMatch(pg);
	return edges.size() == 1;
}

bool Test_MatchMultipleEdges()
{
	ContextGraph cg;
	cg.AddEdge(L"friend", L"Dan", L"John");
	cg.AddEdge(L"friend", L"Dan", L"John");
	cg.AddEdge(L"friend", L"Dan", L"John");

	ContextGraph pg;
	pg.AddEdge(L"friend", L"?1", L"?2");
	pg.AddEdge(L"friend", L"?1", L"?2");

	ContextGraph pg1;
	pg1.AddEdge(L"friend", L"?1", L"?2");
	pg1.AddEdge(L"friend", L"?2", L"?1");

	auto edges = cg.GetMaximumMatch(pg);
	auto edges1 = cg.GetMaximumMatch(pg1);

	return edges.size() == 3 && edges1.size() == 3;
}

bool Test_BigGraphMatching()
{
	ContextGraph cg;
	cg.BuildFromDotFile(L"test_biggraph.dot");
	std::set<std::wstring> nodes;
	nodes.emplace(L"Andrei");
	nodes.emplace(L"Today Schedule");

	auto pg = cg.GenerateRandomSubgraph(50, 100, 20, nodes);
	pg.WriteGraphToDotFile(L"pattern.dot", L"AndreiPhonePattern");

	auto edges = cg.GetMaximumMatch(pg);
	return true;
}

void Test_DeleteEdge()
{
	ContextGraph cg;
	cg.AddEdge(L"e", L"1", L"2");
	cg.AddEdge(L"e", L"1", L"2");
	cg._DeleteEdge(*cg.m_edges.begin());
}

void Test_RefreshGraphConsistency()
{
	ContextGraph cg;
	CNode n1(L"1");
	CNode n2(L"2");
	cg.AddEdge(L"e", n1, n2, std::chrono::system_clock::from_time_t(1890));
	cg.AddEdge(L"e", n1, n2, std::chrono::system_clock::from_time_t(2576));
	cg.AddEdge(L"e1", L"2", L"3");

	cg.RefreshGraphConsistency();
}

void Test_PrecomputeRoadsBetweenPairOfNodes()
{
	ContextGraph cg;
	CNode n1(L"1");
	CNode n2(L"2");
	CNode n3(L"3");
	CNode n4(L"4");
	CNode n5(L"5");
	CNode n6(L"6");
	CNode n7(L"7");
	CNode n8(L"8");

	cg.AddEdge(L"e", n1, n2);
	cg.AddEdge(L"e1", n1, n2);
	cg.AddEdge(L"e3", n1, n3);
	cg.AddEdge(L"e4", n3, n2);
	cg.AddEdge(L"e5", n3, n4);
	cg.AddEdge(L"e6", n2, n4);
	cg.AddEdge(L"e7", n5, n7);
	cg.AddEdge(L"e8", n6, n8);
	cg.AddEdge(L"e9", n8, n7);
	cg.AddEdge(L"e10", n1, n1);

	cg.PrecomputeRoadsBetweenPairOfNodes();
}

void Test_PrecomputeRoadsBetweenPairOfNodes_Performance(int nrNodes)
{
	ContextGraph cg;
	std::vector<CNode> nodes;
	for (int i = 0; i < nrNodes; i++)
	{
		nodes.emplace_back(std::to_wstring(i));
	}

	int nrEdge = 0;
	for (int i = 0; i < nrNodes; i++)
	{
		for (int j = 0; j < nrNodes; j++)
		{
			cg.AddEdge(std::to_wstring(nrEdge++), nodes[i], nodes[j]);
		}
	}
	
	cg.PrecomputeRoadsBetweenPairOfNodes();

	IContextGraph::Paths roads([] (const std::vector<CNode const *> &l1, const std::vector<CNode const *> &l2)
	{ return l1.size() > l2.size(); 
	});
	for (unsigned int i = 0; i < nodes.size(); i++)
		for (unsigned int j = 0; j < nodes.size(); j++)
			cg.GetPathBetweenNodes(cg.GetNodeByName(nodes[i].GetLabel()), cg.GetNodeByName(nodes[j].GetLabel()), roads);
}

int main()
{
	Test_DeleteEdge();
	Test_RefreshGraphConsistency();
	Test_PrecomputeRoadsBetweenPairOfNodes();
	Test_PrecomputeRoadsBetweenPairOfNodes_Performance(3);

	if (Test_AddStringEdge())
		std::cout << "OK 1\n";
	if (Test_AddDistinctEdges())
		std::cout << "OK 2\n";
	if (Test_AddDifferentEdgeBetweenSameNodes())
		std::cout << "OK 3\n";
	if (Test_AddSameEdgeBetweenSameNodes())
		std::cout << "OK 4\n";
	if (Test_AddUnknownNode())
		std::cout << "OK 5\n";

	if (Test_GetPathBetweenNodes_NoPath())
		std::cout << "OK 6\n";
	if (Test_GetPathBetweenNodes_DirectEdge())
		std::cout << "OK 7\n";
	if (Test_GetPathBetweenNodes_DirectMultipleEdges())
		std::cout << "OK 8\n";
	if (Test_GetPathBetweenNodes_TransitivePath())
		std::cout << "OK 9\n";
	if (Test_GetPathBetweenNodes_MultipleTransitivePathsSameSize())
		std::cout << "OK 10\n";
	if (Test_GetPathBetweenNodes_MultipleTransitivePathsDifferentSize())
		std::cout << "OK 11\n";
	if (Test_GetPathBetweenNodes_CombinationWithSameNode())
		std::cout << "OK 12\n";
	if (Test_GetPathBetweenNodes_Cicle())
		std::cout << "OK 13\n";
	if (Test_GetPathBetweenNodes_TransitiveCicles())
		std::cout << "OK 14\n";

	if (Test_FindMaxOriginalPathMatchedByRegex_SingleEdge())
		std::cout << "OK 15\n";
	if (Test_FindMaxOriginalPathMatchedByRegex_NoEdge())
		std::cout << "OK 16\n";
	if (Test_FindMaxOriginalPathMatchedByRegex_ChooseMax())
		std::cout << "OK 17\n";
	
	if (Test_FindAllOriginalPathsMatchedByRegex())
		std::cout << "OK 18\n";

	if (Test_GetLabeledNodes_NoLabeled())
		std::cout << "OK 19\n";
	if (Test_GetLabeledNodes_BothLabeledAndUnlabeled())
		std::cout << "OK 20\n";
	if (Test_GetPossibleUnknownNodes())
		std::cout << "OK 21\n";

	if (Test_GetCorrespondingConcreteEdges_ExactEdge())
		std::cout << "OK 22\n";
	if (Test_GetCorrespondingConcreteEdges_UnknownSource())
		std::cout << "OK 23\n";
	if (Test_GetCorrespondingConcreteEdges_UnknownDestination())
		std::cout << "OK 24\n";
	if (Test_GetCorrespondingConcreteEdges_UnknownSourceAndDestination())
		std::cout << "OK 25\n";
	if (Test_GetCorrespondingConcreteEdges_RegexSource())
		std::cout << "OK 26\n";
	if (Test_GetCorrespondingConcreteEdges_RegexDestination())
		std::cout << "OK 27\n";
	if (Test_GetCorrespondingConcreteEdges_RegexSourceAndDestination())
		std::cout << "OK 28\n";

	if (Test_ComputeConnexComponents_Two())
		std::cout << "OK 29\n";

	if (Test_BuildFromDotFile())
		std::cout << "OK 30\n";
	if (Test_GetMaximumMatch())
		std::cout << "OK 31\n";

	if (Test_FindNodeMatchingNodeName())
		std::cout << "OK 32\n";
	if (Test_MatchGraphWithQuestionMarks())
		std::cout << "OK 33 \n";
	if (Test_GenerateRandomSubgraph())
		std::cout << "OK 34 \n";
	if (Test_GenerateRandomSubgraph_MandatoryNodes())
		std::cout << "OK 35 \n";
	if (Test_RemoveNode())
		std::cout << "OK 36 \n";

	if (Test_ReplaceNode())
		std::cout << "OK 37 \n";

	if (Test_GetMaximumMatchSameSolution())
		std::cout << "OK 38 \n";
	if (Test_MatchMultipleEdges())
		std::cout << "OK 39 \n";
	if (Test_BigGraphMatching())
		std::cout << "OK 40 \n";
	
	return 0;
}
