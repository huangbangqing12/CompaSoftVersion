
template <class T, class CONTENT>
CXmlContainer<T, CONTENT>::~CXmlContainer(void)
{
	T *pT = static_cast<T *>(this);
	pT->RemoveAll();
}
template <class T, class CONTENT>
size_t CXmlContainer<T, CONTENT>::GetCount(void)
{
	return _array.GetCount();
}
template <class T, class CONTENT>
CONTENT* CXmlContainer<T, CONTENT>::GetAt(size_t index)
{
	return _array.GetAt(index);
}
template <class T, class CONTENT>
CONTENT* CXmlContainer<T, CONTENT>::Add(void)
{
	T *pT = static_cast<T *>(this);
	CONTENT* content = pT->InsertAt(_array.GetCount());
	return content;
}
template <class T, class CONTENT>
void CXmlContainer<T, CONTENT>::RemoveAt(size_t index)
{
	ATLASSERT(FALSE);
}
template <class T, class CONTENT>
void CXmlContainer<T, CONTENT>::RemoveAll(void)
{
	T *pT = static_cast<T *>(this);
	for (int i = (int)_array.GetCount() - 1; i >= 0; i--)
		pT->RemoveAt(i);
}
template <class T, class CONTENT>
void CXmlContainer<T, CONTENT>::reset(void)
{
	RemoveAll();
}
template <class T, class CONTENT>
CONTENT* CXmlContainer<T, CONTENT>::InsertAt(size_t index)
{
	ATLASSERT(FALSE);
	return NULL;
}
template <class T, class CONTENT>
CONTENT* CXmlContainer<T, CONTENT>::InsertAt(size_t index, CONTENT* content)
{
	_array.InsertAt(index, content);
	content->_parent = _parent;
	return content;
}
