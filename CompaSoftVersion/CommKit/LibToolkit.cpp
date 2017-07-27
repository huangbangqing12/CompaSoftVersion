#include "stdafx.h"
#include "libtoolkit.h"
#include "shellapi.h"

/** 生成 GUID　*/
CString CLibToolkit::CreateGuid(void)
{
	GUID guid;
	CoCreateGuid(&guid);
	TCHAR buf[64] = { 0 };

	_snwprintf(buf, sizeof(buf),
		_T("%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X"),
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);

	return buf;
}

/** 取得模块路径（c:\a.exe）*/
CString CLibToolkit::GetAppModuleFilename(HINSTANCE hInst)
{
	TCHAR m_pstr[MAX_PATH];
	::GetModuleFileName(hInst, m_pstr, MAX_PATH);

	return m_pstr;
}

/** 取得临时路径（c:\）　*/
CString CLibToolkit::GetTempPath(void)
{
	TCHAR m_pstr[MAX_PATH];

	DWORD dwLength = ::GetTempPath(MAX_PATH, m_pstr);
	m_pstr[dwLength] = _T('\0');
	return m_pstr;
}

/** 取得API出错信息　*/
CString CLibToolkit::GetWin32LastError()
{
	CString str;
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
		);
	str = (LPCTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);

	return str;
}

