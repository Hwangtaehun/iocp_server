#include "ServerPacket.h"

ServerPacket::ServerPacket()
{
}

ServerPacket::~ServerPacket()
{
}

//������ ����� Ŭ���̾�Ʈ ip�� ��ο��� ����
void ServerPacket::SendAllConnect(char* msg)
{
	type = ack_con;
	sprintf_s(data, "%s\0", msg);
	size = (short)strlen(data) + 6;
}

void ServerPacket::SendAllMove()
{
	type = ack_move;
	size = (short)strlen(data) + 6;
}


void ServerPacket::SChatMsg(char* name)
{
	char temp[DATASIZE] = {};

	RecvMsg(buf);
	sprintf(temp, "[%s]%s\0", name, data);
	sprintf(data, "%s\0", temp);
	size = (short)strlen(data) + 6;
}