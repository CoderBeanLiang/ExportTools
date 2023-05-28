#include "../Utils/utils.h"


// The FindText2/UpdText2 only used for slidemes.ccb
// It should be putin the same folder with slidemes.ccb and dump.exe

// First use dump/pack(without any _de/_main etc.) to split slidemes.ccb,
// Then use FindText2/UpdText2 to compress/decompress and export/import the txt file.

int ReadCCSTX(long hsf, string hfname);

int main() {
	string hdfname;
	_finddata_t sc_file;
	long lsf, lsf2;

	if ((lsf = _findfirst("*", &sc_file)) == -1)
		return 0;
	
	do {
		if (sc_file.attrib != _A_SUBDIR)
			continue;
			
		hdfname = string(sc_file.name);
		hdfname += "\\headfile.bin";
		if ((lsf2 = _open(hdfname.c_str(), O_RDONLY|O_BINARY)) == -1)
			continue;

		hdfname = hdfname.substr(0, hdfname.length() - 13);
		if (ReadCCSTX(lsf2, hdfname) != 1)
		{
			cout<<hdfname<<" BUG"<<endl;
			continue;
		}
		cout<<hdfname<<" OK"<<endl;
		_close(lsf2);
	} while (_findnext(lsf, &sc_file) == 0);

	_findclose(lsf);
	cout<<"Finished, press any key to exit!"<<endl;
	cin.get();
	return 0;
}

int RHdLine(long hFile,unsigned char buf[])
{
	int i = 0;
	int readLen = _read(hFile, buf, 16);
	if (readLen < 16)
	{
		return 0;//EOF
	}

	if (buf[10] == 0xFF && buf[11] == 0xFF)
	{
		cout<<"one special format"<<endl;
		_lseek(hFile, -12, SEEK_CUR);
		i = RHdLine(hFile, buf);
	}
	else if (buf[6]==0xFF && buf[7]==0xFF)
	{
		i = buf[10] + buf[11]*256;
	}

	return i;
}

int ReadCCSTX(long hsf, string hfname)
{
	int i=0,j=0,k,m,n,l;
	int fnum,stlen,nmlen;
	char sc_path[256];
	string ds_path,tx_path,wk_path,sf_name,str_odr,str_buf;
	unsigned char subname[25];
	unsigned char buf[1024],buf2[1024];
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

	while(i < fnum) {
		_read(hsf,subname,24);
		_read(hsf,buf,4);
		if((buf[2]*256+buf[1]) == 0x08)
		{
			i++;
			continue;
		}

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
		
		_chdir(CT_PATH);
		str_odr = "CrystalTile2/ -d " + str_buf + " -o " + ds_path;
		system(str_odr.c_str());
		_chdir(sc_path);
 
		if((hSub = _open(ds_path.c_str(),O_RDONLY|O_BINARY)) == -1)
		{
			cout<<subname<<"decompress BUG"<<endl;
			return 0;
		}

		// Write Number
		WriteNumber(hTxt, j);
		j++;

		// Write sub filename
		Linefeed(hTxt);
		l = 0;
		while(subname[l] != 0x00)
		{
			buf[0] = subname[l];
			_write(hTxt, buf, 1);
			l++;
		}
		SetFence(hTxt);

		// Skip file head
		_lseek(hSub, 8, SEEK_SET);
		// Read content in sub file
		while(!eof(hSub))
		{
			n=-1;
			if((stlen = RHdLine(hSub, buf)) == 0)
				break;
			
			_read(hSub, buf, stlen);
			for(k = 0; k < stlen; k++)
			{
				if (buf[k] == 0x0A)
				{
					// Add a NextLine("↙") charactor in the sentence
					buf2[++n] = NEXT_LINE[0];
					buf2[++n] = NEXT_LINE[1];
					buf2[++n] = NEXT_LINE[2];
				}
				else if (buf[k] == 0x00)
				{
					if (k+1 < stlen && buf[k+1] == 0x00)
					{
						break;
					}
					else
					{
						// Add a "→" to split the speaker and his words
						buf2[++n] = SLIDE_DOT[0];
						buf2[++n] = SLIDE_DOT[1];
						buf2[++n] = SLIDE_DOT[2];
					}
				}
				else if (buf[k] == UTF8_BOM[0] && k+2 < stlen && buf[k+1] == UTF8_BOM[1] && buf[k+2] == UTF8_BOM[2])
				{
					k += 2;// Skip
				}
				else
				{
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

		// After one file, divide line
		Linefeed(hTxt);
		Linefeed(hTxt);

		_close(hSub);
		i++;
	}

	_close(hTxt);
	return 1;
}