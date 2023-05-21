#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <direct.h>

using namespace std;

// PS: The game(USA) font table uses Unicode(UTF-8)

// 0x0A means "/n" in game and exported as "↙"(0xE28699) in txt
// So if one translation sentence is too long then you can just insert a "↙"
extern unsigned char NEXT_LINE[4];
extern unsigned char FENCE[4];

// This is the linefeed in txt files instead of in game
extern unsigned char CRLF[3];


// Read hex file until meet "0x00"
// Return: valid byte length of buf content
int ReadToZero(long hexFd, unsigned char buf[]);

// Read txt file until meet "0x0D0A"(CRLF)
int ReadToCRLF(long txtFD, unsigned char buf[]);

// Write a CRLF in the txt file
void Linefeed(long txtFD);

// Write format "----------------" in the txt file
void SetFence(long txtFD);

// Read and skip "----------------" or CRLF in the txt file
int EatFence(long txtFD);

void WriteNumber(long txtFD, int number);

int ReadNumber(unsigned char buf[], int readBufLen);
