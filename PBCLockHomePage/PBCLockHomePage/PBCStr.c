#include <ntddk.h>
#include "PBCStr.h"

NTSTATUS PBCGetUrlWithCommandLineBuffer(__in PUNICODE_STRING lpCommandLineBuffer
	, __out PUNICODE_STRING lpUrlStr)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	WCHAR *p1 = NULL;
	ULONG dwAsciiBufferLength = lpCommandLineBuffer->MaximumLength / 2 + lpCommandLineBuffer->MaximumLength % 2;
	ULONG dwIndex = 0;
	if (!lpCommandLineBuffer || !lpUrlStr)
	{
		ntStatus = STATUS_INVALID_PARAMETER;
		goto ret;
	}

	p1 = lpCommandLineBuffer->Buffer;
	while (dwIndex < dwAsciiBufferLength)
	{
		if (*(p1 + dwIndex) == '"' 
			&&
			dwIndex + 1  < dwAsciiBufferLength)
		{
			if (*(p1 + dwIndex + 1) == ' ')
			{
				RtlCopyMemory(lpUrlStr->Buffer + dwIndex ,p1,(dwAsciiBufferLength - dwIndex) * sizeof(WCHAR));
				lpUrlStr->Length = (dwAsciiBufferLength - dwIndex) * sizeof(WCHAR);
				while (dwIndex < dwAsciiBufferLength
					&&
					*(p1 + dwIndex) != ' '
					)
				{
					++dwIndex;
				}
				break;
			}
		}
		++dwIndex;
	}

ret:
	return ntStatus;
}

BOOL _strstr(WCHAR *buffer1, WCHAR *buffer2, ULONG dwLength)
{
	BOOL result = FALSE;
	ULONG dwIndex = 0;
	if (!buffer1 || !buffer2 || !dwLength)
		return result;

	result = TRUE;

	while (dwIndex < dwLength)
	{
		if (*(buffer1 + dwIndex) != *(buffer2 + dwIndex))
		{
			result = FALSE;
			break;
		}
		++dwIndex;
	}

	return result;


}

ULONG PBCUniCodeStrStr(PUNICODE_STRING lpstr1, PUNICODE_STRING lpstr2)
{
	WCHAR *p1 = lpstr1->Buffer;
	WCHAR *p2 = lpstr2->Buffer;
	ULONG dwIndex = 0;
	BOOL bResult = FALSE;
	ULONG dwCharaLen = 2;
	if (!lpstr1 || !lpstr2)
		return bResult;

	while (dwIndex * dwCharaLen < lpstr1->Length)
	{
		p1 = (lpstr1->Buffer) + dwIndex;
		if ((lpstr1->Length - dwIndex) / dwCharaLen < (lpstr2->Length) / dwCharaLen)
		{
			bResult = FALSE;
			break;
		}
		else
		{
			bResult = _strstr(p1, p2,lpstr2->Length/dwCharaLen);
			if (bResult)
			{
				break;
			}

		}
		++dwIndex;
	}

	return bResult ? dwIndex : 0;
}

BOOL PBCIsHaveUrl(PPEB pPeb, PULONG lpIndex)
{
	UNICODE_STRING uCompStr = {0x00};

	RtlInitUnicodeString(&uCompStr, L"www.");
	uCompStr.Length = wcslen(L"www.");

	if (pPeb->ProcessParameters->CommandLine.Length
		== pPeb->ProcessParameters->ImagePathName.Length)
	{
		return FALSE;
	}
	else
	{
		*lpIndex = PBCUniCodeStrStr(&pPeb->ProcessParameters->CommandLine, &uCompStr);
		if (*lpIndex)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

BOOL PBCIsHaveCreateCode(PPEB pPeb, PULONG lpIndex)
{
	UNICODE_STRING uScodeF = { 0x00 };

	RtlInitUnicodeString(&uScodeF, L"SCODEF");
	uScodeF.Length = wcslen(L"SCODEF");
	*lpIndex = PBCUniCodeStrStr(&pPeb->ProcessParameters->CommandLine, &uScodeF);
	if (*lpIndex)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}