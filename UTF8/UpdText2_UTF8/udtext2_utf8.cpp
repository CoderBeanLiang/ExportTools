#include "../Utils/utils.h"


// The FindText2/UpdText2 only used for slidemes.ccb
// It should be putin the same folder with slidemes.ccb and dump.exe

// First use dump/pack(without any _de/_main etc.) to split slidemes.ccb,
// Then use FindText2/UpdText2 to compress/decompress and export/import the txt file.

int WriteCCSTX(long hsf, string fdname);
int RHdLine(long hFile,unsigned char buf[]);
int ReadTwo(long hFile,unsigned char buf[]);
int AddZero(int dt,unsigned char buf[]);
int EatZero(long hFile);
int ReplaceTxtChars(unsigned char dstBuf[], unsigned char srcBuf[], int srcBufLen);

int main()
{
	int code_num;
	string fdrname, txtname;

	_finddata_t sc_file;
	long lsf, lsf2;

	if ((lsf = _findfirst("*", &sc_file)) == -1)
		return 0;

	do {
		if (sc_file.attrib != _A_SUBDIR)
			continue;
			
		fdrname = string(sc_file.name);
		txtname = fdrname + "\\decompress\\" + fdrname + ".txt";
		if ((lsf2 = _open(txtname.c_str(), O_RDONLY|O_BINARY)) == -1)
			continue;

		if (WriteCCSTX(lsf2, fdrname) != 1)
		{
			cout<<fdrname<<" BUG"<<endl;
			continue;
		}
		cout<<fdrname<<" OK"<<endl;
		_close(lsf2);
	} while (_findnext(lsf, &sc_file) == 0);

	_findclose(lsf);
	cout<<"Finished, press any key to exit!"<<endl;
	cin.get();
	return 0;
}

//导入函数
int WriteCCSTX(long hsf, string fdname)
{
	int i,n;
	int buflen,rdlen,gtlen,newlen,edlen,flag;
	char sc_path[256];
	string sb_path,de_path,tp_path,subname,str_buf,str_del,str_odr;
	unsigned char buf[1024], buf2[1024], buf3[1024];
	long hSub,hTmp;

	_getcwd(sc_path, 256);

	//开始读txt，一个标号为一段大循环，写一次temp并压缩，标号内一句一次小循环
	while(!eof(hsf))
	{
		//Read txt content number
		buflen = ReadToCRLF(hsf, buf);
		n = ReadNumber(buf, buflen);
		// Read file name, and open the file
		EatFence(hsf);
		buflen = ReadToCRLF(hsf, buf);
		buf[buflen] = '\0';
		subname = string((char*)buf);
		sb_path = fdname + "\\" + subname;
		if ((hSub = _open(sb_path.c_str(), O_RDONLY|O_BINARY)) == -1)
		{
			cout<<"Error: Can not open sub file="<<sb_path<<endl;
			return 0;
		}
		
		de_path = fdname + "\\decompress\\" + subname;
		_read(hSub, buf, 1);
		if (buf[0]==0x11)
		{ de_path += ".lz"; flag=2; }
		else if (buf[0]==0x30)
		{ de_path += ".lf"; flag=1; }
		else if (buf[0]==0x00)
		{ de_path += ".nu"; flag=0; }
		_close(hSub);
		if ((hSub=_open(de_path.c_str(), O_RDONLY|O_BINARY)) == -1)
		{
			cout<<"Error: Can not open decompressed file="<<sb_path<<endl;
			return 0;
		}

		tp_path = "temp.bin";
		hTmp = _open(tp_path.c_str(), O_WRONLY|O_BINARY|O_CREAT,S_IREAD|S_IWRITE);
		_read(hSub, buf, 8);
		_write(hTmp, buf, 8);//Write file header's first 8 bytes

		while(!eof(hSub))
		{
			if ((rdlen = RHdLine(hSub, buf)) == 0)
				break;
			
			_lseek(hSub, rdlen, SEEK_CUR);//原文件（解压出的文件）跳过内容
			if ((gtlen = ReadTwo(hsf, buf2)) == 0)
			{
				cout<<"Error: Can not readtwo from file="<<de_path<<endl;
				return 0; // Error
			}

			// Replace "→" to 0x00, and add UTF-8 BOM chars
			gtlen = ReplaceTxtChars(buf3, buf2, gtlen);

			// Append 0x00 at end
			newlen = AddZero(gtlen, buf3);
			// Change the sentence length
			buf[10] = newlen%256;
			buf[11] = newlen/256;
			// Write the sentence header
			_write(hTmp,buf, 16);
			// Write the translated sentence
			_write(hTmp,buf3, newlen);
		}
		EatFence(hsf);// Finish the temp file

		_close(hSub);
		_close(hTmp);

		//Delete the src ccStx file
		str_buf = string((char*)sc_path); //Workspace path, where the temp.bin at
		str_del = "del " + str_buf + "\\" + sb_path;
		tp_path = str_buf + "\\" + tp_path;
		system(str_del.c_str()); //Delete the src file
		
		// Compress the temp.bin as new ccStx file
		if (flag == 0)
		{
			// No compress
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
			// Compress
			_chdir(CT_PATH);
			str_odr = "CrystalTile2/ -c ";
			if (flag == 2)
				str_odr += "-L ";
			if (flag == 1)
				str_odr += "-r ";
			str_odr = str_odr + tp_path + " -o " + str_buf + "\\" + sb_path;
			system(str_odr.c_str());
			_chdir(sc_path);
		}
		// Delete the temp.bin file
		str_del = "del " + tp_path;
		system(str_del.c_str());
	}

	return 1;
}

//Get the count of "0x00" at the end of file
int EatZero(long hFile)
{
	int len=4;
	unsigned char buf[5];
	if (_read(hFile, buf, len) == 0)
		return 0;
	
	if ((buf[0]|buf[1]|buf[2]|buf[3]) == 0x00)
		len += EatZero(hFile);
	else
	{
		_lseek(hFile, -len, SEEK_CUR);
		len = 0;
	}
	return len;
}

// Append 0x00 to the file end, to make length%4=0 and at least two 0x00
int AddZero(int dt, unsigned char buf[])
{
	int i, newlen;
	i = dt % 4;
	if (i == 3)
		newlen = dt + 5;
	else
		newlen = dt + 4 - i;

	for (i = dt; i < newlen; i++)
	{
		buf[i] = 0x00;
	}
	return newlen;
}

// Read the translated line from txt file
int ReadTwo(long hFile,unsigned char buf[])
{
	int a, b;
	EatFence(hFile);
	a = ReadToCRLF(hFile, buf);// The reference sentence
	EatFence(hFile);
	b = ReadToCRLF(hFile, buf);// The translated sentence
	return b;// Return 0 means error
	
}

// Read the sentence's start head part
int RHdLine(long hFile, unsigned char buf[])
{
	int i;
	if (_read(hFile,buf,16) < 16)
		return 0;
	
	if (buf[10]==0xFF && buf[11]==0xFF)
	{
		cout<<"one special format"<<endl;
		_lseek(hFile, -12, SEEK_CUR);
		i = RHdLine(hFile,buf);
	}
	else if (buf[6]==0xFF && buf[7]==0xFF)
	{
		i=buf[10]+buf[11]*256;
	}
	else
	{
		i=0;
	}

	return i;
}

int ReplaceTxtChars(unsigned char dstBuf[], unsigned char srcBuf[], int srcBufLen)
{
	int dstLen = 0;
	dstBuf[0] = UTF8_BOM[0];
	dstBuf[1] = UTF8_BOM[1];
	dstBuf[2] = UTF8_BOM[2];
	dstLen += 3;
	for (int i = 0; i < srcBufLen; i++)
	{
		if (srcBuf[i] == SLIDE_DOT[0] && i + 2 < srcBufLen
			 && srcBuf[i+1] == SLIDE_DOT[1] && srcBuf[i+2] == SLIDE_DOT[2])
		{
			dstBuf[dstLen] = 0x00;
			dstBuf[dstLen+1] = UTF8_BOM[0];
			dstBuf[dstLen+2] = UTF8_BOM[1];
			dstBuf[dstLen+3] = UTF8_BOM[2];
			dstLen += 4;
			i += 2;
		}
		else
		{
			dstBuf[dstLen] = srcBuf[i];
			dstLen++;
		}
		
	}
	return dstLen;
}