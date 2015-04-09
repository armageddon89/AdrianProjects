#include "DNSLookup.h"

CDNSLookup::CDNSLookup(LPWSTR domain ): m_Result(NULL) , m_Domain(domain), m_LookupDone(FALSE),m_bnoInternetConnection(TRUE)
{
}

CDNSLookup::~CDNSLookup()
{
	 DnsRecordListFree(m_Result, DnsFreeRecordListDeep);
}

BOOL CDNSLookup::getLookupStatus()
{
	return m_LookupDone;
}

HRESULT CDNSLookup::query()
{
	DNS_STATUS m_Status = myDnsQuery_W(m_Domain, DNS_TYPE_MX, DNS_QUERY_STANDARD, NULL, &m_Result, NULL);
	
	m_LookupDone = TRUE;

	if( m_Status != ERROR_SUCCESS )
	{
		m_bnoInternetConnection = FALSE;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CDNSLookup::nextEmailServer()
{
	if((m_Result != NULL && m_Result->pNext!=NULL) && (m_Result->pNext != m_Result))
	{
		if (DNS_TYPE_MX == m_Result->wType)
		{
			m_HostList.push_back((std::wstring)m_Result->Data.MX.pNameExchange);
		}
                           
		m_Result = m_Result->pNext; 
		return S_OK;
	}
	else
		return S_FALSE;
}

LPWSTR CDNSLookup::getDomainName()
{
	return m_Domain;
}

std::vector<std::wstring> CDNSLookup::getEmailServerNames()
{
	return m_HostList;
}

BOOL CDNSLookup::isDomainValid()
{
	if(m_LookupDone == FALSE)
	{
		if(query() == S_FALSE) 
			return FALSE;
		updateHostList();
	}

	if(m_HostList.size() == 0 ) 
		return FALSE;

	return TRUE;

}

HRESULT CDNSLookup::updateHostList()
{
	if(m_LookupDone == FALSE) 
		return S_FALSE;

	m_HostList.clear();

	while(nextEmailServer() == S_OK);

	return S_OK;
}

DNS_STATUS CDNSLookup::myDnsQuery_W( __in PCWSTR pszName, __in WORD wType,__in DWORD Options,
	__inout_opt PVOID pExtra,__deref_out_opt PDNS_RECORD * ppQueryResults,__deref_opt_out_opt PVOID * pReserved)
{
	return DnsQuery_W(pszName,wType,Options,pExtra,ppQueryResults,pReserved);
}