

#ifdef COMPASOFTVERSION_EXPORTS
#define COMPASOFTVERSION_API __declspec(dllexport)
#else
#define COMPASOFTVERSION_API __declspec(dllimport)
#endif


#define LOCK_API COMPASOFTVERSION_API

// 对比2个xml之间的version，版本相同返回0，版本不同返回1。
LOCK_API int __stdcall CompareSoftVersion(char* lpszInstallXml, char* lpszLocalXml);