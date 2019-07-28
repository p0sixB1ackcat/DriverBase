#include <stdio.h>

int main()
{

	int isInVirtual = 0;
	__asm
	{
		push ebx
		push ecx
		push edx
		mov eax,'VMXh'
		mov ebx,0
		mov ecx,10
		mov edx,'VX'
		in eax,dx
		cmp ebx,'VMXh'
		jne ret	
		mov isInVirtual,1
		
	}
ret:
	__asm
	{
		pop edx
		pop ecx
		pop ebx
	}

	if(isInVirtual)
	{
		printf("在虚拟机中\n");
	}
	else
	{
		printf("不在虚拟机中\n");
	}

	getch();

	return 0;
}