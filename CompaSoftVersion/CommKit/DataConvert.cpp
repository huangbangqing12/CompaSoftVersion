#include "StdAfx.h"
#include "DataConvert.h"

// 浮点数比较
bool DataConvert::FloatEqual(double var1, double var2, int dot)
{
	if (dot == -1)
	{
		return (fabs(var1 - var2) <= 0.000001);
	}
	else
	{
		ATLASSERT(dot < 10);

		long nMultiple = 1;
		while (dot > 0)
		{
			nMultiple *= 10;
			dot--;
		}

		return (fabs(var1 - var2) * nMultiple < 1);
	}
}

// 浮点数四舍五入
double DataConvert::FloatRound(double num, int dot)
{
	if (dot < 0) return num;

	long dSign = 1;
	if (num < 0) dSign = -1;
	long cs = (long)pow((double)10, dot);
	LONGLONG ullVal = (LONGLONG)(fabs(num) * cs + 0.5);

	return ((double)(ullVal * dSign)) / cs;
}

CString DataConvert::Float2Text(double num, int dot)
{
	CString strFormat = L"%f";

	if (-1 != dot)
		strFormat.Format(L"%%.%df", dot);

	CString strText;
	strText.Format(strFormat, num);

	if (strText.Find(L'.') != -1)
	{
		strText.TrimRight(L'0');
		strText.TrimRight(L'.');
	}
	if (strText == L"-0")
		strText = L"0";
	
	return strText;
}

LPCTSTR DataConvert::Float2Text(double num, TCHAR* buffer, int dot /*= -1*/, bool FullZero /*= false*/)
{
	ATLASSERT(dot >= -1 && buffer != NULL);

	TCHAR fmt[_CVTBUFSIZE];

	if (dot >= 0)
	{
		_stprintf(fmt, _T("%%0.%df"), dot);
		_stprintf(buffer, fmt, FloatRound(num, dot));
	}
	else
	{
		_stprintf(buffer, _T("%f"), num);
	}

	if (FullZero == false || dot <= 0)
	{
		// 查找小数点位置
		int dotNum = -1;
		for (int i = 0; i < (int)_tcslen(buffer); i++)
			if (buffer[i] == _T('.'))
			{
			dotNum = i;
			break;
			}

		// 删除小数点后面的 0
		if (dotNum >= 0)
		{
			for (int i = (int)_tcslen(buffer) - 1; i >= dotNum; i--)
				if (buffer[i] == _T('0'))
					buffer[i] = NULL;
				else
					break;

			if (buffer[_tcslen(buffer) - 1] == _T('.'))
				buffer[_tcslen(buffer) - 1] = NULL;
		}
	}

	return buffer;
}

CString DataConvert::Float2Text(double num, int dot /*= -1*/, bool FullZero /*= false*/)
{
	TCHAR fmtbuf[_CVTBUFSIZE];
	return Float2Text(num, fmtbuf, dot, FullZero);
}

/**
* long 转换成字符串
* @param [in] val		要格式化的数值
* @return	返回long格式化后的字符串
*/
CString DataConvert::Long2Text(long val)
{
	CString strText;
	strText.Format(_T("%d"), val);
	return strText;
}

/**
* __int64 转换成字符串
* @param [in] val		要格式化的数值
* @return	返回__int64格式化后的字符串
*/
CString DataConvert::I642Text(__int64 val)
{
	TCHAR buff[_CVTBUFSIZE] = {0};

	errno_t err = _i64tot_s(val, buff, _CVTBUFSIZE, 10);
	if (err != 0)
		return _T("");

	return buff;
}

/**
* bool 转换成字符串
* @param [in] val		要格式化的数值
* @return	返回bool格式化后的字符串
*/
LPCTSTR DataConvert::Bool2Text(BOOL val)
{
	return val ? _T("True") : _T("False");
}

/**
* 字符串 转换成bool
* @param [in] val		要格式化的数值
* @return	返回字符串格式化后的bool
*/
bool DataConvert::Text2Bool(LPCTSTR val)
{
	CString str(val);
	str.MakeUpper();
	return str == _T("TRUE") || str == _T("T");
}

/**
* double 转换成double
* @param [in] val		要格式化的数值
* @return	返回double取整后的double
*/
double DataConvert::FloatQuZheng(double dNum)
{
	CString strNum = Float2Text(dNum, -1);
	int nIndex = strNum.ReverseFind(L'.');

	// 没有小数点，本身就是整数，返回即可
	if (nIndex == -1)
		return dNum;

	// 有小数点，取整
	strNum = strNum.Left(nIndex);
	double dResult = _wtof(strNum);
	return dResult;
}
