#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

using namespace std;

// PS: The game(USA) font table uses Unicode(UTF-8)
// 0x0A means "/n" in game and exported as "↙"(0xE28699) in txt
unsigned char NEXT_LINE[4] = {0xE2, 0x86, 0x99, '\0'};
unsigned char FENCE[4] = {0xEF, 0xBC, 0x8D, '\0'};
unsigned char CRLF[3] = {0x0D, 0x0A, '\0'};

int ReadMes(char* fname);
int main() {
	int i=0;
	unsigned char buf='\0';
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.ccmes",&sc_file))==-1) return 0;

	if(ReadMes(sc_file.name)!=1) return 0;
	
	while(_findnext(lsf,&sc_file)==0)
	{
		if(ReadMes(sc_file.name)!=1) return 0;
	}

	_findclose(lsf);
	return 0;
}

int RToZero(long hFile,unsigned char buf[])
{
	unsigned char uc[2];
	int i = 0;
	while (true)
	{
		_read(hFile,uc,1);
		if (uc[0] != 0x00)
		{
			buf[i] = uc[0];
			i++;
		}
		else
		{
			break;
		}
	}
	return i;
}

void Linefeed(long hFile)
{
	_write(hFile, CRLF, 2);
}

// Set txt format like "-------------------------"
void SetFence(long hFile)
{
	Linefeed(hFile);
	for(int i = 0; i < 16; i++)
		_write(hFile, FENCE, 3);
	Linefeed(hFile);
}

int ReadMes(char* fname)
{
	unsigned char buf[512],buf2[512];
	int flen,ofslen,hdlen,buflen,i,j,k,m,n;
	int paranum[1024];
	string iname,tname,scname;
	long hMes,hIdx,hTxt;
	if((hMes = _open(fname,O_RDONLY|O_BINARY)) == -1) return 0;
	mkdir("textout");
	scname=string(fname);
	iname="textout//"+scname+".index";
	tname="textout//"+scname+".txt";

	hIdx=_open(iname.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
	hTxt=_open(tname.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

	_read(hMes,buf,32);
	hdlen=buf[21]*64+buf[20]/4;
	ofslen=buf[29]*256+buf[28];
	flen=(buf[9]-buf[21])*256+(buf[8]-buf[20])-32;
	_write(hIdx,buf,32);

	//获取分段情况，每段00数量
	for(i=0;i<hdlen;i++)
	{
		_read(hMes,buf,4);
		_write(hIdx,buf,4);
		if(i<ofslen)
			paranum[i]=buf[3];
	}

	//按段落循环，段落数来源于索引第29个字节，一段包含若干个00结尾句
	for(i = 0; i < ofslen; i++)
	{
		if (i < 100)
		{
			buf[0] = 0x30 + i/10;
			buf[1] = 0x30 + i%10;
			buf[2] = '\0';
			_write(hTxt, buf, 2);
		}
		else
		{
			buf[0] = 0x30 + i/100;
			buf[1] = 0x30 + (i%100)/10;
			buf[2] = 0x30 + i%10;
			buf[3] = '\0';
			_write(hTxt, buf, 3);
		}
		SetFence(hTxt);

		//按句子循环，单条句子以00结尾，一段的句子数paranum[i]来源于索引偏移址后第2个字节
		for(j = 0; j < paranum[i]; j++)
		{
			buflen = RToZero(hMes, buf);
			n = -1;
			
			for(k = 0;k < buflen; k++)
			{
				if (buf[k] == 0x0A) {
					// NextLine
					buf2[++n] = NEXT_LINE[0];
					buf2[++n] = NEXT_LINE[1];
					buf2[++n] = NEXT_LINE[2];
				} else {
					buf2[++n] = buf[k];
				}
			}

			// Write the source sentence to txt, this line is for reference
			_write(hTxt, buf2, n+1);
			SetFence(hTxt);
			// Write again, this line is for replacing translation
			_write(hTxt, buf2, n+1);
			SetFence(hTxt);
			// To next line
			Linefeed(hTxt);
		}

		// After one paragraph, divide line
		Linefeed(hTxt);
		Linefeed(hTxt);
	}

	_close(hTxt);
	_close(hIdx);
	return 1;
}