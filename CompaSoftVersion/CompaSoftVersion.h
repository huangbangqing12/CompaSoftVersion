

#ifdef COMPASOFTVERSION_EXPORTS
#define COMPASOFTVERSION_API __declspec(dllexport)
#else
#define COMPASOFTVERSION_API __declspec(dllimport)
#endif


#define LOCK_API COMPASOFTVERSION_API

// �Ա�2��xml֮���version���汾��ͬ����0���汾��ͬ����1��
LOCK_API int __stdcall CompareSoftVersion(char* lpszInstallXml, char* lpszLocalXml);