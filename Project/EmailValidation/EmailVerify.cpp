#include "DNSLookup.h"
#include "EmailVerifySintax.h"
#include "EmailVerify.h"
#include <tchar.h>

//S_OK - if valid
//S_FALSE - if not valid
HRESULT testEmailAddress(wchar_t *a_pwzEmailAddress)
{
	CEmailVerifySintax emailSintaxChecker(a_pwzEmailAddress);
	emailSintaxChecker.parse();
	if(!emailSintaxChecker.hasValidSintax())
	{
		fwprintf_s(stderr, L"incorrect sintax\n");
		return S_FALSE;
	}
	else
	{
		CDNSLookup dnsLookup(emailSintaxChecker.getDomainName());
		if(!dnsLookup.isDomainValid())
		{
			if(dnsLookup.m_bnoInternetConnection == FALSE) 
				return S_OK;
			fwprintf_s(stderr, L"domain name not found\n");
			return S_FALSE;
		}
	}
	return S_OK;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc != 2)
	{
		wprintf(L"incorrect number of arguments\n");
		ExitProcess(EXIT_FAILURE);
	}

	if(testEmailAddress(argv[1]) == S_OK)
		wprintf(L"VALID!\n");
	return 0;
}