//直接导入，长度减少部分补0x00，多出提示错误

#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

#define TABLENUM 4000

using namespace std;

int OpenTbl(char* fname,unsigned char arr_code[][3],unsigned char arr_char[][3]);
int DeWrite(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[],int cdnum,unsigned char arr_code[][3],unsigned char arr_char[][3]);
void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[]);
int EatFence(long hFile);
int RToEnd(long hFile,unsigned char buf[]);

int main() {
	int i=0,code_num;
	unsigned char buf='\0';
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) return 0;

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) return 0;

	if((lsf = _findfirst("*.txt",&sc_file))==-1) return 0;

	if(DeWrite(sc_file.name,ch,ch2,code_num)!=1) return 0;
	
	while(_findnext(lsf,&sc_file)==0)
	{
		if(DeWrite(sc_file.name,ch,ch2,code_num)!=1) return 0;
	}

	_findclose(lsf);
	return 0;
}
//以上是主程序

int DeWrite(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
{
	int i,len,ofst,a,b;
	long hSub,hTxt,hIdx;
	string sb_path,ix_path;
	unsigned char ofsbuf[5];
	unsigned char buf[256],buf2[256];
	ofsbuf[4]=0x00;

	hTxt=_open(fname.c_str(),O_RDONLY|O_BINARY);
	sb_path=fname.substr(0,fname.length()-4);
	sb_path += ".ccbwi";
	hSub=_open(sb_path.c_str(),O_WRONLY|O_BINARY);
	ix_path=fname.substr(0,fname.length()-4);
	ix_path += ".idx";
	hIdx=_open(ix_path.c_str(),O_RDONLY|O_BINARY);

	_lseek(hTxt,2,SEEK_SET);
	while(!eof(hIdx))
	{
		_read(hIdx,ofsbuf,4);
		ofst=ofsbuf[0]+ofsbuf[1]*256+ofsbuf[2]*256*256;
		_lseek(hSub,ofst,SEEK_SET);

		//读txt
		a=RToEnd(hTxt,buf);
		EatFence(hTxt);
		b=RToEnd(hTxt,buf);
		EatFence(hTxt);
		len=UnToANSI(b,buf,buf2,cdnum,arr_code,arr_char);
		//len与2*a比较
		if(len>(2*a)) cout<<buf2<<" too long"<<endl;
		while(len<(2*a))
		{
			buf2[len]=0x00;
			len++;
		}
		_write(hSub,buf2,2*a);
	}
	_close(hSub);
	_close(hTxt);
	_close(hIdx);

	return 1;
}

int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[],int cdnum,unsigned char arr_code[][3],unsigned char arr_char[][3])
{
	int i,j,flen=0;
	for(i=0;i<buflen;i++)
	{
		if(sbuf[2*i]==0x92 && sbuf[2*i+1]==0x21)
		{
			dbuf[flen]=0x00;
			flen++;
			continue;
		}
		for(j=0;j<cdnum;j++) 
		{
			if(sbuf[2*i]==arr_char[j][0] && sbuf[2*i+1]==arr_char[j][1]) 
			{
				if(arr_code[j][1] == '\0') 
				{
					dbuf[flen]=arr_code[j][0];
					flen++;break;
				} else {
					dbuf[flen]=arr_code[j][0];
					dbuf[flen+1]=arr_code[j][1];
					flen+=2;break;
				}
			}
		}
		if(j==cdnum)
			cout<<"no "<<hex<<(int)sbuf[2*i]<<(int)sbuf[2*i+1]<<endl;
	}
	return flen;
}

int EatFence(long hFile)
{
	unsigned char uc[3];
	do{
		_read(hFile,uc,2);
		if(uc[0]==0x0D && uc[1]==0x00)
			continue;
		else if(uc[0]==0x0A && uc[1]==0x00)
			continue;
		else if(uc[0]==0x0D && uc[1]==0xFF)
			continue;
		else
		{
			_lseek(hFile,-2,SEEK_CUR);
			break;
		}
	}
	while(!eof(hFile));
	return 0;
}

//读txt，一次两个字节
int RToEnd(long hFile,unsigned char buf[])
{
	unsigned char uc[3];
	int i=0;
	do{
		_read(hFile,uc,2);
		buf[i]=uc[0];
		buf[i+1]=uc[1];
		i+=2;
	}while(uc[0]!=0x0D || uc[1]!=0x00);
	if(uc[1]==0x00){
		_read(hFile,uc,2);
		return i/2-1;}
	return -1;
}

//打开码表
unsigned char Change(unsigned char c)
{
	if(c<0x3A)
		return c-0x30;
	else
		return c-0x37;
}

int OpenTbl(char* fname,unsigned char arr_code[][3],unsigned char arr_char[][3]) 
{
	unsigned char buf1[3],buf2[7];
	int i,f_len,c_num=0;
	long hTbl;
	if((hTbl = _open(fname,O_RDONLY|O_BINARY)) == -1) return 0;

	_read(hTbl,buf1,2);
	if(buf1[0] != 0xFF || buf1[1] != 0xFE) return 0;

	f_len = filelength(hTbl)/2 - 1;
	for(i=0;i<f_len;i++)
	{
		_read(hTbl,buf1,2);
		if((buf1[0]==0x0D || buf1[0]==0x0A)&& buf1[1]==0x00) continue;
		if((buf1[0]==0x38 || buf1[0]==0x39)&& buf1[1]==0x00)
		{
			_read(hTbl,buf2,6);
			i+=3;
			arr_code[c_num][0]=Change(buf1[0])*16+Change(buf2[0]);
			arr_code[c_num][1]=Change(buf2[2])*16+Change(buf2[4]);
			arr_code[c_num][2]='\0';
		}
		else
		{
			_read(hTbl,buf2,2);
			i++;
			if(buf1[0]==0x3D && buf1[1]==0x00) {
				arr_char[c_num][0]=buf2[0];
				arr_char[c_num][1]=buf2[1];
				arr_char[c_num][2]='\0';
				c_num++;
			}else if(buf1[0]==0x30 && buf1[1]==0x00 && buf2[0]==0x30 && buf2[1]==0x00){
				arr_code[c_num][1]=0x00;
				arr_code[c_num][2]='\0';
			}else {
				arr_code[c_num][0]=Change(buf1[0])*16+Change(buf2[0]);
				arr_code[c_num][1]='\0';
			}
		}
	}
	_close(hTbl);
	return c_num;
}