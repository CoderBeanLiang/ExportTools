/*  基本流程：
  1.寻找data（_findfirst），找不到程序结束，找到第一个后继续
  2.进入do.while循环，如果不是文件夹属性，跳过本次循环接着寻找（_findnext），否则继续本次循环
  3.以文件夹名作路径，打开路径下的headfile.bin文件，打开失败说明该文件夹不是要打包的文件夹，跳过本次循环（_findnext），否则使用句柄lsf2打开
  4.以文件夹名作ccb文件名，使用ldf句柄创建目的文件，读headfile.bin头8字节入buf，取7、8字节计算子文件个数存入f_num
  5.循环读取headfile.bin文件，分f_num次，每次读28字节，存入c_head[]（前24字节文件名，后4字节第2、3字节存放长度信息，注意下标是25、26，且低位在前）
  6.循环读子文件名（c_head[]）去空字符存入文件名数组subfname[]，使用句柄lsf2打开子文件，取头4字节存入c_head2[]并得到文件长度-4存入f_len[]，同时修改c_head[]长度信息
  7.写入ccb头8个字节（4步骤里的buf），循环写ccb头文件：一次循环写c_head[]中28字节加c_head2[]中4字节
  8.使用句柄lsf2打开子文件，从第5字节开始循环读取写入ccb
  9.关闭循环内句柄等，回到do.while循环判断，当下一个data寻找成功时（返回值为0）文件更新，进入步骤2，否则关闭lsf句柄，退出程序
*/
//关于ccb文件的数据格式会在附带的txt文本内说明

#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <direct.h>
#define FILENUM 512

using namespace std;

int main()
{
    int f_num,fname_len;
    int i,j;
	int f_len[FILENUM];
	string sfname,dfname;
	unsigned char buf[10];
	unsigned char c_head[FILENUM][29];
	unsigned char c_head2[FILENUM][5];
	string subfname[FILENUM];

    _finddata_t sc_file;
	long lsf,lsf2,ldf;

	if((lsf = _findfirst("*",&sc_file))==-1) return 0;

	cout<<"火速打包中（@_@）请耐心等待..."<<endl;
	do{
		if(sc_file.attrib != _A_SUBDIR) continue;
		sfname=string(sc_file.name);
		sfname += "//headfile.bin";
		if((lsf2 = _open(sfname.c_str(),O_RDONLY|O_BINARY))==-1) continue;

		dfname=string(sc_file.name);
		dfname += ".ccb";
		ldf=_open(dfname.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);

		_read(lsf2,buf,8);

		//读headfile文件
		f_num=int(buf[7])*16*16 + int(buf[6]);
		for(i=0;i<f_num;i++)
		{
			_read(lsf2,c_head[i],28);
		}
		_close(lsf2);

		//循环打开子文件，存文件名，取头4字节存储，计算长度等
		for(i=0;i<f_num;i++)
		{
			for(int k=0;k<24;k++)
			{
				if(c_head[k]==0x00)
				{	fname_len=k;
				break;}
			}
			subfname[i] = string(sc_file.name);
			subfname[i] += "//";
			subfname[i].append(string((char*)c_head[i]),0,fname_len);
			lsf2 = _open(subfname[i].c_str(),O_RDONLY|O_BINARY);
			_read(lsf2,c_head2[i],4);
			//计算文件长度并修改c_head
			f_len[i] = filelength(lsf2) - 4;
			c_head[i][27]=f_len[i]/(256*256);
			c_head[i][26]=f_len[i]/256;
			c_head[i][25]=f_len[i]%256;
			_close(lsf2);
		}

		//写ccb文件头，文件列表
		_write(ldf,buf,8);
		for(i=0;i<f_num;i++)
		{
			_write(ldf,c_head[i],28);
			_write(ldf,c_head2[i],4);
		}

		//循环读取子文件，从首部推后4字节，加入ccb文件
		for(i=0;i<f_num;i++)
		{
			lsf2 = _open(subfname[i].c_str(),O_RDONLY|O_BINARY);
			_lseek(lsf2,4,SEEK_CUR);
			for(j=0;j<f_len[i];j++)
			{
				_read(lsf2,buf,1);
				_write(ldf,buf,1);
			}
			_close(lsf2);
		}
		_close(ldf);

	}while(_findnext(lsf,&sc_file)==0);
	_findclose(lsf);

	cout<<"打包完成！"<<endl;
    return 0;
}
