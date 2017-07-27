#pragma once

#include "IDataStream.h"

//! �����ĵ���������
class  CFileDataReadStream : public IRandomReadStream
{
public:
	CFileDataReadStream(HANDLE file);
	~CFileDataReadStream(void);

	/**
	* �����ļ��Ƿ��
	* @return	�����ļ��Ƿ��
	* - true �򿪣�false �ر�
	*/
	BOOL IsOpen(void) { return HANDLE() != NULL; }

	/**
	* ���ļ�
	* @param [in] szFilename	�ļ�����
	* @return	���ش��ļ��Ƿ�ɹ�
	* - true �򿪳ɹ��� false ��ʧ��
	*/
	BOOL OpenFile(LPCTSTR szFilename);

	/**
	* �ر��ļ�
	*/
	void CloseFile(void);
	operator HANDLE() const;

	/**
	* ��ȡָ���ֽڵ�������
	* @param [out] pBuffer		��������
	* @param [in] nBufSize		��������С
	* @param [out] nBytesRead	�ֽ���
	* @return	����TRUE
	*/
	virtual BOOL Read(LPVOID pBuffer, DWORD nBufSize, DWORD& nBytesRead);

	/**
	* ȡ�ᵱǰָ��λ��
	* @return	����ָ��λ��
	*/
	virtual DWORD GetPosition();
	/**
	* ���õ�ǰָ��λ��
	* �ļ���ȡ����λ��Ҫ�� 0��GetLength()��Χ��
	* @return	�ɹ����أ�ʧ�ܷ��ؼ٣����ַ��Ч��
	*/
	virtual BOOL SetPosition(DWORD position);
	/**
	* ȡ�õ�ǰ�ɷ������ݳ���
	* @return	�������ݳ���
	*/
	virtual DWORD GetLength();

protected:
	// �� Cache �ж�����
	void ReadBuffer(LPVOID pBuffer, DWORD nBufSize, DWORD& nBytesRead);

	/** �������ļ� */
	CAtlFile	_File;

	/** Ĭ�������� Cache ��С */
	DWORD		_DefCacheBytes;

	/** �ļ� Cache��Ĭ��Ϊ DEFAULT_STREAM_BLOCKSIZE ���ֽ� */
	char*		_CacheBlock;

	/** ��ǰ Cache ��λ�� */
	DWORD		_CachePosition;

	/** ��ǰ Cache �пɶ����ݵĳ��� */
	DWORD		_CacheReadBytes;

	/** cache���ļ��е�λ�� */
	ULONGLONG	_CacheFilePosition;
};


//! �����ĵ�д������
class  CFileDataWriteStream : public IRandomWriteStream
{
public:
	CFileDataWriteStream(HANDLE file);
	~CFileDataWriteStream(void);

	/**
	* д�����ݵ�������
	* @param [in] pBuffer		��������
	* @param [in] nBufSize		��������С
	*/
	virtual BOOL Write(LPCVOID pBuffer, DWORD nBufSize);

	/** ������д���ļ������������ */
	BOOL FlushStream(void);

	/**
	* ȡ�ᵱǰָ��λ��
	* @return	����ָ��λ��
	*/
	virtual DWORD GetPosition();
	/**
	* ���õ�ǰָ��λ��
	* @return	�ɹ����أ�ʧ�ܷ��ؼ٣����ַ��Ч��
	*/
	virtual BOOL SetPosition(DWORD position);
	/**
	* ȡ�õ�ǰ�ɷ������ݳ���
	* @return	�������ݳ���
	*/
	virtual DWORD GetLength();
protected:

	CAtlFile	_File;

	/** �ļ� Cache��Ĭ��Ϊ DEFAULT_STREAM_BLOCKSIZE ���ֽ� */
	char*		_CacheBlock;

	/** ��ǰ Cache ��λ�� */
	DWORD		_CachePosition;

	/** ��д���cache���� */
	DWORD		_CacheLength;

	/** Ĭ�������� Cache ��С */
	DWORD		_DefCacheBytes;

	/** ���һ��д��λ�� */
	ULONGLONG	_LastWritePosition;
};