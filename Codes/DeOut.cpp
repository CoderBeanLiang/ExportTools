//对于格式不好的文件直接导出可修改部分，长度不允许改变

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
int DeRead(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
int AscToUn(int len,unsigned char buf[],unsigned char buf2[],int cdnum,unsigned char arr_code[][3],unsigned char arr_char[][3]);
void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[]);
int RToZero(long hFile,unsigned char buf[]);

int main() {
	int i=0,code_num;
	unsigned char buf='\0';
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) return 0;

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) return 0;

	if((lsf = _findfirst("*.decp",&sc_file))==-1) return 0;

	if(DeRead(sc_file.name,ch,ch2,code_num)!=1) return 0;
	
	while(_findnext(lsf,&sc_file)==0)
	{
		if(DeRead(string(sc_file.name),ch,ch2,code_num)!=1) return 0;
	}

	_findclose(lsf);
	return 0;
}

int DeRead(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
{
	int i,j,ofst,bf1len,bf2len;
	long hSub,hTxt,hIdx;
	string tx_path,ix_path;
	unsigned char buf[256],buf2[256],ofsbuf[5],rd1[2],rd2[2];
	unsigned char ctrl[5];
	unsigned char fence[3]={0x0D,0xFF};
	ofsbuf[4]=0x00;
	rd1[1]=rd2[1]=0x00;

	hSub=_open(fname.c_str(),O_RDONLY|O_BINARY);
	tx_path=fname.substr(0,fname.length()-5);
	tx_path += ".txt";
	hTxt=_open(tx_path.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
	ix_path=fname.substr(0,fname.length()-5);
	ix_path += ".idx";
	hIdx=_open(ix_path.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

	ctrl[0]=0xFF;
	ctrl[1]=0xFE;
	ctrl[2]=0x0A;
	ctrl[3]=0x00;
	ctrl[4]='\0';
	_write(hTxt,ctrl,2);
	ctrl[0]=0x0D;
	ctrl[1]=0x00;

	_lseek(hSub,8,SEEK_SET);
	_read(hSub,buf,3);//取偏移地址
	ofst=buf[0]+buf[1]*256+buf[2]*256*256;
	_lseek(hSub,ofst,SEEK_SET);
	i=0;
	while(!eof(hSub))
	{
		bf1len=RToZero(hSub,buf);//会读末尾0x00，但返回句长少1，计算ofst时需加1
		for(j=0;j<bf1len;j++)
		{
			if(buf[j]>0x80 && buf[j]<0x99) 
			{bf2len=AscToUn(bf1len,buf,buf2,cdnum,arr_code,arr_char);break;}
		}
		if(j==bf1len)
		{
			ofst=ofst+bf1len+1;
			continue;//该句无汉字或为0x00，不作导出，偏移地址加，读新句子
		}

		//写txt
		_write(hTxt,buf2,bf2len);
		SetFence(hTxt,fence,ctrl);
		_write(hTxt,buf2,bf2len);
		SetFence(hTxt,fence,ctrl);
		_write(hTxt,ctrl,4);
		//写该句句首偏移址
		ofsbuf[0]=ofst%256;
		ofsbuf[1]=ofst/256;
		ofsbuf[2]=ofst/(256*256);
		ofsbuf[3]=ofst/(256*256*256);
		_write(hIdx,ofsbuf,4);

		ofst=ofst+bf1len+1;
	}
	return 1;
}

int AscToUn(int len,unsigned char buf[],unsigned char buf2[],int cdnum,unsigned char arr_code[][3],unsigned char arr_char[][3])
{
	int i,j,k=0;
	for(i=0;i<len;i++)
	{
		if(buf[i]>0x80 && buf[i]<0x9F) 
		{
			for(j=0;j<cdnum;j++) {
				if(buf[i]==arr_code[j][0] && buf[i+1]==arr_code[j][1]) {
					buf2[k]=arr_char[j][0];buf2[k+1]=arr_char[j][1];
					k+=2;break;
				}
			}
			if(j==cdnum)
				cout<<"no "<<hex<<(int)buf[i]<<(int)buf[i+1]<<endl;
			i++;
		}
		else {
			for(j=0;j<cdnum;j++) {
				if(buf[i]==arr_code[j][0]) {
					buf2[k]=arr_char[j][0];buf2[k+1]=arr_char[j][1];
					k+=2;break;
				}
			}
			if(j==cdnum)
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