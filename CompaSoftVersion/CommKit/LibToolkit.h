#pragma once

//! Ӧ�ó�����������
class  CLibToolkit
{
public:
	/**
	* ���� GUID
	* @return	�������ɵ�GUID�ַ���
	*/
	static CString CreateGuid(void);

	/**
	* ȡ��ģ��·��
	* @param [in] hInst		ģ��ľ��
	* @return	����ģ��·�����磨c:\\a.exe��
	*/
	static CString GetAppModuleFilename(HINSTANCE hInst);

	/**
	* ȡ����ʱ·��
	* @return	������ʱ·���ַ������磨c:\\��
	*/
	static CString GetTempPath(void);

	/**
	* ȡ��API������Ϣ
	* @return	����API������Ϣ
	*/
	static CString GetWin32LastError(void);
};