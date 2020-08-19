// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the ZBAR64_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// ZBAR64_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ZBAR64_EXPORTS
#define ZBAR64_API __declspec(dllexport)
#else
#define ZBAR64_API __declspec(dllimport)
#endif

// This class is exported from the dll
class ZBAR64_API Czbar64 {
public:
	Czbar64(void);
	// TODO: add your methods here.
};

extern ZBAR64_API int nzbar64;

ZBAR64_API int fnzbar64(void);
