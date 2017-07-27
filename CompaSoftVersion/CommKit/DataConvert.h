#pragma once
#include <math.h>

//! ����ת����
class  DataConvert
{
public:
	/**
	* �Ƚ������������Ƿ����
	* @param [in] var1	������1
	* @param [in] var2	������2
	* @param [in] dot	С��λ����Ĭ��Ϊ-1����ʾ���������루�����0.000001����Ϊ��ȣ�
	* @return	���������������Ƿ����
	* - true ��ȣ�false �����
	*/
	static bool FloatEqual(double var1, double var2, int dot = -1);

	/**
	* ��������������
	* @param [in] num		Ҫ��ʽ������ֵ
	* @param [in] dot		С��λ����Ϊ -1 ��ʾ�ǲ���������
	* @return	���ظ�ʽ�������ֵ
	*/
	static double FloatRound(double num, int dot);

	/**
	* ����������������ַ���
	* @param [in] num		Ҫ��ʽ������ֵ
	* @param [in] dot		С��λ����Ϊ -1 ��ʾ�ǲ���������
	* @return	���ظ�������ʽ������ַ���
	*/
	static CString Float2Text(double num, int dot = -1);

	/**
	* ����������������ַ���
	* @param [in] num		Ҫ��ʽ������ֵ
	* @param [in] dot		С��λ����Ϊ -1 ��ʾ�ǲ��������루ʵ������C++���ԭ��ֻ���6λС����
	* @param [in] buffer	�������
	* @param [in] FullZero	���С��λ�������Ƿ��� 0 ���
	* @return	���ظ�������ʽ������ַ���
	*/
	static LPCTSTR Float2Text(double num, TCHAR* buffer, int dot = -1, bool FullZero = false);

	/**
	* ����������������ַ���
	* @param [in] num		Ҫ��ʽ������ֵ
	* @param [in] dot		С��λ����Ϊ -1 ��ʾ�ǲ���������
	* @param [in] FullZero	���С��λ�������Ƿ��� 0 ���
	* @return	���ظ�������ʽ������ַ���
	*/
	static CString Float2Text(double num, int dot, bool FullZero);

	/**
	* long ת�����ַ���
	* long��Χ��-2,147,483,648 �� 2,147,483,647
	* @param [in] val		Ҫ��ʽ������ֵ
	* @return	����long��ʽ������ַ���
	*/
	static CString Long2Text(long val);

	/**
	* __int64 ת�����ַ���
	* -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
	* @param [in] val		Ҫ��ʽ������ֵ
	* @return	����__int64��ʽ������ַ���
	*/
	static CString I642Text(__int64 val);

	/**
	* bool ת�����ַ���
	* @param [in] val		Ҫ��ʽ������ֵ
	* @return	����bool��ʽ������ַ���
	*/
	static LPCTSTR Bool2Text(BOOL val);

	/**
	* �ַ��� ת����bool
	* valתΪ��д����£�val�ǡ�TRUE����T������true�����򷵻�false
	* @param [in] val		Ҫ��ʽ������ֵ
	* @return	�����ַ�����ʽ�����bool
	*/
	static bool Text2Bool(LPCTSTR val);

	/**
	*  @brief    double ȡ��
	*
	*  @param    double dNum ��ȡ��������
	*  @return   double		 ���
	*/
	static double FloatQuZheng(double dNum);
};