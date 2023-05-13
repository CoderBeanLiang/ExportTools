#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

#define TABLENUM 4000

using namespace std;

int OpenTbl(char* fname);
int DeWrite(string fname);
int RToEnd(long hFile,unsigned char buf[]);
int EatFence(long hFile);
int ReadTwo(long hFile,unsigned char buf[]);
unsigned char Change(unsigned char c);
int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[]);

int code_num;
unsigned char arr_code[TABLENUM][3];
unsigned char arr_char[TABLENUM][3];

int main() {
	string txfname,txfpath;
	
	_finddata_t sc_file;
	long lsf,lsf2;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) {cout<<"NO TBL"<<endl;return 0;}
	if((code_num=OpenTbl(sc_file.name))==0) {cout<<"ER TBL"<<endl;return 0;}
	_findclose(lsf);

	lsf = _findfirst("*",&sc_file);
	do {
		if(sc_file.attrib != _A_SUBDIR) continue;

		txfname=string(sc_file.name);
		txfpath = txfname + "\\" + txfname + ".txt";
		if((lsf2 = _open(txfpath.c_str(),O_RDONLY|O_BINARY))==-1)
		{	continue; }
		_close(lsf2);

		if(DeWrite(txfname)!=1)
		{
			cout<<txfname<<" BUG"<<endl;
			continue;
		}
		cout<<txfname<<" OK"<<endl;
	}while(_findnext(lsf,&sc_file)==0);

	cout<<"导入完毕"<<endl;
	_findclose(lsf);
	return 0;
}

int DeWrite(string fname)
{
	int i,j,slen,nlen,ofst;
	long hSub,hTxt;
	string sbfile,txfile,sbname;
	unsigned char buf[33],buf2[33],sbn[25];

	buf[32]=buf2[32]=sbn[24]=0x00;

	txfile = fname + "\\" + fname + ".txt";
	if((hTxt = _open(txfile.c_str(),O_RDONLY|O_BINARY))==-1)
	{	cout<<"NO TXT "<<txfile<<endl;return 0; }

	_lseek(hTxt,2,SEEK_SET);

	while(!eof(hTxt))
	{
		slen=RToEnd(hTxt,buf);
		for(i=0;i<slen;i++)
		{
			if(buf[2*i]==0x92 && buf[2*i+1]==0x21)
			{
				ofst = 16*Change(buf[2*i+2]) + Change(buf[2*i+4]);
				for(j=0;j<i;j++)
					sbn[j]=buf[2*j];
				sbn[j]=0x00;
				break;
			}
		}
		sbfile = fname + "\\" + string((char *)sbn);
		if((hSub = _open(sbfile.c_str(),O_WRONLY|O_BINARY))==-1)
		{	cout<<"NO SBF "<<string((char *)sbn)<<endl;return 0; }

		slen=ReadTwo(hTxt,buf);
		EatFence(hTxt);
		_lseek(hSub,ofst,SEEK_SET);
		nlen=UnToANSI(slen,buf,buf2);
		if(nlen<24) {
			for(i=nlen;i<24;i++)
				buf2[i]=0x00;
		}
		_write(hSub,buf2,24);
		_close(hSub);
	}
	_close(hTxt);

	return 1;
}

int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[])
{
	int i,j,flen=0;
	for(i=0;i<buflen;i++)
	{
		for(j=0;j<code_num;j++) 
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
		if(j==code_num)
			cout<<"no "<<hex<<(int)sbuf[2*i]<<(int)sbuf[2*i+1]<<endl;
	}
	return flen;
}

int ReadTwo(long hFile,unsigned char buf[])
{
	int a,b;
	EatFence(hFile);
	a=RToEnd(hFile,buf);
	EatFence(hFile);
	b=RToEnd(hFile,buf);

	return b;
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

unsigned char Change(unsigned char c)
{
	if(c<0x3A)
		return c-0x30;
	else
		return c-0x37;
}

int OpenTbl(char* fname) 
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