#pragma once

#include "Node.h"
#include "Statistics.h"

template <typename E, typename N>
class Edge
{
public:
	explicit Edge(/* in */ const E &edge,
		 /* in */ const Node<N> & source,
		 /* in */ const Node<N> & destination,
		 /* in */ std::chrono::system_clock::time_point expireTime = NEVER_EXPIRE,
		 /* in */ Duration duration = PERMANENT_DURATION,
		 /* in */ const std::function<const std::wstring(const E &)> toString = 
		 [] (/* in */ const E &e) { return e; })	
	:m_edge(edge),
	 m_label(toString(edge)),
	 m_bRegex(false),
	 m_bAlreadyUsed(false),
	 m_expireTime(expireTime),
	 m_duration(duration),
         m_nodes(&source, &destination)
	{
		m_expirationFrames.emplace(m_expireTime);
	}

	Edge(/* in */ const E &edge,
		 /* in */ const Node<N> && source,
		 /* in */ const Node<N> && destination,
		 /* in */ time_t expireTime = 0,
		 /* in */ Duration duration = PERMANENT_DURATION,
		 /* in */ const std::function<const std::wstring(const E &)> toString = 
		 [] (/* in */ const E &e) { return e; }
	) = delete;
	Edge(/* in */ const E &edge,
		 /* in */ const Node<N> & source,
		 /* in */ const Node<N> && destination,
		 /* in */ time_t expireTime = 0,
		 /* in */ Duration duration = PERMANENT_DURATION,
		 /* in */ const std::function<const std::wstring(const E &)> toString = 
		 [] (/* in */ const E &e) { return e; }
	) = delete;
	Edge(/* in */ const E &edge,
		 /* in */ const Node<N> && source,
		 /* in */ const Node<N> & destination,
		 /* in */ time_t expireTime = 0,
		 /* in */ Duration duration = PERMANENT_DURATION,
		 /* in */ const std::function<const std::wstring(const E &)> toString = 
		 [] (/* in */ const E &e) { return e; }
	) = delete;

	 Edge(/* in */ const E &edge,
		  /* in */ const std::function<const std::wstring(const E &)> toString = 
		  [] (/* in */ const E &e) { return e; }) 
	 : Edge(edge, NIL, NIL) { }

	inline const std::wstring &GetLabel(void) const { return m_label; }
	inline const std::pair<const Node<N> *, const Node<N> *>& GetNodes(void) const { return m_nodes; }
	inline const E& GetEdge(void) const { return m_edge; }
	inline const Node<N> & GetSource(void) const { return *m_nodes.first; }
	inline const Node<N> & GetDestination(void) const { return *m_nodes.second; }
	inline bool IsRegex(void) const { return m_bRegex; }
	inline void SetRegex(/* in */ std::wstring regex) { m_bRegex = true; m_label = regex; }
	inline bool operator== (/* in */ const Edge &other) const { return m_label == other.m_label; }
	inline bool operator== (/* in */ const std::wstring &label) const { return m_label == label; }
	inline friend bool operator==(/* in */ const std::wstring &label, /* in */ const Edge &other) { return label == other.m_label; }
	inline bool FullEqual (/* in */ const Edge &other) const { return m_label == other.m_label && *m_nodes.first == *other.m_nodes.first && *m_nodes.second == *other.m_nodes.second; }
	inline bool FullEqual (/* in */ const std::wstring &label, /* in */ const CNode &source, /* in */ const CNode &dest) const
	{ return m_label == label && *m_nodes.first == source && *m_nodes.second == dest; }
	inline void SetNodes(/* in */ const CNode &source, /* in */ const CNode &dest) { m_nodes.first = &source; m_nodes.second = &dest; }
	inline void SetAlreayUsed(/* in */ bool bUsed) { m_bAlreadyUsed = bUsed; }
	inline bool IsAlreadyUsed(void) const { return m_bAlreadyUsed; }
	inline bool IsExpired(void) const { return m_expireTime < std::chrono::system_clock::now(); }
	void RefreshExpiration(void)
	{
		auto now = std::chrono::system_clock::now();
		auto exp = m_expirationFrames.begin();
		for (; exp != m_expirationFrames.end(); exp++)
		{
			if (*exp > now)
				break;
		}

		m_expirationFrames.erase(m_expirationFrames.cbegin(), m_expirationFrames.cend());
	}
	
	inline void AddExpirationTime(/* in */ std::chrono::system_clock::time_point expireTime) 
	{
		m_expirationFrames.emplace(expireTime);
		m_expireTime = expireTime > m_expireTime ? expireTime : m_expireTime;
	}
	inline std::chrono::system_clock::time_point GetFirstExpirationTime(void) const { return *m_expirationFrames.begin(); }
	inline std::chrono::system_clock::time_point GetLastExpirationTime(void) const { return *(--m_expirationFrames.end()); }
	inline Duration GetDuration() const { return m_duration; }

	friend std::wostream& operator<<(/* inout */ std::wostream &stream, /* in */ const Edge &e)
	{
		stream << e.GetLabel() << "-> [ " << e.GetSource().GetLabel() << ", " << e.GetDestination().GetLabel() << "]\n";
		return stream;
	}

public:

	Edge() { };

	E m_edge;
        std::wstring m_label;
	bool m_bRegex;
	bool m_bAlreadyUsed;
	std::chrono::system_clock::time_point m_expireTime;
	std::set<std::chrono::system_clock::time_point> m_expirationFrames;
	Duration m_duration;
        std::pair<const Node<N> *, const Node<N> *> m_nodes;
};

typedef Edge<std::wstring, std::wstring> CEdge;
static const CEdge VOID_EDGE = CEdge(L"", NIL, NIL);
