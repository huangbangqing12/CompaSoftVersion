#include "stdafx.h"
#include "XmlDocument.h"
#include "FileDataStream.h"
#include "..\commkit\LibToolkit.h"
#include "..\commkit\DataConvert.h"
#include <list>

// ����ͳһ��������
//#define _XMLDOC_ALLOC

#ifdef _XMLDOC_ALLOC
#include "AllocObjectPool.h"
#include "AllocMemoryPool.h"
#endif

//===================================================================
// ThXmlDoc
//===================================================================

#ifdef _XMLDOC_ALLOC
// ����ȫ�ֵĽ��������
CAllocObjectPool<CXmlElement>		ElementAlloc;
CAllocObjectPool<CXmlAttribute>		AttributeAlloc;
CAllocObjectPool<CXmlElements>		ElementsAlloc;
CAllocObjectPool<CXmlAttributes>	AttributesAlloc;
CAllocMemoryPool					MemoryPool;

#define ELEMENTGROWBY 2000
#define ATTRIBUTEGROWBY 4000

#define ELEMENTSGROWBY 2000
#define ATTRIBUTESGROWBY 2000

//	#define MemoryPool_Alloc(x) MemoryPool.Alloc((x),5000)
//	#define MemoryPool_Free(x) MemoryPool.Free((x))
//#else
//	#define MemoryPool_Alloc(x) malloc(x)
//	#define MemoryPool_Free(x) free(x)
#endif


// ��ʽ��� ��Formatting flags��
const unsigned int format_indent = 0x010000;// ���ʱ����ȶԽڵ�����. Ĭ�Ͽ���.
const unsigned int format_write_bom = 0x020000;// �������ָ���� BOM. Ĭ�Ϲر�.
const unsigned int format_raw = 0x040000;// ʹ��RAWģʽ���(��������������). Ĭ�Ϲر�.
const unsigned int format_no_declaration = 0x080000;// ʡ��Ĭ�ϵ�XML��������ʹ�ĵ���û������. Ĭ�Ϲر�.
const unsigned int format_no_escapes = 0x100000;// Don't escape attribute values and PCDATA contents. This flag is off by default.
const unsigned int format_default = format_indent;// Ĭ�ϸ�ʽ���.��������, �ĵ���û������ʱ���Ĭ������.

// XML����ѡ�� ��Parsing options��
const unsigned int parse_minimal = 0x0000;// ��С����ģʽ (�൱�ڹر������������).ֻ�� elements �� PCDATA ����ӵ�DOM��, �������ı�ת��.
const unsigned int parse_pi = 0x0001;// �Ƿ� (node_pi) ��ӵ�DOM��. Ĭ�Ϲر�.
const unsigned int parse_comments = 0x0002;// �Ƿ� ע�ͽڵ�(node_comment) ��ӵ�DOM��. Ĭ�Ϲر�.
const unsigned int parse_cdata = 0x0004;// �Ƿ� (node_cdata)�� ��ӵ�DOM��. Ĭ�Ͽ���.
const unsigned int parse_ws_pcdata = 0x0008;// �Ƿ� �հ׵�(node_pcdata) ��ӵ�DOM��. // Ĭ�Ϲر�; �򿪸ñ��ͨ�������ת���ٶȣ������ڴ�����.
// This flag determines if character and entity references are expanded during parsing. This flag is on by default.
const unsigned int parse_escapes = 0x0010;
const unsigned int parse_eol = 0x0020;// �����������Ƿ����滯��β(EOL)�ַ� (ת��Ϊ #xA). Ĭ�Ͽ���.
// This flag determines if attribute values are normalized using CDATA normalization rules during parsing. This flag is on by default.
const unsigned int parse_wconv_attribute = 0x0040;
// This flag determines if attribute values are normalized using NMTOKENS normalization rules during parsing. This flag is off by default.
const unsigned int parse_wnorm_attribute = 0x0080;
// �Ƿ�(node_declaration) ��ӵ�DOM��. Ĭ�Ϲر�.
const unsigned int parse_declaration = 0x0100;
// �Ƿ�(node_doctype) ��ӵ�DOM��. Ĭ�Ϲر�.
const unsigned int parse_doctype = 0x0200;
// This flag determines if plain character data (node_pcdata) that is the only child of the parent node and that consists only
// of whitespace is added to the DOM tree.
// This flag is off by default; turning it on may result in slower parsing and more memory consumption.
const unsigned int parse_ws_pcdata_single = 0x0400;
// Ĭ�Ͻ���ģʽ.
// ��Elements, PCDATA and CDATA ����ӵ�DOM��, character/reference entities are expanded,
// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
const unsigned int parse_default = parse_cdata | parse_escapes | parse_wconv_attribute | parse_eol;
// ��ȫ����ģʽ.
// Nodes of all types are added to the DOM tree, character/reference entities are expanded,
// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
const unsigned int parse_full = parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype;

// XML����ӿ�. Writer interface for node printing (see xml_node::print)
class xml_writer
{
public:
	virtual ~xml_writer() {}

	// ����ڴ�鵽�����ļ�������
	virtual void write(const void* data, size_t size) = 0;
};

// XML����ӿڵ��ļ�ʵ��
class xml_writer_file : public xml_writer
{
public:
	xml_writer_file(void* file) : _file((HANDLE)file) {}
	~xml_writer_file(){ _file.Detach(); }

	virtual void write(const void* data, size_t size)
	{
		_file.Write(data, DWORD(size));
	}

private:
	CAtlFile _file;
};

enum xml_encoding
{
	encode_UTF8 = 0,
	encode_ANSI = 1,
	encode_UTF16 = 2,
	encode_UTF32 = 4,
	encode_UNKONW,
};

struct opt_false
{
	enum { value = 0 };
};

struct opt_true
{
	enum { value = 1 };
};

void* default_allocate(size_t size)
{
	return malloc(size);
}

void default_deallocate(void* ptr)
{
	free(ptr);
}

// Memory allocation function interface; returns pointer to allocated memory or NULL on failure
typedef void* (*allocation_function)(size_t size);

// Memory deallocation function interface
typedef void(*deallocation_function)(void* ptr);

template <typename T>
struct xml_memory_management_function_storage
{
	static allocation_function allocate;
	static deallocation_function deallocate;
};

template <typename T> allocation_function xml_memory_management_function_storage<T>::allocate = default_allocate;
template <typename T> deallocation_function xml_memory_management_function_storage<T>::deallocate = default_deallocate;

typedef xml_memory_management_function_storage<int> xml_memory;


static const size_t xml_memory_page_size =
#ifdef PUGIXML_MEMORY_PAGE_SIZE
PUGIXML_MEMORY_PAGE_SIZE
#else
32768
#endif
;

static const uintptr_t xml_memory_page_alignment = 32;
static const uintptr_t xml_memory_page_pointer_mask = ~(xml_memory_page_alignment - 1);
static const uintptr_t xml_memory_page_name_allocated_mask = 16;
static const uintptr_t xml_memory_page_value_allocated_mask = 8;
static const uintptr_t xml_memory_page_type_mask = 7;

struct xml_memory_string_header
{
	WORD page_offset; // offset from page->data
	WORD full_size; // 0 if string occupies whole page
};


struct xml_allocator
{
	struct xml_memory_page
	{
		void* memory;

		size_t busy_size;
		size_t freed_size;

		char data[1];

	};
	std::list<xml_memory_page*> pages;
	xml_allocator() : _root(0), _busy_size(xml_memory_page_size + 1)
	{
		xml_memory_page* page = allocate_page(xml_memory_page_size);
		pages.push_back(page);
		_root = page;
		_root->busy_size = _busy_size = 0;

		//_root = allocate_page(xml_memory_page_size);
	}
	~xml_allocator()
	{
		std::list<xml_memory_page*>::iterator i;
		for (i = pages.begin(); i != pages.end(); ++i)
			deallocate_page(*i);
		pages.clear();
	}

	xml_memory_page* allocate_page(size_t data_size)
	{
		size_t size = offsetof(xml_memory_page, data) + data_size;

		// allocate block with some alignment, leaving memory for worst-case padding
		void* memory = xml_memory::allocate(size);
		if (!memory) return 0;
		memset(memory, 0, size);

		return static_cast<xml_memory_page*>(memory);
	}

	static void deallocate_page(xml_memory_page* page)
	{
		xml_memory::deallocate(page);
	}

	void* allocate_memory_oob(size_t size, xml_memory_page*& out_page)
	{
		const size_t large_allocation_threshold = xml_memory_page_size / 4;

		xml_memory_page* page = allocate_page(size <= large_allocation_threshold ? xml_memory_page_size : size);
		out_page = page;

		if (!page) return 0;

		if (size <= large_allocation_threshold)
		{
			_root->busy_size = _busy_size;

			// insert page at the end of linked list
			pages.push_back(page);
			_root = page;

			_busy_size = size;
		}
		else
		{
			// insert page before the end of linked list, so that it is deleted as soon as possible
			// the last page is not deleted even if it's empty (see deallocate_memory)
			pages.insert(--(pages.end()), page);
		}

		// allocate inside page
		page->busy_size = size;

		return page->data;
	}

	void* allocate_memory(size_t size, xml_memory_page*& out_page)
	{
		if (_busy_size + size > xml_memory_page_size) return allocate_memory_oob(size, out_page);

		void* buf = _root->data + _busy_size;

		_busy_size += size;

		out_page = _root;

		return buf;
	}

	void deallocate_memory(void* ptr, size_t size, xml_memory_page* page)
	{
		if (page == _root) page->busy_size = _busy_size;

		ATLASSERT(ptr >= page->data && ptr < page->data + page->busy_size);

		page->freed_size += size;
		ATLASSERT(page->freed_size <= page->busy_size);

		if (page->freed_size == page->busy_size)
		{
			if (page == pages.back())
			{
				ATLASSERT(_root == page);

				// top page freed, just reset sizes
				page->busy_size = page->freed_size = 0;
				_busy_size = 0;
			}
			else
			{
				ATLASSERT(_root != page);
				ATLASSERT(page == *(pages.begin()));

				// remove from the list
				pages.remove(page);

				// deallocate
				deallocate_page(page);
			}
		}
	}

	TCHAR* allocate_string(size_t length)
	{
		// allocate memory for string and header block
		size_t size = sizeof(xml_memory_string_header) + length * sizeof(TCHAR);

		// round size up to pointer alignment boundary
		size_t full_size = (size + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);

		xml_memory_page* page;
		xml_memory_string_header* header = static_cast<xml_memory_string_header*>(allocate_memory(full_size, page));

		if (!header) return 0;

		// setup header
		ptrdiff_t page_offset = reinterpret_cast<char*>(header)-page->data;

		ATLASSERT(page_offset >= 0 && page_offset < (1 << 16));
		header->page_offset = static_cast<WORD>(page_offset);

		// full_size == 0 for large strings that occupy the whole page
		ATLASSERT(full_size < (1 << 16) || (page->busy_size == full_size && page_offset == 0));
		header->full_size = static_cast<WORD>(full_size < (1 << 16) ? full_size : 0);

		// round-trip through void* to avoid 'cast increases required alignment of target type' warning
		// header is guaranteed a pointer-sized alignment, which should be enough for TCHAR
		return static_cast<TCHAR*>(static_cast<void*>(header + 1));
	}

	void deallocate_string(TCHAR* string)
	{
		if (string == NULL)
			return;
		// this function casts pointers through void* to avoid 'cast increases required alignment of target type' warnings
		// we're guaranteed the proper (pointer-sized) alignment on the input string if it was allocated via allocate_string

		// get header
		xml_memory_string_header* header = static_cast<xml_memory_string_header*>(static_cast<void*>(string)) - 1;

		// deallocate
		size_t page_offset = offsetof(xml_memory_page, data) + header->page_offset;
		xml_memory_page* page = reinterpret_cast<xml_memory_page*>(static_cast<void*>(reinterpret_cast<char*>(header)-page_offset));

		// if full_size == 0 then this string occupies the whole page
		size_t full_size = header->full_size == 0 ? page->busy_size : header->full_size;

		deallocate_memory(header, full_size, page);
	}

	xml_memory_page* _root;
	size_t _busy_size;
};





xml_encoding guess_buffer_encoding(BYTE d0, BYTE d1, BYTE d2, BYTE d3)
{
	// look for BOM in first few bytes
	if (d0 == 0xff && d1 == 0xfe && d2 == 0 && d3 == 0) return encode_UTF32;
	if (d0 == 0xff && d1 == 0xfe) return encode_UTF16;
	if (d0 == 0xef && d1 == 0xbb && d2 == 0xbf) return encode_UTF8;

	// look for <, <? or <?xm in various encodings
	if (d0 == 0x3c && d1 == 0 && d2 == 0 && d3 == 0) return encode_UTF32;
	if (d0 == 0x3c && d1 == 0 && d2 == 0x3f && d3 == 0) return encode_UTF16;
	if (d0 == 0x3c && d1 == 0x3f && d2 == 0x78 && d3 == 0x6d) return encode_ANSI;

	// look for utf16 < followed by node name (this may fail, but is better than ansi since it's zero terminated so early)
	if (d0 == 0x3c && d1 == 0) return encode_UTF16;

	// no known BOM detected, assume utf8
	return encode_UNKONW;
}



enum chartype_t
{
	ct_parse_pcdata = 1,	// \0, &, \r, <
	ct_parse_attr = 2,		// \0, &, \r, ', "
	ct_parse_attr_ws = 4,	// \0, &, \r, ', ", \n, tab
	ct_space = 8,			// \r, \n, space, tab
	ct_parse_cdata = 16,	// \0, ], >, \r
	ct_parse_comment = 32,	// \0, -, >, \r
	ct_symbol = 64,			// Any symbol > 127, a-z, A-Z, 0-9, _, :, -, .
	ct_start_symbol = 128	// Any symbol > 127, a-z, A-Z, _, :
};

static const unsigned char chartype_table[256] =
{
	55, 0, 0, 0, 0, 0, 0, 0, 0, 12, 12, 0, 0, 63, 0, 0,   // 0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 16-31
	8, 0, 6, 0, 0, 0, 7, 6, 192, 192, 192, 192, 192, 224, 192, 0,   // 32-47
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 1, 0, 48, 0,   // 48-63
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, // 64-79
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 0, 208, 192, 192, // 80-95
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, // 96-111
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,           // 112-127

	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, // 128+
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
};


enum chartypex_t
{
	ctx_special_pcdata = 1,   // Any symbol >= 0 and < 32 (except \t, \r, \n), &, <, >
	ctx_special_attr = 2,     // Any symbol >= 0 and < 32 (except \t), &, <, >, "
	ctx_start_symbol = 4,	  // Any symbol > 127, a-z, A-Z, _
	ctx_digit = 8,			  // 0-9
	ctx_symbol = 16			  // Any symbol > 127, a-z, A-Z, 0-9, _, -, .
};

static const unsigned char chartypex_table[256] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 2, 3, 3,     // 0-15
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // 16-31
	0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 16, 16, 0,     // 32-47
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0, 3, 0, 3, 0,     // 48-63

	0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,    // 64-79
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0, 0, 0, 0, 20,    // 80-95
	0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,    // 96-111
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0, 0, 0, 0, 0,     // 112-127

	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,    // 128+
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20
};

#define PUGI__IS_CHARTYPE_IMPL(c, ct, table) ((static_cast<unsigned int>(c) < 128 ? table[static_cast<unsigned int>(c)] : table[128]) & (ct))

#define PUGI__IS_CHARTYPE(c, ct) PUGI__IS_CHARTYPE_IMPL(c, ct, chartype_table)
#define PUGI__IS_CHARTYPEX(c, ct) PUGI__IS_CHARTYPE_IMPL(c, ct, chartypex_table)
#define LINE_END '\r', '\n'

bool convert_buffer(TCHAR*& out_buffer, size_t& out_length, xml_encoding encoding, const void* contents, size_t size, bool is_mutable)
{
	// fast path: no conversion required
	if (encoding == encode_UTF16)
	{
		if (is_mutable)
		{
			out_buffer = static_cast<TCHAR*>(const_cast<void*>(contents));
		}
		else
		{
			void* buffer = malloc(size > 0 ? size : 1);
			if (!buffer) return false;

			memcpy(buffer, contents, size);

			out_buffer = static_cast<TCHAR*>(buffer);
		}

		out_length = size / sizeof(TCHAR);
	}
	else
	{
		int code_page = (encoding == encode_UTF8) ? CP_UTF8 : CP_ACP;
		char *contentbuffer = static_cast<char*>(const_cast<void*>(contents));
		CA2W wideStr(contentbuffer, code_page);

		out_length = wcslen(wideStr);
		void* buffer = malloc(out_length*sizeof(TCHAR));
		if (!buffer) return false;
		memcpy(buffer, wideStr.m_psz, out_length*sizeof(TCHAR));
		out_buffer = static_cast<TCHAR*>(buffer);
	}
	return false;
}



bool strcpy_insitu_allow(size_t length, TCHAR* target)
{
	if (target == NULL)
		return true;

	size_t target_length = _tcslen(target);

	// reuse heap memory if waste is not too great
	const size_t reuse_threshold = 32;

	return target_length >= length && (target_length < reuse_threshold || target_length - length < target_length / 2);
}


bool strcpy_insitu(xml_allocator* alloc, TCHAR*& dest, const TCHAR* source)
{
	size_t source_length = _tcslen(source);

	if (source_length == 0)
	{
		// empty string and null pointer are equivalent, so just deallocate old memory
		alloc->deallocate_string(dest);

		// mark the string as not allocated
		dest = 0;
		return true;
	}
	else if (dest && strcpy_insitu_allow(source_length, dest))
	{
		// we can reuse old buffer, so just copy the new data (including zero terminator)
		memcpy(dest, source, (source_length + 1) * sizeof(TCHAR));

		return true;
	}
	else
	{
		// allocate new buffer
		TCHAR* buf = alloc->allocate_string(source_length + 1);
		if (!buf) return false;

		// copy the string (including zero terminator)
		memcpy(buf, source, (source_length + 1) * sizeof(TCHAR));

		// deallocate old buffer (*after* the above to protect against overlapping memory and/or allocation failures)
		alloc->deallocate_string(dest);

		// the string is now allocated, so set the flag
		dest = buf;
		return true;
	}
}



struct gap
{
	TCHAR* end;
	size_t size;

	gap() : end(0), size(0)
	{
	}

	// Push new gap, move s count bytes further (skipping the gap).
	// Collapse previous gap.
	void push(TCHAR*& s, size_t count)
	{
		if (end) // there was a gap already; collapse it
		{
			// Move [old_gap_end, new_gap_start) to [old_gap_start, ...)
			ATLASSERT(s >= end);
			memmove(end - size, end, reinterpret_cast<char*>(s)-reinterpret_cast<char*>(end));
		}

		s += count; // end of current gap

		// "merge" two gaps
		end = s;
		size += count;
	}

	// Collapse all gaps, return past-the-end pointer
	TCHAR* flush(TCHAR* s)
	{
		if (end)
		{
			// Move [old_gap_end, current_pos) to [old_gap_start, ...)
			ATLASSERT(s >= end);
			memmove(end - size, end, reinterpret_cast<char*>(s)-reinterpret_cast<char*>(end));

			return s - size;
		}
		else return s;
	}
};


TCHAR* strconv_escape(TCHAR* s, gap& g)
{
	TCHAR* stre = s + 1;

	switch (*stre)
	{
	case '#':	// &#...
	{
		unsigned int ucsc = 0;

		if (stre[1] == 'x') // &#x... (hex code)
		{
			stre += 2;

			TCHAR ch = *stre;

			if (ch == ';') return stre;

			for (;;)
			{
				if (static_cast<unsigned int>(ch - '0') <= 9)
					ucsc = 16 * ucsc + (ch - '0');
				else if (static_cast<unsigned int>((ch | ' ') - 'a') <= 5)
					ucsc = 16 * ucsc + ((ch | ' ') - 'a' + 10);
				else if (ch == ';')
					break;
				else // cancel
					return stre;

				ch = *++stre;
			}

			++stre;
		}
		else	// &#... (dec code)
		{
			TCHAR ch = *++stre;

			if (ch == ';') return stre;

			for (;;)
			{
				if (static_cast<unsigned int>(ch - '0') <= 9)
					ucsc = 10 * ucsc + (ch - '0');
				else if (ch == ';')
					break;
				else // cancel
					return stre;

				ch = *++stre;
			}

			++stre;
		}

		*s = static_cast<TCHAR>(ucsc);
		++s;

		g.push(s, stre - s);
		return stre;
	}

	case 'a':	// &a
	{
		++stre;

		if (*stre == 'm') // &am
		{
			if (*++stre == 'p' && *++stre == ';') // &amp;
			{
				*s++ = '&';
				++stre;

				g.push(s, stre - s);
				return stre;
			}
		}
		else if (*stre == 'p') // &ap
		{
			if (*++stre == 'o' && *++stre == 's' && *++stre == ';') // &apos;
			{
				*s++ = '\'';
				++stre;

				g.push(s, stre - s);
				return stre;
			}
		}
		break;
	}

	case 'g': // &g
	{
		if (*++stre == 't' && *++stre == ';') // &gt;
		{
			*s++ = '>';
			++stre;

			g.push(s, stre - s);
			return stre;
		}
		break;
	}

	case 'l': // &l
	{
		if (*++stre == 't' && *++stre == ';') // &lt;
		{
			*s++ = '<';
			++stre;

			g.push(s, stre - s);
			return stre;
		}
		break;
	}

	case 'q': // &q
	{
		if (*++stre == 'u' && *++stre == 'o' && *++stre == 't' && *++stre == ';') // &quot;
		{
			*s++ = '"';
			++stre;

			g.push(s, stre - s);
			return stre;
		}
		break;
	}

	default:
		break;
	}

	return stre;
}

// Utility macro for last character handling
#define ENDSWITH(c, e) ((c) == (e) || ((c) == 0 && endch == (e)))

TCHAR* strconv_comment(TCHAR* s, TCHAR endch)
{
	gap g;

	while (true)
	{
		while (!PUGI__IS_CHARTYPE(*s, ct_parse_comment)) ++s;

		if (*s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
		{
			*s++ = '\n'; // replace first one with 0x0a

			if (*s == '\n') g.push(s, 1);
		}
		else if (s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>')) // comment ends here
		{
			*g.flush(s) = 0;

			return s + (s[2] == '>' ? 3 : 2);
		}
		else if (*s == 0)
		{
			return 0;
		}
		else ++s;
	}
}

TCHAR* strconv_cdata(TCHAR* s, TCHAR endch)
{
	gap g;

	while (true)
	{
		while (!PUGI__IS_CHARTYPE(*s, ct_parse_cdata)) ++s;

		if (*s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
		{
			*s++ = '\n'; // replace first one with 0x0a

			if (*s == '\n') g.push(s, 1);
		}
		else if (s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>')) // CDATA ends here
		{
			*g.flush(s) = 0;

			return s + 1;
		}
		else if (*s == 0)
		{
			return 0;
		}
		else ++s;
	}
}

typedef TCHAR* (*strconv_pcdata_t)(TCHAR*);

template <typename opt_eol, typename opt_escape> struct strconv_pcdata_impl
{
	static TCHAR* parse(TCHAR* s)
	{
		gap g;

		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_pcdata)) ++s;

			if (*s == '<') // PCDATA ends here
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (opt_eol::value && *s == '\r') // Either a single 0x0d or 0x0d 0x0a pair
			{
				*s++ = '\n'; // replace first one with 0x0a

				if (*s == '\n') g.push(s, 1);
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (*s == 0)
			{
				return s;
			}
			else ++s;
		}
	}
};

strconv_pcdata_t get_strconv_pcdata(unsigned int optmask)
{
	switch ((optmask >> 4) & 3) // get bitmask for flags (eol escapes)
	{
	case 0: return strconv_pcdata_impl<opt_false, opt_false>::parse;
	case 1: return strconv_pcdata_impl<opt_false, opt_true>::parse;
	case 2: return strconv_pcdata_impl<opt_true, opt_false>::parse;
	case 3: return strconv_pcdata_impl<opt_true, opt_true>::parse;
	default: return 0; // should not get here
	}
}

typedef TCHAR* (*strconv_attribute_t)(TCHAR*, TCHAR);

template <typename opt_escape> struct strconv_attribute_impl
{
	static TCHAR* parse_wnorm(TCHAR* s, TCHAR end_quote)
	{
		gap g;

		// trim leading whitespaces
		if (PUGI__IS_CHARTYPE(*s, ct_space))
		{
			TCHAR* str = s;

			do ++str;
			while (PUGI__IS_CHARTYPE(*str, ct_space));

			g.push(s, str - s);
		}

		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr_ws | ct_space)) ++s;

			if (*s == end_quote)
			{
				TCHAR* str = g.flush(s);

				do *str-- = 0;
				while (PUGI__IS_CHARTYPE(*str, ct_space));

				return s + 1;
			}
			else if (PUGI__IS_CHARTYPE(*s, ct_space))
			{
				*s++ = ' ';

				if (PUGI__IS_CHARTYPE(*s, ct_space))
				{
					TCHAR* str = s + 1;
					while (PUGI__IS_CHARTYPE(*str, ct_space)) ++str;

					g.push(s, str - s);
				}
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static TCHAR* parse_wconv(TCHAR* s, TCHAR end_quote)
	{
		gap g;

		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr_ws)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (PUGI__IS_CHARTYPE(*s, ct_space))
			{
				if (*s == '\r')
				{
					*s++ = ' ';

					if (*s == '\n') g.push(s, 1);
				}
				else *s++ = ' ';
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static TCHAR* parse_eol(TCHAR* s, TCHAR end_quote)
	{
		gap g;

		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (*s == '\r')
			{
				*s++ = '\n';

				if (*s == '\n') g.push(s, 1);
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}

	static TCHAR* parse_simple(TCHAR* s, TCHAR end_quote)
	{
		gap g;

		while (true)
		{
			while (!PUGI__IS_CHARTYPE(*s, ct_parse_attr)) ++s;

			if (*s == end_quote)
			{
				*g.flush(s) = 0;

				return s + 1;
			}
			else if (opt_escape::value && *s == '&')
			{
				s = strconv_escape(s, g);
			}
			else if (!*s)
			{
				return 0;
			}
			else ++s;
		}
	}
};

strconv_attribute_t get_strconv_attribute(unsigned int optmask)
{
	switch ((optmask >> 4) & 15) // get bitmask for flags (wconv wnorm eol escapes)
	{
	case 0:  return strconv_attribute_impl<opt_false>::parse_simple;
	case 1:  return strconv_attribute_impl<opt_true>::parse_simple;
	case 2:  return strconv_attribute_impl<opt_false>::parse_eol;
	case 3:  return strconv_attribute_impl<opt_true>::parse_eol;
	case 4:  return strconv_attribute_impl<opt_false>::parse_wconv;
	case 5:  return strconv_attribute_impl<opt_true>::parse_wconv;
	case 6:  return strconv_attribute_impl<opt_false>::parse_wconv;
	case 7:  return strconv_attribute_impl<opt_true>::parse_wconv;
	case 8:  return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 9:  return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 10: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 11: return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 12: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 13: return strconv_attribute_impl<opt_true>::parse_wnorm;
	case 14: return strconv_attribute_impl<opt_false>::parse_wnorm;
	case 15: return strconv_attribute_impl<opt_true>::parse_wnorm;
	default: return 0; // should not get here
	}
}

// ����״̬, ��Ϊ xml_parse_result �����һ����
enum xml_parse_status
{
	status_ok = 0,				// û�д���

	status_file_not_found,		// load_file()ʱ�Ҳ����ļ�
	status_io_error,			// Error reading from file/stream
	status_out_of_memory,		// Could not allocate memory
	status_internal_error,		// Internal error occurred

	status_unrecognized_tag,	// Parser could not determine tag type

	status_bad_pi,				// Parsing error occurred while parsing document declaration/processing instruction
	status_bad_comment,			// Parsing error occurred while parsing comment
	status_bad_cdata,			// Parsing error occurred while parsing CDATA section
	status_bad_doctype,			// Parsing error occurred while parsing document type declaration
	status_bad_pcdata,			// Parsing error occurred while parsing PCDATA section
	status_bad_start_element,	// Parsing error occurred while parsing start element tag
	status_bad_attribute,		// Parsing error occurred while parsing element attribute
	status_bad_end_element,		// Parsing error occurred while parsing end element tag
	status_end_element_mismatch // There was a mismatch of start-end tags (closing tag had incorrect name, some tag was not closed or there was an excessive closing tag)
};


struct xml_parser
{
	xml_allocator &alloc;
	TCHAR* error_offset;
	xml_parse_status error_status;

	// Parser utilities.
#define PUGI__SKIPWS()			{ while (PUGI__IS_CHARTYPE(*s, ct_space)) ++s; }
#define PUGI__OPTSET(OPT)			( optmsk & (OPT) )
#define PUGI__PUSHNODE(TYPE)		{ CXmlElement*temp = cursor; cursor = cursor->GetChildElements()->Add(); cursor->_alloc = temp->_alloc; cursor->_Type = TYPE;  if (!cursor) PUGI__THROW_ERROR(status_out_of_memory, s); }
#define PUGI__POPNODE()			{ cursor = cursor->_parent; }
#define PUGI__SCANFOR(X)			{ while (*s != 0 && !(X)) ++s; }
#define PUGI__SCANWHILE(X)		{ while ((X)) ++s; }
#define PUGI__ENDSEG()			{ ch = *s; *s = 0; ++s; }
#define PUGI__THROW_ERROR(err, m)	return error_offset = m, error_status = err, false
#define PUGI__CHECK_ERROR(err, m)	{ if (*s == 0) PUGI__THROW_ERROR(err, m); }

	xml_parser(xml_allocator& alloc_) : alloc(alloc_), error_offset(0)
	{
		error_status = status_ok;
	}

	// DOCTYPE consists of nested sections of the following possible types:
	// <!-- ... -->, <? ... ?>, "...", '...'
	// <![...]]>
	// <!...>
	// First group can not contain nested groups
	// Second group can contain nested groups of the same type
	// Third group can contain all other groups
	TCHAR* parse_doctype_primitive(TCHAR* s)
	{
		if (*s == '"' || *s == '\'')
		{
			// quoted string
			TCHAR ch = *s++;
			PUGI__SCANFOR(*s == ch);
			if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

			s++;
		}
		else if (s[0] == '<' && s[1] == '?')
		{
			// <? ... ?>
			s += 2;
			PUGI__SCANFOR(s[0] == '?' && s[1] == '>'); // no need for ENDSWITH because ?> can't terminate proper doctype
			if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

			s += 2;
		}
		else if (s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-')
		{
			s += 4;
			PUGI__SCANFOR(s[0] == '-' && s[1] == '-' && s[2] == '>'); // no need for ENDSWITH because --> can't terminate proper doctype
			if (!*s) PUGI__THROW_ERROR(status_bad_doctype, s);

			s += 4;
		}
		else PUGI__THROW_ERROR(status_bad_doctype, s);

		return s;
	}

	TCHAR* parse_doctype_ignore(TCHAR* s)
	{
		ATLASSERT(s[0] == '<' && s[1] == '!' && s[2] == '[');
		s++;

		while (*s)
		{
			if (s[0] == '<' && s[1] == '!' && s[2] == '[')
			{
				// nested ignore section
				s = parse_doctype_ignore(s);
				if (!s) return s;
			}
			else if (s[0] == ']' && s[1] == ']' && s[2] == '>')
			{
				// ignore section end
				s += 3;

				return s;
			}
			else s++;
		}

		PUGI__THROW_ERROR(status_bad_doctype, s);
	}

	TCHAR* parse_doctype_group(TCHAR* s, TCHAR endch, bool toplevel)
	{
		ATLASSERT(s[0] == '<' && s[1] == '!');
		s++;

		while (*s)
		{
			if (s[0] == '<' && s[1] == '!' && s[2] != '-')
			{
				if (s[2] == '[')
				{
					// ignore
					s = parse_doctype_ignore(s);
					if (!s) return s;
				}
				else
				{
					// some control group
					s = parse_doctype_group(s, endch, false);
					if (!s) return s;
				}
			}
			else if (s[0] == '<' || s[0] == '"' || s[0] == '\'')
			{
				// unknown tag (forbidden), or some primitive group
				s = parse_doctype_primitive(s);
				if (!s) return s;
			}
			else if (*s == '>')
			{
				s++;

				return s;
			}
			else s++;
		}

		if (!toplevel || endch != '>') PUGI__THROW_ERROR(status_bad_doctype, s);

		return s;
	}

	TCHAR* parse_exclamation(TCHAR* s, CXmlElement* cursor, unsigned int optmsk, TCHAR endch)
	{
		// parse node contents, starting with exclamation mark
		++s;

		if (*s == '-') // '<!-...'
		{
			++s;

			if (*s == '-') // '<!--...'
			{
				++s;

				TCHAR* text = NULL;
				if (PUGI__OPTSET(parse_comments))
				{
					PUGI__PUSHNODE(node_comment); // Append a new node on the tree.
					text = s;	// Save the offset.
				}

				if (PUGI__OPTSET(parse_eol) && PUGI__OPTSET(parse_comments))
				{
					s = strconv_comment(s, endch);

					if (!s) PUGI__THROW_ERROR(status_bad_comment, text);
				}
				else
				{
					// Scan for terminating '-->'.
					PUGI__SCANFOR(s[0] == '-' && s[1] == '-' && ENDSWITH(s[2], '>'));
					PUGI__CHECK_ERROR(status_bad_comment, s);

					if (PUGI__OPTSET(parse_comments))
						*s = 0; // Zero-terminate this segment at the first terminating '-'.

					s += (s[2] == '>' ? 3 : 2); // Step over the '\0->'.
				}

				if (PUGI__OPTSET(parse_comments))
				{
					if (text != NULL)
						cursor->SetElementText(text);
				}


			}
			else PUGI__THROW_ERROR(status_bad_comment, s);
		}
		else if (*s == '[')
		{
			// '<![CDATA[...'
			if (*++s == 'C' && *++s == 'D' && *++s == 'A' && *++s == 'T' && *++s == 'A' && *++s == '[')
			{
				++s;

				if (PUGI__OPTSET(parse_cdata))
				{
					PUGI__PUSHNODE(node_cdata); // Append a new node on the tree.
					TCHAR* value = s; // Save the offset.

					if (PUGI__OPTSET(parse_eol))
					{
						s = strconv_cdata(s, endch);

						if (!s) PUGI__THROW_ERROR(status_bad_cdata, value);
					}
					else
					{
						// Scan for terminating ']]>'.
						PUGI__SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
						PUGI__CHECK_ERROR(status_bad_cdata, s);

						*s++ = 0; // Zero-terminate this segment.
					}
					cursor->SetElementText(s);
				}
				else // Flagged for discard, but we still have to scan for the terminator.
				{
					// Scan for terminating ']]>'.
					PUGI__SCANFOR(s[0] == ']' && s[1] == ']' && ENDSWITH(s[2], '>'));
					PUGI__CHECK_ERROR(status_bad_cdata, s);

					++s;
				}

				s += (s[1] == '>' ? 2 : 1); // Step over the last ']>'.
			}
			else PUGI__THROW_ERROR(status_bad_cdata, s);
		}
		else if (s[0] == 'D' && s[1] == 'O' && s[2] == 'C' && s[3] == 'T' && s[4] == 'Y' && s[5] == 'P' && ENDSWITH(s[6], 'E'))
		{
			s -= 2;

			if (cursor->_parent) PUGI__THROW_ERROR(status_bad_doctype, s);

			TCHAR* mark = s + 9;

			s = parse_doctype_group(s, endch, true);
			if (!s) return s;

			if (PUGI__OPTSET(parse_doctype))
			{
				while (PUGI__IS_CHARTYPE(*mark, ct_space)) ++mark;

				PUGI__PUSHNODE(node_doctype);

				TCHAR * value = mark;

				ATLASSERT((s[0] == 0 && endch == '>') || s[-1] == '>');
				s[*s == 0 ? 0 : -1] = 0;

				cursor->SetElementText(mark);

				//PUGI__POPNODE();
			}
		}
		else if (*s == 0 && endch == '-') PUGI__THROW_ERROR(status_bad_comment, s);
		else if (*s == 0 && endch == '[') PUGI__THROW_ERROR(status_bad_cdata, s);
		else PUGI__THROW_ERROR(status_unrecognized_tag, s);

		return s;
	}

	TCHAR* parse_question(TCHAR* s, CXmlElement*& ref_cursor, unsigned int optmsk, TCHAR endch)
	{
		// load into registers
		CXmlElement* cursor = ref_cursor;
		TCHAR ch = 0;

		// parse node contents, starting with question mark
		++s;

		// read PI target
		TCHAR* target = s;

		if (!PUGI__IS_CHARTYPE(*s, ct_start_symbol)) PUGI__THROW_ERROR(status_bad_pi, s);

		PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol));
		PUGI__CHECK_ERROR(status_bad_pi, s);

		// determine node type; stricmp / strcasecmp is not portable
		bool declaration = (target[0] | ' ') == 'x' && (target[1] | ' ') == 'm' && (target[2] | ' ') == 'l' && target + 3 == s;

		if (declaration ? PUGI__OPTSET(parse_declaration) : PUGI__OPTSET(parse_pi))
		{
			if (declaration)
			{
				// disallow non top-level declarations
				if (cursor->_parent) PUGI__THROW_ERROR(status_bad_pi, s);

				PUGI__PUSHNODE(node_declaration);
			}
			else
			{
				PUGI__PUSHNODE(node_pi);
			}

			PUGI__ENDSEG();
			cursor->SetElementName(target);

			// parse value/attributes
			if (ch == '?')
			{
				// empty node
				if (!ENDSWITH(*s, '>')) PUGI__THROW_ERROR(status_bad_pi, s);
				s += (*s == '>');

				PUGI__POPNODE();
			}
			else if (PUGI__IS_CHARTYPE(ch, ct_space))
			{
				PUGI__SKIPWS();

				// scan for tag end
				TCHAR* value = s;

				PUGI__SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
				PUGI__CHECK_ERROR(status_bad_pi, s);

				if (declaration)
				{
					// replace ending ? with / so that 'element' terminates properly
					*s = '/';

					// we exit from this function with cursor at node_declaration, which is a signal to parse() to go to LOC_ATTRIBUTES
					s = value;
				}
				else
				{
					// replace ending ? with / so that 'element' terminates properly
					*s = '/';

					// we exit from this function with cursor at node_declaration, which is a signal to parse() to go to LOC_ATTRIBUTES
					s = value;
					// 						PUGI__ENDSEG();
					// 						// store value and step over >
					// 						cursor->SetElementText(value);
					// 						PUGI__POPNODE();
					// 
					// 						s += (*s == '>');
				}
			}
			else PUGI__THROW_ERROR(status_bad_pi, s);
		}
		else
		{
			// scan for tag end
			PUGI__SCANFOR(s[0] == '?' && ENDSWITH(s[1], '>'));
			PUGI__CHECK_ERROR(status_bad_pi, s);

			s += (s[1] == '>' ? 2 : 1);
		}

		// store from registers
		ref_cursor = cursor;

		return s;
	}

	bool parse(TCHAR* s, CXmlElement* xmldoc, unsigned int optmsk, TCHAR endch)
	{
		strconv_attribute_t strconv_attribute = get_strconv_attribute(optmsk);
		strconv_pcdata_t strconv_pcdata = get_strconv_pcdata(optmsk);

		TCHAR ch = 0;
		CXmlElement* cursor = xmldoc;
		TCHAR* mark = s;

		while (*s != 0)
		{
			if (*s == '<')
			{
				++s;

			LOC_TAG:
				if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) // '<#...'
				{
					PUGI__PUSHNODE(node_element); // Append a new node to the tree.

					TCHAR* name = s;

					PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol)); // Scan for a terminator.
					PUGI__ENDSEG(); // Save char in 'ch', terminate & step over.

					cursor->SetElementName(name);

					if (ch == '>')
					{
						// end of tag
					}
					else if (PUGI__IS_CHARTYPE(ch, ct_space))
					{
					LOC_ATTRIBUTES:
						while (true)
						{
							PUGI__SKIPWS(); // Eat any whitespace.

							if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) // <... #...
							{
								CXmlAttribute* a = cursor->GetAttributes(TRUE)->Add(); // Make space for this attribute.
								if (!a) PUGI__THROW_ERROR(status_out_of_memory, s);

								TCHAR *name = s; // Save the offset.

								PUGI__SCANWHILE(PUGI__IS_CHARTYPE(*s, ct_symbol)); // Scan for a terminator.
								PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

								PUGI__ENDSEG(); // Save char in 'ch', terminate & step over.
								PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

								a->SetAttrName(name);

								if (PUGI__IS_CHARTYPE(ch, ct_space))
								{
									PUGI__SKIPWS(); // Eat any whitespace.
									PUGI__CHECK_ERROR(status_bad_attribute, s); //$ redundant, left for performance

									ch = *s;
									++s;
								}

								if (ch == '=') // '<... #=...'
								{
									PUGI__SKIPWS(); // Eat any whitespace.

									if (*s == '"' || *s == '\'') // '<... #="...'
									{
										ch = *s; // Save quote char to avoid breaking on "''" -or- '""'.
										++s; // Step over the quote.

										TCHAR *value = s; // Save the offset.


										s = strconv_attribute(s, ch);

										if (!s) PUGI__THROW_ERROR(status_bad_attribute, value);
										a->SetAttrValue(value);

										// After this line the loop continues from the start;
										// Whitespaces, / and > are ok, symbols and EOF are wrong,
										// everything else will be detected
										if (PUGI__IS_CHARTYPE(*s, ct_start_symbol)) PUGI__THROW_ERROR(status_bad_attribute, s);
									}
									else PUGI__THROW_ERROR(status_bad_attribute, s);
								}
								else PUGI__THROW_ERROR(status_bad_attribute, s);
							}
							else if (*s == '/')
							{
								++s;

								if (*s == '>')
								{
									// Ϊ��֮ǰXML���ݣ������ڵ㲻��ջ�������ڵ���Ϊ���ӽڵ�
									if (cursor->GetType() != node_declaration && cursor->GetType() != node_pi)
										PUGI__POPNODE();
									s++;
									break;
								}
								else if (*s == 0 && endch == '>')
								{
									PUGI__POPNODE();
									break;
								}
								else PUGI__THROW_ERROR(status_bad_start_element, s);
							}
							else if (*s == '>')
							{
								++s;

								break;
							}
							else if (*s == 0 && endch == '>')
							{
								break;
							}
							else PUGI__THROW_ERROR(status_bad_start_element, s);
						}

						// !!!
					}
					else if (ch == '/') // '<#.../'
					{
						if (!ENDSWITH(*s, '>')) PUGI__THROW_ERROR(status_bad_start_element, s);


						PUGI__POPNODE(); // Pop.

						s += (*s == '>');
					}
					else if (ch == 0)
					{
						// we stepped over null terminator, backtrack & handle closing tag
						--s;

						if (endch != '>') PUGI__THROW_ERROR(status_bad_start_element, s);
					}
					else PUGI__THROW_ERROR(status_bad_start_element, s);
				}
				else if (*s == '/')
				{
					++s;

					const TCHAR* name = cursor->GetElementName();
					if (!name) PUGI__THROW_ERROR(status_end_element_mismatch, s);

					while (PUGI__IS_CHARTYPE(*s, ct_symbol))
					{
						if (*s++ != *name++) PUGI__THROW_ERROR(status_end_element_mismatch, s);
					}

					if (*name)
					{
						if (*s == 0 && name[0] == endch && name[1] == 0) PUGI__THROW_ERROR(status_bad_end_element, s);
						else PUGI__THROW_ERROR(status_end_element_mismatch, s);
					}

					PUGI__POPNODE(); // Pop.

					PUGI__SKIPWS();

					if (*s == 0)
					{
						if (endch != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
					}
					else
					{
						if (*s != '>') PUGI__THROW_ERROR(status_bad_end_element, s);
						++s;
					}
				}
				else if (*s == '?') // '<?...'
				{
					s = parse_question(s, cursor, optmsk, endch);
					if (!s) return false;

					ATLASSERT(cursor);
					if ((cursor->GetType() == node_declaration && parse_declaration & optmsk))
						goto LOC_ATTRIBUTES;
					if ((cursor->GetType() == node_pi && parse_pi & optmsk))
						goto LOC_ATTRIBUTES;
				}
				else if (*s == '!') // '<!...'
				{
					s = parse_exclamation(s, cursor, optmsk, endch);
					if (!s) return false;
				}
				else if (*s == 0 && endch == '?') PUGI__THROW_ERROR(status_bad_pi, s);
				else PUGI__THROW_ERROR(status_unrecognized_tag, s);
			}
			else
			{
				mark = s; // Save this offset while searching for a terminator.

				PUGI__SKIPWS(); // Eat whitespace if no genuine PCDATA here.

				if (*s == '<')
				{
					// We skipped some whitespace characters because otherwise we would take the tag branch instead of PCDATA one
					ATLASSERT(mark != s);

					if (!PUGI__OPTSET(parse_ws_pcdata | parse_ws_pcdata_single))
					{
						continue;
					}
					else if (PUGI__OPTSET(parse_ws_pcdata_single))
					{
						if (s[1] != '/' || cursor->GetChildElementCount()) continue;
					}
				}

				s = mark;

				if (cursor->_parent)
				{
					//PUGI__PUSHNODE(node_pcdata); // Append a new node on the tree.
					TCHAR* value = s; // Save the offset.


					s = strconv_pcdata(s);
					cursor->SetElementText(value);

					//PUGI__POPNODE(); // Pop since this is a standalone.

					if (!*s) break;
				}
				else
				{
					PUGI__SCANFOR(*s == '<'); // '...<'
					if (!*s) break;

					++s;
				}

				// We're after '<'
				goto LOC_TAG;
			}
		}

		// ���ʱ�������ڵ��ջ
		while (cursor->GetParent() && (cursor->GetType() == node_declaration || cursor->GetType() == node_pi))
			PUGI__POPNODE();

		// check that last tag is closed
		if (cursor != xmldoc) PUGI__THROW_ERROR(status_end_element_mismatch, s);

		return true;
	}

	static bool parse(TCHAR* buffer, size_t length, CXmlDocument* doc, unsigned int optmsk)
	{
		//xml_document_struct* xmldoc = static_cast<xml_document_struct*>(root);

		// store buffer for offset_debug
		//xmldoc->buffer = buffer;

		// early-out for empty documents
		if (length == 0) return false;

		// create parser on stack
		xml_parser parser(*(static_cast<xml_allocator*>(doc->alloc)));

		// save last character and make buffer zero-terminated (speeds up parsing)
		TCHAR endch = buffer[length - 1];
		buffer[length - 1] = 0;

		// perform actual parsing
		bool result = parser.parse(buffer, doc->GetDocumentElement(), optmsk, endch);

		if (result) return true;

		UINT row = 1, col = 0;
		TCHAR* error_offset = parser.error_offset;

		// �������λ��������
		while (--error_offset >= buffer)
			if (*error_offset == '\n')
			{
			col = UINT(parser.error_offset - error_offset - 1);
			break;
			}


		// �������λ��������
		error_offset = parser.error_offset;
		while (--error_offset >= buffer)
		{
			if (*error_offset == '\n')
				++row;
		}

		CString error_desc;
		switch (parser.error_status)
		{
		case status_out_of_memory: error_desc = _T("�ڴ����ʧ��"); break;
		case status_unrecognized_tag: error_desc = _T("����޷�ʶ��"); break;
		case status_bad_pi: error_desc = _T("��Ч��ָ��(PI)�ڵ�"); break;
		case status_bad_comment: error_desc = _T("��Ч��ע�ͽڵ�"); break;
		case status_bad_pcdata: error_desc = _T("��Ч�Ľڵ��ı�"); break;
		case status_bad_start_element: error_desc = _T("��Ч����ʼ�ڵ�"); break;
		case status_bad_attribute: error_desc = _T("��Ч�Ľڵ�����"); break;
		case status_end_element_mismatch: error_desc = _T("�ڵ������ǲ�ƥ��"); break;
		}

		doc->_LastError.Format(_T("����ʧ�ܣ�%s λ�ã�%u��,%u��"), error_desc, row, col);

		return result;
	}
};







class xml_buffered_writer
{
	xml_buffered_writer(const xml_buffered_writer&);
	xml_buffered_writer& operator=(const xml_buffered_writer&);

public:
	xml_buffered_writer(xml_writer& writer_, xml_encoding user_encoding) : writer(writer_), bufsize(0), encoding(user_encoding)
	{
		memset(buffer, 0, bufcapacity);
	}

	~xml_buffered_writer()
	{
		flush();
	}

	void flush()
	{
		flush(buffer, bufsize);
		bufsize = 0;
	}

	void flush(TCHAR* data, size_t size)
	{
		if (size == 0) return;

		// fast path, just write data
		if (encoding == encode_UTF16)
			writer.write(data, size * sizeof(TCHAR));
		else
		{
			// Ϊ�������ת���������ݺ�����ַ���������������ʹ�����ԭ
			TCHAR *end = data + size;
			TCHAR endChar = *end;
			*end = 0;
			int code_page = (encoding == encode_UTF8) ? CP_UTF8 : CP_ACP;
			// convert chunk
			CW2A strBuffer(data, code_page);
			size_t result = strlen(strBuffer);

			*end = endChar;
			// write data
			writer.write(strBuffer.m_psz, result);
		}
	}

	size_t get_valid_length(const TCHAR* data, size_t length)
	{
		ATLASSERT(length > 0);

		// discard last character if it's the lead of a surrogate pair 
		return (sizeof(wchar_t) == 2 && static_cast<unsigned int>(static_cast<WORD>(data[length - 1]) - 0xD800) < 0x400) ? length - 1 : length;
	}

	void write(TCHAR* data, size_t length)
	{
		if (bufsize + length > bufcapacity)
		{
			// flush the remaining buffer contents
			flush();

			// handle large chunks
			if (length > bufcapacity)
			{
				if (encoding == encode_UTF16)
				{
					// fast path, can just write data chunk
					writer.write(data, length * sizeof(TCHAR));
					return;
				}

				// need to convert in suitable chunks
				while (length > bufcapacity)
				{
					// get chunk size by selecting such number of characters that are guaranteed to fit into scratch buffer
					// and form a complete codepoint sequence (i.e. discard start of last codepoint if necessary)
					size_t chunk_size = get_valid_length(data, bufcapacity);

					// convert chunk and write
					flush(data, chunk_size);

					// iterate
					data += chunk_size;
					length -= chunk_size;
				}

				// small tail is copied below
				bufsize = 0;
			}
		}

		memcpy(buffer + bufsize, data, length * sizeof(TCHAR));
		bufsize += length;
	}

	void write(TCHAR* data)
	{
		write(data, _tcslen(data));
	}

	void write(TCHAR d0)
	{
		if (bufsize + 1 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		bufsize += 1;
	}

	void write(TCHAR d0, TCHAR d1)
	{
		if (bufsize + 2 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		buffer[bufsize + 1] = d1;
		bufsize += 2;
	}

	void write(TCHAR d0, TCHAR d1, TCHAR d2)
	{
		if (bufsize + 3 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		buffer[bufsize + 1] = d1;
		buffer[bufsize + 2] = d2;
		bufsize += 3;
	}

	void write(TCHAR d0, TCHAR d1, TCHAR d2, TCHAR d3)
	{
		if (bufsize + 4 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		buffer[bufsize + 1] = d1;
		buffer[bufsize + 2] = d2;
		buffer[bufsize + 3] = d3;
		bufsize += 4;
	}

	void write(TCHAR d0, TCHAR d1, TCHAR d2, TCHAR d3, TCHAR d4)
	{
		if (bufsize + 5 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		buffer[bufsize + 1] = d1;
		buffer[bufsize + 2] = d2;
		buffer[bufsize + 3] = d3;
		buffer[bufsize + 4] = d4;
		bufsize += 5;
	}

	void write(TCHAR d0, TCHAR d1, TCHAR d2, TCHAR d3, TCHAR d4, TCHAR d5)
	{
		if (bufsize + 6 > bufcapacity) flush();

		buffer[bufsize + 0] = d0;
		buffer[bufsize + 1] = d1;
		buffer[bufsize + 2] = d2;
		buffer[bufsize + 3] = d3;
		buffer[bufsize + 4] = d4;
		buffer[bufsize + 5] = d5;
		bufsize += 6;
	}

	// utf8 maximum expansion: x4 (-> utf32)
	// utf16 maximum expansion: x2 (-> utf32)
	// utf32 maximum expansion: x1
	enum
	{
		bufcapacitybytes =
#ifdef PUGIXML_MEMORY_OUTPUT_STACK
		PUGIXML_MEMORY_OUTPUT_STACK
#else
		10240
#endif
		,
		bufcapacity = bufcapacitybytes / (sizeof(TCHAR) + 4)
	};

	TCHAR buffer[bufcapacity];

	xml_writer& writer;
	size_t bufsize;
	xml_encoding encoding;
};







void text_output_escaped(xml_buffered_writer& writer, TCHAR* s, chartypex_t type)
{
	while (*s)
	{
		TCHAR* prev = s;

		// While *s is a usual symbol
		while (!PUGI__IS_CHARTYPEX(*s, type)) ++s;

		writer.write(prev, static_cast<size_t>(s - prev));

		switch (*s)
		{
		case 0: break;
		case '&':
			writer.write('&', 'a', 'm', 'p', ';');
			++s;
			break;
		case '<':
			writer.write('&', 'l', 't', ';');
			++s;
			break;
		case '>':
			writer.write('&', 'g', 't', ';');
			++s;
			break;
		case '"':
			writer.write('&', 'q', 'u', 'o', 't', ';');
			++s;
			break;
		default: // s is not a usual symbol
		{
			unsigned int ch = static_cast<unsigned int>(*s++);
			ATLASSERT(ch < 32);

			TCHAR t1 = static_cast<TCHAR>(ch / 16) > 9 ? static_cast<TCHAR>((ch / 16) + 'A' - 10) : static_cast<TCHAR>((ch / 16) + '0');
			TCHAR t0 = static_cast<TCHAR>(ch % 16) > 9 ? static_cast<TCHAR>((ch % 16) + 'A' - 10) : static_cast<TCHAR>((ch % 16) + '0');

			writer.write('&', '#', 'x', t1, t0, ';');
		}
		}
	}
}


void node_output_attributes(xml_buffered_writer& writer, CXmlElement& node, unsigned int flags)
{
	TCHAR* default_name = _T(":anonymous");

	CXmlAttributes* atts = node.GetAttributes(FALSE);
	if (atts == NULL)
		return;
	for (size_t i = 0; i < atts->GetCount(); ++i)
	{
		CXmlAttribute &a = *(atts->GetAt(i));
		writer.write(' ');
		writer.write(a.GetAttrName()[0] ? const_cast<TCHAR*>(a.GetAttrName()) : default_name);
		writer.write('=', '"');

		text_output_escaped(writer, const_cast<TCHAR*>(a.GetStrValue()), ctx_special_attr);

		writer.write('"');
	}
}



void node_output(xml_buffered_writer& writer, CXmlElement& node, TCHAR* indent, unsigned int flags, unsigned int depth)
{
	TCHAR* default_name = _T(":anonymous");

	if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
		for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

	switch (node.GetType())
	{
	case node_document:
	{
		CXmlElements* childNodes = node.GetChildElements(FALSE);
		for (size_t i = 0; i < childNodes->GetCount(); ++i)
			node_output(writer, *(childNodes->GetAt(i)), indent, flags, depth);
		break;
	}
	case node_element:
	{
		TCHAR* name = node.GetElementName()[0] ? const_cast<TCHAR*>(node.GetElementName()) : default_name;

		writer.write('<');
		writer.write(name);

		node_output_attributes(writer, node, flags);

		if (flags & format_raw)
		{
			if (!node.GetChildElementCount())
				writer.write(' ', '/', '>');
			else
			{
				writer.write('>');

				CXmlElements* childNodes = node.GetChildElements(FALSE);
				for (size_t i = 0; i < childNodes->GetCount(); ++i)
					node_output(writer, *(childNodes->GetAt(i)), indent, flags, depth + 1);

				writer.write('<', '/');
				writer.write(name);
				writer.write('>');
			}
		}
		else if (!node.GetChildElementCount())
		{
			if (_tcslen(node.GetElementText()) != 0)
			{
				writer.write('>');
				text_output_escaped(writer, const_cast<TCHAR*>(node.GetElementText()), ctx_special_pcdata);

				writer.write('<', '/');
				writer.write(name);
				writer.write('>', LINE_END);
			}
			else
				writer.write(' ', '/', '>', LINE_END);
		}
		// 			else if (node.first_child() == node.last_child() && node.first_child().type() == node_cdata)
		// 			{
		// 				writer.write('>');
		// 
		// 				text_output_cdata(writer, node.first_child().value());
		// 
		// 				writer.write('<', '/');
		// 				writer.write(name);
		// 				writer.write('>', LINE_END);
		// 			}
		else
		{
			writer.write('>', LINE_END);

			CXmlElements* childNodes = node.GetChildElements(FALSE);
			for (size_t i = 0; i < childNodes->GetCount(); ++i)
				node_output(writer, *(childNodes->GetAt(i)), indent, flags, depth + 1);

			if ((flags & format_indent) != 0 && (flags & format_raw) == 0)
				for (unsigned int i = 0; i < depth; ++i) writer.write(indent);

			writer.write('<', '/');
			writer.write(name);
			writer.write('>', LINE_END);
		}

		break;
	}

		// 		case node_pcdata:
		// 			text_output(writer, node.value(), ctx_special_pcdata, flags);
		// 			if ((flags & format_raw) == 0) writer.write(LINE_END);
		// 			break;

		// 	case node_cdata:
		// 		text_output_cdata(writer, node.value());
		// 		if ((flags & format_raw) == 0) writer.write(LINE_END);
		// 		break;

	case node_comment:
		writer.write('<', '!', '-', '-');
		writer.write(const_cast<TCHAR*>(node.GetElementText()));
		writer.write('-', '-', '>');
		if ((flags & format_raw) == 0) writer.write(LINE_END);
		break;

	case node_pi:
	case node_declaration:
		writer.write('<', '?');
		writer.write(node.GetElementName()[0] ? const_cast<TCHAR*>(node.GetElementName()) : default_name);

		if (node.GetType() == node_declaration)
		{
			node_output_attributes(writer, node, flags);
		}
		else if (node.GetElementText()[0])
		{
			writer.write(' ');
			writer.write(const_cast<TCHAR*>(node.GetElementText()));
		}

		writer.write('?', '>');
		if ((flags & format_raw) == 0) writer.write(LINE_END);

		{// ���������ڵ�ʱ��ʵ�ʸ��ڵ���Ϊ�����ڵ��ӽڵ�
			CXmlElements* childNodes = node.GetChildElements(FALSE);
			for (size_t i = 0; i < childNodes->GetCount(); ++i)
				node_output(writer, *(childNodes->GetAt(i)), indent, flags, depth);
		}

		break;

	case node_doctype:
		writer.write('<', '!', 'D', 'O', 'C');
		writer.write('T', 'Y', 'P', 'E');

		if (node.GetElementText()[0])
		{
			writer.write(' ');
			writer.write(const_cast<TCHAR*>(node.GetElementText()));
		}

		writer.write('>');
		if ((flags & format_raw) == 0) writer.write(LINE_END);
		break;

	default:
		ATLASSERT(!"Invalid node type");
	}
}



void save(xml_writer& writer, CXmlDocument doc, TCHAR* indent, unsigned int flags, xml_encoding encoding)
{
	xml_buffered_writer buffered_writer(writer, encoding);

	//if ((flags & format_write_bom))
	{
		if (encode_UTF8 == encoding)
			buffered_writer.write('\xef', '\xbb', '\xbf');
		else if (encode_UTF16 == encoding)
			buffered_writer.write('\xff', '\xfe');
	}

	//if (!(flags & format_no_declaration) && !impl::has_declaration(*this))
		{
			buffered_writer.write(_T("<?xml version=\"1.0\""));
			buffered_writer.write('?', '>');
			if (!(flags & format_raw)) buffered_writer.write(LINE_END);
		}

	node_output(buffered_writer, *doc.GetElementRoot(), indent, flags, 0);
}




// xml �ĵ���
CXmlDocument::CXmlDocument(void)
{
	_Option = format_default;
	_ElementRoot = NULL;
	alloc = NULL;

#ifdef _XMLDOC_ALLOC
	_Doc_node = ElementAlloc.Alloc (ELEMENTGROWBY);
#else
	_Doc_node = new CXmlElement();
#endif
	Clear();
	_Doc_node->_Type = node_document;
}

CXmlDocument::CXmlDocument(CXmlDocument&)
{

}

CXmlDocument::~CXmlDocument(void)
{
	if (alloc != NULL)
	{
		delete static_cast<xml_allocator*>(alloc);
	}

#ifdef _XMLDOC_ALLOC
	ElementAlloc.Free (_Doc_node);

	// �������
	ElementAlloc.ClearUp ();
	AttributeAlloc.ClearUp ();
	ElementsAlloc.ClearUp ();
	AttributesAlloc.ClearUp ();
#else
	delete _Doc_node;
#endif
}

// ���� XML �ļ�
BOOL CXmlDocument::LoadFile(LPCTSTR XmlFile, DocFormatEnum format)
{
	// ���ļ�
	CAtlFile file;
	//if (file.Create (XmlFile, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING) != S_OK)
	if (file.Create(XmlFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING) != S_OK)
	{
		_LastError = CLibToolkit::GetWin32LastError();
		return FALSE;
	}

	BOOL ok = FALSE;
	switch (format)
	{
	case fmtXML:
		ok = LoadXmlFile(file);
		break;

	case fmtXMD:
	case fmtXMDANSI:
		ok = LoadXmdFile(file);
		break;

	case fmtXMLUTF8:
		ok = LoadXmlUtf8File(file);
		break;

	default:
		ATLASSERT(FALSE);
		break;
	}

	file.Close();
	return ok;
}

bool CXmlDocument::load_buffer_impl(void* contents, size_t size, unsigned int options, int encode, bool is_mutable, bool own)
{
	Clear();

	// check input buffer
	ATLASSERT(contents || size == 0);

	if (size < 4) return false;

	BYTE *content_buffer = static_cast<BYTE *>(contents);
	// get actual encoding
	xml_encoding buffer_encoding = guess_buffer_encoding(content_buffer[0], content_buffer[1], content_buffer[2], content_buffer[0]);
	if (buffer_encoding == encode_UNKONW)
	{
		buffer_encoding = xml_encoding(encode);
	}

	// get private buffer
	TCHAR* buffer = 0;
	size_t length = 0;

	if (convert_buffer(buffer, length, buffer_encoding, contents, size, is_mutable))
		return false;

	// delete original buffer if we performed a conversion
	if (own && buffer != contents && contents) xml_memory::deallocate(contents);

	// parse
	bool res = xml_parser::parse(buffer, length, this, options);
	//SetXmlString(buffer);
	// buffer != contentsʱ��ʾbuffer���ڲ����䣬��Ҫ�ͷ�
	if (buffer != contents) xml_memory::deallocate(buffer);
	if (own && buffer == contents) xml_memory::deallocate(contents);


	CXmlElements*  children = _Doc_node->GetChildElements(FALSE);
	if (children != NULL)
	{
		ATLASSERT(children->GetCount() == 1);
		_ElementRoot = children->GetAt(0);

		// 			for (size_t i = 0; i < children->GetCount(); ++i)
		// 			{
		// 				if(children->GetAt(i)->_Type == node_element)
		// 				{
		// 					_ElementRoot = children->GetAt(i);
		// 					break;
		// 				}
		// 			}
	}

	return res;
}



BOOL CXmlDocument::LoadXmlFile(HANDLE FileHandle)
{
	CAtlFile file(FileHandle);
	file.Seek(0, FILE_BEGIN);

	// �����ݳ���
	ULONGLONG fileSize = 0;
	if (file.GetSize(fileSize) != S_OK)
	{
		_LastError = _T("�޷���ȡXML�ļ���С��");
		return FALSE;
	}

	unsigned size = unsigned(fileSize);

	void *fileContent = xml_memory::allocate(size);
	file.Read(fileContent, size);
	file.Detach();
	return load_buffer_impl(fileContent, size, parse_declaration | parse_escapes | parse_pi, encode_ANSI, true, true);
}

// ���� UTF-8���ļ� 
BOOL CXmlDocument::LoadXmlUtf8File(HANDLE FileHandle)
{
	CAtlFile file(FileHandle);
	file.Seek(0, FILE_BEGIN);

	// �����ݳ���
	ULONGLONG fileSize = 0;
	if (file.GetSize(fileSize) != S_OK)
	{
		_LastError = _T("�޷���ȡXML�ļ���С��");
		return FALSE;
	}
	unsigned size = unsigned(fileSize);

	void *fileContent = xml_memory::allocate(size);
	file.Read(fileContent, size);
	file.Detach();
	return load_buffer_impl(fileContent, size, parse_declaration | parse_escapes | parse_pi, encode_UTF8, true, true);
}

// ���ض����ƣأͣ��ļ�
BOOL CXmlDocument::LoadXmdFile(HANDLE FileHandle)
{
	this->Clear();
	CFileDataReadStream stream(FileHandle);

	return LoadXmdFromStream(&stream);
}

// ���� XML �ļ�
BOOL CXmlDocument::SaveFile(LPCTSTR XmlFile, DocFormatEnum format)
{
	// �����ļ��������ļ��Ѵ���
	CAtlFile file;
	if (file.Create(XmlFile, FILE_WRITE_DATA, FILE_SHARE_READ, OPEN_ALWAYS) != S_OK)
		return FALSE;

	// �����ļ�����Ϊ 0
	file.SetSize(0);

	BOOL ok = FALSE;
	switch (format)
	{
	case fmtXML:
		ok = SaveXmlFile(file);
		break;

	case fmtXMLUTF8:
		ok = SaveXmlUtf8File(file);
		break;

	case fmtXMD:
		ok = SaveXmdFile(file);
		break;

	case fmtXMLANSI:
		ok = SaveXmlANSIFile(file);
		break;

	case fmtXMDANSI:
		ok = SaveXmdFile(file, XMD_FLAGS_SAVEFORMAT_ANSI);
		break;

	default:
		ATLASSERT(FALSE);
		break;
	}

	file.Close();

	return ok;
}

BOOL CXmlDocument::SaveXmlFile(HANDLE FileHandle)
{
	CAtlFile file(FileHandle);
	BYTE head[2] = { 0xFF, 0xFE };
	file.Write(head, sizeof(head));
	file.Detach();

	xml_writer_file writer(FileHandle);
	xml_buffered_writer buffered_writer(writer, encode_UTF16);

	node_output(buffered_writer, *GetDocumentElement(), _T("\t"), _Option, 0);

	return TRUE;
}

// ��������ƣأͣ��ļ�
BOOL CXmlDocument::SaveXmdFile(HANDLE FileHandle, DWORD dwFlags)
{
	CFileDataWriteStream stream(FileHandle);

	return SaveXmdToStream(&stream, dwFlags);
}

// ����ANSI�����ļ�
BOOL CXmlDocument::SaveXmlANSIFile(HANDLE FileHandle)
{
	xml_writer_file writer(FileHandle);
	xml_buffered_writer buffered_writer(writer, encode_ANSI);

	node_output(buffered_writer, *GetDocumentElement(), _T("\t"), _Option, 0);

	return TRUE;
}

BOOL CXmlDocument::SaveXmlUtf8File(HANDLE FileHandle)
{
	CAtlFile file(FileHandle);
	BYTE head[3] = { 0xEF, 0xBB, 0xBF };
	file.Write(head, sizeof(head));
	file.Detach();

	xml_writer_file writer(FileHandle);
	xml_buffered_writer buffered_writer(writer, encode_UTF8);

	node_output(buffered_writer, *GetDocumentElement(), _T("\t"), _Option, 0);

	return TRUE;
}

// ���� XML �ַ���
BOOL CXmlDocument::SetXmlString(LPCTSTR xml)
{
	// ת�����
	return load_buffer_impl((void*)xml, (_tcslen(xml) + 1)*sizeof(TCHAR), parse_declaration | parse_escapes, sizeof(TCHAR), false, false);
}

// ȡ�� XML �ַ���
BOOL CXmlDocument::GetXmlString(CString& xml, CString* error)
{
	GetElementRoot();
	ATLASSERT(_ElementRoot->GetElementName() && _tcslen(_ElementRoot->GetElementName()));

	return _ElementRoot->GetXmlString(xml, error);
}

// �����м���XMD����
BOOL CXmlDocument::LoadXmdFromStream(IDataReadStream *pDataStream)
{
	ATLASSERT(pDataStream != NULL);
	if (pDataStream == NULL)
		return FALSE;

	// �����ļ�ͷ
	StringStreamData &docHeaderString = pDataStream->ReadStringData();

	CString strHeader;
	if (docHeaderString.bUnicode)
	{
		strHeader = (LPCWSTR)docHeaderString.Buffer;
	}
	else
	{
		strHeader = CA2W((LPCSTR)docHeaderString.Buffer);
	}

	// �����2.0�����ٶ�ȡһ��������TODO������汾�����������ϣ�������һЩ�����������ģ�Ҫ����
	CString strHeaderName(strHeader);
	if (strHeaderName.CompareNoCase(_T("<XMD 2.0>")) == 0)
	{
		pDataStream->ReadLong();
	}
	else if (strHeaderName.CompareNoCase(_T("<XMD 1.0>")) != 0)
	{
		_LastError = _T("�ļ���ʽ�����޷���ȡ");
		return false;
	}

	return GetElementRoot()->ParseXmdElement(pDataStream);
}

// ����XMD���ݵ���������
BOOL CXmlDocument::SaveXmdToStream(IDataWriteStream *pDataStream, DWORD dwFlags)
{
	ATLASSERT(pDataStream != NULL);
	if (pDataStream == NULL)
		return FALSE;

	pDataStream->WriteString(_T("<XMD 1.0>"));

	return GetElementRoot()->CreateXmdElement(pDataStream, dwFlags);
}

// ȡ�� XML �����
CXmlElement* CXmlDocument::GetElementRoot(void)
{
	if (_ElementRoot) return _ElementRoot;

	CXmlElements* topChildren = _Doc_node->GetChildElements(TRUE);
	for (size_t i = 0; i < topChildren->GetCount(); ++i)
	{
		if (topChildren->GetAt(i)->GetType() == node_element)
		{
			return topChildren->GetAt(i);
		}
	}

	_ElementRoot = topChildren->Add();
	_ElementRoot->_Type = node_element;
	_ElementRoot->SetElementName(_T("Document"));
	return _ElementRoot;
}

// ɾ������
void CXmlDocument::Clear()
{
	_Doc_node->reset();
	if (alloc != NULL)
	{
		delete static_cast<xml_allocator*>(alloc);
	}
	alloc = new xml_allocator;

	_ElementRoot = NULL;
	_Doc_node->_alloc = alloc;
	_Doc_node->SetElementName(_T("Document"));
}

BOOL CXmlDocument::GetIsFormatSave(void)
{
	return _Option & format_indent;
}

void CXmlDocument::SetIsFormatSave(BOOL bFormat /*= true*/)
{
	if (bFormat)
		_Option |= format_indent;
	else _Option &= ~format_indent;
}

LPCTSTR CXmlDocument::GetLastError(void)
{
	return _LastError;
}

CXmlElement* CXmlDocument::GetDocumentElement(void)
{
	return _Doc_node;
}

//------------------------------------------------------------------
// ����
//------------------------------------------------------------------
CXmlAttribute::CXmlAttribute(void)
{
	_parent = NULL;
	reset();
}

void CXmlAttribute::reset(void)
{
	_name = NULL;
	_value = NULL;
}

// ��������ֵ
void CXmlAttribute::SetAttrValue(int value)
{
	SetAttrValue((long)value);
}

void CXmlAttribute::SetAttrValue(long value)
{
	CString val;
	val.Format(_T("%d"), (int)value);
	SetAttrValue(val);
}

void CXmlAttribute::SetAttrValue(double value)
{
	CString val;
	val.Format(_T("%f"), value);
	SetAttrValue(val);
}

void CXmlAttribute::SetAttrValue(LPCTSTR value)
{
	strcpy_insitu(static_cast<xml_allocator*>(_parent->GetAllocator()), _value, value);
}

void CXmlAttribute::SetAttrName(LPCTSTR name)
{
	strcpy_insitu(static_cast<xml_allocator*>(_parent->GetAllocator()), _name, name);
}

int CXmlAttribute::GetIntValue(void)
{
	return _ttoi(_value);
}

long CXmlAttribute::GetLngValue(void)
{
	return _ttol(_value);
}

double CXmlAttribute::GetDblValue(void)
{
	return _tstof(_value);
}

LPCTSTR CXmlAttribute::GetStrValue(void)
{
	return _value ? _value : _T("");
}

LPCTSTR CXmlAttribute::GetAttrName(void)
{
	return _name ? _name : _T("");
}

//------------------------------------------------------------------
// ���
//------------------------------------------------------------------
CXmlElement* CXmlElements::InsertAt(size_t index)
{
	CXmlElement *ele;
#ifdef _XMLDOC_ALLOC
	ele =  __super::InsertAt (index, ElementAlloc.Alloc (ELEMENTGROWBY));
#else
	ele = __super::InsertAt(index, new CXmlElement());
#endif
	ele->_alloc = _parent->_alloc;
	return ele;
}

void CXmlElements::RemoveAt(size_t index)
{
	ATLASSERT(index < _array.GetCount());
#ifdef _XMLDOC_ALLOC
	ElementAlloc.Free (_array[index]);
#else
	delete _array[index];
#endif
	_array.RemoveAt(index);
}


CXmlAttribute* CXmlAttributes::InsertAt(size_t index)
{
#ifdef _XMLDOC_ALLOC
	return __super::InsertAt (index, AttributeAlloc.Alloc (ATTRIBUTEGROWBY));
#else
	return __super::InsertAt(index, new CXmlAttribute());
#endif
}

void CXmlAttributes::RemoveAt(size_t index)
{
	ATLASSERT(index < _array.GetCount());
#ifdef _XMLDOC_ALLOC
	AttributeAlloc.Free (_array[index]);
#else
	delete _array[index];
#endif
	_array.RemoveAt(index);
}

// xml ���
CXmlElement::CXmlElement()
{
	_ElementContainer = NULL;
	_AttributeContainer = NULL;

	_alloc = NULL;

	_parent = NULL;
	_Type = node_element;

	_ElementName = NULL;
	_ElementText = NULL;

	_Type = node_element;
	_IsAddDoubleQuot = FALSE;		// Ĭ���Զ����˫����
}

CXmlElement::~CXmlElement(void)
{
	if (_ElementContainer)
	{
#ifdef _XMLDOC_ALLOC
		ElementsAlloc.Free (_ElementContainer);
#else
		delete _ElementContainer;
#endif
		_ElementContainer = NULL;
	}

	if (_AttributeContainer)
	{
#ifdef _XMLDOC_ALLOC
		AttributesAlloc.Free (_AttributeContainer);
#else
		delete _AttributeContainer;
#endif
		_AttributeContainer = NULL;
	}

}

// ȡ���ӽ������
int CXmlElement::GetChildElementCount(void)
{
	if (!_ElementContainer)
		return 0;
	else
		return (int)_ElementContainer->GetCount();
}

CXmlElements* CXmlElement::GetChildElements(BOOL NotExistCreate)
{
	if (!_ElementContainer && NotExistCreate)
	{
#ifdef _XMLDOC_ALLOC
		_ElementContainer = ElementsAlloc.Alloc (ELEMENTSGROWBY);
#else
		_ElementContainer = new CXmlElements();
#endif
		_ElementContainer->_parent = this;
	}

	return _ElementContainer;
}

CXmlElement* CXmlElement::GetChildElementAt(LPCTSTR name, BOOL NotExistCreate)
{
	// ���ҽ��
	for (int i = 0; i < GetChildElementCount(); i++)
	{
		CXmlElement* cur = GetChildElements()->GetAt(i);
		if (!lstrcmp(cur->GetElementName(), name))
			return cur;
	}

	// �������򴴽�
	if (NotExistCreate)
	{
		CXmlElement* child = GetChildElements()->Add();
		child->SetElementName(name);
		return child;
	}

	return NULL;
}

// ȡ����������
int CXmlElement::GetAttributeCount(void)
{
	if (!_AttributeContainer)
		return 0;
	else
		return (int)_AttributeContainer->GetCount();
}

CXmlAttributes* CXmlElement::GetAttributes(BOOL NotExistCreate)
{
	if (!_AttributeContainer && NotExistCreate)
	{
#ifdef _XMLDOC_ALLOC
		_AttributeContainer = AttributesAlloc.Alloc (ATTRIBUTESGROWBY);
#else
		_AttributeContainer = new CXmlAttributes();
#endif
		_AttributeContainer->_parent = _parent;
	}

	return _AttributeContainer;
}

CXmlAttribute* CXmlElement::GetAttributeAt(LPCTSTR name, BOOL NotExistCreate)
{
	// ���ҽ��
	for (int i = 0; i < GetAttributeCount(); i++)
	{
		CXmlAttribute* cur = GetAttributes()->GetAt(i);
		if (!lstrcmp(cur->GetAttrName(), name))
			return cur;
	}

	// �������򴴽�
	if (NotExistCreate)
	{
		CXmlAttribute* attr = GetAttributes()->Add();
		attr->SetAttrName(name);

		return attr;
	}

	return NULL;
}

LPCTSTR CXmlElement::GetElementText(void)
{
	return _ElementText ? _ElementText : _T("");
}

void CXmlElement::SetElementText(LPCTSTR text)
{
	strcpy_insitu(static_cast<xml_allocator*>(_alloc), _ElementText, text);
	//_ElementText = text;
}

LPCTSTR CXmlElement::GetElementName(void)
{
	return _ElementName ? _ElementName : _T("");
}

void CXmlElement::SetElementName(LPCTSTR name)
{
	strcpy_insitu(static_cast<xml_allocator*>(_alloc), _ElementName, name);

	//_ElementName = name;
}

// �������ԣ�AddDoubleQuote = �Ƿ���ֵ��������˫����"����"
void CXmlElement::SetAttrValue(LPCTSTR AttrName, LPCTSTR AttrValue)
{
	// �������ڴ��ڵ�
	int index = -1;
	if (FindAttribute(AttrName, index))
	{
		GetAttributes()->GetAt(index)->SetAttrValue(AttrValue);
		return;
	}

	// ���
	CXmlAttribute* attr = GetAttributes()->Add();
	attr->SetAttrName(AttrName);
	attr->SetAttrValue(AttrValue);
}

// ������ԣ���SetAttrValue������ͬ���������ڲ�����������Ƿ��Ѵ���
void CXmlElement::AddAttrValue(LPCTSTR AttrName, LPCTSTR AttrValue)
{
	//int index = -1;
	//ATLASSERT (FindAttribute (AttrName, index);

	CXmlAttribute* attr = GetAttributes()->Add();
	attr->SetAttrName(AttrName);
	attr->SetAttrValue(AttrValue);
}

// ��������
BOOL CXmlElement::FindAttribute(LPCTSTR AttrName, int& index)
{
	for (int i = 0; i < GetAttributeCount(); i++)
	{
		CXmlAttribute*attr = GetAttributes()->GetAt(i);
		if (!lstrcmp(attr->GetAttrName(), AttrName))
		{
			index = i;
			return TRUE;
		}
	}

	return FALSE;
}

// ���ƽ��
void CXmlElement::CopyFrom(CXmlElement* root)
{
	this->reset();

	// copy node
	SetElementName(root->GetElementName());
	SetElementText(root->GetElementText());

	_Type = root->_Type;
	_IsAddDoubleQuot = root->_IsAddDoubleQuot;

	// copy attr
	if (root->GetAttributes(FALSE))
	{
		for (size_t i = 0; i < root->GetAttributes()->GetCount(); i++)
		{
			CXmlAttribute* srcAttr = root->GetAttributes()->GetAt(i);
			CXmlAttribute* newAttr = this->GetAttributes()->Add();

			newAttr->SetAttrName(srcAttr->GetAttrName());
			newAttr->SetAttrValue(srcAttr->GetStrValue());
		}
	}

	if (root->GetChildElements(FALSE))
	{
		for (size_t i = 0; i < root->GetChildElements()->GetCount(); i++)
		{
			CXmlElement* srcElmt = root->GetChildElements()->GetAt(i);
			CXmlElement* newElmt = this->GetChildElements()->Add();
			newElmt->CopyFrom(srcElmt);
		}
	}
}

// ��ȡ����
LPCTSTR CXmlElement::GetAttrValue(LPCTSTR AttrName, LPCTSTR DefaultValue)
{
	// �Ȳ���
	for (int i = 0; i < GetAttributeCount(); i++)
	{
		CXmlAttribute* attr = GetAttributes()->GetAt(i);
		if (!lstrcmp(attr->GetAttrName(), AttrName))
			return attr->GetStrValue();
	}
	// ����Ĭ��ֵ
	return DefaultValue;
}

// �����������
void CXmlElement::reset(void)
{
	if (_ElementContainer)
	{
#ifdef _XMLDOC_ALLOC
		ElementsAlloc.Free (_ElementContainer);
#else
		delete _ElementContainer;
#endif
		_ElementContainer = NULL;
	}

	if (_AttributeContainer)
	{
#ifdef _XMLDOC_ALLOC
		AttributesAlloc.Free (_AttributeContainer);
#else
		delete _AttributeContainer;
#endif
		_AttributeContainer = NULL;
	}

	xml_allocator *alloc = static_cast<xml_allocator *>(_alloc);
	if (_ElementName)
	{
		alloc->deallocate_string(_ElementName);
		_ElementName = NULL;
	}
	if (_ElementText)
	{
		alloc->deallocate_string(_ElementText);
		_ElementText = NULL;
	}

	_IsAddDoubleQuot = FALSE;
}

// Ϊ�ַ�������˫����
CString CXmlElement::AddDoubleQuot(LPCTSTR str)
{
	if (str == NULL) return _T("");

	CString s1, s2 = str;

	//// ��s2�еġ�"�����ɡ�<QUOT>��
	//s2.Replace (_T("\""), _T("<QUOT>"));

	// ���ַ������Ҽ���˫����
	s1.Format(_T("\"%s\""), s2);
	return s1;
}

// ɾ���ַ���˫����
CString CXmlElement::DelDoubleQuot(LPCTSTR str)
{
	if (str == NULL) return _T("");

	CString s;
	// ȥ���������ߵ�˫����
	if (_tcslen(str) >= 2 && str[0] == _T('\"') && str[_tcslen(str) - 1] == _T('\"'))
	{
		str++;
		s = CString(str, (int)_tcslen(str) - 1);
	}
	else
	{
		s = str;
	}

	// ���ַ����е� <QUOT> ת����˫����
	s.Replace(_T("<QUOT>"), _T("\""));
	return s;
}

// �Ѷ�����XML�ļ�ת����Element
BOOL CXmlElement::ParseXmdElement(IDataReadStream *pStream)
{
	ATLASSERT(pStream != NULL);

	CString strTempString;
	StringStreamData tempData;

	// ��ȡ������ơ�ֵ
	tempData = pStream->ReadStringData();
	if (!tempData.bUnicode)
	{
		strTempString = (LPCTSTR)CA2W((LPCSTR)tempData.Buffer);
	}
	else
	{
		strTempString = (LPCWSTR)tempData.Buffer;
	}
	SetElementName(strTempString);
	//_ElementName = strTempString;

	tempData = pStream->ReadStringData();
	if (!tempData.bUnicode)
	{
		strTempString = (LPCTSTR)CA2W((LPCSTR)tempData.Buffer);
	}
	else
	{
		strTempString = (LPCWSTR)tempData.Buffer;
	}
	SetElementText(strTempString);
	//_ElementText = strTempString;

	// ��ȡ����
	long AttrCount = pStream->ReadLong();
	for (long i = 0; i < AttrCount; i++)
	{
		CXmlAttribute* attr = GetAttributes()->Add();

		tempData = pStream->ReadStringData();
		if (!tempData.bUnicode)
		{
			strTempString = (LPCTSTR)CA2W((LPCSTR)tempData.Buffer);
		}
		else
		{
			strTempString = (LPCWSTR)tempData.Buffer;
		}
		attr->SetAttrName(strTempString);

		tempData = pStream->ReadStringData();
		if (!tempData.bUnicode)
		{
			strTempString = (LPCTSTR)CA2W((LPCSTR)tempData.Buffer);
		}
		else
		{
			strTempString = (LPCWSTR)tempData.Buffer;
		}
		attr->SetAttrValue(strTempString);
	}

	// ��ȡ�� Element
	long ElmtCount = pStream->ReadLong();
	for (long i = 0; i < ElmtCount; i++)
	{
		CXmlElement* elmt = GetChildElements()->Add();
		elmt->ParseXmdElement(pStream);
	}

	return TRUE;
}

// ��Elementת���ɶ�����XML�ļ�
BOOL CXmlElement::CreateXmdElement(IDataWriteStream *pStream, DWORD dwFlags)
{
	ATLASSERT(pStream != NULL);

	// д�������ơ�ֵ
	if (dwFlags & XMD_FLAGS_SAVEFORMAT_ANSI)
	{
		pStream->WriteString(CW2A(GetElementName()));
		pStream->WriteString(CW2A(GetElementText()));

		// д������
		pStream->WriteLong((long)GetAttributes()->GetCount());
		for (size_t i = 0; i < GetAttributes()->GetCount(); i++)
		{
			CXmlAttribute* attr = GetAttributes()->GetAt(i);

			pStream->WriteString(CW2A(attr->GetAttrName()));
			pStream->WriteString(CW2A(attr->GetStrValue()));
		}
	}
	else
	{
		pStream->WriteString(GetElementName());
		pStream->WriteString(GetElementText());

		// д������
		pStream->WriteLong((long)GetAttributes()->GetCount());
		for (size_t i = 0; i < GetAttributes()->GetCount(); i++)
		{
			CXmlAttribute* attr = GetAttributes()->GetAt(i);

			pStream->WriteString(attr->GetAttrName());
			pStream->WriteString(attr->GetStrValue());
		}
	}

	// д���� Element
	pStream->WriteLong((long)GetChildElements()->GetCount());
	for (size_t i = 0; i < GetChildElements()->GetCount(); i++)
	{
		CXmlElement* elmt = GetChildElements()->GetAt(i);
		elmt->CreateXmdElement(pStream, dwFlags);
	}
	return TRUE;
}



// ȡ�� XML �ַ���
BOOL CXmlElement::GetXmlString(CString& xml, CString* error)
{
	struct xml_memory_writer : xml_writer
	{
		BYTE* buffer;
		size_t capacity;

		size_t result;

		xml_memory_writer() : buffer(0), capacity(0), result(0) {}

		xml_memory_writer(BYTE* buffer, size_t capacity) : buffer(buffer), capacity(capacity), result(0) {}

		size_t written_size() const
		{
			return result < capacity ? result : capacity;
		}

		virtual void write(const void* data, size_t size)
		{
			if (result < capacity)
			{
				size_t chunk = (capacity - result < size) ? capacity - result : size;

				memcpy(buffer + result, data, chunk);
			}

			result += size;
		}
	};

	xml_encoding encoding = (sizeof(TCHAR) == 1) ? encode_ANSI : encode_UTF16;

	xml_memory_writer counter;
	{
		xml_buffered_writer buffered_writer(counter, encoding);
		node_output(buffered_writer, *this, _T("\t"), 0, 0);
	}
	BYTE* buffer = new BYTE[counter.result + 2];

	xml_memory_writer writer(buffer, counter.result);
	{
		xml_buffered_writer buffered_writer(writer, encoding);
		node_output(buffered_writer, *this, _T("\t"), 0, 0);
	}
	buffer[writer.written_size()] = 0;
	buffer[writer.written_size() + 1] = 0;

	xml = (TCHAR*)buffer;
	delete[]buffer;

	return TRUE;
}

void CXmlElement::SetIsDeclaration(BOOL bDeclaration /*= false*/)
{
	_Type = bDeclaration ? node_declaration : node_element;
	if (bDeclaration)
	{
		SetElementName(_T("xml"));
	}
}