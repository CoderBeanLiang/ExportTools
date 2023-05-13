#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

#define TABLENUM 4000

using namespace std;

int code_num;
unsigned char arr_code[TABLENUM][3];
unsigned char arr_char[TABLENUM][3];

int OpenTbl(char* fname);
int RToZero(long hFile,unsigned char buf[]);
int AscToUn(int len,unsigned char buf[],unsigned char buf2[]);
void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[]);
int WT_LIB(long hSub,long hTxt);
int DeRead(string fname);

int main()
{
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
	getchar();
	return 0;
}

int DeRead(string fname)
{
	int rdlen;//BM三大段每段长度
	long hSub,hTxt,hHdf;
	string hdfile,sbfile,txfile,sbname;
	unsigned char fnm[33],buf[25],buf2[49];
	unsigned char bs[3]={0xFF,0xFE};
	unsigned char bmlen[5];

	buf[24]=buf2[48]=fnm[32]=bmlen[4]=0x00;

	hdfile = fname + "\\headfile.bin";
	if((hHdf = _open(hdfile.c_str(),O_RDONLY|O_BINARY))==-1)
	{	cout<<"NO HDF "<<hdfile<<endl;return 0; }
	
	_lseek(hHdf,8,SEEK_SET);

	while(!eof(hHdf))
	{
		//从headfile.bin读子文件名
		_read(hHdf,fnm,32);
		sbname = string((char*)fnm);
		sbfile = fname + "\\" + sbname;
		if((hSub = _open(sbfile.c_str(),O_RDONLY|O_BINARY))==-1)
		{	_close(hSub);cout<<"NO SBF "<<sbfile<<endl;return 0; }

		//根据子文件名创建对应txt文档
		txfile = fname + "\\" + sbname + ".txt";
		if((hTxt=_open(txfile.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE))==-1)
		{	cout<<"ER TXT "<<hdfile<<endl;return 0; }
		_write(hTxt,bs,2);//头两个字节控制txt编码格式

		//分文件，不同文件起址不同，但读取方式相同
		if(sbname.substr(sbname.length() - 5, 5) == "cclhd")
		{
			_lseek(hSub,0x10,SEEK_SET);//hd文件从第二行读起（因格式未懂，顺序读）
			while(!eof(hSub)){
				WT_LIB(hSub,hTxt);
			}
		}else if(sbname.substr(sbname.length() - 5, 5) == "cclbm")
		{
			_lseek(hSub,0x04,SEEK_SET);//bm文件从第4个字节读起
			//每个文本有三段，多余无用
			for(int i=0;i<3;i++)
			{
				_read(hSub,bmlen,4);
				rdlen = bmlen[1]*256 + bmlen[0] - 4;
				rdlen -= WT_LIB(hSub,hTxt);
				rdlen -= WT_LIB(hSub,hTxt);
				_lseek(hSub,rdlen,SEEK_CUR);
			}
		}
			

		_close(hTxt);
		_close(hSub);
	}
	_close(hHdf);

	return 1;
}

int WT_LIB(long hSub,long hTxt)
{
	unsigned char rdbuf[1024],wtbuf[1024];
	int j,rdlen,wtlen;
	unsigned char ctrl[5]={0x0D,0x00,0x0A,0x00};
	unsigned char fence[3]={0x0D,0xFF};

	//读取
	rdlen = RToZero(hSub,rdbuf);//会读末尾0x00，但返回句长少1
	for(j=0;j<rdlen;j++)
	{
		if(rdbuf[j]>0x80 && rdbuf[j]<0x99) 
		{wtlen=AscToUn(rdlen,rdbuf,wtbuf);break;}
	}
	if(j==rdlen)
	{
		return 1;//该句为0x00，读新句子
	}

	//写txt
	_write(hTxt,wtbuf,wtlen);
	SetFence(hTxt,fence,ctrl);
	_write(hTxt,wtbuf,wtlen);
	SetFence(hTxt,fence,ctrl);
	_write(hTxt,ctrl,4);

	return rdlen+1;
}

//后面不需要管
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
		}else {
			for(j=0;j<code_num;j++) {
				if(buf[i]==0x0D) {
					buf2[k]=0x92;buf2[k+1]=0x21;//改0x0d为右箭头
					k+=2;break;
				}else if(buf[i]==arr_code[j][0]) {
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

int RToZero(long hFile,unsigned char buf[])
{
	unsigned char uc[2];
	int i=0;
	do{
		_read(hFile,uc,1);
		buf[i]=uc[0];
		i++;
	}while(uc[0]!=0x00);
	return i-1;
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