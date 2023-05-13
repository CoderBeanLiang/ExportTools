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
int RToEnd(long hFile,unsigned char buf[]);
int ReadTwo(long hFile,unsigned char buf[]);
int EatFence(long hFile);
int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[]);
int DeWrite(string fname);

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
		if(DeWrite(hdfname)!=1)
		{
			cout<<hdfname<<" BUG"<<endl;
			continue;
		}
		cout<<hdfname<<" OK"<<endl;
	}while(_findnext(lsf,&sc_file)==0);

	cout<<"导入完毕"<<endl;
	_findclose(lsf);
	getchar();
	return 0;
}

int DeWrite(string fname)
{
	int rdlen,wtlen,allen;//allen是读取总长，hd文件内容长或bm一段长
	long hSub,hTxt,hHdf;
	string hdfile,sbfile,txfile,sbname;
	unsigned char fnm[33],buf[25],buf2[49];
	unsigned char kong[5]={0x00,0x00,0x00,0x00,0x00};
	unsigned char head[3];
	unsigned char bmlen[5];
	unsigned char rdbuf[1024],wtbuf[1024];

	buf[24]=buf2[48]=fnm[32]=bmlen[4]=head[2]=0x00;

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
		if((hSub = _open(sbfile.c_str(),O_WRONLY|O_BINARY))==-1)
		{	_close(hSub);cout<<"NO SBF "<<sbfile<<endl;return 0; }

		//根据子文件名打开对应txt文档
		txfile = fname + "\\" + sbname + ".txt";
		if((hTxt=_open(txfile.c_str(),O_RDONLY|O_BINARY))==-1)
		{	cout<<"ER TXT "<<hdfile<<endl;return 0; }
		_lseek(hTxt,2,SEEK_SET);//头两个字节控制txt编码格式，跳过
		allen = 0;

		//分文件
		if(sbname.substr(sbname.length() - 5, 5) == "cclhd")
		{
			_lseek(hSub,0x10,SEEK_SET);//hd文件从第二行读起（因格式未懂，顺序读）
			while(!eof(hTxt)){
				rdlen = ReadTwo(hTxt,rdbuf);
				wtlen = UnToANSI(rdlen,rdbuf,wtbuf);
				//句尾补零
				wtbuf[wtlen] = 0x00;
				allen += wtlen;
				allen += 1;
				_write(hSub,wtbuf,wtlen + 1);
			}
			int i=0;
			if((i = filelength(hSub) - allen - 16) > 0) {
				for(int j=0;j<i;j++)
					wtbuf[j] = 0x00;//文件长出来的长度全补0
			}
			_write(hSub,wtbuf,i);
		}else if(sbname.substr(sbname.length() - 5, 5) == "cclbm")
		{
			_lseek(hSub,0x04,SEEK_SET);//bm文件从第4个字节读起
			//每个文本有三段，多余无用
			for(int i=0;i<3;i++)
			{
				_write(hSub,kong,4);//头四字节先置空
				rdlen = ReadTwo(hTxt,rdbuf);
				wtlen = UnToANSI(rdlen,rdbuf,wtbuf);
				wtbuf[wtlen] = 0x00;
				allen = wtlen + 1;
				_write(hSub,wtbuf,wtlen+1);
				//读写第二部分
				rdlen = ReadTwo(hTxt,rdbuf);
				wtlen = UnToANSI(rdlen,rdbuf,wtbuf);
				wtbuf[wtlen] = 0x00;
				allen += wtlen;
				allen += 1;
				_write(hSub,wtbuf,wtlen+1);
				//补零成双
				if(1 == allen%2) {
					_write(hSub,kong,1);
					allen += 1;
				}
				//开始计算段长，并修改头四字节
				allen += 4;
				head[0] = allen%256;
				head[1] = allen/256;
				_lseek(hSub,-allen,SEEK_CUR);
				_write(hSub,head,2);
				_lseek(hSub,allen-2,SEEK_CUR);
			}
		}		

		_close(hTxt);
		_close(hSub);
	}
	_close(hHdf);

	return 1;
}

//buflen是ReadTwo返回的句长，为字数，非字节数
int UnToANSI(int buflen,unsigned char sbuf[],unsigned char dbuf[])
{
	int i,j,flen=0;
	for(i=0;i<buflen;i++)
	{
		if(sbuf[2*i]==0x92 && sbuf[2*i+1]==0x21)
		{
			dbuf[flen]=0x0D;
			flen++;
			continue;
		}
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

//读两行
int ReadTwo(long hFile,unsigned char buf[])
{
	int a,b;
	a=RToEnd(hFile,buf);
	EatFence(hFile);
	b=RToEnd(hFile,buf);
	EatFence(hFile);

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