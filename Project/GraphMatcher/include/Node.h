#pragma once

#include "Statistics.h"

template <typename N>
class Node
{
public:
	Node(/* in */ const N &node,
		 /* in */ const std::function<const std::wstring(const N &)> toString =
			   [] (/* in */ const N &n) { return n; })
	:m_node(node),
	 m_label(toString(node)),
	 m_bUnknown(false),
	 m_bHasCorrespondent(false),
	 m_bHasAssignment(false),
         m_correspondent(nullptr),
         m_bRegex(false)
	{
		if (m_label.empty() || m_label.front() == L'?')
			m_bUnknown = true;
	}

	inline const std::wstring GetLabel(void) const { return m_label; }
	inline void SetUnknown(/* in */ bool bUnknown = true, std::wstring newLabel = L"") { m_bUnknown = bUnknown; m_label = newLabel; }
	inline bool IsUnknown(void) const { return m_bUnknown; }
	inline const N& GetNode(void) const { return m_node; }
	inline bool HasAssignment(void) const { return m_bHasAssignment; }
	inline void SetAssignment(/* in */ bool bHasAssignment) { m_bHasAssignment = bHasAssignment; }
	inline bool operator== (/* in */ const Node<N> &other) const { return m_label == other.GetLabel(); }
	inline bool operator!= (/* in */ const Node<N> &other) const { return m_label != other.GetLabel(); }
	inline bool IsRegex(void) const { return m_bRegex; }
	inline void SetRegex(/* in */ std::wstring regex) { m_bRegex = true; m_label = regex; }
	inline void SetCorespondent(/* in */ bool bCorrespondent, /* in */ const Node<N> *correspondent = nullptr) 
	{ 
		m_bHasCorrespondent = bCorrespondent;
		if (correspondent) 
			m_correspondent = const_cast<Node<N> *>(correspondent); 
	}
	inline bool HasCorrespondent(void) const { return m_bHasCorrespondent; }
	inline const Node<N> &GetCorrespondent(void) const { return *m_correspondent; }

	friend std::wostream& operator<<(/* inout */ std::wostream &stream, /* in */ const Node &n)
	{
		stream << n.GetLabel();
		return stream;
	}

private:
	N m_node;
	std::wstring m_label;
	bool m_bUnknown;
        bool m_bHasCorrespondent;
	bool m_bHasAssignment;
	Node<N> *m_correspondent;
        bool m_bRegex;
};

typedef Node<std::wstring> CNode;
static const CNode NIL = CNode(L"?");