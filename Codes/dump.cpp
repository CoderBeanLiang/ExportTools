/*  基本流程：
  1.寻找当前目录下的ccb文件，找到第一个后继续，保存句柄lsf
  2.进入do.while循环，使用新句柄lsf2打开该文件，以该文件名创建目录
  3.读源文件前8个字节，后两位为文件包个数，赋值给f_num
  4.循环读取文件名（下标0-23）和长度信息（下标25、26）28字节，存入c_head[]，压缩内容头4字节存入c_head2[]
  5.使用句柄ldf创建headfile.bin存文件头
  6.进入循环：根据c_head[]确定子文件名和长度（f_len），使用句柄ldf2创建子文件，存入c_head2[]4字节并从0到f_len循环读取源文件写入子文件
  7.关闭循环内句柄，回到do.while大循环判断，当下一个findnext（*.ccb）成功（返回0）时，文件更新，回到第2步，否则结束do.while循环，关闭lsf句柄，结束程序
*/
//关于ccb文件的数据格式会在附带的txt文本内说明

#include <iostream>
#include <sstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <direct.h>

#define FILENUM 512

using namespace std;

int main() {
	int f_num,f_len,fname_len;
	int i=1,j;
	string f_name,f_name2;
	unsigned char buf[10];
	unsigned char c_head[FILENUM][29];
	unsigned char c_head2[FILENUM][5];

	_finddata_t sc_file;
	long lsf,lsf2,ldf,ldf2;

	if((lsf = _findfirst("*.ccb",&sc_file))==-1) return 0;

	cout<<"火速拆包中（@_@）请耐心等待..."<<endl;
	do {
		lsf2=_open(sc_file.name,O_RDONLY|O_BINARY);
		f_name=string(sc_file.name);
		f_name=f_name.substr(0,f_name.length()-4);
		mkdir(f_name.c_str());

		_read(lsf2,buf,8);
		f_num=int(buf[7])*16*16 + int(buf[6]);

		//读源文件文件头
		for(i=0;i<f_num;i++)
		{
			_read(lsf2,c_head[i],28);
			_read(lsf2,c_head2[i],4);
		}

		f_name2=f_name+"//headfile.bin";
		ldf=_open(f_name2.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
		_write(ldf,buf,8);

		//往headfile.bin里写28个字节，同时新建一个子文件
		for(i=0;i<f_num;i++)
		{
			_write(ldf,c_head[i],28);

			for(int k=0;k<24;k++)
			{
				if(c_head[k]==0x00) 
				{	fname_len=k;
				break;}
			}
			f_name2.assign(string((char*)c_head[i]),0,fname_len);
			f_name2=f_name+"//"+f_name2;
			ldf2=_open(f_name2.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
			_write(ldf2,c_head2[i],4);
			f_len=int(c_head[i][26])*16*16 + int(c_head[i][25]);
			for(j=0;j<f_len;j++)
			{
				_read(lsf2,buf,1);
				_write(ldf2,buf,1);
			}
			_close(ldf2);
		}

		_close(ldf);
		_close(lsf2);
	}while(_findnext(lsf,&sc_file)==0);
	_findclose(lsf);

	cout<<"拆包完成！"<<endl;
	return 0;
}
