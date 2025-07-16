#pragma once
#include "Packet.h"

class ServerPacket : public Packet
{
public:
	ServerPacket();
	~ServerPacket();
	void SendAllConnect(char* msg);
	void SendAllMove();
	void SChatMsg(char* name);
};