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
int DeRead(string fname);
int AscToUn(int len,unsigned char buf[],unsigned char buf2[]);
void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[]);
void WT_ADDR(long hFile,unsigned char fname[],int ofst,unsigned char buf[],int len);

int code_num;
unsigned char arr_code[TABLENUM][3];
unsigned char arr_char[TABLENUM][3];

int main() {
	string hdfname;
	
	_finddata_t sc_file;
	long lsf,lsf2;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) {cout<<"NO TBL"<<endl;return 0;}
	if((code_num=OpenTbl(sc_file.name))==0) {cout<<"ER TBL"<<endl;return 0;}
	_findclose(lsf);

	lsf = _findfirst("*",&sc_file);
	do {
		if(sc_file.attrib != _A_SUBDIR) continue;

		hdfname=string(sc_file.name);
		hdfname += "\\headfile.bin";
		if((lsf2 = _open(hdfname.c_str(),O_RDONLY|O_BINARY))==-1)
		{	continue; }
		_close(lsf2);

		hdfname=hdfname.substr(0,hdfname.length()-13);
		if(DeRead(hdfname)!=1)
		{
			cout<<hdfname<<" BUG"<<endl;
			continue;
		}
		cout<<hdfname<<" OK"<<endl;
	}while(_findnext(lsf,&sc_file)==0);

	cout<<"导出完毕"<<endl;
	_findclose(lsf);
	return 0;
}

int DeRead(string fname)
{
	int i,slen,nlen,ofst;
	long hSub,hTxt,hHdf;
	string hdfile,sbfile,txfile,sbname;
	unsigned char fnm[33],buf[25],buf2[49];
	unsigned char bs[3]={0xFF,0xFE};

	buf[24]=buf2[48]=fnm[32]=0x00;

	hdfile = fname + "\\headfile.bin";
	if((hHdf = _open(hdfile.c_str(),O_RDONLY|O_BINARY))==-1)
	{	cout<<"NO HDF "<<hdfile<<endl;return 0; }
	txfile = fname + "\\" + fname + ".txt";
	if((hTxt=_open(txfile.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE))==-1)
	{	cout<<"ER TXT "<<hdfile<<endl;return 0; }
	_write(hTxt,bs,2);
	_lseek(hHdf,8,SEEK_SET);

	while(!eof(hHdf))
	{
		_read(hHdf,fnm,32);
		sbname = string((char*)fnm);
		sbfile = fname + "\\" + sbname;
		if((hSub = _open(sbfile.c_str(),O_RDONLY|O_BINARY))==-1)
		{	cout<<"NO SBF "<<sbfile<<endl;return 0; }

		_lseek(hSub,0x10,SEEK_SET);
		for(i=0x10;i<0x8F;i++)
		{
			_read(hSub,buf,1);
			if(buf[0]>0x80 && buf[0]<0x9A)
			{
				_read(hSub,buf,1);
				if(buf[0]>0x3F)
				{
					_lseek(hSub,-2,SEEK_CUR);
					_read(hSub,buf,24);
					slen=strlen((char*)buf);
					nlen=AscToUn(slen,buf,buf2);
					WT_ADDR(hTxt,fnm,i,buf2,nlen);
					break;
				}
				else
					_lseek(hSub,-1,SEEK_CUR);
			}
		}
		if(i==0x8F)
		{
			cout<<sbname<<" no addr"<<endl;
		}
	}

	return 1;
}

void WT_ADDR(long hFile,unsigned char fname[],int ofst,unsigned char buf[],int len)
{
	int slen,nlen;
	unsigned char fgf[3]={0x92,0x21};
	unsigned char ctrl[5]={0x0D,0x00,0x0A,0x00};
	unsigned char fence[3]={0x0D,0xFF};
	unsigned char ofbf[5];
	unsigned char nmbuf[49];

	ofbf[1]=ofbf[3]=ofbf[4]=0x00;

	slen=strlen((char*)fname);
	nlen=AscToUn(slen,fname,nmbuf);
	_write(hFile,nmbuf,nlen);
	_write(hFile,fgf,2);
	ofbf[0] = 0x30 + ofst/16;
	ofbf[2] = 0x30 + ofst%16;
	if((ofst%16)>9) ofbf[2] += 7;
	_write(hFile,ofbf,4);

	SetFence(hFile,fence,ctrl);
	_write(hFile,buf,len);
	SetFence(hFile,fence,ctrl);
	_write(hFile,buf,len);
	SetFence(hFile,fence,ctrl);
	_write(hFile,ctrl,4);
}

int AscToUn(int len,unsigned char buf[],unsigned char buf2[])
{
	int i,j,k=0;
	for(i=0;i<len;i++)
	{
		if(buf[i]>0x80 && buf[i]<0x9F) 
		{
			for(j=0;j<code_num;j++) {
				if(buf[i]==arr_code[j][0] && buf[i+1]==arr_code[j][1]) {
					buf2[k]=arr_char[j][0];buf2[k+1]=arr_char[j][1];
					k+=2;break;
				}
			}
			if(j==code_num)
				cout<<"no "<<hex<<(int)buf[i]<<(int)buf[i+1]<<endl;
			i++;
		}
		else {
			for(j=0;j<code_num;j++) {
				if(buf[i]==arr_code[j][0]) {
					buf2[k]=arr_char[j][0];buf2[k+1]=arr_char[j][1];
					k+=2;break;
				}
			}
			if(j==code_num)
				cout<<"no "<<hex<<(int)buf[i]<<endl;
		}
	}
	return k;
}

void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[])
{
	_write(hFile,ctrl,4);
	for(int i=0;i<16;i++)
		_write(hFile,fence,2);
	_write(hFile,ctrl,4);
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