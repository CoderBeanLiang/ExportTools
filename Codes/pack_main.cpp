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
#include <sstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <direct.h>

#define FILENUM 512
#define CT_PATH "E:\\CrystalTile2\\"

using namespace std;

int Compress(string sname,string dname,unsigned char flag);

int main()
{
    int i;
	int f_len,f_num;
	string sfname,dfname,subfname;
	unsigned char buf[10];
	unsigned char c_head[FILENUM][29];
	unsigned char c_head2[FILENUM][5];

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
		_write(ldf,buf,8);

		//读headfile文件，同时写ccb头
		f_num=int(buf[7])*16*16 + int(buf[6]);
		for(i=0;i<f_num;i++)
		{
			_read(lsf2,c_head[i],28);
			_write(ldf,c_head[i],28);
			_read(lsf2,c_head2[i],4);
			_write(ldf,c_head2[i],4);
		}
		_close(lsf2);

		//循环打开子文件，存长度，头四字节，写ccb内容，删除中间文件
		for(i=0;i<f_num;i++)
		{
			subfname = string((char*)c_head[i]);
			subfname = string(sc_file.name) + "\\" + subfname;
			Compress(subfname,"temp.bin",c_head2[i][0]);

			lsf2 = _open("temp.bin",O_RDONLY|O_BINARY);
			
			//计算文件长度并修改c_head
			f_len = filelength(lsf2) - 4;
			c_head[i][27]=f_len/(256*256);
			c_head[i][26]=f_len/256;
			c_head[i][25]=f_len%256;
			//修改c_head2
			_read(lsf2,c_head2[i],4);

			while(!eof(lsf2))
			{
				_read(lsf2,buf,1);
				_write(ldf,buf,1);
			}

			_close(lsf2);
			system("del temp.bin");
		}

		//修改ccb头部索引
		_lseek(ldf,8,SEEK_SET);
		for(i=0;i<f_num;i++)
		{
			_write(ldf,c_head[i],28);
			_write(ldf,c_head2[i],4);
		}
		_close(ldf);

	}while(_findnext(lsf,&sc_file)==0);
	_findclose(lsf);

	cout<<"打包完成！"<<endl;
    return 0;
}

//压缩
int Compress(string sname,string dname,unsigned char flag)
{
	char sc_path[256];
	long sf,df;
	long scf,nodepf;//不用解压的文件
	string spath,dpath,odr;
	unsigned char buf[5];
	unsigned char tbuf[2];
	tbuf[1] = 0x00;

	_getcwd(sc_path,256);
	spath = string(sc_path) + "\\" + sname;
	dpath = string(sc_path) + "\\" + dname;

	if(spath.substr(spath.length() - 5, 5) != "ccmes") {
		scf = _open(spath.c_str(),O_RDONLY|O_BINARY);//temp.bin
		nodepf = _open(dpath.c_str(),O_RDWR|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
		while(_read(scf,tbuf,1) > 0) {
			_write(nodepf,tbuf,1);
		}
		_close(scf);
		_close(nodepf);
		return 1;
	}

	if(flag==0x00)
	{
		sf=_open(sname.c_str(),O_RDONLY|O_BINARY);
		df=_open("temp.bin",O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
		buf[0]=buf[4]=0x00;
		buf[3]=filelength(sf)/(256*256);
		buf[2]=filelength(sf)/256;
		buf[1]=filelength(sf)%256;
		_write(df,buf,4);
		while(!eof(sf))
		{
			_read(sf,buf,1);
			_write(df,buf,1);
		}
		cout<<endl<<"一个无压缩包"<<endl;
		_close(sf);
		_close(df);
		return 1;
	}
	else if(flag==0x11) {odr = "CrystalTile2/ -c -L ";}
	else if(flag==0x30) {odr = "CrystalTile2/ -c -r ";}
	else {cout<<"no compress format"<<endl;return 0;}

	_chdir(CT_PATH);
	odr = odr + spath + " -o " + dpath;
	system(odr.c_str());
	_chdir(sc_path);

	return 1;
}