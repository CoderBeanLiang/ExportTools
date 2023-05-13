#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>

using namespace std;
#define TABLENUM 8000

int main() 
{
	int i;
	int j,k=0;
	int flen;
	unsigned char buf[3];
	unsigned char ch[TABLENUM][3];
	string chrf="outchar.txt";
	_finddata_t sc_file;
	long lsf,lsf2,ldf;

	if((lsf = _findfirst("*.txt",&sc_file))==-1) return 0;

	do{
		i=0;
		if((lsf2 = _open(sc_file.name,O_RDONLY|O_BINARY)) == -1)
		{
			cout<<"txt open bug"<<endl;return 0;
		}
		_lseek(lsf2,2,SEEK_SET);
		flen=filelength(lsf2)/2 - 1;
		while(i<flen)
		{
			_read(lsf2,buf,2);
			i++;
			if(buf[1]<0x4E || buf[1]>0x9F) continue;
			for(j=0;j<k;j++)
			{
				if(buf[0]==ch[j][0] && buf[1]==ch[j][1]) break;
			}

			if(j==k)
			{
				ch[k][0]=buf[0];
				ch[k][1]=buf[1];
				k++;		
			}		
		}
		_close(lsf2);
	}while(_findnext(lsf,&sc_file)==0);
	_findclose(lsf);
	
	if((ldf = _open(chrf.c_str(),O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE)) == -1)
	{
		cout<<"out file err"<<endl;return 0;
	}
	buf[0]=0xFF;
	buf[1]=0xFE;
	buf[2]='\0';

	_write(ldf,buf,2);
	for(j=0;j<k;j++)
	{
		_write(ldf,ch[j],2);
	}
	_close(ldf);
	return 0;
}

	
