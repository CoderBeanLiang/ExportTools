#include "utils.h"


// 0x0A means "/n" in game and exported as "↙"(0xE28699) in txt
// So if one translation sentence is too long then you can just insert a "↙"
unsigned char NEXT_LINE[4] = {0xE2, 0x86, 0x99, '\0'};
unsigned char FENCE[4] = {0xEF, 0xBC, 0x8D, '\0'};

// This is the linefeed in txt files instead of in game
unsigned char CRLF[3] = {0x0D, 0x0A, '\0'};


// Check if next "checkLen" chars are total matched
bool IsAfterMatch(long hFile, unsigned char ref[], int checkLen)
{
	unsigned char uc[4];
	int readLen = _read(hFile, uc, checkLen);
	if (readLen == checkLen)
	{
		bool isMatched = true;
		for (int i = 0; i < checkLen; i++)
		{
			if (uc[i] != ref[i+1])
			{
				isMatched = false;
			}
		}
		
		if (isMatched)
		{
			return true;
		}
	}

	if (readLen > 0)
	{
		_lseek(hFile, -readLen, SEEK_CUR);
	}
	return false;
}

// Remember to trans "↙" to 0x0A
int ReadToCRLF(long hFile, unsigned char buf[])
{
	unsigned char uc[3];
	
	int i = 0;
	while (!eof(hFile))
	{
		int readLen = _read(hFile, uc, 1);
		if (readLen <= 0)
		{
			return i;
		}

		if (uc[0] == NEXT_LINE[0])
		{
			if (IsAfterMatch(hFile, NEXT_LINE, 2))
			{
				buf[i] = 0x0A;
				i++;
			}
			else
			{
				buf[i] = uc[0];
				i++;
			}
		}
		else if (uc[0] == CRLF[0])
		{
			if (IsAfterMatch(hFile, CRLF, 1))
			{
				return i;
			}
			else
			{
				buf[i] = uc[0];
				i++;
			}
		}
		else
		{
			buf[i] = uc[0];
			i++;
		}
	}
	return i;
}

int ReadToZero(long hFile,unsigned char buf[])
{
	unsigned char uc[2];
	int i = 0;
	while (true)
	{
		int readLen = _read(hFile,uc,1);
		if (readLen > 0 && uc[0] != 0x00)
		{
			buf[i] = uc[0];
			i++;
		}
		else
		{
			break;
		}
	}
	return i;
}

// Move to next line in txt
void Linefeed(long txtFileDesp)
{
	_write(txtFileDesp, CRLF, 2);
}

// Set one line txt format "----------------"
void SetFence(long txtFileDesp)
{
	Linefeed(txtFileDesp);
	for(int i = 0; i < 16; i++)
		_write(txtFileDesp, FENCE, 3);
	Linefeed(txtFileDesp);
}

// Skip until not fence or linefeed
int EatFence(long hFile)
{
	unsigned char uc[3];
	while (!eof(hFile))
	{
		int readLen = _read(hFile, uc, 1);
		if (readLen <= 0)
		{
			return 0;
		}
		
		if(uc[0] == CRLF[0])
		{
			if (IsAfterMatch(hFile, CRLF, 1))
			{
				continue;
			}
			else
			{
				_lseek(hFile, -1, SEEK_CUR);
				break;
			}
		}
		else if (uc[0] == FENCE[0])
		{
			if (IsAfterMatch(hFile, FENCE, 2))
			{
				continue;
			}
			else
			{
				_lseek(hFile, -1, SEEK_CUR);
				break;
			}
		}
		else
		{
			_lseek(hFile, -1, SEEK_CUR);
			break;
		}
	}
	return 0;
}

void WriteNumber(long txtFileDesp, int number)
{
    unsigned char buf[5];
    if (number < 100)
    {
        buf[0] = 0x30 + number/10;
        buf[1] = 0x30 + number%10;
        buf[2] = '\0';
        _write(txtFileDesp, buf, 2);
    }
    else
    {
        buf[0] = 0x30 + number/100;
        buf[1] = 0x30 + (number%100)/10;
        buf[2] = 0x30 + number%10;
        buf[3] = '\0';
        _write(txtFileDesp, buf, 3);
    }
}

int ReadNumber(unsigned char buf[], int readBufLen)
{
	if (readBufLen == 3) {
		return (buf[0]-0x30)*100 + (buf[1]-0x30)*10 + (buf[2]-0x30);
	} else {
		return (buf[0]-0x30)*10 + (buf[1]-0x30);
	}
}
