#define MAX_PATH	260
typedef ULONG DWORD;

#pragma warning(disable:4996)

typedef struct _OP_INFO
{
    WCHAR     m_ProcessName[MAX_PATH];
	WCHAR     m_DestName[MAX_PATH];
    DWORD     m_ulProcessID;
    ULONG     m_ulWaitID;
    LIST_ENTRY m_List;
} OP_INFO, *POP_INFO;

typedef struct _RING3_OP_INFO
{
    WCHAR     m_ProcessName[MAX_PATH];
	WCHAR     m_DestName[MAX_PATH];
    DWORD     m_ulProcessID;
    ULONG     m_ulWaitID;
} RING3_OP_INFO, *PRING3_OP_INFO;

typedef struct _RING3_REPLY
{
    ULONG	m_ulWaitID;
    ULONG	m_ulBlocked;
}RING3_REPLY;

typedef struct _WAIT_LIST_ENTRY
{
	LIST_ENTRY	m_List;
	ULONG		m_ulWaitID;
	KEVENT		m_ulWaitEvent;
	ULONG		m_bBlocked;
}WAIT_LIST_ENTRY;


typedef enum _R3_RESULT
{
    R3Result_Pass,
    R3Result_Block, 
    R3Result_DefaultNon,	
}R3_RESULT;

R3_RESULT __stdcall GetResultFromUser(HANDLE pID, PUNICODE_STRING strDest);