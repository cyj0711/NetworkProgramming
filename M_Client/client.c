#include <stdio.h>				// ǥ�� �����
#include <stdlib.h>				// ǥ�� ���̺귯��
#include <unistd.h>				// ���н� ǥ��
#include <string.h>				// ���ڿ� ó��
#include <arpa/inet.h>			// ���ͳ� ��������
#include <sys/socket.h>			// ���� �Լ�
#include <pthread.h>			// ������ �Լ�

#define BUF_SIZE	256			// �޽��� ���� ����
#define NAME_SIZE 	256			// ���̸��� ����

void * send_msg(void * arg);	// �������Լ� - �۽ſ�
void * recv_msg(void * arg);	// �������Լ� - ���ſ�
void error_handling(char * msg);	// ����ó�� �Լ�

char name[NAME_SIZE] = "[DEFAULT]";	// ����� �̸� - �ʱ� ���� "[DEFALUT]"
char msg[BUF_SIZE];					// �޽��� ����

char nick[NAME_SIZE] = { 0 };
// ���� �Լ�
int main(int argc, char *argv[])	// ���ڷ� IP�� Port, ����� �̸��� �޴´�.
{
	int sock;						// ��� �� ���� ���� ��ũ����
	struct sockaddr_in serv_addr;	// ���� �ּ� ����ü ����
	pthread_t snd_thread, rcv_thread;	// �۽� ������, ���� ������
	void * thread_return;			// ������ ��ȯ ���ΰ�?

	if (argc != 4) {					// ����ڰ� ���α׷� �߸� ����������
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);	// ������ �ȳ��Ѵ�.
		exit(1);					//  �� �� ���α׷� ������ ����
	}


	strcpy(name, argv[3]);			// ������� �̸�
	sock = socket(PF_INET, SOCK_STREAM, 0);	// TCP ��� ���� ����

	memset(&serv_addr, 0, sizeof(serv_addr));		// ������ ���� �ּ� ����
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// ������ ���� �õ�
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	//����� �̸� ����
	write(sock, name, BUF_SIZE);

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);	// �۽� ������ ����
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);	// ���� ������ ����
	pthread_join(snd_thread, &thread_return);	// �����Լ��� �ƹ� �͵� ���ϰ� ��ٸ���.
	pthread_join(rcv_thread, &thread_return);	// ����� 2���� �����尡 ó���Ѵ�.
	close(sock);  // ���α׷� ������ �� ���� �Ҹ� ��Ű�� ����
	return 0;
}

void * send_msg(void * arg)   // send thread main
{
	int sock = *((int*)arg);		// ������ �޴´�.
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
		write(sock, name_msg, BUF_SIZE);		// ������ �޽��� ������
	}
	return NULL;
}

// ������ ������ �Լ�
void * recv_msg(void * arg)   // read thread main
{
	int sock = *((int*)arg);		// ��� �� ������ �ް�
	char name_msg[BUF_SIZE];	// �̸� + �޽��� ����
	int str_len;				// ���ڿ� ����

	while (1)	// ���ѷ��� ���鼭
	{
		str_len = read(sock, name_msg, BUF_SIZE); // �޽����� ������
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

