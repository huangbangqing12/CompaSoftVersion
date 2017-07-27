#include "StdAfx.h"
#include "FileDataStream.h"

//------------------------------------------------------------------
// CFileDataReadStream �����ĵ���������
//------------------------------------------------------------------

#define DEFCACHEBYTES (32 * 1024)

CFileDataReadStream::CFileDataReadStream(HANDLE file)
{
	if (file)
	{
		_File.Attach(file);
	}

	_DefCacheBytes = DEFCACHEBYTES;

	_CacheBlock = new char[_DefCacheBytes];

	_CachePosition = 0;
	_CacheReadBytes = 0;
	_CacheFilePosition = 0;
}

CFileDataReadStream::~CFileDataReadStream(void)
{
	_File.Detach();

	if (_CacheBlock)
		delete[] _CacheBlock;
}

BOOL CFileDataReadStream::OpenFile(LPCTSTR szFilename)
{
	ATLASSUME(((HANDLE)_File) == NULL);

	HANDLE hFile = ::CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	_File.Attach(hFile);

	return TRUE;
}

void CFileDataReadStream::CloseFile(void)
{
	_CachePosition = 0;
	_CacheReadBytes = 0;

	_File.Close();
}

CFileDataReadStream::operator HANDLE() const throw()
{
	return (HANDLE)_File;
}

/** ��ȡָ���ֽڵ������� */
BOOL CFileDataReadStream::Read(LPVOID pBuffer, DWORD nBufSize, DWORD& nBytesRead)
{
	DWORD	sumReadBytes = 0;	// ��д���ֽ���
	LPBYTE	bBuffer = (LPBYTE)pBuffer;

	while (sumReadBytes < nBufSize)
	{
		// ���� Cache ���Ƿ������ݿɶ������û����Ҫ���»���
		if (_CachePosition >= _CacheReadBytes)
		{
			// �Ӵ����ж�����
			_File.GetPosition(_CacheFilePosition);
			if (_File.Read(_CacheBlock, _DefCacheBytes, _CacheReadBytes) != S_OK)
				return FALSE;

			_CachePosition = 0;
		}

		// ����ʣ��Ҫ��ȡ���ݳ���
		DWORD needReadBytes = nBufSize - sumReadBytes;

		// ���� cache �пɶ����ݳ���
		DWORD cacheBackBytes = _CacheReadBytes - _CachePosition;

		// ������Դ� cache �ж�ȡ�����ݳ��ȣ����û�����ݿɶ�ȡ������ܳ�����
		DWORD cacheReadBytes = cacheBackBytes >= needReadBytes ? needReadBytes : cacheBackBytes;
		if (cacheReadBytes == 0)
			break;

		// ���ڴ��и�������
		memcpy(bBuffer, &_CacheBlock[_CachePosition], cacheReadBytes);
		_CachePosition += cacheReadBytes;
		bBuffer += cacheReadBytes;
		sumReadBytes += (DWORD)cacheReadBytes;
	}

	nBytesRead = sumReadBytes;

	return sumReadBytes == nBufSize;
}

/**
* ȡ�ᵱǰָ��λ��
* @return	����ָ��λ��
*/
DWORD CFileDataReadStream::GetPosition()
{
	return (DWORD)(_CacheFilePosition + _CachePosition);
}
/**
* ���õ�ǰָ��λ��
* @return	�ɹ����أ�ʧ�ܷ��ؼ٣����ַ��Ч��
*/
BOOL CFileDataReadStream::SetPosition(DWORD position)
{
	// �ļ���ȡ����λ��Ҫ�� 0��GetLength()��Χ�� SHl
	if (position > GetLength())
		return FALSE;

	// ����λ���Ƿ���cache��Χ��
	if (position >= _CacheFilePosition && position < _CacheFilePosition + _CacheReadBytes)
	{
		_CachePosition = (DWORD)(position - _CacheFilePosition);
	}
	else
	{
		HRESULT r = _File.Seek(position, FILE_BEGIN);
		if (r == S_OK)
		{
			_CachePosition = 0;
			_CacheReadBytes = 0;
			_CacheFilePosition = position;
			return TRUE;
		}
		return FALSE;
	}

	return TRUE;
}
/**
* ȡ�õ�ǰ�ɷ������ݳ���
* @return	�������ݳ���
*/
DWORD CFileDataReadStream::GetLength()
{
	ULONGLONG fileSize = 0;
	if (_File.GetSize(fileSize) != S_OK)
		return 0;
	return (DWORD)fileSize;
}

//------------------------------------------------------------------
// CFileDataWriteStream �����ĵ�д������
//------------------------------------------------------------------
CFileDataWriteStream::CFileDataWriteStream(HANDLE file)
{
	_File.Attach(file);
	_DefCacheBytes = DEFCACHEBYTES;

	_CacheBlock = (char*) ::malloc(_DefCacheBytes);
	_CachePosition = 0;
	_LastWritePosition = 0;
	_CacheLength = 0;
}

CFileDataWriteStream::~CFileDataWriteStream(void)
{
	// ��δд���ļ�������д���ļ�
	FlushStream();

	_File.Detach();

	if (_CacheBlock)
	{
		::free(_CacheBlock);
	}
}

BOOL CFileDataWriteStream::FlushStream(void)
{
	if (_CacheLength > 0)
	{
		DWORD writeLen = _CacheLength;
		DWORD byWrite = 0;
		_File.Write(_CacheBlock, writeLen, &byWrite);
		_CachePosition = 0;
		_CacheLength = 0;

		if (_File.GetSize(_LastWritePosition) != S_OK)
			return FALSE;

		return byWrite == writeLen;
	}

	return TRUE;
}

/** д�����ݵ������� */
BOOL CFileDataWriteStream::Write(LPCVOID pBuffer, DWORD nBufSize)
{
	if (pBuffer == NULL || nBufSize == 0)
		return FALSE;

	DWORD	sumWriteBytes = 0;	// ��д���ֽ���
	LPBYTE	bBuffer = (LPBYTE)pBuffer;

	while (sumWriteBytes < nBufSize)
	{
		// ���� Cache ���Ƿ��пռ䣬���û���������д��
		if (_CachePosition >= _DefCacheBytes)
		{
			if (!FlushStream())
				return FALSE;
		}

		// ����ʣ��Ҫд�����ݳ���
		DWORD needWriteBytes = nBufSize - sumWriteBytes;

		// ���� cache �п�д���ݳ���
		DWORD cacheBackBytes = _DefCacheBytes - _CachePosition;

		// �������д�� cache �����ݳ��ȣ����û�пռ�����ܳ�����
		DWORD cacheWriteBytes = cacheBackBytes >= needWriteBytes ? needWriteBytes : cacheBackBytes;
		if (cacheWriteBytes == 0)
			break;

		// ������д���ڴ�
		memcpy(&_CacheBlock[_CachePosition], bBuffer, cacheWriteBytes);
		_CachePosition += cacheWriteBytes;
		if (_CachePosition > _CacheLength)
			_CacheLength = _CachePosition;

		bBuffer += cacheWriteBytes;
		sumWriteBytes += (DWORD)cacheWriteBytes;
	}

	return sumWriteBytes == nBufSize;
}

/**
* ȡ�ᵱǰָ��λ��
* @return	����ָ��λ��
*/
DWORD CFileDataWriteStream::GetPosition()
{
	return (DWORD)(_LastWritePosition + _CachePosition);
}
/**
* ���õ�ǰָ��λ��
* @return	�ɹ����أ�ʧ�ܷ��ؼ٣����ַ��Ч��
*/
BOOL CFileDataWriteStream::SetPosition(DWORD position)
{
	// �о����ڻ����У��������ֻ�ǻ���ָ���ƶ��������������
	if (position >= _LastWritePosition && position <= _LastWritePosition + _CacheLength)
	{
		_CachePosition = (DWORD)(position - _LastWritePosition);
	}
	else
	{
		// ��д��ǰ����д���ļ�
		FlushStream();

		if (_File.Seek(position, FILE_BEGIN) != S_OK)
			return FALSE;

		_CachePosition = 0;
		_CacheLength = 0;
		_LastWritePosition = position;
	}

	return TRUE;
}
/**
* ȡ�õ�ǰ�ɷ������ݳ���
* @return	�������ݳ���
*/
DWORD CFileDataWriteStream::GetLength()
{
	ULONGLONG fileSize = 0;
	if (_File.GetSize(fileSize) != S_OK)
		return 0;
	return (DWORD)fileSize;
}