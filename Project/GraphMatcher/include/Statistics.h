#pragma once

struct Statistics
{
	Statistics() : nrExpanded(0),
				   nrVisited(0),
				   nrComparations(0),
				   nrFullComparations(0),
				   nrCopyConstructor(0),
				   nrMoveConstructor(0),
				   nrAssignmentOperator(0),
				   nrMoveAssignmentOperator(0),
				   nrConstructor(0),
				   nrDestructor(0)
	{}

	int nrExpanded;
	int nrVisited;
	int nrComparations;
	int nrFullComparations;
	int nrCopyConstructor;
	int nrMoveConstructor;
	int nrAssignmentOperator;
	int nrMoveAssignmentOperator;
	int nrConstructor;
	int nrDestructor;
};