#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static size_t SIZE_BUF=16;
static char * buf;

typedef int (*func_for_gets)(const char * buf); 
void LoadGets(const char * fullname, func_for_gets func);

//-----------------------------------------------------------------------------

void PadCR(char * buffer)
{
	char * p=&buffer[strlen(buffer)-1];
	if((*p==0x0A) || (*p==0x0D)) *p=0;
}

//-----------------------------------------------------------------------------

void ClearBuf()
{
	if(buf)	free(buf);
	SIZE_BUF=16;
	buf=0;
}

char * NewBuf()
{
	if(buf)	free(buf);
	SIZE_BUF=2*SIZE_BUF;
	buf = (char*)malloc(SIZE_BUF);
	buf[SIZE_BUF]=0;
//	printf("set size %d\n",SIZE_BUF);
	return buf;
}

//-----------------------------------------------------------------------------

void LoadGets(const char * fullname, func_for_gets func)
{
	FILE * f = fopen(fullname, "r");
	if(!f) 
	{
		printf("Not open %s\n",fullname);
		return;
	}

	long pos=0;
	buf=NewBuf();

	while(buf)
	{
		pos=ftell(f);
		if(!fgets(buf,SIZE_BUF,f)) break;

		if(strlen(buf)==(SIZE_BUF-1))
		{
			buf=NewBuf();
			fseek(f,pos,0); 
			continue;
		}
		PadCR(buf);
		if(strlen(buf))
		{	
			if(func(buf)) break;
		}	
	}

	fclose(f);
	ClearBuf();
}

//-----------------------------------------------------------------------------

