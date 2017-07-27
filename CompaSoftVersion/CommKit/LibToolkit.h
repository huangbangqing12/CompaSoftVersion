#pragma once

//! 应用程序辅助工具类
class  CLibToolkit
{
public:
	/**
	* 生成 GUID
	* @return	返回生成的GUID字符串
	*/
	static CString CreateGuid(void);

	/**
	* 取得模块路径
	* @param [in] hInst		模块的句柄
	* @return	返回模块路径，如（c:\\a.exe）
	*/
	static CString GetAppModuleFilename(HINSTANCE hInst);

	/**
	* 取得临时路径
	* @return	返回临时路径字符串，如（c:\\）
	*/
	static CString GetTempPath(void);

	/**
	* 取得API出错信息
	* @return	返回API出错信息
	*/
	static CString GetWin32LastError(void);
};