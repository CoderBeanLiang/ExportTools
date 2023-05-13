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
int WriteMes(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
int main()
{
	int i=0,code_num;
	string fname;
	unsigned char buf='\0';
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];
	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) return 0;

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) return 0;

	if((lsf = _findfirst("textout\\*.index",&sc_file))==-1) return 0;

	fname=string(sc_file.name);
	if(WriteMes(fname,ch,ch2,code_num)!=1) return 0;

	while(_findnext(lsf,&sc_file) == 0)
	{
		fname=string(sc_file.name);
		if(WriteMes(fname,ch,ch2,code_num)!=1)
			return 0;
	}

	cout<<"完成，请按任意键结束！"<<endl;
	cin.get();
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
int WriteMes(string fname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
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

	flen=0;
	_lseek(hTxt,2,SEEK_SET);
	//按段落循环
	for(i=0;i<ofsnum;i++)
	{
		//记录指针
		offst[i][0]=flen%256;
		offst[i][1]=flen/256;
		offst[i][2]='\0';

		buflen=RToEnd(hTxt,buf);
		if(buflen==2){
			n = (buf[0]-0x30)*10 + (buf[2]-0x30);
		}else{
			n = (buf[0]-0x30)*100 + (buf[2]-0x30)*10 + (buf[4]-0x30);
		}
		if(n == i)
		{
			EatFence(hTxt);//_lseek(hTxt,36,SEEK_CUR);//跳过编号附带16个-加一次换行符号
			for(j=0;j<paranum[i];j++)
			{
				buflen=RToEnd(hTxt,buf);//第一行日文原文略过
				EatFence(hTxt);//_lseek(hTxt,36,SEEK_CUR);//跳过日文行附带16个-加一次换行符号
				buflen=RToEnd(hTxt,buf);
				for(k=0;k<buflen;k++)
				{
					for(m=0;m<cdnum;m++) {
						if(buf[2*k]==arr_char[m][0] && buf[2*k+1]==arr_char[m][1]) {
							if(arr_code[m][1] == '\0') {
								_write(hMes,arr_code[m],1);
								flen++;break;
							} else {
								_write(hMes,arr_code[m],2);
								flen+=2;break;
							}
						}
						if(m==cdnum)
							cout<<"no "<<hex<<(int)buf[2*k]<<(int)buf[2*k+1]<<endl;
					}
				}
				_write(hMes,bf_end,1);flen++;
				EatFence(hTxt);//_lseek(hTxt,40,SEEK_CUR);//跳过中文行附带16个-加两次换行符号
			}
		}
		EatFence(hTxt);//_lseek(hTxt,8,SEEK_CUR);//跳过段后两次换行
	}
	//覆写文件长度
	_lseek(hMes,8,SEEK_SET);
	flen += hdlen;
	buf[0]=flen%256;buf[1]=flen/256;buf[2]='\0';
	_write(hMes,buf,2);

	_lseek(hMes,22,SEEK_CUR);
	//覆写指针
	for(i=0;i<ofsnum;i++)
	{
		_write(hMes,offst[i],2);
		_lseek(hMes,2,SEEK_CUR);
	}

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