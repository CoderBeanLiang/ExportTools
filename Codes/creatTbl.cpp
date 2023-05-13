#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

using namespace std;

int main() 
{
	int i=0;
	int j,k=0;
	int flen;
	unsigned char buf[3];
	unsigned char ch[5000][3];
	char chrf[6]="*.tbl";
	_finddata_t sc_file;
	long lsf,lsf2,ldf;

	if((lsf = _findfirst("*.txt",&sc_file))==-1) return 0;

	if((lsf2 = _open(sc_file.name,O_RDONLY|O_BINARY)) == -1) {cout<<"txt open bug"<<endl;return 0;}

	if((ldf = _findfirst("*.tbl",&sc_file))==-1) return 0;

	if((ldf= _open(sc_file.name,O_WRONLY|O_BINARY)) == -1) {cout<<"tbl open bug"<<endl;return 0;}

	buf[0]=0xFF;
	buf[1]=0xFE;
	buf[2]='\0';

	_lseek(lsf2,2,SEEK_SET);
	_lseek(ldf,9740,SEEK_SET);

	flen = filelength(lsf2)/2 -1;
	for(i=0;i<flen;i++)
	{
		_read(lsf2,buf,2);
		_write(ldf,buf,2);
		_lseek(ldf,14,SEEK_CUR);
	}

	cout<<i<<endl;
	cout<<(flen+1)*2<<endl;
	return 0;
}