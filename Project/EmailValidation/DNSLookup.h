#pragma once

#include <winsock2.h>
#include <windns.h>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

class CDNSLookup
{
	public:
		CDNSLookup(LPWSTR = NULL);
		virtual ~CDNSLookup();	
		virtual HRESULT query();
		virtual HRESULT updateHostList();
		virtual std::vector<std::wstring> getEmailServerNames();
		virtual LPWSTR getDomainName();
		virtual BOOL isDomainValid();
		virtual BOOL getLookupStatus();
		virtual DNS_STATUS myDnsQuery_W( __in PCWSTR pszName, __in WORD wType,
			__in DWORD Options,__inout_opt PVOID pExtra,__deref_out_opt PDNS_RECORD * ppQueryResults,
			__deref_opt_out_opt PVOID * pReserved);

		BOOL m_bnoInternetConnection;
	private:

		HRESULT nextEmailServer();

		DNS_RECORD* m_Result;
		std::vector<std::wstring> m_HostList ;
		LPWSTR m_Domain;
		BOOL m_LookupDone;

};