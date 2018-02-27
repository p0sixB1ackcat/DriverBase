#include <ntddk.h>
#include <ntstrsafe.h>

int main()
{
	//从栈上给UNICODE_STRING的buffer分配空间
	UNICODE_STRING uStrWithStack = {0};//初始化一个UNICODE_STRING
	WCHAR buffer[512] = L"Hello world";
	uStrWithStack.Buffer = buffer;
	uStrWithStack.Length = (wcslen(L"Hello world")) * sizeof(WCHAR);
	uStrWithStack.MaximumLength = sizeof(buffer);

	//从堆上给UNICODE_STRING分配内存
	UNICODE_STRING uStrWithHeap = {0};
	uStrWithHeap.MaximumLength = MAX_PATH * sizeof(WCHAR);
	uStrWithHeap.Buffer = ExAllocatePoolWithTag(PagedPool,uStrWithHeap.MaximumLength);
	uStrWithHeap.Length = (wcslen(L"Hello world") * sizeof(WCHAR));
	if(uStrWithHeap.Buffer == NULL)
	{
		return -1;
	}
	
	RtlZeroMemory(uStrWithHeap,uStrWithHeap.MaximumLength);
	RtlCopyMemory(uStrWithHeap.Buffer,L"Hello world",wcslen(L"Hello world"));
	DbgPrint("%ws\n",&uStrWithHeap);
	ExFreePool(uStrWithHeap);


	//常用的操作函数
	UNICODE_STRING uStr = {0};
	WCHAR *szStr = L"Hello world";
	UNICODE_STRING uStr1 = {0};
	//使用RtlInitUnicodeString并没有将szStr中的字符串拷贝到uStr中
	//而是uStr.Buffer直接指向了szStr指向的字符串所在静态区了
	RtlInitUnicodeString(&uStr,szStr);
	//将uStr1拼接到uStr的后面，两个参数都是UNICODE_STRING类型指针
	RtlAppendUnicodeStringToString(&uStr,&uStr1);


	//将普通unicode编码的字符串拼接到UNICODE_STRING类型字符串的后面
	RtlAppendUnicodeToString(&uStr,L"nopnopnop...");

	//将uStr1拷贝到uStr
	RtlCopyUnicodeString(&uStr,&uStr1);

	//比较两个字符串是否相等，相等返回0，最后一个参数表示是否忽略大小写
	RtlCompareUnicodeString(&uStr,&uStr1,TRUE);

	char *astr = "ascii str";
	//将astr字符串转换为unicode编码的字符串，转换之后存放到uStr中，uStr存放
	//这个字符串的内存可以由系统分配，也可以程序员自己指定
	//第三个参数如果设置为TRUE，那么则有系统从堆上给uStr分配内存，使用完别忘了ExFreeMemory
	//FALSE则需要程序员自己设置
	RtlAnsiStringToUnicodeString(&uStr,astr,TRUE);

	//安全的函数，含有溢出检测---#include <ntstrsafe.h>
	RtlUnicodeStringInit(&uStr,szStr);
	
	RtlUnicodeStringCopy(&uStr,&uStr1);

	RtlUnicodeStringCat(&uStr,&uStr1);

}