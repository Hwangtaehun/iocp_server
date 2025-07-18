#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string>
#define BUFSIZE 512    // 데이터 송수신을 위한 버퍼 크기
#define DATASIZE 506

using namespace std;

enum Type {
	req_con = 0,
	ack_con,
	req_move,
	ack_move,
	chat_string,
	req_dis
};

class Packet {
private:
	float m_x;
	float m_y;
	float m_z;

	void Separate(char* ip);
	void Separate(char* ip, char* x, char* y, char* z);
	int Input(char* value, int i, int len);
protected:
	short size;
	short type;
	short endmark;
	char name[21] = {};
	char buf[BUFSIZE + 1] = {};
	char data[DATASIZE + 1] = {};
public:
	Packet();
	~Packet();
	void RecvMsg(char* str);
	void SetClose(char* msg);
	void GetData(char* temp);
	void GetBuf(char* msg);
	int GetSize();
	int GetType();
	void GetPos(float* x, float* y, float* z);
	char* GetName();
};