#include <stdio.h>				// ǥ�� �����
#include <stdlib.h>				// ǥ�� ���̺귯��
#include <string.h>				// ���ڿ� ó��
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>

#define BUF_SIZE	256			// �޽��� ���� ����
#define NAME_SIZE 	256			// ���̸��� ����

unsigned WINAPI send_msg(void * arg);	// �������Լ� - �۽ſ�
unsigned WINAPI recv_msg(void * arg);	// �������Լ� - ���ſ�
void error_handling(char * msg);	// ����ó�� �Լ�

char name[NAME_SIZE] = "[DEFAULT]";	// ����� �̸� - �ʱ� ���� "[DEFALUT]"
char msg[BUF_SIZE];					// �޽��� ����

char nick[NAME_SIZE] = { 0 };
// ���� �Լ�
int main(int argc, char *argv[])	// ���ڷ� IP�� Port, ����� �̸��� �޴´�.
{
	WSADATA wsaData;
	SOCKET sock;					// ��� �� ���� ���� ��ũ����
	unsigned sndID;
	unsigned rcvID;
	SOCKADDR_IN serv_addr;	// ���� �ּ� ����ü ����
	HANDLE snd_thread, rcv_thread;	// �۽� ������, ���� ������
	void * thread_return;			// ������ ��ȯ ���ΰ�?

	if (argc != 4) {					// ����ڰ� ���α׷� �߸� ����������
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);	// ������ �ȳ��Ѵ�.
		exit(1);					//  �� �� ���α׷� ������ ����
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		error_handling("WSAStartup() error");

	strcpy(name, argv[3]);			// ������� �̸�
	sock = socket(PF_INET, SOCK_STREAM, 0);	// TCP ��� ���� ����

	memset(&serv_addr, 0, sizeof(serv_addr));		// ������ ���� �ּ� ����
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// ������ ���� �õ�
	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		error_handling("connect() error");

	//����� �̸� ����
	//write(sock, name, BUF_SIZE);
	send(sock, name, BUF_SIZE, 0);

	snd_thread=(HANDLE)_beginthreadex(NULL, 0, send_msg, (void*)&sock, 0, &sndID);
	rcv_thread = (HANDLE)_beginthreadex(NULL, 0, recv_msg, (void*)&sock, 0, &rcvID);
	WaitForSingleObject(snd_thread, INFINITE);
	WaitForSingleObject(rcv_thread, INFINITE);
	close(sock);  // ���α׷� ������ �� ���� �Ҹ� ��Ű�� ����
	return 0;
}

unsigned WINAPI send_msg(void * arg)   // send thread main
{
	SOCKET sock = *((SOCKET*)arg);		// ������ �޴´�.
	char name_msg[BUF_SIZE];

	while (1) // ���ѷ��� ���鼭
	{
		fgets(msg, BUF_SIZE, stdin);	// Ű���� �Է��� �޾Ƽ�
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) // q�� Q�� �����ϰ�
		{
			close(sock);
			exit(0);
		}
		// �������� �Է��� �ϸ�
		sprintf(name_msg, "[%s] %s", name, msg);	// �̸� + ���� + �޽��� ������ ����
		send(sock, name_msg, BUF_SIZE, 0);		// ������ �޽��� ������
	}
	return NULL;
}

// ������ ������ �Լ�
unsigned WINAPI recv_msg(void * arg)   // read thread main
{
	SOCKET sock = *((SOCKET*)arg);		// ��� �� ������ �ް�
	char name_msg[BUF_SIZE];	// �̸� + �޽��� ����
	int str_len;				// ���ڿ� ����

	while (1)	// ���ѷ��� ���鼭
	{
		str_len = recv(sock, name_msg, BUF_SIZE, 0); // �޽����� ������
		if (str_len == -1) { // ���� ����� ��������
			return (void*)-1;	// ������ ����
		}
		else if (!strcmp(name_msg, "q\n") || !strcmp(name_msg, "Q\n")) {
			close(sock);
			exit(0);
		}

		fputs(name_msg, stdout);	// ȭ�鿡 ���ŵ� �޽��� ǥ��
	}
	return NULL;
}

// ���� ó�� �Լ�
void error_handling(char *msg)
{
	fputs(msg, stderr);		// ���� �޽����� ǥ���ϰ�
	fputc('\n', stderr);
	exit(1);				// ������ ���� ó��
}

