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
int WriteStx(long hsf,string fdname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
unsigned char Change(unsigned char c);
int RToEnd(long hFile,unsigned char buf[]);
int RHdLine(long hFile,unsigned char buf[]);
//int RHdLine(long hFile,unsigned char buf[],int len);
int EatFence(long hFile);
int ReadTwo(long hFile,unsigned char buf[]);
int AddZero(int dt,unsigned char buf[]);
int EatZero(long hFile);
int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[],int cdnum,unsigned char arr_code[][3],unsigned char arr_char[][3]);

//函数主体
int main()
{
	int code_num;
	string fdrname,txtname;
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];

	_finddata_t sc_file;
	long lsf,lsf2;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) {cout<<"no tbl"<<endl;return 0;}

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) {cout<<"tbl error"<<endl;return 0;}

	if((lsf = _findfirst("*",&sc_file))==-1) return 0;

	do{
		if(sc_file.attrib != _A_SUBDIR) continue;
			
		fdrname = string(sc_file.name);
		txtname = fdrname + "\\decompress\\" + fdrname + ".txt";
		if((lsf2 = _open(txtname.c_str(),O_RDONLY|O_BINARY))==-1) continue;

		if(WriteStx(lsf2,fdrname,ch,ch2,code_num)!=1)
		{
			cout<<fdrname<<" BUG"<<endl;
			continue;
		}
		cout<<fdrname<<" OK"<<endl;
		_close(lsf2);
	}while(_findnext(lsf,&sc_file)==0);

	cout<<"导入完毕"<<endl;
	_findclose(lsf);

	return 0;
}
//函数主体

//导入函数
int WriteStx(long hsf,string fdname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
{
	int i,n;
	int buflen,rdlen,gtlen,newlen,edlen,flag;
	char sc_path[256];
	string sb_path,de_path,tp_path,subname,str_buf,str_del,str_odr;
	unsigned char buf[1024],buf2[1024],buf3[1024];//buf通用，2用来读txt，3存U2A串
	long hSub,hTmp;

	_getcwd(sc_path,256);
	//wk_path = fdname + "\\decompress";

	_lseek(hsf,2,SEEK_SET);

	//开始读txt，一个标号为一段大循环，写一次temp并压缩，标号内一句一次小循环
	while(!eof(hsf))
	{
		buflen=RToEnd(hsf,buf);//读txt内容标号
		if(buflen==2) { n = (buf[0]-0x30)*10 + (buf[2]-0x30); }
		else {n = (buf[0]-0x30)*100 + (buf[2]-0x30)*10 + (buf[4]-0x30); }
		EatFence(hsf);
		buflen=RToEnd(hsf,buf);//读文件名，得到的是双字节
		newlen=UnToANSI(buflen,buf,buf2,cdnum,arr_code,arr_char);
		buf2[newlen]='\0';//buf2暂存文件名
		subname = string((char*)buf2);
		sb_path = fdname + "\\" + subname;
		de_path = fdname + "\\decompress\\" + subname;
		if((hSub = _open(sb_path.c_str(),O_RDONLY|O_BINARY)) == -1) return 0;
		_read(hSub,buf,1);
		if(buf[0]==0x11) {de_path += ".lz";flag=2;}
		else if(buf[0]==0x30) {de_path += ".lf";flag=1;}
		else if(buf[0]==0x00) {de_path += ".nu";flag=0;}
		_close(hSub);
		if((hSub=_open(de_path.c_str(),O_RDONLY|O_BINARY)) == -1) return 0;

		tp_path = "temp.bin";
		hTmp=_open(tp_path.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
		_read(hSub,buf,8);
		_write(hTmp,buf,8);//写解压后文件头8字节

		while(!eof(hSub))
		{
			if((rdlen=RHdLine(hSub,buf))==0) break;
			_lseek(hSub,rdlen,SEEK_CUR);//原文件（解压出的文件）跳过内容
			if((gtlen=ReadTwo(hsf,buf2))==0)
			{ cout<<de_path<<" BUG"<<endl;return 0;}//错误返回
			gtlen=UnToANSI(gtlen,buf2,buf3,cdnum,arr_code,arr_char);

			newlen=AddZero(gtlen,buf3);//写个判断新句长度的函数，末尾补零，返回newlen
			buf[10]=newlen%256; buf[11]=newlen/256;//修改句长

			_write(hTmp,buf,16);//写句头
			_write(hTmp,buf3,newlen);//写新内容
		}
		EatFence(hsf);//一个temp文件写完

		_close(hSub);
		_close(hTmp);
		//删源ccStx，压缩temp替换源ccStx，删temp
		str_buf=string((char*)sc_path);              //工作路径，temp.bin所在
		str_del="del "+str_buf+"\\"+sb_path;
		
		tp_path=str_buf+"\\"+tp_path;
		system(str_del.c_str());                     //删源
		//压缩部分
		if(flag==0)
		{
			hSub=_open(sb_path.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
			hTmp=_open(tp_path.c_str(),O_RDONLY|O_BINARY);
			buf[0]=0x00;
			buf[1]=filelength(hTmp)%256;
			buf[2]=filelength(hTmp)/256;
			buf[3]=filelength(hTmp)/(256*256);
			_write(hSub,buf,4);
			while(!eof(hTmp))
			{
				_read(hTmp,buf,2);
				_write(hSub,buf,2);
			}
			_close(hSub);
			_close(hTmp);
		}
		else
		{
			str_odr="CrystalTile2/ -c ";
			if(flag==2)
				str_odr += "-L ";
			if(flag==1)
				str_odr += "-r ";
			_chdir("E:\\CrystalTile2\\");
			str_odr=str_odr+tp_path+" -o "+str_buf+"\\"+sb_path;
			system(str_odr.c_str());
			_chdir(sc_path);//压缩完成
		}
		str_del="del "+tp_path;
		system(str_del.c_str());
	}

	return 1;
}
//导入函数

//取文件结尾0字符
int EatZero(long hFile)
{
	int len=4;
	unsigned char buf[5];
	if(_read(hFile,buf,len)==0) return 0;
	if((buf[0]|buf[1]|buf[2]|buf[3])==0x00)
		len += EatZero(hFile);
	else {
		_lseek(hFile,-len,SEEK_CUR);
		len = 0;
	}
	return len;
}

//句末补0
int AddZero(int dt,unsigned char buf[])
{
	int i,newlen;
	i=dt%4;
	if(i==3)
		newlen=dt+5;
	else
		newlen=dt+4-i;

	for(i=dt;i<newlen;i++)
	{
		buf[i]=0x00;
	}
	return newlen;
}

int ReadTwo(long hFile,unsigned char buf[])
{
	int a,b;
	EatFence(hFile);
	a=RToEnd(hFile,buf);
	EatFence(hFile);
	b=RToEnd(hFile,buf);
	/*if(a==b)//不能这样判断
	{
		return b;
	}*/
	return b;//错误返回0
	
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

unsigned char Change(unsigned char c)
{
	if(c<0x3A)
		return c-0x30;
	else
		return c-0x37;
}

int RHdLine(long hFile,unsigned char buf[])
{
	int i;
	if(_read(hFile,buf,16) < 16) return 0;
	if(buf[10]==0xFF && buf[11]==0xFF)
	{
		cout<<"one special format"<<endl;
		_lseek(hFile,-12,SEEK_CUR);
		i=RHdLine(hFile,buf);
	}else if(buf[6]==0xFF && buf[7]==0xFF) {
		i=buf[10]+buf[11]*256;
	}else
		i=0;

	return i;
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