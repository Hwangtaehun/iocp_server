#pragma once
#include "Packet.h"

class ClientPacket : public Packet
{
private:
	bool crt;

	int InputMsg(char* msg, char* value, int i, int len);
	void MsgToTras(char* msg, float* x, float* y, float* z);
public:
	ClientPacket();
	ClientPacket(char* nickname);
	~ClientPacket();
	void SetConnect(char* ip);
	void SetMove(char* ip, char* msg);
	void SendMsg(char* msg);
	bool Check();
};