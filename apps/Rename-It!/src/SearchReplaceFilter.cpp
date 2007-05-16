#include "StdAfx.h"
#include "SearchReplaceFilter.h"
#include "SearchReplaceDlg.h"
#include "id3/readers.h"

// ID3Lib fix: END_OF_READER unreferenced
const ID3_Reader::int_type ID3_Reader::END_OF_READER = std::char_traits<ID3_Reader::char_type>::eof();

const LPCTSTR CSearchReplaceFilter::CASE_MARKER = _T("\x06\x03\x05");
const int CSearchReplaceFilter::CASE_MARKER_LENGTH = (int) _tcslen(CASE_MARKER);

CSearchReplaceFilter::CSearchReplaceFilter(void) :
	m_regexpCompiled(NULL),
	m_regexpExtra(NULL)
{
	LoadDefaultArgs();
}

CSearchReplaceFilter::~CSearchReplaceFilter(void)
{
	RegExpFree();
}

/** Filter the file name.
 * @param strOriginalFilename	Path to the original file name.
 * @param strFilename		Current new file name, without extension.
 * @param ext				Current new extension.
 * @return True if file should be renamed.
 */
void CSearchReplaceFilter::FilterPath(const CFileName& fnOriginalFilename, CString& strFilename)
{
	bool bIncrementCounter = false;	// Some replacement done, so we should increment the enumeration counter?
	CStringList	slMacros,
				slValues;

	// ID3 Tag mode?
	if (m_bID3Tag)
	{
		if (!AddID3TagMacros(fnOriginalFilename, slMacros, slValues))
			return;
	}

	// Locale
	CString strOldLocale = _tsetlocale(LC_ALL, NULL);
	_tsetlocale(LC_ALL, m_strLocale);

	// If search and replace are empty but change case is activated
	if (m_strSearch.IsEmpty() && m_strReplace.IsEmpty() && m_nChangeCase != caseNone)
	{
		ChangeCase(strFilename, m_nChangeCase);
		bIncrementCounter = true;
	}
	else
	{
		// Prepare the replacement string.
		CString	strReplace = m_strReplace;
		{
			// Number series.
			if (m_bSeries)
				// Replace %d by 123 in strReplace.
				ReplaceSeries(strReplace, m_nSeriesCounter);

			// Replace special macros.
			slMacros.AddHead( _T("$(FileDir)") );
			slValues.AddHead( m_fnOriginalFile.GetDrive() + m_fnOriginalFile.GetDirectory() );
			slMacros.AddHead( _T("$(FileName)") );
			slValues.AddHead( m_fnOriginalFile.GetFileName() );
			slMacros.AddHead( _T("$(FileExt)") );
			slValues.AddHead( m_fnOriginalFile.GetExtension() );
			slMacros.AddHead( _T("$(UnfilteredName)") );
			slValues.AddHead( m_strUnfilteredName );
			slMacros.AddHead( _T("$(FilteredName)") );
			slValues.AddHead( strFilename );
			ReplaceMacrosIn( strReplace, slMacros, slValues );

			// Add markers in the replacement string.
			if (m_nChangeCase != caseNone)
				strReplace = CASE_MARKER + strReplace + CASE_MARKER;
		}

		// Replace search by replace in file name.
		switch (m_nUse)
		{
		case useWildcards:	// Wildcard
			// If we use Wildcards, we convert them to RegExp.
			// Change the Wildcard into a RegExp.
			EscapeRegExp(strReplace);
			{
				int nCapture = 1;
				for (int i=0; i<strReplace.GetLength(); ++i)
				{
					if (strReplace[i] == '*')
					{
						strReplace.SetAt(i, '0' + nCapture);
						if (++nCapture > 9)
							break;
					}
				}
			}
		case useRegExp:	// RegExp
			{
				CString strOutput = strFilename;
				if (FilterRegExp(strFilename, strOutput, strReplace) > 0)
				{
					strFilename = strOutput;
					bIncrementCounter = true;
				}
			}
			break;

		default:	// General search & replace
			{
				// Prepare the search string.
				CString strSearch = m_strSearch;
				// If search string is empty.
				if (strSearch.IsEmpty())
				{
					// Replace the whole file name.
					if (m_nUse == useNone)	// Other uses are dealt with in CompileRegExp();
						strSearch = strFilename;
				}

				FilterString(strFilename, strSearch, strReplace);
			}
		}

		// Change the case in the replaced string.
		if (m_nChangeCase != caseNone)
		{
			if (ChangeCase(strFilename, m_nChangeCase))
				bIncrementCounter = true;
		}
	}

	// End Locale
	_tsetlocale(LC_ALL, strOldLocale);

	// Number series
	if (m_bSeries && bIncrementCounter)
	{
		// Increment counter's value
		m_nSeriesCounter += m_nSeriesStep;
	}
}

bool CSearchReplaceFilter::AddID3TagMacros(const CFileName& fnOriginalFilename, CStringList& slMacros, CStringList& slValues)
{
	// Load ID3 tag info.
#ifdef _UNICODE
	// A workaround in VC7
	FILE* f = NULL;
	if (_wfopen_s(&f, fnOriginalFilename.GetFullPath(), _T("rb")) != 0)
		return false;	// File not found.
	ifstream stm(f);
#else
	ifstream stm(fnOriginalFilename.GetFullPath(), ios_base::in | ios_base::binary);
#endif
	ID3_IFStreamReader id3Reader(stm);

	ID3_Tag	id3Tag;
	id3Tag.Link(id3Reader);

	// No ID3 tag?
	if (!id3Tag.HasV1Tag() && !id3Tag.HasV2Tag())
	{
#ifdef _UNICODE
		fclose(f);
#endif
		return false;		// then don't change the file name
	}

	// Add to the list of Macros -> Values.
	slMacros.AddHead( _T("$(ID3Track)") );
	{
		int nTrackNum = _ttoi(GetID3TagValue(id3Tag, ID3FID_TRACKNUM));
		CString strTrackNum;
		strTrackNum.Format(_T("%d"), nTrackNum);
		slValues.AddHead( strTrackNum );
	}

	slMacros.AddHead( _T("$(ID3Artist)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_LEADARTIST) );

	slMacros.AddHead( _T("$(ID3Title)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_TITLE) );

	slMacros.AddHead( _T("$(ID3Album)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_ALBUM) );

	slMacros.AddHead( _T("$(ID3Year)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_YEAR) );

	slMacros.AddHead( _T("$(ID3Comment)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_COMMENT) );

	slMacros.AddHead( _T("$(ID3Genre)") );
	slValues.AddHead( GetID3TagValue(id3Tag, ID3FID_CONTENTTYPE) );

#ifdef _UNICODE
	fclose(f);
#endif
	return true;
}

void CSearchReplaceFilter::ReplaceInvalidChars(CString& strSubject)
{
	for (int i=0; i<strSubject.GetLength(); ++i)
		switch (strSubject[i])
		{
		case '/':
		case '\\':
		case '|':
			strSubject.SetAt(i, '-');
			break;

		case '*':
		case '?':
			strSubject.SetAt(i, '_');
			break;

		case '<':
			strSubject.SetAt(i, '(');
			break;

		case '>':
			strSubject.SetAt(i, ')');
			break;

		case ':':
			// Replace "xxx: xxx" by "xxx - xxx", but other cases.
			if (i > 0 && strSubject[i-1] != ' ' &&
				i+1 < strSubject.GetLength() && strSubject[i+1] == ' ')
			{
				strSubject.Insert(i++, ' ');
			}
			strSubject.SetAt(i, '-');
			break;

		case '"':
			strSubject.SetAt(i, '\'');
			strSubject.Insert(i++, '\'');
			break;
		}
}

void CSearchReplaceFilter::ReplaceSeries(CString& strReplace, int nCounterValue)
{
	// Check the format string
	for (int p=0; (p = strReplace.Find('%', p)) != -1; p+=2)
	{
		int n = p+1;

		// Escaped %?
		if (strReplace[n] == '%')
			continue;

		// Flags
		if (strReplace[n] == '-' || strReplace[n] == '+' || strReplace[n] == '0' || strReplace[n] == ' ')
			++n;

		// Width
		while (strReplace[n] >= '0' && strReplace[n] <= '9')
			++n;
		
		// Type
		if (strReplace[n] != 'd')
		{
			// Invalide %, so escape it
			strReplace.Insert(p, '%');
			continue;
		}

		// The format of this %d is correct

		// Escape any further un-escaped % (% -> %%)
		while ((n = strReplace.Find('%', n)) != -1)
		{
			if (strReplace[++n] != '%')
				strReplace.Insert(n, '%');
			++n;
		}
		break;
	}

	// Replace %d by the counter's value
	strReplace.Format(strReplace, nCounterValue);
}

/** Display a window to configure the filter.
 * @param strOriginalFilename	Original sample file name.
 * @param strFilename		Current sample file name.
 * @param ext				Current sample file extension.
 * @return The value returned by CDialog::DoModal().
 */
int CSearchReplaceFilter::ShowDialog(const CFileName& fnOriginalFilename, const CString& strFilename)
{
	CSearchReplaceDlg dlg(*this, fnOriginalFilename, strFilename);
	return (int) dlg.DoModal();
}

// Return the unique name of the filter.
CString CSearchReplaceFilter::GetFilterCodeName() const
{
	return _T("Search and replace");
}

// Return the human name of the filter in user language.
CString CSearchReplaceFilter::GetFilterName() const
{
	CString strName;
	strName.LoadString(IDS_SEARCH_AND_REPLACE);
	return strName;
}

// User friendly description of the filter's actions over file's name.
CString CSearchReplaceFilter::GetFilterDescription() const
{
	CString ruleDescr,
			strBuffer;

	if (m_bID3Tag)
		ruleDescr.LoadString(IDS_DESCR_ID3);
	else
		ruleDescr.Empty();

	// + "Replace"
	switch (m_nUse)
	{
	case useRegExp:	// RegExp
		strBuffer.LoadString(IDS_DESCR_REPLACE_REGEXP);
		break;

	case useWildcards:	// Wildcard
		strBuffer.LoadString(IDS_DESCR_REPLACE_WILDCARD);
		break;

	default:
		strBuffer.LoadString(IDS_DESCR_REPLACE);
	}
	ruleDescr += strBuffer;

	// + SEARCH
	if (!m_strSearch.IsEmpty())
		ruleDescr += _T(" \"") + m_strSearch + _T("\"");

	// + "width" + REPLACE
	strBuffer.LoadString(IDS_DESCR_WITH);
	ruleDescr += _T(" ") + strBuffer + _T(" \"") +  m_strReplace + _T("\"");

	ruleDescr += _T(" [");
	if (m_bCaseSensitive)
		strBuffer.LoadString(IDS_DESCR_MATCH_CASE);
	else
		strBuffer.LoadString(IDS_DESCR_SKIP_CASE);
	ruleDescr += strBuffer;
	if (m_bOnce)
	{
		strBuffer.LoadString(IDS_DESCR_ONCE);
		ruleDescr += _T(", ") + strBuffer;
	}
	if (m_bMatchWholeText)
	{
		strBuffer.LoadString(IDS_DESCR_WHOLE_TEXT);
		ruleDescr += _T(", ") + strBuffer;
	}
	if (m_bSeries)
	{
		strBuffer.LoadString(IDS_DESCR_SERIES);
		ruleDescr += _T(", ") + strBuffer;
	}
	ruleDescr += _T("]");

	// TODO: nChangeCase
	// TODO: locale

	return ruleDescr;
}

// Return filter's parameters to be exported/saved.
void CSearchReplaceFilter::GetArgs(CMapStringToString& mapArgs) const
{
	mapArgs[_T("search")] = m_strSearch;
	mapArgs[_T("replace")] = m_strReplace;
	mapArgs[_T("casesensitive")].Format(_T("%d"), m_bCaseSensitive);
	mapArgs[_T("once")].Format(_T("%d"), m_bOnce);
	mapArgs[_T("matchWholeText")].Format(_T("%d"), m_bMatchWholeText);
	mapArgs[_T("use")].Format(_T("%d"),  m_nUse);
	mapArgs[_T("changeCase")].Format(_T("%d"),  m_nChangeCase);
	mapArgs[_T("locale")] = m_strLocale;
	mapArgs[_T("series")].Format(_T("%d"),  m_bSeries);
	mapArgs[_T("seriesStart")].Format(_T("%d"),  m_nSeriesStart);
	mapArgs[_T("seriesStep")].Format(_T("%d"),  m_nSeriesStep);
	mapArgs[_T("id3tag")].Format(_T("%d"),  m_bID3Tag);
}

// Load default filter's parameters.
void CSearchReplaceFilter::LoadDefaultArgs()
{
	m_strSearch.Empty();
	m_strReplace.Empty();
	m_bOnce = true;
	m_bCaseSensitive = true;
	m_bMatchWholeText = false;
	m_nUse = useWildcards;
	m_nChangeCase = caseNone;
	m_strLocale = _T("");
	m_bSeries = false;
	m_nSeriesStart = 1;
	m_nSeriesStep = 1;
	m_bID3Tag = false;

	// Re-compile the regexp.
	CompileRegExp();
}

// Define some or all of filter's parameters (for import/load).
void CSearchReplaceFilter::SetArgs(const CMapStringToString& mapArgs)
{
	CString strValue;

	if (mapArgs.Lookup(_T("search"), strValue))
		m_strSearch = strValue;

	if (mapArgs.Lookup(_T("replace"), strValue))
		m_strReplace = strValue;

	if (mapArgs.Lookup(_T("casesensitive"), strValue))
		m_bCaseSensitive = _ttoi( (LPCTSTR)strValue ) != 0;

	if (mapArgs.Lookup(_T("once"), strValue))
		m_bOnce = _ttoi( (LPCTSTR)strValue ) != 0;

	if (mapArgs.Lookup(_T("matchWholeText"), strValue))
		m_bMatchWholeText = _ttoi( (LPCTSTR)strValue ) != 0;

	if (mapArgs.Lookup(_T("use"), strValue))
	{
		int nUse = _ttoi( (LPCTSTR)strValue );
		if (nUse >= useNone && nUse <= useWildcards)
			m_nUse = (EUse) nUse;
	}

	if (mapArgs.Lookup(_T("changeCase"), strValue))
	{
		int nChangeCase = _ttoi( (LPCTSTR)strValue );
		if (nChangeCase >= caseNone && caseInvert <= caseInvert)
			m_nChangeCase = (EChangeCase) nChangeCase;
	}

	if (mapArgs.Lookup(_T("locale"), strValue))
		m_strLocale = strValue;

	if (mapArgs.Lookup(_T("series"), strValue))
		m_bSeries = _ttoi( (LPCTSTR) strValue ) != 0;

	if (mapArgs.Lookup(_T("seriesStart"), strValue))
		m_nSeriesStart = _ttoi( (LPCTSTR)strValue );

	if (mapArgs.Lookup(_T("seriesStep"), strValue))
		m_nSeriesStep = _ttoi( (LPCTSTR)strValue );

	if (mapArgs.Lookup(_T("id3tag"), strValue))
		m_bID3Tag = _ttoi( (LPCTSTR)strValue ) != 0;

	// Re-compile the regexp.
	CompileRegExp();
}

void CSearchReplaceFilter::OnStartRenamingList(ERenamePart nPathRenamePart)
{
	// Initialize counter
	m_nSeriesCounter = m_nSeriesStart;
}

void CSearchReplaceFilter::OnStartRenamingFile(const CFileName& fnPath, const CString& strName)
{
	m_fnOriginalFile = fnPath;
	m_strUnfilteredName = strName;
}

void CSearchReplaceFilter::OnEndRenamingFile()
{
}

void CSearchReplaceFilter::OnEndRenamingList()
{
}

void CSearchReplaceFilter::RegExpFree()
{
	// Free the PCRE compiled RegExp.
	if (m_regexpCompiled != NULL)
	{
		pcre_free(m_regexpCompiled);
		m_regexpCompiled = NULL;
	}
	if (m_regexpExtra != NULL)
	{
		pcre_free(m_regexpExtra);
		m_regexpExtra = NULL;
	}

	// Empty the error string.
	m_strRegExpCompileError.Empty();
}

bool CSearchReplaceFilter::CompileRegExp()
{
	// Free the previous one.
	RegExpFree();

	// Are we going to use PCRE?
	if (m_nUse == useNone)
		return true;

	CString strSearch = m_strSearch;

	// If search string is empty.
	if (strSearch.IsEmpty())
	{
		// Replace the whole file name.
		strSearch = _T(".*");
	}
	else
	{
		// If we use Wildcards, we convert them to RegExp.
		if (m_nUse == useWildcards)
		{
			// Change the Wildcard into a RegExp.
			EscapeRegExp(strSearch);
			strSearch.Replace(_T("\\?"), _T("."));
			strSearch.Replace(_T("\\*"), _T("(.*)"));
			if (m_bMatchWholeText)
				strSearch = '^' + strSearch + '$';
		}
	}

	// Compile regexp...
	int	 nOptions = 0;
	if (!m_bCaseSensitive)
		nOptions |= PCRE_CASELESS;

	const char *pchError;
	int nErrorOffset;

	CHAR	szPattherA[1024];
#ifdef _UNICODE
	szPattherA[WideCharToMultiByte(CP_ACP, 0, strSearch, -1, szPattherA, sizeof(szPattherA)/sizeof(szPattherA[0]), NULL, NULL)] = '\0';
#else
	strcpy_s(szPattherA, strSearch);
#endif
	m_regexpCompiled = pcre_compile(szPattherA, nOptions, &pchError, &nErrorOffset, NULL);
	if (m_regexpCompiled == NULL)
	{
		m_strRegExpCompileError = _T("*** RegExp Error: ");
		m_strRegExpCompileError += pchError;
	}
	else
		m_regexpExtra = pcre_study(m_regexpCompiled, 0, &pchError);

	return m_regexpCompiled != NULL && m_regexpExtra != NULL;
}

unsigned CSearchReplaceFilter::FilterRegExp(const CString &strIn, CString& strOut, const CString &strReplace) const
{
	if (m_regexpCompiled == NULL)
	{
		strOut = m_strRegExpCompileError;
		return true;
	}

	// UGLY CODE MODE ON
	unsigned	nReplacements = 0;
	const int	OFFSETS_SIZE = 3*10;
	int			nvOffsets[OFFSETS_SIZE];

	if (m_bOnce)
	{
		CHAR	szInA[1024];
#ifdef _UNICODE
		szInA[WideCharToMultiByte(CP_ACP, 0, strIn, -1, szInA, sizeof(szInA)/sizeof(szInA[0]), NULL, NULL)] = '\0';
#else
		strcpy_s(szInA, strIn);
#endif
		int nCount = pcre_exec(m_regexpCompiled, m_regexpExtra, szInA, (int)strlen(szInA)/sizeof(szInA[0]), 0, 0, nvOffsets, OFFSETS_SIZE);
		if (nCount > 0)
		{
			// We found something
			++nReplacements;
			strOut = strIn.Left(nvOffsets[0]);
			CString strTmpReplace = strReplace;

			// Find all \1 \2 \0 in replace
			for (int nFindPos; (nFindPos = strTmpReplace.Find(_T('\\'))) != -1; strTmpReplace = myMid(strTmpReplace, nFindPos+2))
			{
				strOut += strTmpReplace.Left(nFindPos);
				if ((nFindPos+1) < strTmpReplace.GetLength())
				{
					TCHAR chr = strTmpReplace.GetAt(nFindPos+1);
					if (chr >= _T('0') && chr <= _T('9'))
					{
						int index = chr - _T('0');
						if (index < nCount)
							if (nvOffsets[index*2] != -1)
								strOut += myMid(strIn, nvOffsets[index*2], nvOffsets[index*2+1]-nvOffsets[index*2]);
					}
					else
						strOut += chr;
				}
			}
			strOut += strTmpReplace;
			strOut += myMid(strIn, nvOffsets[1]);
		}
	}
	else
	{
		// Many
		CString strTmpIn = strIn;
		strOut.Empty();

		while (true)
		{
			CHAR	szInA[1024];
#ifdef _UNICODE
			szInA[WideCharToMultiByte(CP_ACP, 0, strTmpIn, -1, szInA, sizeof(szInA)/sizeof(szInA[0]), NULL, NULL)] = '\0';
#else
			strcpy_s(szInA, strTmpIn);
#endif
			int nCount = pcre_exec(m_regexpCompiled, m_regexpExtra, szInA, strTmpIn.GetLength(), 0, 0, nvOffsets, OFFSETS_SIZE);
			if (nCount <= 0 || nvOffsets[0] == nvOffsets[1])
				break;

			// We found something
			++nReplacements;
			strOut += strTmpIn.Left(nvOffsets[0]);
			CString strTmpReplace = strReplace;

			// Find all \1 \2 \0 in replace
			for (int nFindPos; (nFindPos = strTmpReplace.Find(_T('\\'))) != -1; )
			{
				strOut += strTmpReplace.Left(nFindPos);
				if ((nFindPos+1) < strTmpReplace.GetLength())
				{
					TCHAR chr = strTmpReplace.GetAt(nFindPos+1);
					if (chr >= _T('0') && chr <= _T('9'))
					{
						int index = chr - _T('0');
						if (index < nCount)
							if (nvOffsets[index*2] != -1)
								strOut += myMid(strTmpIn, nvOffsets[index*2], nvOffsets[index*2+1]-nvOffsets[index*2]);
					}
					else
						strOut += chr;
				}
				if (nFindPos + 2 > strTmpReplace.GetLength())
					strTmpReplace.Empty();
				else
					strTmpReplace = myMid(strTmpReplace, nFindPos+2);
			}
			strOut += strTmpReplace;
			strTmpIn = myMid(strTmpIn, nvOffsets[1]);
		}
		strOut += strTmpIn;
	}

	return nReplacements;
}

void CSearchReplaceFilter::EscapeRegExp(CString& str)
{
	// The special regular expression characters are:
	// . \ + * ? [ ^ ] $ ( ) { } = ! < > | :
	for (int i=0; i<str.GetLength(); ++i)
		switch (str[i])
		{
		case '.': case '\\': case '+': case '*':
		case '?': case '[':  case '^': case ']': 
		case '$': case '(':  case ')': case '{':
		case '}': case '=': case '!':  case '<':
		case '>': case '|':
			str.Insert(i++, '\\');
		}
}

bool CSearchReplaceFilter::FilterString(CString& strSubject, const CString& strSearch, const CString& strReplace) const
{
	if (m_bMatchWholeText)
	{
		if (strSubject.CompareNoCase(strSearch) == 0)
		{
			strSubject = strReplace;
			return true;
		}
		else
			return false;
	}
	else
	{
		CString strCaseSubject = strSubject;
		CString strCaseSearch = strSearch;

		// Case insensitive search?
		if (!m_bCaseSensitive)
		{
			strCaseSubject.MakeLower();
			strCaseSearch.MakeLower();
		}

		bool bReplacedString = false;
		CString strOutput;
		int nStart = 0;
		while (true)
		{
			// Search
			int nFoundPos = strCaseSubject.Find(strCaseSearch, nStart);
			if (nFoundPos == -1)
				break;
			// Replace
			bReplacedString = true;
			strOutput += strSearch.Mid(nStart, nFoundPos-nStart);
			strOutput += strReplace;
			nStart = nFoundPos + strCaseSearch.GetLength();
			// Only replace once
			if (m_bOnce)
				break;
		}
		strOutput += strSubject.Mid(nStart);

		strSubject = strOutput;
		return bReplacedString;
	}
}

CString CSearchReplaceFilter::myMid(const CString &str, int nFirst)
{
	if (nFirst > str.GetLength())
		return CString("");

	return str.Mid(nFirst);
}

CString CSearchReplaceFilter::myMid(const CString &str, int nFirst, int nCount)
{
	if (nFirst + nCount > str.GetLength())
		return myMid(str, nFirst);

	return str.Mid(nFirst, nCount);
}

/** Replace all macros in string by their relative values.
 */
void CSearchReplaceFilter::ReplaceMacrosIn(CString &strInOut, const CStringList &slMacros, const CStringList &slValues) const
{
	CString		strMacro,
				strValue;
	POSITION	posMacros,
				posValues;
	int			i;

	ASSERT(slMacros.GetCount() == slValues.GetCount());

	// Search & replace macros.
	for (i=0; i<strInOut.GetLength(); ++i)
		for (posMacros = slMacros.GetHeadPosition(), posValues = slValues.GetHeadPosition(); posMacros != NULL, posValues != NULL; )
		{
			strMacro = slMacros.GetNext(posMacros);
			strValue = slValues.GetNext(posValues);
			if (strInOut.Mid(i, strMacro.GetLength()).CompareNoCase( strMacro ) == 0)
			{
				// Replace.
				strInOut = strInOut.Left(i) + strValue + strInOut.Mid(i+strMacro.GetLength());
				i += strValue.GetLength() - 1;
			}
		}
}

CString CSearchReplaceFilter::GetID3TagValue(ID3_Tag &id3Tag, ID3_FrameID nid3ID)
{
	char		szTag[256];
	ID3_Frame*	pid3Frame;
	ID3_Field*	pid3Field;

	// Don't free frame or field with delete. 
	// id3lib will do that for us...
	if ((pid3Frame = id3Tag.Find(nid3ID)) != NULL &&
		(pid3Field = pid3Frame->GetField(ID3FN_TEXT)) != NULL)
	{
		pid3Field->Get(szTag, sizeof(szTag)/sizeof(szTag[0]));
		CString strTag;
		strTag = szTag;
		ReplaceInvalidChars(strTag);
		return strTag;
	}
	return _T(""); // That's bad!
}

bool CSearchReplaceFilter::ChangeCase(CString& strSubject, EChangeCase nCaseChangeOption)
{
	// Search for the markers.
	int nStartMarker = strSubject.Find(CASE_MARKER);
	int nEndMarker = -1;
	do
	{
		int nFound = strSubject.Find(CASE_MARKER, nEndMarker+1);
		if (nFound == -1)
			break;
		nEndMarker = nFound;
	} while (true);
	if (nStartMarker == -1 || nEndMarker == -1)
		return false;

	// Change the case between the markers.
	CString strChangeCaseIn = strSubject.Mid(nStartMarker+CASE_MARKER_LENGTH, nEndMarker-nStartMarker-CASE_MARKER_LENGTH);
	LPTSTR szString = strChangeCaseIn.GetBuffer();
	switch (nCaseChangeOption)
	{
	case caseLower: // LOWER
		_tcslwr(szString);
		break;

	case caseUpper: // UPPER
		_tcsupr(szString);
		break;

	case caseSentense: // SENTENSE
		_tcslwr(szString);
		szString[0] = _totupper(szString[0]);
		break;

	case caseWord: // WORD
		{
			_tcslwr(szString);
			bool inWord = false;

			for (int i=0; i<(int)_tcsclen(szString); i++)
			{
				switch (szString[i])
				{
				case _T(' '): case _T('.'): case _T(','): case _T('_'): case _T('-'):
				case _T('+'): case _T('~'): case _T('#'): case _T('='): case _T('&'):
				case _T('('): case _T(')'):
				case _T('['): case _T(']'):
				case _T('{'): case _T('}'):
					inWord = false;
					break;
				default:
					if (!inWord)
					{
						szString[i] = _totupper(szString[i]);
						inWord = true;
					}
				}
			}
		}
		break;

	case caseInvert:	// INVERT
		for (int i=0; i<(int)_tcsclen(szString); i++)
		{
			// If it's not an ASCII
//			if (_istascii(szString[i]))	// tolower/upper won't return right value for non-ascii
				if (_istupper(szString[i]))
					szString[i] = _totlower(szString[i]);
				else
					szString[i] = _totupper(szString[i]);
		}
		break;

	default:
		ASSERT(false);
	}

	strChangeCaseIn.ReleaseBuffer();

	// Remove the markers and replace them by the new string.
	strSubject = strSubject.Left(nStartMarker) + strChangeCaseIn + strSubject.Mid(nEndMarker+CASE_MARKER_LENGTH);
	return true;
}