//////////////////////////////////////////////////////////////////////
// �ļ����ƣ�XmlDocument.h
// �ļ���;��ThXmlDoc,��XML�ĵ��Ĳ���
// ����ʱ�䣺2008.3.17, ����һ
// �ļ���Ȩ��������΢�¼������޹�˾
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IDataStream.h"

// XML���ڵ�����
enum xml_node_type
{
	node_null,			// �� (null) �ڵ���
	node_document,		// XML�ĵ����ĸ��ڵ�
	node_element,		// Ԫ�ؽڵ�, �� '<node/>'
	node_cdata,			// Character data, i.e. '<![CDATA[text]]>'
	node_comment,		// XMLע��, �� '<!-- text -->'
	node_pi,			// Processing instruction, i.e. '<?name?>'
	node_declaration,	// �ĵ�����, �� '<?xml version="1.0"?>'
	node_doctype		// Document type declaration, i.e. '<!DOCTYPE doc>'
};




class  CXmlAttribute;
class  CXmlElement;
class  CXmlDocument;

//! ���/��������
template <class T, class CONTENT>
class CXmlContainer
{
public:
	virtual ~CXmlContainer(void);

	/**
	* ���/���Ը���
	* @return ���ؽ��/���Եĸ���
	*/
	size_t GetCount(void);

	/**
	* ��ȡ���/���Զ���
	* @param [in] index	���/���Ե�����
	* @return ���ؽ��/���Զ���
	*/
	CONTENT* GetAt(size_t index);

	/**
	* ���ӽ��/����
	* @return �������Ӻ�������
	*/
	CONTENT* Add(void);

	/**
	* �Ƴ����/����
	* @param [in] index	���/���Ե�����
	*/
	void RemoveAt(size_t index);

	/**
	* �Ƴ����н��/����
	*/
	void RemoveAll(void);

	/**
	* ������ý��/��������
	*/
	void reset(void);
	CONTENT* InsertAt(size_t index);
protected:

	/**
	* ������/����
	* @param [in] index		�����λ��
	* @param [in] content	���/���Զ���
	* @return ���/���Զ���
	*/
	CONTENT* InsertAt(size_t index, CONTENT* content);
protected:
	/** ���/������������ */
	CAtlArray<CONTENT*>		_array;
	CXmlElement*			_parent;
};
#include "XmlDocument.hpp"


//! CXmlElements
class  CXmlElements : public CXmlContainer < CXmlElements, CXmlElement >
{
	friend class CXmlElement;
public:
	/**
	* ������
	* @param [in] index		�����λ��
	* @return ������
	*/
	CXmlElement* InsertAt(size_t index);

	/**
	* �Ƴ����
	* @param [in] index		�������
	*/
	void RemoveAt(size_t index);
};

//! CXmlAttributes
class  CXmlAttributes : public CXmlContainer < CXmlAttributes, CXmlAttribute >
{
	friend class CXmlElement;
public:

	/**
	* ��������
	* @param [in] index		�����λ��
	* @return ���Զ���
	*/
	CXmlAttribute* InsertAt(size_t index);

	/**
	* �Ƴ�����
	* @param [in] index		��������
	*/
	void RemoveAt(size_t index);
};

//! �������
class  CXmlAttribute
{
	friend class CXmlContainer < CXmlAttributes, CXmlAttribute > ;
public:
	CXmlAttribute(void);
	/**
	* ��������ֵ
	* @param [in] value	int��������ֵ
	*/
	void SetAttrValue(int value);

	/**
	* ��������ֵ
	* @param [in] value	long��������ֵ
	*/
	void SetAttrValue(long value);

	/**
	* ��������ֵ
	* @param [in] value	double��������ֵ
	*/
	void SetAttrValue(double value);

	/**
	* ��������ֵ
	* @param [in] value	LPCTSTR��������ֵ
	*/
	void SetAttrValue(LPCTSTR value);

	/**
	* ������������
	* @param [in] name	��������
	*/
	void SetAttrName(LPCTSTR name);

	/**
	* ��ȡ����ֵ
	* @return ����int��������ֵ
	*/
	int GetIntValue(void);

	/**
	* ��ȡ����ֵ
	* @return ����long��������ֵ
	*/
	long GetLngValue(void);

	/**
	* ��ȡ����ֵ
	* @return ����double��������ֵ
	*/
	double GetDblValue(void);

	/**
	* ��ȡ����ֵ
	* @return ����LPCTSTR��������ֵ
	*/
	LPCTSTR GetStrValue(void);

	/**
	* ��ȡ����ֵ
	* @return ����LPCTSTR��������
	*/
	LPCTSTR GetAttrName(void);

	/**
	* ����������Ϣ
	*/
	void reset(void);

protected:
	/** �������� */
	LPTSTR _name;
	/** ����ֵ */
	LPTSTR _value;

	CXmlElement *_parent;
};

//! CXmlElement
class  CXmlElement
{
	friend class CXmlDocument;
	friend class CXmlContainer < CXmlElements, CXmlElement > ;
	friend class CXmlElements;
	friend struct xml_parser;
public:
	CXmlElement(void);
	virtual ~CXmlElement(void);

	/**
	* ȡ���ӽ����������
	* @return	�����ӽ����������
	*/
	int GetChildElementCount(void);

	/**
	* ȡ���ӽ������
	* @param [in] NotExistCreate	�����ڵĽ���Ƿ񴴽� ��Ĭ��Ϊtrue��
	* @return	�����ӽ������
	*/
	CXmlElements* GetChildElements(BOOL NotExistCreate = true);

	/**
	* ȡ���ӽ��
	* @param [in] name				�������
	* @param [in] NotExistCreate	�����ڵĽ���Ƿ񴴽���Ĭ��Ϊtrue��
	* @return	�����ӽ��
	*/
	CXmlElement* GetChildElementAt(LPCTSTR name, BOOL NotExistCreate = true);

	/**
	* ȡ��������������
	* @return	����������������
	*/
	int GetAttributeCount(void);

	/**
	* ȡ����������
	* @param [in] NotExistCreate	�����ڵ������Ƿ񴴽���Ĭ��Ϊtrue��
	* @return	������������
	*/
	CXmlAttributes* GetAttributes(BOOL NotExistCreate = true);

	/**
	* ȡ������
	* @param [in] name				��������
	* @param [in] NotExistCreate	�����ڵ������Ƿ񴴽���Ĭ��Ϊtrue��
	* @return	��������
	*/
	CXmlAttribute* GetAttributeAt(LPCTSTR name, BOOL NotExistCreate = true);

	/**
	* ȡ������ֵ
	* @return	��������ֵ
	*/
	LPCTSTR GetElementText(void);

	/**
	* ��������ֵ
	* @param [in] text				����ֵ
	*/
	void SetElementText(LPCTSTR text);

	/**
	* ȡ����������
	* @return	������������
	*/
	LPCTSTR GetElementName(void);

	/**
	* ������������
	* @param [in] name				��������
	*/
	void SetElementName(LPCTSTR name);

	/**
	* ��������
	* @param [in] AttrName			��������
	* @param [in] AttrValue			����ֵ
	*/
	void SetAttrValue(LPCTSTR AttrName, LPCTSTR AttrValue);

	/**
	* �������
	* @param [in] AttrName			��������
	* @param [in] AttrValue			����Ĭ��ֵ
	*/
	void AddAttrValue(LPCTSTR AttrName, LPCTSTR AttrValue);

	/**
	* ��ȡ����
	* @param [in] AttrName			��������
	* @param [in] DefaultValue		����Ĭ��ֵ��Ĭ��Ϊ�գ�
	*/
	LPCTSTR GetAttrValue(LPCTSTR AttrName, LPCTSTR DefaultValue = _T(""));

	/**
	* ��������
	* @param [in] AttrName			��������
	* @param [out] index			��������
	* @return	�����Ƿ��ҵ�����
	*/
	BOOL FindAttribute(LPCTSTR AttrName, int& index);

	/**
	* ���ƽ��
	* @param [in] root			���ƵĽ��
	*/
	void CopyFrom(CXmlElement* root);

	/**
	* ������������
	*/
	void reset(void);


	/**
	* ��ȡ��ǰ�ڵ��Ƿ�Ϊ�������
	* @return	���ص�ǰ�ڵ��Ƿ�Ϊ�������
	* - TRUE �ǣ� FALSE ��
	*/
	BOOL GetIsDeclaration() { return _Type == node_declaration; };

	/**
	* ���õ�ǰ�ڵ��Ƿ�Ϊ�������
	* @param [in] bDeclaration		�Ƿ�Ϊ������㣨Ĭ��Ϊfalse��
	*/
	void SetIsDeclaration(BOOL bDeclaration = false);



	/**
	* ��ȡ�ı������Ƿ�����˫����
	* @return	�����ı������Ƿ�����˫����
	* - TRUE �ǣ� FALSE ��
	*/
	BOOL GetIsAddDoubleQuot() { return _IsAddDoubleQuot; }

	/**
	* �����ı������Ƿ�����˫����
	* @param [in] bAdd		�Ƿ�����˫���ţ�Ĭ��Ϊtrue��
	*/
	void SetIsAddDoubleQuot(BOOL bAdd = true) { _IsAddDoubleQuot = bAdd; }


	/**
	* ȡ�� XML �ַ���
	* @param [out] xml		XML�ַ���
	* @param [in] error
	* @return	����ȡ�� XML �ַ����Ƿ�ɹ�
	*/
	BOOL GetXmlString(CString& xml, CString* error = NULL);

	xml_node_type GetType() { return _Type; }
	void SetType(xml_node_type type){ _Type = type; }

	CXmlElement *GetParent() { return _parent; }

	void * GetAllocator() { return _alloc; }
protected:

	/**
	* Ϊ�ַ�������˫����
	* @param [in] str	�ַ���
	* @return	��������˫���ź���ַ���
	*/
	CString AddDoubleQuot(LPCTSTR str);

	/**
	* ɾ���ַ���˫����
	* @param [in] str	�ַ���
	* @return	����ɾ��˫���ź���ַ���
	*/
	CString DelDoubleQuot(LPCTSTR str);

	/**
	* �Ѷ�����XML�ļ�ת����Element
	* @param [in] pStream	��������ȡ����
	* @return	����TRUE
	*/
	BOOL ParseXmdElement(IDataReadStream *pStream);

	/**
	* ��Elementת���ɶ�����XML�ļ�
	* @param [in] pStream	������д�����
	* @param [in] dwFlags ��ʽ��� XMD_FLAGS_SAVEFORMAT_UNICODE �� XMD_FLAGS_SAVEFORMAT_ANSI
	* @return	����TRUE
	*/
	BOOL CreateXmdElement(IDataWriteStream *pStream, DWORD dwFlags);

protected:
	/** ���������� */
	CXmlElements*		_ElementContainer;
	/** ���Բ������� */
	CXmlAttributes*		_AttributeContainer;

	/** ������� */
	LPTSTR				_ElementText;
	/** ������� */
	LPTSTR				_ElementName;

	/** _ElementText����ʱ�Ƿ��Զ����˫���� */
	BOOL				_IsAddDoubleQuot;

	CXmlElement*		_parent;

	xml_node_type		_Type;

	void*				_alloc;
};

//--------------------------------------------------------
// CXmlDocument
//--------------------------------------------------------

/** �����ĵ���ʽö�����͡�*/
enum DocFormatEnum
{
	fmtXML,	/**< XML �ı���ʽ */
	fmtXMD,	/**< XML �����Ƹ�ʽ */
	fmtXMLUTF8, /**< UTF-8���͵��ı���ʽ(Ŀǰֻ֧�ֶ�ȡUTF-8���ͣ���֧�ֱ���ɴ˸�ʽ */
	fmtXMLANSI,		/**< ��XML�ļ������ANSI��ʽ*/
	fmtXMDANSI,		/**< XML �����Ƹ�ʽ(�ַ���ANSI��ʽ) */
};

// �����UNICODE��ʽ��<XMD 1.0>Ĭ�ϸ�ʽ��
#define XMD_FLAGS_SAVEFORMAT_UNICODE			0x00
// �����ANSI��ʽ
#define XMD_FLAGS_SAVEFORMAT_ANSI				0x01

//! �ĵ�����
class  CXmlDocument
{
	friend struct xml_parser;
private:
	CXmlDocument(CXmlDocument&);
public:
	CXmlDocument(void);
	~CXmlDocument(void);

	/**
	* ���� XML �ļ�
	* @param [in] XmlFile			XML�ļ�
	* @param [in] format			XML��ʽö��
	* @return	���ؼ��� XML �ļ��Ƿ�ɹ�
	* - TRUE ���سɹ��� FALSE ����ʧ��
	*/
	BOOL LoadFile(LPCTSTR XmlFile, DocFormatEnum format);

	/**
	* ���� XML �ļ�
	* @param [in] FileHandle		XML�ļ����
	* @return	���ؼ��� XML �ļ��Ƿ�ɹ�
	* - TRUE ���سɹ��� FALSE ����ʧ��
	*/
	BOOL LoadXmlFile(HANDLE FileHandle);
	/**
	* ���� XMD �ļ�
	* @param [in] FileHandle		XMD�ļ����
	* @return	���ؼ��� XML �ļ��Ƿ�ɹ�
	* - TRUE ���سɹ��� FALSE ����ʧ��
	*/
	BOOL LoadXmdFile(HANDLE FileHandle);

	/**
	* ���� UTF-8���ļ�
	* @param [in] FileHandle		UTF-8���ļ����
	* @return	���ؼ��� XML �ļ��Ƿ�ɹ�
	* - TRUE ���سɹ��� FALSE ����ʧ��
	*/
	BOOL LoadXmlUtf8File(HANDLE FileHandle);

	/**
	* ���� XML �ļ�
	* @param [in] XmlFile			XML�ļ�
	* @param [in] format			XML��ʽö��
	* @return	���ر��� XML �ļ��Ƿ�ɹ�
	* - TRUE ����ɹ��� FALSE ����ʧ��
	*/
	BOOL SaveFile(LPCTSTR XmlFile, DocFormatEnum format);

	/**
	* ���� XML �ļ�
	* @param [in] FileHandle		XML�ļ����
	* @return	���ر��� XML �ļ��Ƿ�ɹ�
	* - TRUE ����ɹ��� FALSE ����ʧ��
	*/
	BOOL SaveXmlFile(HANDLE FileHandle);
	/**
	* ���� XMD �ļ�
	* @param [in] FileHandle		XMD�ļ����
	* @param [in] dwFlags		��ʽ��� XMD_FLAGS_SAVEFORMAT_UNICODE �� XMD_FLAGS_SAVEFORMAT_ANSI
	* @return	���ر��� XMD �ļ��Ƿ�ɹ�
	* - TRUE ����ɹ��� FALSE ����ʧ��
	*/
	BOOL SaveXmdFile(HANDLE FileHandle, DWORD dwFlags = XMD_FLAGS_SAVEFORMAT_UNICODE);

	/**
	* ���� XML ANSI���ļ�
	* @param [in] FileHandle		XML�ļ����
	* @return	���ر��� XML �ļ��Ƿ�ɹ�
	*/
	BOOL SaveXmlANSIFile(HANDLE FileHandle);

	/**
	* ���� XML UTF-8���ļ�
	* @param [in] FileHandle		XML�ļ����
	* @return	���ر��� XML �ļ��Ƿ�ɹ�
	* - TRUE ����ɹ��� FALSE ����ʧ��
	*/
	BOOL SaveXmlUtf8File(HANDLE FileHandle);

	/**
	* ���� XML �ַ���
	* @param [in] xml		XML�ַ���
	* @param [in] error		�������ṹ��Ĭ��ΪNULL��
	* @return	���ؼ��� XML �ַ����Ƿ�ɹ�
	* - TRUE ���سɹ��� FALSE ����ʧ��
	*/
	BOOL SetXmlString(LPCTSTR xml);

	/**
	* ȡ�� XML �ַ���
	* @param [out] xml		XML�ַ���
	* @param [in] error		�������ṹ��Ĭ��ΪNULL��
	* @return	����ȡ�� XML �ַ����Ƿ�ɹ�
	* - TRUE ��ȡ�ɹ��� FALSE ��ȡʧ��
	*/
	BOOL GetXmlString(CString& xml, CString* error = NULL);

	/**
	* �����м���XMD����
	* @param [in] pDataStream	��������ȡ����
	* @return	���ؼ���XMD�����Ƿ�ɹ�
	* - TRUE ��ȡ�ɹ��� FALSE ��ȡʧ��
	*/
	BOOL LoadXmdFromStream(IDataReadStream *pDataStream);

	/**
	* ����XMD���ݵ���������
	* @param [in] pDataStream	������д�����
	* @param [in] dwFlags	��ʽ��� XMD_FLAGS_SAVEFORMAT_UNICODE �� XMD_FLAGS_SAVEFORMAT_ANSI
	* @return	���ر���XMD�����Ƿ�ɹ�
	* - TRUE ����ɹ��� FALSE ����ʧ��
	*/
	BOOL SaveXmdToStream(IDataWriteStream *pDataStream, DWORD dwFlags = XMD_FLAGS_SAVEFORMAT_UNICODE);

	/**
	* ȡ�� XML �����
	* @return	����XML �����
	*/
	CXmlElement* GetElementRoot(void);

	CXmlElement* GetDocumentElement(void);

	/**
	* ɾ������
	*/
	void Clear();

	/**
	* ȡ�����һ�δ�����Ϣ
	* @return	�������һ�δ�����Ϣ
	*/
	LPCTSTR GetLastError(void);

	/**
	* ����ʱ�Ƿ��ʽ���ı�
	* @return	�����Ƿ��ʽ���ı�
	*/
	BOOL GetIsFormatSave(void);

	/**
	* �����Ƿ��ʽ�������ı�
	* @param [in] bFormat	�Ƿ��ʽ�������ı���Ĭ��Ϊtrue��
	*/
	void SetIsFormatSave(BOOL bFormat = true);

	void *alloc;
protected:

	bool load_buffer_impl(void* contents, size_t size, unsigned int options, int encode, bool is_mutable, bool own);

	CXmlElement		*_Doc_node;

	/** ���ڵ� */
	CXmlElement		*_ElementRoot;

	/** ���һ�δ����ı� */
	CString			_LastError;
	/** ����ʱ�Ƿ��ʽ���ı� (���ṩ�ȽϺõĿɶ���) */
	DWORD			_Option;
};