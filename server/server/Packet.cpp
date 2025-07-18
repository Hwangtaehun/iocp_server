#include "Packet.h"

Packet::Packet()
{
	size = 0;
	type = -1;
	endmark = 0xFF;
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 0.0f;
	memset(buf, 0, sizeof(buf));
	memset(name, 0, sizeof(name));
	memset(data, 0, sizeof(data));
}

Packet::~Packet(){}

void Packet::Separate(char* ip)
{
	int len = 0, i = 0, cnt = 0;

	for (i = 0; i <= DATASIZE; i++) {
		if (data[i] == '\0') {
			len = i;
			i = 0;
			break;
		}
	}

	while (cnt < 2) {
		switch (cnt)
		{
		case 0:
			i = Input(name, i, len);
			break;
		case 1:
			i = Input(ip, i, len);
			break;
		}

		cnt++;
	}
}

void Packet::Separate(char* ip, char* x, char* y, char* z)
{
	int len = 0, i = 0, cnt = 0;

	for (i = 0; i <= DATASIZE; i++) {
		if (data[i] == '\0') {
			len = i;
			i = 0;
			break;
		}
	}

	while (cnt < 4) {
		switch (cnt)
		{
		case 0:
			i = Input(ip, i, len);
			break;
		case 1:
			i = Input(x, i, len);
			break;
		case 2:
			i = Input(y, i, len);
			break;
		case 3:
			i = Input(z, i, len);
			break;
		}

		cnt++;
	}
}

int Packet::Input(char* value, int i, int len)
{
	int j = 0;

	for (i; i < len; i++) {
		if (data[i] == ',') {
			value[j] = '\0';
			i++;
			break;
		}
		else {
			value[j] = data[i];
		}
		j++;
	}

	if (i == len) {
		value[j] = '\0';
	}

	return i;
}

void Packet::RecvMsg(char* str) //비직렬화
{
	int len;

	memset(data, 0, sizeof(data));
	memcpy(&size, str, sizeof(size));
	memcpy(&type, str + sizeof(size), sizeof(type));
	len = (int)size - sizeof(size) - sizeof(type) - sizeof(endmark);
	memcpy(data, str + sizeof(size) + sizeof(type), len);
	data[len] = '\0';
	memcpy(&endmark, str + sizeof(size) + sizeof(type) + len, sizeof(endmark));
	memcpy(buf, str, size);
}

void Packet::SetClose(char* msg)
{
	type = req_dis;
	sprintf_s(data, "%s\0", msg);
	size = (short)strlen(data) + 6;
}

void Packet::GetData(char* temp)
{
	char ip[16], x[16], y[16], z[16], m_temp[DATASIZE];

	switch (type)
	{
	case req_con:
		Separate(ip);
		sprintf(data, "%s", ip);
		sprintf(temp, "%s, connection success", data);
		break;
	case ack_con:
		sprintf(temp, "%s, connection completed", data);
		break;
	case req_move:
		Separate(ip, x, y, z);
		m_x = stof(x);
		m_y = stof(y);
		m_z = stof(z);
		sprintf(temp, "%s, request to move %.2f, %.2f, %.2f", ip, m_x, m_y, m_z);
		break;
	case ack_move:
		Separate(ip, x, y, z);
		m_x = stof(x);
		m_y = stof(y);
		m_z = stof(z);
		sprintf(temp, "%s, move to %.2f, %.2f, %.2f", ip, m_x, m_y, m_z);
		break;
	case chat_string:
		sprintf(temp, "%s", data);
		break;
	case req_dis:
		sprintf(temp, "%s, disconnected", data);
		break;
	case -1:
	default:
		sprintf(temp, "error");
	}
}

void Packet::GetBuf(char *msg) //직렬화
{
	memset(buf, 0, sizeof(buf));
	memcpy(buf, &size, sizeof(size));
	memcpy(buf + sizeof(size), &type, sizeof(type));
	memcpy(buf + sizeof(size) + sizeof(type), data, strlen(data));
	memcpy(buf + sizeof(size) + sizeof(type) + strlen(data), &endmark, sizeof(endmark));
	memcpy(msg, &buf, size);
}

int Packet::GetSize()
{
	return (int)size;
}

int Packet::GetType()
{
	return (int)type;
}

void Packet::GetPos(float* x, float* y, float* z)
{
	*x = m_x;
	*y = m_y;
	*z = m_z;
}

char* Packet::GetName()
{
	return name;
}
