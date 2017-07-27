// CompaSoftVersion.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "CompaSoftVersion.h"
#include ".\XmlDoc\XmlDocument.h"


LOCK_API int __stdcall CompareSoftVersion(char* lpszInstallXml, char* lpszLocalXml)/* IsDllInUse(LPCSTR lpFileName)*/
{
	CString strInstallXml = CA2W(lpszInstallXml);
	CXmlDocument xmlInstallDoc;
	if (!xmlInstallDoc.LoadFile(strInstallXml, fmtXML))
	{
		return 1;
	}
	CXmlElement* pInstallRootElement = xmlInstallDoc.GetElementRoot();
	if (pInstallRootElement == NULL)
	{
		return 1;
	}

	CString strInstallVersion = pInstallRootElement->GetAttrValue(_T("version"));

	//��ȡ�����Ѱ�װ��xml
	CString strLocalXml = CA2W(lpszLocalXml);
	CXmlDocument xmlLocalDoc;
	if (!xmlLocalDoc.LoadFile(strLocalXml, fmtXML))
	{
		return 1;
	}

	CXmlElement* pLocalRootElement = xmlLocalDoc.GetElementRoot();
	if (pLocalRootElement == NULL)
	{
		return 1;
	}

	CString strLocalVersion = pLocalRootElement->GetAttrValue(_T("version"));

	if (strLocalVersion == strInstallVersion)
	{
		return 0;
	}

	return 1;
}