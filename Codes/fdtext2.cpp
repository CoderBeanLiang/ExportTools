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
int ReadStx(long hsf,string hfname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum);
int main() {
	int i=0,code_num;
	string hdfname;
	unsigned char buf='\0';
	unsigned char ch[TABLENUM][3];
	unsigned char ch2[TABLENUM][3];
	
	_finddata_t sc_file;
	long lsf,lsf2;

	if((lsf = _findfirst("*.tbl",&sc_file))==-1) {cout<<"no tbl"<<endl;return 0;}

	if((code_num=OpenTbl(sc_file.name,ch,ch2))==0) {cout<<"tbl error"<<endl;return 0;}

	if((lsf = _findfirst("*",&sc_file))==-1) return 0;
	
	do{
		if(sc_file.attrib != _A_SUBDIR) continue;
			
		hdfname=string(sc_file.name);
		hdfname += "\\headfile.bin";
		if((lsf2 = _open(hdfname.c_str(),O_RDONLY|O_BINARY))==-1) continue;

		hdfname=hdfname.substr(0,hdfname.length()-13);
		if(ReadStx(lsf2,hdfname,ch,ch2,code_num)!=1)
		{
			cout<<hdfname<<" BUG"<<endl;
			continue;
		}
		cout<<hdfname<<" OK"<<endl;
		_close(lsf2);
	}while(_findnext(lsf,&sc_file)==0);

	cout<<"导出完毕"<<endl;
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
int RHdLine(long hFile,unsigned char buf[])
{
	int i;
	_read(hFile,buf,16);
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
void SetFence(long hFile,unsigned char fence[],unsigned char ctrl[])
{
	_write(hFile,ctrl,4);
	for(int i=0;i<16;i++)
		_write(hFile,fence,2);
	_write(hFile,ctrl,4);
}
int ReadStx(long hsf,string hfname,unsigned char arr_code[][3],unsigned char arr_char[][3],int cdnum)
{
	int i=0,j=0,k,m,n,l;
	int fnum,stlen,nmlen;
	char sc_path[256];
	string ds_path,tx_path,wk_path,sf_name,str_odr,str_buf;
	unsigned char subname[25];
	unsigned char buf[1024],buf2[1024];
	unsigned char ctrl[5];
	unsigned char fence[3]={0x0D,0xFF};
	long hTxt,hSub;

	wk_path = "decompress";
	wk_path = hfname + "\\" + wk_path;
	mkdir(wk_path.c_str());
	_getcwd(sc_path,256);

	_lseek(hsf,6,SEEK_SET);
	_read(hsf,buf,2);
	fnum = buf[0] + buf[1]*256;

	//不导出可删除
	tx_path = wk_path + "\\" + hfname + ".txt";
	hTxt=_open(tx_path.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
	ctrl[0]=0xFF;
	ctrl[1]=0xFE;
	ctrl[2]=0x0A;
	ctrl[3]=0x00;
	ctrl[4]='\0';
	_write(hTxt,ctrl,2);
	ctrl[0]=0x0D;
	ctrl[1]=0x00;

	while(i<fnum) {
		_read(hsf,subname,24);
		_read(hsf,buf,4);
		if((buf[2]*256+buf[1]) == 0x08) continue;

		sf_name = string((char*)subname);
		str_buf = hfname + "\\" + sf_name;
		if((hSub = _open(str_buf.c_str(),O_RDONLY|O_BINARY)) == -1) {cout<<subname<<" BUG"<<endl;return 0;}
		ds_path = string(sc_path) + "\\" + wk_path + "\\" + sf_name;
		str_buf = string(sc_path) + "\\" + hfname + "\\" + sf_name;//str_buf暂时用作<src>

		_read(hSub,buf,1);
		if(buf[0]==0x30) 
			ds_path += ".lf";
		else if(buf[0]==0x11)
			ds_path += ".lz";
		else if(buf[0]==0x00)
			ds_path += ".nu";
		else
			cout<<subname<<":no such format"<<endl;
		_close(hSub);
		
		_chdir("E:\\CrystalTile2\\");
		str_odr = "CrystalTile2/ -d " + str_buf + " -o " + ds_path;
		system(str_odr.c_str());
		_chdir(sc_path);
 
		if((hSub = _open(ds_path.c_str(),O_RDONLY|O_BINARY)) == -1) {cout<<subname<<"decompress BUG"<<endl;return 0;}
		if(j<100){
			buf[1]=0x00;buf[3]=0x00;buf[4]='\0';
			buf[0]=0x30 + j/10;
			buf[2]=0x30 + j%10;
			_write(hTxt,buf,4);
		}
		else{
			buf[1]=0x00;buf[3]=0x00;buf[5]=0x00;buf[6]='\0';
			buf[0]=0x30 + j/100;
			buf[2]=0x30 + (j%100)/10;
			buf[4]=0x30 + j%10;
			_write(hTxt,buf,6);
		}j++;
		_write(hTxt,ctrl,4);
		l=0;
		while(subname[l]!=0x00)
		{
			buf[0]=subname[l];
			buf[1]=0x00;
			_write(hTxt,buf,2);
			l++;
		}
		SetFence(hTxt,fence,ctrl);
		_lseek(hSub,8,SEEK_SET);

		while(!eof(hSub))
		{
			n=-1;
			if((stlen=RHdLine(hSub,buf))==0) break;
			_read(hSub,buf,stlen);
			for(k=0;k<stlen;k++)
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
				}else if(buf[k]==0x00) {
					if(buf[k+1]==0x00) break;
					buf2[++n]=0x92;buf2[++n]=0x21;    //箭头标注
				}else {
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
		_close(hSub);
		i++;
	}

	_close(hTxt);
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