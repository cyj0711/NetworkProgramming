#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>			// ǥ�� ����� :wq
#include <stdlib.h>			// ǥ�� ���̺귯��
#include <string.h>			// ���ڿ� ó��	
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>

#define BUF_SIZE 256			// ä���� �� �޽��� �ִ� ����
#define MAX_CLNT 256			// �ִ� ���� ������ ��
#define MAX_ROOM 256			// �ִ� ���� ������ ���� ����
#define ROOM_ID_DEFAULT		-1	// ���� �ʱ� ID ��(���� ����Ʈ�� �����ǰ� ID�� ������.)

typedef int BOOL;			// boolean Ÿ�� ����(��, �ƴϿ� �Ǵ� ���)
#define TRUE	1			// boolean ��(��)
#define FALSE	0			// boolean ����(�ƴϿ�)

							// �Լ��� ����

HANDLE mutx;		// ��ȣ������ ���� ��������

							// ������ ������ Ŭ���̾�Ʈ ����ü
struct Client
{
	int roomId;				// ���� ��ȣ
	SOCKET socket;			// custom: ���� ���ϵ�ũ���ʹ� �����ϹǷ�, Ŭ���̾�Ʈ ID�� Ȱ���� 
	char name[BUF_SIZE];
};
typedef struct Client Client;	// struct Client�� �׳� Client�� ���� �ְ� ��

int sizeClient = 0;				// �������� ����� ��(arrClient �迭�� size)
Client arrClient[MAX_CLNT];		// �������� ����� ����ü���� �迭

struct Room					// ä�ù� ����ü ����
{
	int id;					// ���� ��ȣ
	char name[BUF_SIZE];	// ���� �̸�
	char notice[BUF_SIZE];
};
typedef struct Room Room;

int sizeRoom = 0;				// arrRoom �迭�� size
Room arrRoom[MAX_ROOM];		// Room�� �迭(���� ������ ���� �迭)

int issuedId = 0;			// �߱޵� ID

unsigned WINAPI handle_serv(void* arg); // ���� ������� �Լ�
unsigned WINAPI handle_clnt(void* arg);		// Ŭ���̾�Ʈ ������� �Լ�(�Լ� ������)
void error_handling(char* msg);	// ���� ó�� �Լ�
void sendMessageUser(char* msg, SOCKET socket);
BOOL isEmptyRoom(int roomId);
void printHowToUse(Client* client);
void printWaitingRoomMenu(Client* client);
void removeClient(SOCKET socket);
void serveRoomMenu(char* menu, Client* client, char* msg);
void setNotice(Client* client, char* notice);
void sendMessageRoom(char* msg, int roomId);
void exitRoom(Client* client);
void listMember(Client* client, int roomId);
void printRoomMenu(Client* client);
void printWaitingRoomMenu(Client* client);

// ��ο��� �޽����� �����°� �ƴ϶�, Ư�� ����ڿ��Ը� �޽����� ������.
void sendMessageUser(char* msg, SOCKET socket)   // send to a members 
{
	//int length = write(socket, msg, BUF_SIZE);
	int length = send(socket, msg, BUF_SIZE, 0);
}

// Ŭ���̾�Ʈ�� �迭�� �߰� - ������ �ָ� Ŭ���̾�Ʈ ����ü������ ������ �ش�.
Client *addClient(SOCKET socket, char *nick) //custom: ���� �Ű�����Ÿ���� SOCKET���� �ٲ�
{
	WaitForSingleObject(mutx,INFINITE);		// �Ӱ迵�� ����
	Client *client = &(arrClient[sizeClient++]);	// �̸� �Ҵ�� ���� ȹ��
	client->roomId = ROOM_ID_DEFAULT;					// �ƹ��濡�� ������� ����
	client->socket = socket;						// ���ڷ� ���� ���� ����
	strcpy(client->name, nick);
	ReleaseMutex(mutx);	// �Ӱ迵�� ��
	return client;	// Ŭ���̾�Ʈ ����ü ���� ��ȯ
}

// Ŭ���̾�Ʈ�� �迭���� ���� - ������ �ָ� Ŭ���̾�Ʈ�� �迭���� �����Ѵ�.
void removeClient(SOCKET socket)
{
	WaitForSingleObject(mutx, INFINITE);		// �Ӱ迵�� ����
	int i = 0;
	for (i = 0; i < sizeClient; i++)   // ������ ���� Ŭ���̾�Ʈ�� �����Ѵ�.
	{
		if (socket == arrClient[i].socket)	// ���� Ŭ���̾�Ʈ�� ã������
		{
			printf("User(%d) quit the program.\n", (int)socket);
			while (i++ < sizeClient - 1)	// ã�� ���ϵڷ� ��� ���Ͽ� ����
				arrClient[i] = arrClient[i + 1]; // ��ĭ�� ������ ���
			break;	// for�� Ż��
		}
	}
	sizeClient--;	// �������� Ŭ���̾�Ʈ �� 1 ����
	ReleaseMutex(mutx);	// �Ӱ迵�� ��
}

// �濡�� �޴��� �������� �� ������ ����(������ �ؾ� �� ��)
void serveRoomMenu(char* menu, Client* client, char* msg)
{
	char buf[BUF_SIZE] = "";
	char notice[BUF_SIZE] = "";
	char name[BUF_SIZE] = "";

	printf("Server Menu : %s\n", menu);   // ���� �ʿ� ������ �޽���(������)

	if (strcmp(menu, "@exit") == 0) {         // exit - �� ������
		sprintf(buf, "\t\t** %s out **\n", client->name);
		sendMessageRoom(buf, client->roomId);
		exitRoom(client);
	}
	else if (strcmp(menu, "@list") == 0) {         // list - ���� �濡 �ִ� ��� ��� ǥ��
		listMember(client, client->roomId);
	}
	else if (strcmp(menu, "@help") == 0) {            // help - ���� �޴� ǥ��
		printRoomMenu(client);
	}

	else if (strcmp(menu, "@setn") == 0) {
		recv(client->socket, buf, sizeof(buf), 0);
		setNotice(client, buf);
	}

	else if (strcmp(menu, "@getn") == 0) {
		sendMessageUser(arrRoom[client->roomId].notice, client->socket);
	}

	else    // send normal chat message
		sendMessageRoom(msg, client->roomId);   // �Ϲ� ä�� �޽���
}

void setNotice(Client* client, char* notice)
{
	int index = client->roomId;
	strcpy(arrRoom[index].notice, notice);

	return;
}



unsigned WINAPI handle_serv(void * arg)
{
	Client *client = (Client *)arg;
	SOCKET serv_sock = client->socket; //custom
	char srcv_msg[BUF_SIZE] = "";
	int i = 0;

	while (1)
	{
		fgets(srcv_msg, BUF_SIZE, stdin);
		if (!strcmp(srcv_msg, "q\n") || !strcmp(srcv_msg, "Q\n"))
		{
			for (i = 0; i < sizeClient; i++)
			{
				sendMessageUser(srcv_msg, arrClient[i].socket);

				closesocket(arrClient[i].socket);
			}
			exit(0);
		}

	}
	return NULL;
}



// Ư�� �� �ȿ� �ִ� ��� ������� �޽��� ������
void sendMessageRoom(char *msg, int roomId)   // send to the same room members 
{
	int i;

	WaitForSingleObject(mutx, INFINITE);		// �Ӱ� ���� ����
	for (i = 0; i < sizeClient; i++)		// ��� ����ڵ� �߿���
	{
		if (arrClient[i].roomId == roomId)	// Ư�� ���� ����鿡��
			sendMessageUser(msg, arrClient[i].socket); // ���� �޽�������
	}
	ReleaseMutex(mutx);	// �Ӱ� ���� ��
}

// Ư�� ����ڰ� �濡 �� �ֽ��ϱ�?
BOOL isInARoom(SOCKET socket)
{
	int i = 0;
	for (i = 0; i < sizeClient; i++)	// Ŭ���̾�Ʈ �迭���� ������
	{
		if (arrClient[i].socket == socket	// Ư�� ����ڰ�
			&& arrClient[i].roomId != ROOM_ID_DEFAULT)	// room id�� ����������
			return TRUE;	// �濡 �� �ִ� ���̴�.
	}
	return FALSE;	// �ƴϸ� �濡 �� ���� �ʴ�.
}

// Ư�� ���ڿ����� space ���ڰ� �ִ� ���� index ��ȣ�� �����ش�.
int getIndexSpace(char *msg)
{
	int indexSpace = 0;
	int length = strlen(msg);
	int i = 0;
	for (i = 0; i < length; i++)
	{
		if (msg[i] == ' ')	// ���� ���ڸ� ã�Ƽ�
		{
			indexSpace = i;
			break;
		}
	}

	if (indexSpace + 1 >= length)
	{
		return -1;
	}

	return indexSpace;	// ���鹮���� ��ġ ��ȯ
}

// "������ �޴�"���� ������ �޴��� ���´�.(����� �޴��� 1�ڸ� �����̴�.)
int getSelectedWaintingRoomMenu(char *msg)
{
	if (msg == NULL) 
		return -1;

	int indexSpace = getIndexSpace(msg); // ����ڸ޽������� ���鹮�ڴ� �����ڷ� Ȱ��ȴ�.
	if (indexSpace < 0)
		return 0;

	char firstByte = msg[indexSpace + 1];	// ���鹮�� ���Ŀ� ó�� ��Ÿ���� ���� ���

	return firstByte - '0';	// ����ڰ� ������ �޴����� 48�� ����. atoi() �ᵵ��
}

// "���� �޴�" - ä���ϴٰ� ������ ���� �� ������ ����� �ʿ��ϴ�.
void getSelectedRoomMenu(char * menu, char *msg)
{
	if (msg == NULL) 
		return;	// ���� ó�� 

	int indexSpace = getIndexSpace(msg);	// ���鹮�� ��ġ ���
	if (indexSpace < 0) 
		return;	// ������ �߸��� ��Ŷ

	char *firstByte = &msg[indexSpace + 1];	// ���������� ���ڿ� ����
	strcpy(menu, firstByte);

	// all menus have 4 byte length. remove \n
	menu[5] = 0;	// 4����Ʈ ���� NULL ���� �־� ���ڿ� ����
	return;
}

// �� �����ϴ� �Լ�
Room *addRoom(char *name) // ���� �̸��� ������ �� �ִ�.
{
	WaitForSingleObject(mutx, INFINITE);		// �Ӱ� ���� ����
	Room *room = &(arrRoom[sizeRoom++]);	// ������ Ŭ���̾�Ʈ�� ����
	room->id = issuedId++;			// ���� ID �߱� - �����ؾ� ��
	strcpy(room->name, name);		// ���� �̸� ����
	ReleaseMutex(mutx);	// �Ӱ� ���� ��
	return room;	// ������ �� ����ü ������ �ּ� ��ȯ
}

// �� �����ϱ�
void removeRoom(int roomId)	// ���� ��ȣ�� �ָ� �迭���� ã�� �����Ѵ�.
{
	int i = 0;

	WaitForSingleObject(mutx, INFINITE);	// �Ӱ� ���� ����
	for (i = 0; i < sizeRoom; i++)	// ��� �濡 ���ؼ�
	{
		if (arrRoom[i].id == roomId)	// ���࿡ ���� ã������
		{
			while (i++ < sizeRoom - 1)	// �� ������ ��� ����
				arrRoom[i] = arrRoom[i + 1]; // ������ ��ĭ�� ����.
			break;
		}
	}
	sizeRoom--;	// ������ ���� ������ �ϳ� ���δ�.
	ReleaseMutex(mutx);	// �Ӱ� ���� ��
}

// Ư�� ���� ���� �մϱ�?
BOOL isExistRoom(int roomId)	// ���� ��ȣ�� �ְ�, ������� ������ Ȯ���Ѵ�.
{
	int i = 0;
	for (i = 0; i < sizeRoom; i++)		// ��� �濡 ���ؼ�
	{
		if (arrRoom[i].id == roomId)	// ���࿡ Ư�� ���� ã������
			return TRUE;	// ���� ��ȯ�Ѵ�.
	}
	return FALSE;	// �� �����µ� ��ã���� ������ ��ȯ�Ѵ�.
}

// ����ڰ� Ư�� �濡 ����
void enterRoom(Client * client, int roomId) // Ŭ���̾�Ʈ�� roomID�� �濡 ����.
{
	char her[BUF_SIZE] = "- - - - - - - Commands in ChatRoom - - - - - - - -\n\t\t@exit : Exit Room \n\t\t@list : User in this room\n\t\t@help : This message\n\t\t@setn : Set notice\n\t\t@getn : Get notice\n- - - - - - - - - - - - - - - - - - - - - - - - -\n";
	char buf[BUF_SIZE] = "";
	if (!isExistRoom(roomId))	// ���� �������� ������
	{
		sprintf(buf, "[server] : invalid room Id\n");	// ����� ����. ���� �޽��� �ۼ�
	}
	else	// ���� ã������
	{
		client->roomId = roomId;	// Ŭ���̾�Ʈ�� ���� ID�� ������ �ȴ�.
		sprintf(buf, "\t\t** %s Join the Room **\n", client->name); // Ȯ�� �޽��� �ۼ�
		sendMessageUser("cls\n", client->socket);
		sendMessageUser(her, client->socket);
	}

	// ��� �޽����� Ŭ���̾�Ʈ���� �������� �����ش�.
	sendMessageRoom(buf, client->roomId);

}

// �� ����� �Լ�
void createRoom(Client *client)	// Ư�� ����ڰ� ���� �����Ѵ�.
{
	int i;
	char name[BUF_SIZE];
	char originRoomname[BUF_SIZE];
	char cmpRoomname[BUF_SIZE];

	char buf[BUF_SIZE] = "";	// ����ڿ��� ������ �޽��� �ӽ� ���� �ʱ�ȭ
	sprintf(buf, "[server] : Input The Room Name:\n");	// "�� �̸��� �Է��ϼ���."

	sendMessageUser(buf, client->socket);	// Ŭ���̾�Ʈ���� �ȳ��޽��� ����

											// ���̸��� �ٷ� �޾� �����Ѵ�.



	if (recv(client->socket, buf, BUF_SIZE, 0) > 0)	// �� �޾�����
	{
		for (i = 0; i < sizeRoom; i++)      // ��� �濡 ���ؼ�
		{
			sscanf(arrRoom[i].name, "%s %s", name, originRoomname);
			sscanf(buf, "%s %s", name, cmpRoomname);
			if (strcmp(originRoomname, cmpRoomname) == 0) {    // �̸��� ���� ���� ã������
				enterRoom(client, arrRoom[i].id);
				return;
			}
		}

		Room *room = addRoom(buf);			// ���� �ϳ� �����
		enterRoom(client, room->id);		// ����ڴ� �濡 ����.
	}
}


// ���� ����� ǥ���϶�.
void listRoom(Client * client)	// Ư�� ����ڰ� ���� ����� ���� �;��Ѵ�.
{
	char buf[BUF_SIZE] = "";	// Ŭ���̾�Ʈ���� ������ �޽��� ����
	int i = 0;					// ���� ����

	printWaitingRoomMenu(client);

	sprintf(buf, "[server] : List Room:\n");	// "�� ����� ǥ���ϰڽ��ϴ�."	
	sendMessageUser(buf, client->socket);

	// ��� ���� ����� �����Ѵ�.
	for (i = 0; i < sizeRoom; i++)	// ��� �濡 ���ؼ�	
	{
		Room *room = &(arrRoom[i]);	// ������ ���� ���ͼ�
										// ID�� �̸��� ���·� ���๮�ڸ� �־� ���پ� �����Ѵ�.
		sprintf(buf, "RoomName : %s \n", room->name);
		sendMessageUser(buf, client->socket);
	}

	// ������ �� ���� ������ ǥ���Ѵ�.
	sprintf(buf, "Total %d rooms\n", sizeRoom);
	sendMessageUser(buf, client->socket);
}

// Ư�� �濡 �ִ� ������� ����� ǥ���Ѵ�.
void listMember(Client *client, int roomId) // list client in a room
{
	char buf[BUF_SIZE] = "";		// ����ڿ��� ������ �޽����� ����
	int i = 0;					// �����
	int sizeMember = 0;			// �������� ������� ��

	//printWaitingRoomMenu(client);

	if (roomId == -1)				// �ش��ϴ� ���� ���� ���
	{
		sprintf(buf, "[server] : invalid room Id\n");	// �׷� �� ���� ���
		sendMessageUser(buf, client->socket);
		return;
	}
								// "Ư�� �濡 �ִ� ������� ����� ǥ���ϰڽ��ϴ�."
	sprintf(buf, "- - - - - - List Member In This Room - - - - - -\n");
	sendMessageUser(buf, client->socket); // �ȳ� �޽��� ǥ��

	for (i = 0; i < sizeClient; i++)	// ��� ����ڿ� ���ؼ�
	{
		if (arrClient[i].roomId == roomId)	// ���� Ư�� �濡 �� ������
		{
			// ��ȣ, ���Ϲ�ȣ(���� ID) ������ ǥ���� �ش�.
			sprintf(buf, "Name :\t\t%s\n", arrClient[i].name);
			sendMessageUser(buf, client->socket);
			sizeMember++;	// Ư�� �濡 �ִ� �� ������� �� ���
		}
	}

	// Ư�� �濡 �ִ� ������� ���� ǥ���Ѵ�.
	sprintf(buf, "\tTotal: %d Members\n- - - - - - - - - - - - - - - - - - - - - - - - -\n", sizeMember);
	sendMessageUser(buf, client->socket);
}



int getRoomId(SOCKET socket)      // socket�� Ŭ���̾�Ʈ ID
{
	int i, roomId = -1;         // ���� ID �ʱⰪ�� -1(��ã��)
	char buf[BUF_SIZE] = "";   // ����ڿ��� ���� �޽��� ����
	char Roomname[BUF_SIZE] = "";
	char originRoomname[BUF_SIZE] = "";
	// �ȳ� �޽��� ǥ��

	sprintf(buf, "[system] : Input Room Name:\n");   // "���� ID�� �Է��Ͻÿ�"
	sendMessageUser(buf, socket);         // ����ڿ��� �޽��� ����

	if (recv(socket, buf, sizeof(buf), 0) > 0)      // ���� ID�� �Է� �޴´�.
	{
		char name[BUF_SIZE] = "";
		sscanf(buf, "%s %s", name, Roomname);
		//Roomname[str_len] = 0;
	}

	for (i = 0; i < sizeRoom; i++)      // ��� �濡 ���ؼ�
	{
		sscanf(arrRoom[i].name, "%s %s", buf, originRoomname);
		if (strcmp(originRoomname, Roomname) == 0)   // ���࿡ Ư�� ���� ã������
			return arrRoom[i].id;   //  ���� ��ȣ ��ȯ
	}

	return roomId;   // ������ ������ ���� ��ȣ ��ȯ
}


// Ŭ���̾�Ʈ���� ���� �޴��� ���� �ش�.
void printWaitingRoomMenu(Client * client)
{
	char buf[BUF_SIZE] = "";

	sendMessageUser("cls\n", client->socket);

	sprintf(buf, "- - - - - - - - - - - Menu - - - - - - - - - - -\n");	// ���� �޴��� �̷����ϴ�.
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t1) Create Room\n");				// �� �����
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t2) List Room\n");					// �� ��� ǥ���ϱ�
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t3) Enter Room\n");					// Ư�� �濡 ����
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t4) Info Room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t5) How To Use\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\tq Or Q) Quit\n- - - - - - - - - - - - - - - - - - - - - - - -\n");						// �����ϱ�(���� ����)
	sendMessageUser(buf, client->socket);
}

// ä�� �濡�� ��밡���� �޴� ǥ���ϱ�
void printRoomMenu(Client *client)
{
	char buf[BUF_SIZE] = "";
	sprintf(buf, "- - - - - - - - - - Room Menu - - - - - - - - - -\n");		// �� �޴� ǥ��
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@exit) Exit Room\n");			// �� ������
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@list) List Room\n");			// ������ ���� ��� ǥ���ϱ�
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@help) This message\n");		// ���� ǥ���ϱ�
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@setn) set notice\n");         // ���� �����ϱ�
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@getn) get notice\n- - - - - - - - - - - - - - - - - - - - - - - - -\n");         // ���� �޾ƿ���
	sendMessageUser(buf, client->socket);
}

void printHowToUse(Client *client) //���� �����ֱ�
{
	char buf[BUF_SIZE] = "";

	printWaitingRoomMenu(client);

	sprintf(buf, "- - - - - - - - - - - Menu - - - - - - - - - - -\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t1.CreateRoom : Create chat room and enter the room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t2.ListRoom : Show all created chat rooms\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t3.EnterRoom : Enter the chat room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t4.InfoRoom : Show all users in certain chat room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "- - - - - - - - Commands in ChatRoom - - - - - - - -\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@exit : Exit room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@list : Users in the chat room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@help : Manual of Commands in chat room\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@setn : Set notice\n");
	sendMessageUser(buf, client->socket);

	sprintf(buf, "\t\t@getn : Get notice\n- - - - - - - - - - - - - - - - - - - - - - - -\n");
	sendMessageUser(buf, client->socket);

}

// ���� �޴����� ����ڰ� ������ �ϸ� ���񽺸� �����Ѵ�.
void serveWaitingRoomMenu(int menu, Client *client, char *msg) // ����ڿ� ���õ� �޴�
{
	int roomId = ROOM_ID_DEFAULT;
	switch (menu)	// �������� �޴� �߿��� �ϳ��� �����ϴ� ��� select ���� �����ϴ�.
	{
	case 1:
		createRoom(client);					// 1�� �� �����
		break;
	case 2:
		listRoom(client);					// 2�� �� ��� ǥ���ϱ�
		break;
	case 3:
		roomId = getRoomId(client->socket);	// 3�� Ư�� ������ �̵��ϱ�
		enterRoom(client, roomId);
		break;
	case 4:
		roomId = getRoomId(client->socket);	// 4�� Ư�� �濡 �ִ� ����� ��� ����
		listMember(client, roomId);
		break;
	case 5:
		//printWaitingRoomMenu(client);
		printHowToUse(client);
		break;

	default:
		//sendMessageUser(msg, client->socket);
		//printWaitingRoomMenu(client);		// �߸� �Է�������, �޴� �ٽ� ǥ��
		break;
	}
}

// ���� ���� ���� ������
void exitRoom(Client *client)	// ���ڴ� ���� Ŭ���̾�Ʈ
{
	int roomId = client->roomId;			// ���� �� ��ȣ
	client->roomId = ROOM_ID_DEFAULT;		// ������� ���� ���� �ʱ�ȭ �Ѵ�.

	char buf[BUF_SIZE] = "";					// �޽��� ����
	sprintf(buf, "[server] exited room id:%d\n", roomId); //"Ư�� ���� ���Խ��ϴ�."
	sendMessageUser(buf, client->socket);	// �޽��� ����

	printWaitingRoomMenu(client);

	if (isEmptyRoom(roomId))			// �濡 �ƹ��� ������
	{
		removeRoom(roomId);		// ���� �Ҹ� ��Ų��.
	}
}

// Ư�� �濡 ����� �ֽ��ϱ�?
BOOL isEmptyRoom(int roomId)
{
	int i = 0;
	for (i = 0; i < sizeClient; i++)		// ��� Ŭ���̾�Ʈ �߿�
	{
		if (arrClient[i].roomId == roomId)	// �Ѹ��̶� �� �濡 ������
			return FALSE;			// ����� �ƴմϴ�.
	}
	return TRUE;					// ����Դϴ�.
}

// ������� �Լ�
unsigned WINAPI handle_clnt(void * arg)	// ������ ��� Ŭ���̾�Ʈ�� ����ϴ� �Լ�
{
	Client *client = (Client *)arg;	// �����尡 ����� Ŭ���̾�Ʈ ����ü ����
	int str_len = 0, i;
	char msg[BUF_SIZE];			// �޽��� ����


	printWaitingRoomMenu(client);	// ä���� ������ �� ó���� �� �޴� ǥ��

	SOCKET clnt_sock = client->socket;	// ����� ���� ���
	int roomId = client->roomId;	// ���� ��ȣ�� �� ���´�.

									// Ŭ���̾�Ʈ�� ����� ���� �ʴ��� ��� ���񽺸� �����Ѵ�.
	while ((str_len = recv(clnt_sock, msg, BUF_SIZE, 0)) != SOCKET_ERROR)
	{
		printf("Read User(%d):%s\n", (int)clnt_sock, msg); // ���������� ���·α� ǥ��

		if (isInARoom(clnt_sock))	// �� �ȿ��� Ŭ���̾�Ʈ�� �޽����� ������ ���
		{
			char menu[BUF_SIZE] = "";
			getSelectedRoomMenu(menu, msg);		// �� �޴� �߿� � ������ �ߴ��� �Ǵ�
			serveRoomMenu(menu, client, msg);	// ���� ���� ����(exit, list, chat)
		}
		// is in the waiting room
		// ���ǿ��� Ŭ���̾�Ʈ�� �޽����� ������ ���
		else
		{
			// ������ �޴��� �Է� �޴´�(int�� ������ 1~4 ���� �޴�)
			int selectedMenu = getSelectedWaintingRoomMenu(msg);

			// ����� �������� ������ ȭ�鿡 ǥ���� �ش�.
			printf("User(%d) selected menu(%d)\n", client->socket, selectedMenu);

			// ���� �޴��� ���񽺸� �����Ѵ�.
			serveWaitingRoomMenu(selectedMenu, client, msg);
		}
	}

	removeClient(clnt_sock);	// Ŭ���Ʈ�� ������ ����Ǹ� Ŭ���̾�Ʈ �迭���� �����Ѵ�.

	if (isEmptyRoom(client->roomId))			// �濡�� q�� �ٷ� ������ ���(������ ���� ����)�� ���� ��� ����
	{
		removeRoom(client->roomId);		
	}

	closesocket(clnt_sock);			// ������ �ݰ�
	return NULL;				// ������ ����
}

// ���� ó�� �Լ�
void error_handling(char * msg)
{
	fputs(msg, stderr);		// ������ ����ϰ�
	fputc('\n', stderr);
	exit(1);				// ���α׷� ����
}

int main(int argc, char* argv[])	// ���ڷ� ��Ʈ��ȣ ����
{
	WSADATA wsaData;
	SOCKET serv_sock, clnt_sock;		// ������� �� ���� ���ϰ� �ӽ� Ŭ���̾�Ʈ ����
	SOCKADDR_IN serv_adr, clnt_adr;	// ���� �ּ�, Ŭ���̾�Ʈ �ּ� ����ü
	int clnt_adr_sz;				// Ŭ���̾�Ʈ �ּ� ����ü ũ��
	char nick[BUF_SIZE] = { 0 }; //nickname
	int nick_len; //nickname ����
	unsigned clntID;
	unsigned servID;
	HANDLE t_id;					// Ŭ���̾�Ʈ ������� ID
	HANDLE serv_id;
	void* thread_return;

	// ��Ʈ �Է¾�������
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);	// ������ �˷��ش�.
		exit(1);	// ���α׷� ������ ����
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		error_handling("WSAStartup() error!");

	// ���� ������ �ּ� �ʱ�ȭ
	mutx = CreateMutex(NULL, FALSE, NULL);	// Ŀ�ο��� Mutex ���� ���� ���´�.
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);	// TCP�� ���� ���� ����

	memset(&serv_adr, 0, sizeof(serv_adr));		// ���� �ּ� ����ü �ʱ�ȭ
	serv_adr.sin_family = AF_INET;				// ���ͳ� ���
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);	// ���� IP�� �̿��ϰ�
	serv_adr.sin_port = htons(atoi(argv[1]));		// ��Ʈ�� ����ڰ� ������ ��Ʈ ���

													// ���� ���Ͽ� �ּҸ� �Ҵ��Ѵ�.
	if (bind(serv_sock, (SOCKADDR*)& serv_adr, sizeof(serv_adr)) == SOCKET_ERROR)
		error_handling("bind() error");

	// ���� ������ ���������� �����Ѵ�.
	if (listen(serv_sock, MAX_CLNT) == -SOCKET_ERROR) //custom: backlog ���� MAX_CLNT�� �ٲ�
		error_handling("listen() error");

	while (1)	// ���ѷ��� ���鼭
	{
		clnt_adr_sz = sizeof(clnt_adr);	// Ŭ���̾�Ʈ ����ü�� ũ�⸦ ���
		memset(nick, 0, sizeof(BUF_SIZE));
		// Ŭ���̾�Ʈ�� ������ �޾� ���̱� ���� Block �ȴ�.(�����)
		clnt_sock = accept(serv_sock, (SOCKADDR*)& clnt_adr, &clnt_adr_sz);

		//Ŭ���̾�Ʈ���� �г��� �޴´�
		//read(clnt_sock, nick, sizeof(nick));
		nick_len = recv(clnt_sock, nick, BUF_SIZE - 1, 0);


		// Ŭ���̾�Ʈ�� ������ �Ǹ� Ŭ���̾�Ʈ ������ �迭�� �߰��ϰ� �� �ּҸ� ��´�.
		Client* client = addClient(clnt_sock, nick);

		// Ŭ���̾�Ʈ ����ü�� �ּҸ� �����忡�� �ѱ��.(��Ʈ ���Ե�)
		t_id = (HANDLE)_beginthreadex(NULL, 0, handle_clnt, (void*)client, 0, &clntID);
		serv_id = (HANDLE)_beginthreadex(NULL, 0, handle_serv, (void*)client, 0, &servID);

		printf("%s is connected \n", client->name);

	}
	closesocket(clnt_sock);
	closesocket(serv_sock);
	WSACleanup();
	return 0;
}