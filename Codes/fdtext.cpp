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
int ReadMes(char* fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
int main() {
	int i=0,code_num;
	unsigned char buf='\0';
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) return 0;

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) return 0;

	if((lsf = _findfirst("*.ccmes",&sc_file))==-1) return 0;

	if(ReadMes(sc_file.name,ch,ch2,code_num)!=1) return 0;
	
	while(_findnext(lsf,&sc_file)==0)
	{
		if(ReadMes(sc_file.name,ch,ch2,code_num)!=1) return 0;
	}

	_findclose(lsf);
	return 0;
}

unsigned char Change(unsigned char c)
{
	if(c<0x3A)
		return c-0x30;
	else
		return c-0x37;
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
int ReadMes(char* fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
{
	unsigned char buf[512],buf2[512];
	unsigned char ctrl[5];
	unsigned char fence[3]={0x0D,0xFF};
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

	ctrl[0]=0xFF;
	ctrl[1]=0xFE;
	ctrl[2]=0x0A;
	ctrl[3]=0x00;
	ctrl[4]='\0';
	_write(hTxt,ctrl,2);
	ctrl[0]=0x0D;
	ctrl[1]=0x00;

	//获取分段情况，每段00数量
	for(i=0;i<hdlen;i++)
	{
		_read(hMes,buf,4);
		_write(hIdx,buf,4);
		if(i<ofslen)
			paranum[i]=buf[3];
	}

	//按段落循环，段落数来源于索引第29个字节，一段包含若干个00结尾句
	for(i=0;i<ofslen;i++)
	{
		if(i<100){
			buf[1]=0x00;buf[3]=0x00;buf[4]='\0';
			buf[0]=0x30 + i/10;
			buf[2]=0x30 + i%10;
			_write(hTxt,buf,4);
		}
		else{
			buf[1]=0x00;buf[3]=0x00;buf[5]=0x00;buf[6]='\0';
			buf[0]=0x30 + i/100;
			buf[2]=0x30 + (i%100)/10;
			buf[4]=0x30 + i%10;
			_write(hTxt,buf,6);
		}
		SetFence(hTxt,fence,ctrl);

		//按句子循环，单条句子以00结尾，一段的句子数paranum[i]来源于索引偏移址后第2个字节
		for(j=0;j<paranum[i];j++)
		{
			buflen=RToZero(hMes,buf);
			n=-1;//buf2下标从-1开始
			//读取单句分析并写入buf2
			for(k=0;k<buflen;k++)
			{
				
				//找到文字
				if(buf[k]>0x80 && buf[k]<0x9F) {
					for(m=0;m<cdnum;m++)
					{
						if(buf[k]==arr_code[m][0] && buf[k+1]==arr_code[m][1]) {
							buf2[++n]=arr_char[m][0];buf2[++n]=arr_char[m][1];break;}
					}
					if(m==cdnum)
						cout<<"no "<<hex<<(int)buf[k]<<(int)buf[k+1]<<endl;
					k++;
				}else if(buf[k]==0x0A) {    //找到换行符
					buf2[++n]=arr_char[0][0];buf2[++n]=arr_char[0][1];    //换行
				}else {                     //英文字符等控制符
					for(m=0;m<cdnum;m++)
					{
						if(buf[k]==arr_code[m][0]){
							buf2[++n]=arr_char[m][0];buf2[++n]=arr_char[m][1];break;}
					}
					if(m==cdnum)
						cout<<"no "<<hex<<(int)buf[k]<<endl;
				}
			}
			_write(hTxt,buf2,n+1);
			SetFence(hTxt,fence,ctrl);
			_write(hTxt,buf2,n+1);//再写一遍该句，方便翻译对照
			SetFence(hTxt,fence,ctrl);
			_write(hTxt,ctrl,4);//分析完一句，换行

		}
		//一段完成，隔行
		_write(hTxt,ctrl,4);
		_write(hTxt,ctrl,4);
	}
	_close(hTxt);
	_close(hIdx);
	return 1;
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