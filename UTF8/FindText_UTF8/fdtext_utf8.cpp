#include "../Utils/utils.h"


// Read and export text in *.ccmes files
int ReadCCMES(char* fname);


int main() {	
	_finddata_t sc_file;
	long lsf;

	if((lsf = _findfirst("*.ccmes",&sc_file))==-1) return 0;

	if(ReadCCMES(sc_file.name)!=1) return 0;
	
	while(_findnext(lsf,&sc_file)==0)
	{
		if(ReadCCMES(sc_file.name)!=1) return 0;
	}

	_findclose(lsf);
	return 0;
}


int ReadCCMES(char* fname)
{
	unsigned char buf[512],buf2[512];
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

	// Copy and save hex header part
	_read(hMes,buf,32);
	hdlen=buf[21]*64+buf[20]/4;
	ofslen=buf[29]*256+buf[28];
	flen=(buf[9]-buf[21])*256+(buf[8]-buf[20])-32;
	_write(hIdx,buf,32);

	// Get each paragraph's sentences num
	for(i = 0; i < hdlen; i++)
	{
		_read(hMes,buf,4);
		_write(hIdx,buf,4);
		if (i < ofslen)
			paranum[i] = buf[3];
	}

	// 按段落循环，段落数来源于索引第29个字节，一段包含若干个00结尾句
	// Iterator paragraphs, paragraph num equals "ofslen"
	// each paragraph contains one or more sentence end with "0x00"
	for(i = 0; i < ofslen; i++)
	{
		// Write paragraph index number to txt
		WriteNumber(hTxt, i);
		SetFence(hTxt);

		//按句子循环，单条句子以00结尾，一段的句子数paranum[i]来源于索引偏移址后第2个字节
		for(j = 0; j < paranum[i]; j++)
		{
			buflen = ReadToZero(hMes, buf);
			n = -1;
			
			for(k = 0;k < buflen; k++)
			{
				if (buf[k] == 0x0A) {
					// Add a NextLine("↙") charactor in the sentence
					buf2[++n] = NEXT_LINE[0];
					buf2[++n] = NEXT_LINE[1];
					buf2[++n] = NEXT_LINE[2];
				} else {
					buf2[++n] = buf[k];
				}
			}

			// Write the source sentence to txt, this line is for reference
			_write(hTxt, buf2, n+1);
			SetFence(hTxt);
			// Write again, this line is for replacing translation
			_write(hTxt, buf2, n+1);
			SetFence(hTxt);
			// To next line
			Linefeed(hTxt);
		}

		// After one paragraph, divide line
		Linefeed(hTxt);
		Linefeed(hTxt);
	}

	_close(hTxt);
	_close(hIdx);
	return 1;
}