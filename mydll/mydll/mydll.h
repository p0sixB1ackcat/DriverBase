// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MYDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MYDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MYDLL_EXPORTS
#define MYDLL_API __declspec(dllexport)
#else
#define MYDLL_API __declspec(dllimport)
#endif

// This class is exported from the mydll.dll
class MYDLL_API Cmydll {
public:
	Cmydll(void);
	// TODO: add your methods here.
};

extern MYDLL_API int nmydll;

MYDLL_API int fnmydll(void);
extern"C" MYDLL_API int fnMyfunc(void);