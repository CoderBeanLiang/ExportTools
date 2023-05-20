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

int WriteMes(string fname);

int main()
{
	int i=0,code_num;
	string fname;
	unsigned char buf='\0';
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("textout\\*.index",&sc_file))==-1) return 0;

	fname = string(sc_file.name);
	if (WriteMes(fname) != 1)
		return 0;

	while(_findnext(lsf, &sc_file) == 0)
	{
		fname = string(sc_file.name);
		if (WriteMes(fname) != 1)
			return 0;
	}

	cout<<"Finished, press any key to exit!"<<endl;
	cin.get();
	_findclose(lsf);
	return 0;
}

// Read one line until meet the CRLF(0x0D0A)
int RToEnd(long hFile,unsigned char buf[])
{
	unsigned char uc[3];
	unsigned char uc2[3];
	int i = 0;
	while (true)
	{
		_read(hFile, uc, 1);
		if (uc[0] == 0x0D)
		{
			_read(hFile, uc2, 1);
			if (uc2[0] == 0x0A)
			{
				return i;
			}
			else
			{
				buf[i] = uc[0];
				buf[i+1] = uc2[0];
				i += 2;
			}
		}
		else
		{
			buf[i] = uc[0];
			i++;
		}
	}
	return -1;
}

// Skip until not fence or linefeed
int EatFence(long hFile)
{
	unsigned char uc[3];
	do {
		_read(hFile, uc, 1);
		if(uc[0] == 0x0D)
		{
			// There may need left length check...
			_read(hFile, uc, 1);
			if (uc[0] == 0x0A) {
				continue;
			} else {
				_lseek(hFile, -2, SEEK_CUR);
				break;
			}
		}
		else if(uc[0] == 0xEF)
		{
			// There may need left length check...
			_read(hFile, uc, 2);
			if (uc[0] == 0xBC && uc[1] == 0x8D) {
				continue;
			} else {
				_lseek(hFile, -3, SEEK_CUR);
				break;
			}
		}
		else
		{
			_lseek(hFile, -1, SEEK_CUR);
			break;
		}
	}
	while(!eof(hFile));
	return 0;
}

int WriteMes(string fname)
{
	int flen,hdnum,ofsnum,hdlen,buflen,i,j,k,m,n;//ofsnum:指针数量;hdnum:指针区块数,一块4字节;hdlen:头部索引长度(指针区长+32)
	int paranum[1024];         //存每段落句子数量
	unsigned char buf[512];   //存句子
	unsigned char bf_end[2];  //存句末0x00
	unsigned char offst[1024][3];
	bf_end[0]=0x00;

	long hMes,hIdx,hTxt;
	fname="textout\\"+fname;
	if((hIdx = _open(fname.c_str(),O_RDONLY|O_BINARY)) == -1) return 0;
	fname=fname.substr(0,fname.length()-5);
	fname += "txt";
	if((hTxt = _open(fname.c_str(),O_RDONLY|O_BINARY)) == -1) return 0;
	fname=fname.substr(8,fname.length()-12);
	if((hMes=_open(fname.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE)) == -1) return 0;

	_read(hIdx,buf,32);
	_write(hMes,buf,32);
	hdlen = buf[21]*256 + buf[20];
	hdnum = hdlen/4;
	hdlen += 32;
	ofsnum=buf[28] + buf[29]*256;

	//获取分段情况，每段00数量，写头部索引区
	for(i=0;i<hdnum;i++)
	{
		_read(hIdx,buf,4);
		_write(hMes,buf,4);
		if(i<ofsnum)
			paranum[i]=buf[3];
	}

	flen = 0;
	
	//按段落循环
	for(i=0;i<ofsnum;i++)
	{
		//记录指针
		offst[i][0]=flen%256;
		offst[i][1]=flen/256;
		offst[i][2]='\0';

		buflen = RToEnd(hTxt, buf);
		if (buflen == 3) {
			n = (buf[0]-0x30)*100 + (buf[1]-0x30)*10 + (buf[2]-0x30);
		} else {
			n = (buf[0]-0x30)*10 + (buf[1]-0x30);
		}
		if (n == i)
		{
			EatFence(hTxt);		// Skip the format "----------------" and linefeed
			for(j = 0; j < paranum[i]; j++)
			{
				buflen = RToEnd(hTxt, buf);	// Read(Skip) the first text line used for referrence
				EatFence(hTxt);				// Skip the format "----------------" and linefeed
				buflen = RToEnd(hTxt, buf);	// Read the translation line
				if (buflen > 0)
				{
					_write(hMes, buf, buflen);
					flen += buflen;
				}
				_write(hMes, bf_end, 1);
				flen++;
				
				EatFence(hTxt);		// Skip the format fence and linefeed
			}
		}

		EatFence(hTxt);// Skip the format linefeed
	}

	//Overwrite the file length
	_lseek(hMes, 8, SEEK_SET);
	flen += hdlen;
	buf[0] = flen%256;
	buf[1] = flen/256;
	buf[2] = '\0';
	_write(hMes, buf, 2);

	//Overwrite the poiner index
	_lseek(hMes, 22, SEEK_CUR);
	for(i = 0; i < ofsnum; i++)
	{
		_write(hMes, offst[i], 2);
		_lseek(hMes, 2, SEEK_CUR);
	}

	return 1;
}