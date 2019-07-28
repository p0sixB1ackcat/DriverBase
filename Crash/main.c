#include <ntddk.h>

VOID OperUnicodeStr(VOID);

NTSTATUS DriverEntry(
		IN PDRIVER_OBJECT pDriverObject, 
		IN PUNICODE_STRING pRegPath)
{
	OperUnicodeStr();

	return STATUS_SUCCESS;
}

VOID OperUnicodeStr(VOID)
{

	UNICODE_STRING 		uStr1 = {0};
	UNICODE_STRING 		uStr2 = {0};
	UNICODE_STRING		uStr3 = {0};
	UNICODE_STRING		uStr4 = {0};

	ANSI_STRING         aStr1 = {0};
	

	RtlInitUnicodeString(&uStr1, L"hello");
	RtlInitUnicodeString(&uStr2, L"Goodbye");
	DbgPrint("uStr1=%wZ\n", uStr1);
	DbgPrint("uStr2=%wZ\n", uStr2);

	RtlInitAnsiString(&aStr1, "Ansi string");
	DbgPrint("aStr1=%Z\n", aStr1);

	RtlCopyUnicodeString(&uStr3, &uStr1);
	DbgPrint("uStr3=%wZ\n", uStr3);

	RtlAppendUnicodeToString(&uStr1, L"world");
	DbgPrint("uStr1=%wZ\n", uStr1);

	RtlAppendUnicodeStringToString(&uStr1, &uStr2);
	DbgPrint("uStr1=%wZ\n", uStr1);


	if (RtlCompareUnicodeString(&uStr1, &uStr2, TRUE) == 0)//TRUE:case sensible
	{
		DbgPrint("%wZ == %wZ\n", uStr1, uStr2);
	}
	else
	{
		DbgPrint("%wZ != %wZ\n", uStr1, uStr2);
	}

	RtlAnsiStringToUnicodeString(&uStr3, &aStr1, TRUE);//TRUE: memory allocation for uStr1 and should be freed by RtlFreeUnicodeString
	DbgPrint("%wZ\n", &uStr3);
	RtlFreeUnicodeString(&uStr3);

	uStr4.Buffer = ExAllocatePoolWithTag(PagedPool, wcslen(L"Nice to meet u")+sizeof(WCHAR), 'POCU');
	if (uStr4.Buffer == NULL)
	{
		return;
	}
	RtlZeroMemory(uStr4.Buffer, wcslen(L"Nice to meet u")+sizeof(WCHAR));
	uStr4.Length = 0;
	uStr4.MaximumLength = wcslen(L"Nice to meet u")+sizeof(WCHAR);

	RtlInitUnicodeString(&uStr4, L"Nice to meet u");
	DbgPrint("%wZ\n", &uStr4);

	ExFreePool(uStr4.Buffer);

}