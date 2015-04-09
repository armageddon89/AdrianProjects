#pragma once
#include <Windows.h>
#include <cctype>
#include <cstdio>
#include <vector>

class CEmailVerifySintax
{
	
	wchar_t *m_pwzAddressToVerify;
	wchar_t *m_pwzDotChars;
	wchar_t *m_pwzForbiddenChars;
	wchar_t *m_pwzAllowedCharsInLocalPart;
	wchar_t *m_pwzQuotedChars;
	wchar_t *m_pwzLocalPart;
	wchar_t *m_pwzDomainName;
	BOOL m_bValidAddress;
	BOOL m_bIsQuotedLocalPart;

public:

	std::vector<std::wstring> m_vTop_level_domains;

	CEmailVerifySintax(wchar_t *);
	BOOL parse();
	wchar_t *getDomainName();
	wchar_t *getLocalPart();
	wchar_t *getAddressToVerify();
	BOOL hasValidSintax();

	HRESULT initTopLevelDomain();
	BOOL domainNameHasLegalLength(wchar_t *);
	BOOL trivialChecks();
	BOOL checkTotalLength();
	BOOL checkHasMinimOneARond();
	BOOL checkFirstCharIsDot();
	BOOL checkLocalPart();
	BOOL checkLocalPartLength();
	BOOL check2PointsConsecutive();
	BOOL checkLabelFirstLetter(std::wstring);
	BOOL checkLabelLetters(std::wstring);
	BOOL checkIfIPAddress(wchar_t*);
	BOOL checkDomain(wchar_t*);
	BOOL isQuote(wchar_t);
	BOOL isARond(wchar_t);
	BOOL isEndOfString(DWORD index);
	BOOL isBackslash(wchar_t);
	BOOL isForbiddenChar(wchar_t);
	BOOL isAllowedChar(wchar_t);
	BOOL isQuotedChar(wchar_t);
	wchar_t nextChar(DWORD);
	wchar_t previousChar(DWORD);
	VOID formLocalPart(DWORD);
	VOID formDomainName(DWORD);
	BOOL domainLabelHaveLegalLength(std::vector<std::wstring>);
	std::vector<std::wstring> getDomainLabels(wchar_t*);

	~CEmailVerifySintax(void);
};

