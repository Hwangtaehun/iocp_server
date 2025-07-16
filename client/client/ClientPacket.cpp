#include "ClientPacket.h"

ClientPacket::ClientPacket()
{
	crt = true;
}

ClientPacket::ClientPacket(char* nickname)
{
	sprintf(name, "%s", nickname);
	crt = true;
}

ClientPacket::~ClientPacket()
{
}

void ClientPacket::MsgToTras(char* msg, float* x, float* y, float* z)
{
	char c_x[16] = {}, c_y[16] = {}, c_z[16] = {};
	int len = 0, i = 0, cnt = 0;

	for (i = 0; i <= DATASIZE; i++) {
		if (msg[i] == '\0') {
			len = i;
			i = 0;
			break;
		}
	}

	while (cnt < 3) {
		switch (cnt)
		{
		case 0:
			i = InputMsg(msg, c_x, i, len);
			break;
		case 1:
			i = InputMsg(msg, c_y, i, len);
			break;
		case 2:
			i = InputMsg(msg, c_z, i, len);
			break;
		}

		cnt++;
	}

	if (atoi(c_x) && atoi(c_y) && atoi(c_z)) {
		*x = stof(c_x);
		*y = stof(c_y);
		*z = stof(c_z);
		crt = true;
	}
	else {
		crt = false;
	}

}

int ClientPacket::InputMsg(char* msg, char* value, int i, int len)
{
	int j = 0;

	for (i; i < len; i++) {
		if (msg[i] == ',') {
			value[j] = '\0';
			i++;
			break;
		}
		else {
			value[j] = msg[i];
		}
		j++;
	}

	if (i == len) {
		value[j] = '\0';
	}

	return i;
}

//클라이언트가 서버 연결
void ClientPacket::SetConnect(char* ip)
{
	type = req_con;
	sprintf_s(data, "%s,%s\0", name, ip);
	size = (short)strlen(data) + 6;
}

void ClientPacket::SetMove(char* ip, char* msg)
{
	float x = 0, y = 0, z = 0;

	type = req_move;
	MsgToTras(msg, &x, &y, &z);
	sprintf_s(data, "%s,%.2f,%.2f,%.2f\0", ip, x, y, z);
	size = (short)strlen(data) + 6;
}

void ClientPacket::SendMsg(char* msg)
{
	type = chat_string;
	sprintf_s(data, "%s\0", msg);
	size = (short)strlen(data) + 6;
}

bool ClientPacket::Check()
{
	return crt;
}