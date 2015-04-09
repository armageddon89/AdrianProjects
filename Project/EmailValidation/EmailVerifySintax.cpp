#include "EmailVerifySintax.h"

CEmailVerifySintax::CEmailVerifySintax(wchar_t *a_pwzMailAddress) : 
	m_pwzDotChars(L"\x002E\x3002\xff0e\xff61"),
	m_pwzAllowedCharsInLocalPart(L"!#$%&'*+-/=?^_`{|}~"),
	m_pwzForbiddenChars(L"\r\n"),
	m_pwzQuotedChars(L"<>()[],:; \"")
{
	m_pwzAddressToVerify = a_pwzMailAddress;
	m_bIsQuotedLocalPart = FALSE;
	if(isQuote(m_pwzAddressToVerify[0]))
		m_bIsQuotedLocalPart = TRUE;
	m_bValidAddress = FALSE;
	m_pwzLocalPart = NULL;
	m_pwzDomainName = NULL;
	initTopLevelDomain();
}
	
CEmailVerifySintax::~CEmailVerifySintax(void)
{
	delete m_pwzLocalPart;
	m_pwzLocalPart = NULL;
	delete m_pwzDomainName;
	m_pwzDomainName = NULL;
}

BOOL CEmailVerifySintax::hasValidSintax()
{
	return m_bValidAddress;
}

wchar_t *CEmailVerifySintax::getDomainName()
{
	return m_pwzDomainName;
}

wchar_t *CEmailVerifySintax::getLocalPart()
{
	return m_pwzLocalPart;
}

wchar_t *CEmailVerifySintax::getAddressToVerify()
{
	return m_pwzAddressToVerify;
}

BOOL CEmailVerifySintax::parse()
{
	if(!trivialChecks()){
		fwprintf(stderr, L"trivial check error\n");
		return FALSE;
	}
	else
	{
		if(!checkLocalPart())
			return FALSE;
		if(!checkDomain(m_pwzDomainName))
			return FALSE;
	}

	m_bValidAddress = TRUE;
	return TRUE;
}

BOOL CEmailVerifySintax::checkFirstCharIsDot()
{
	if(wcschr(m_pwzDotChars, m_pwzAddressToVerify[0]))
		return FALSE;
	return TRUE;
}

BOOL CEmailVerifySintax::checkTotalLength()
{
	if(320 < wcslen(m_pwzAddressToVerify))
		return FALSE;
	return TRUE;
}

BOOL CEmailVerifySintax::checkHasMinimOneARond()
{
	if(!wcschr(m_pwzAddressToVerify, '@'))
		return FALSE;
	return TRUE;
}

HRESULT CEmailVerifySintax::initTopLevelDomain()
{
	m_vTop_level_domains.push_back(L"aero");
	m_vTop_level_domains.push_back(L"asia");
	m_vTop_level_domains.push_back(L"biz");
	m_vTop_level_domains.push_back(L"cat");
	m_vTop_level_domains.push_back(L"com");
	m_vTop_level_domains.push_back(L"coop");
	m_vTop_level_domains.push_back(L"edu");
	m_vTop_level_domains.push_back(L"gov");
	m_vTop_level_domains.push_back(L"info");
	m_vTop_level_domains.push_back(L"int");
	m_vTop_level_domains.push_back(L"jobs");
	m_vTop_level_domains.push_back(L"mil");
	m_vTop_level_domains.push_back(L"museum");
	m_vTop_level_domains.push_back(L"name");
	m_vTop_level_domains.push_back(L"net");
	m_vTop_level_domains.push_back(L"mobi");
	m_vTop_level_domains.push_back(L"net");
	m_vTop_level_domains.push_back(L"org");
	m_vTop_level_domains.push_back(L"pro");
	m_vTop_level_domains.push_back(L"tel");
	m_vTop_level_domains.push_back(L"travel");
	m_vTop_level_domains.push_back(L"xxx");

	return S_OK;
}

BOOL CEmailVerifySintax::domainNameHasLegalLength(wchar_t* domain)
{
	if(wcslen(domain) <= 255) return TRUE;
	else return FALSE;
}

BOOL CEmailVerifySintax::check2PointsConsecutive()
{
	if(wcsstr(m_pwzAddressToVerify, L".."))
		return FALSE;
	return TRUE;
}

BOOL CEmailVerifySintax::trivialChecks()
{
	if(!checkFirstCharIsDot())
		return FALSE;
	if(!checkTotalLength())
		return FALSE;
	if(!checkHasMinimOneARond())
		return FALSE;
	if(!check2PointsConsecutive())
		return FALSE;
	
	return TRUE;
}

BOOL CEmailVerifySintax::isQuote(wchar_t a_wcCharacter)
{
	return a_wcCharacter == '\"';
}

BOOL CEmailVerifySintax::isARond(wchar_t a_wcCharacter)
{
	return a_wcCharacter == '@';
}

BOOL CEmailVerifySintax::isBackslash(wchar_t a_wcCharacter)
{
	return a_wcCharacter == '\\';
}

wchar_t CEmailVerifySintax::nextChar(DWORD index)
{
	return m_pwzAddressToVerify[index + 1];
}

wchar_t CEmailVerifySintax::previousChar(DWORD index)
{
	return m_pwzAddressToVerify[index - 1];
}

BOOL CEmailVerifySintax::isEndOfString(DWORD index)
{
	return m_pwzAddressToVerify[index] == '\0';
}

BOOL CEmailVerifySintax::isForbiddenChar(wchar_t a_wcCharacter)
{
	if(a_wcCharacter == '\0')
		return FALSE;
	if(wcschr(m_pwzForbiddenChars, a_wcCharacter))
		return TRUE;
	return FALSE;
}

BOOL CEmailVerifySintax::isQuotedChar(wchar_t a_wcCharacter)
{
	if(wcschr(m_pwzQuotedChars, a_wcCharacter))
		return TRUE;
	return FALSE;
}

BOOL CEmailVerifySintax::isAllowedChar(wchar_t a_wcCharacter)
{
	if(wcschr(m_pwzAllowedCharsInLocalPart, a_wcCharacter))
		return TRUE;
	return FALSE;
}

VOID CEmailVerifySintax::formLocalPart(DWORD index)
{
	m_pwzLocalPart = new wchar_t[index + 2];
	wcsncpy_s(m_pwzLocalPart, index + 2, m_pwzAddressToVerify, index);
	m_pwzLocalPart[index + 1] = '\0';
}

VOID CEmailVerifySintax::formDomainName(DWORD index)
{
	DWORD dwLen = (DWORD)wcslen(&m_pwzAddressToVerify[index + 1]);
	m_pwzDomainName = new wchar_t[dwLen + 1];
	wcscpy_s(m_pwzDomainName, dwLen + 1, &m_pwzAddressToVerify[index + 1]);
}

BOOL CEmailVerifySintax::checkLocalPart()
{
	int i = 0;
	wchar_t wcCurrentChar;
	do{
		wcCurrentChar = m_pwzAddressToVerify[i];

		if(isQuote(wcCurrentChar) && i != 0 && FALSE == isARond(nextChar(i))
			&& FALSE == isBackslash(previousChar(i)))
		{
			return FALSE;
		}

		if(isForbiddenChar(wcCurrentChar))
			return FALSE;

		if(i > 0 && isQuotedChar(wcCurrentChar) && !isBackslash(previousChar(i)) && !m_bIsQuotedLocalPart)
			return FALSE;

		if( (iswcntrl(wcCurrentChar) && !isBackslash(wcCurrentChar) && !isAllowedChar(wcCurrentChar))
			|| (!iswgraph(wcCurrentChar) && !iswspace(wcCurrentChar)) || !iswprint(wcCurrentChar))
			return FALSE;

		if(isARond(wcCurrentChar) && m_bIsQuotedLocalPart && !isQuote(previousChar(i)) && 
			!isBackslash(previousChar(i)))
		{
			return FALSE;
		}

		if(isARond(wcCurrentChar) && !isBackslash(previousChar(i)))
		{
			formLocalPart(i);
			if(!checkLocalPartLength())
				return FALSE;
			formDomainName(i);
			break;
		}

		i++;

	}while(!isEndOfString(i));

	return TRUE;
}

BOOL CEmailVerifySintax::checkLocalPartLength()
{
	if(wcslen(m_pwzLocalPart) > 64)
		return FALSE;
	return TRUE;
}

BOOL CEmailVerifySintax::domainLabelHaveLegalLength(std::vector<std::wstring> domain_labels)
{
	for(DWORD i = 0 ; i < domain_labels.size() ; ++i)
		if(domain_labels[i].size() > 64) 
			return FALSE;
	return TRUE;
}

std::vector<std::wstring> CEmailVerifySintax::getDomainLabels(wchar_t* domain)
{
	std::vector<std::wstring> result;
	std::wstring test(domain);
	std::wstring temp = L"";
	int nr_crt = 0 ;
	while(domain[nr_crt] != '\0')
	{
		if(wcschr(m_pwzDotChars, domain[nr_crt]))
		{
			result.push_back(temp);
			temp = L"";
		}
		else 
		{
			temp += test[nr_crt];
		}
		nr_crt++;
	}
	result.push_back(temp);

	return result;
}

BOOL CEmailVerifySintax::checkLabelFirstLetter(std::wstring label)
{
	if(label.size() < 1) 
		return TRUE;

	if( ('a' <= label[0] && label[0] <= 'z') ||  ('A' <= label[0] && label[0] <= 'Z') ||
		('0' <= label[0] && label[0] <= '9') || label[0] == '_')  
	{
		return TRUE;
	}

	if(label[0] > 255 && iswprint(label[0]))
		return TRUE;

	return FALSE;
}

BOOL CEmailVerifySintax::checkLabelLetters(std::wstring label)
{
	BOOL res = TRUE;

	if(label.size() < 2) return res;

	for(DWORD i = 1 ; i < label.size() ; ++i)
		if( ('a' <= label[i] && label[i] <= 'z') ||  ('A' <= label[i] && label[i] <= 'Z') ||
			('0' <= label[i] && label[i] <= '9') || (label[i] == '-') || label[i] == '_')  {}
		else 
			if(label[i] < 255 || (label[i] > 255 && !iswprint(label[i])))
				res = FALSE;

	return res;
}


BOOL CEmailVerifySintax::checkIfIPAddress(wchar_t* domain)
{
	if(wcslen(domain) < 1) 
		return FALSE;
	if(domain[0] != L'[') 
		return FALSE;

	std::vector<std::wstring> labels = getDomainLabels(domain);
	if(labels.size() != 4) 
		return FALSE;

	std::wstring temp = labels[0];
	
	int IP[4] = {0,0,0,0};

	for(DWORD i = 1 ; i < labels[0].size() ; ++i)
		if('0' <= labels[0][i] && labels[0][i] <= '9') 
			IP[0] = IP[0] * 10 + (labels[0][i] - '0');
		else 
			return FALSE;
	for(DWORD j = 1 ; j <= 2 ; ++j)
		for(DWORD i = 0 ; i < labels[j].size() ; ++i)
			if('0' <= labels[j][i] && labels[j][i] <= '9') 
				IP[j] = IP[j] * 10 + (labels[j][i] - '0');
			else 
				return FALSE;	
	for(DWORD i = 0 ; i < labels[3].size() - 1 ; ++i)
		if('0' <= labels[3][i] && labels[3][i] <= '9') 
			IP[3] = IP[3] * 10 + (labels[3][i] - '0');
		else 
			return FALSE;
	if(domain[wcslen(domain) - 1] != L']') 
		return FALSE;
	
	for(DWORD i = 0; i < 4; i++)
	{
		if(IP[i] > 255)
			return FALSE;
	}

	return TRUE;
	
}

BOOL CEmailVerifySintax::checkDomain(wchar_t* domain)
{
	if(NULL == domain)
		return FALSE;
	if(wcslen(domain) < 1) 
		return FALSE;
	if(domainNameHasLegalLength(domain) == FALSE) 
		return FALSE;

	std::vector<std::wstring> labels = getDomainLabels(domain);

	if(domainLabelHaveLegalLength(labels) == FALSE) 
		return FALSE;

	if(domain[0] == '[') 
		return checkIfIPAddress(domain);
	else
	{
		for(DWORD i = 0 ; i < labels.size() ; ++i)
		{
			if(checkLabelFirstLetter(labels[i]) == FALSE) 
				return FALSE;
			if(checkLabelLetters(labels[i]) == FALSE) 
				return FALSE;
		}
		if(wcschr(m_pwzDotChars, domain[wcslen(domain) - 1]))
		{
			if(labels.size() <= 1) 
				return FALSE;
			else 
			{
				int ok = 0;
				for(DWORD i = 0 ; i < m_vTop_level_domains.size() ; ++i)
					if(labels[labels.size() - 2].compare( m_vTop_level_domains[i]) == 0) ok = 1;

				if(ok == 0) 
					return FALSE;
			}
		}
	}
	return TRUE;

}