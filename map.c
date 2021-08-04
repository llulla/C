#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
#include <WS2tcpip.h>
#include <process.h>

//�ܼ� �Ӽ� - �۲�: ����ü ����, ������ / ��� ��� 
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN1 3
#define RED 4
#define MAGENTA 5
#define YELLOW 6
#define GRAY 7
#define GRAY2 8
#define BLUE2 9
#define GREEN2 10
#define CYAN2 11
#define RED2 12
#define MAGENTA2 13
#define YELLOW2 14
#define WHITE 15
#define UP 72
#define DOWN 80
#define LEFT 75
#define RIGHT 77
#define SUBMIT 13

typedef struct {
	int pos; //��ġ
	int money;   //��
	int cash;    //����
	int bank;   //�ε��갪
	char mysymbol[4]; //��  �ǹ� 1 �� �ظ� 2 �� ���� 3 �� ���� 4 ��
	int symbolnum; //player1 �̸� 1, player2 �̸� 2
	int pause_move; //���ε� ���� �� ��
	char textcolor;  //define���� �÷��̾� ���� ���缭 ���޾ƿ���
	int goldkey; //���ε� Ż���
	int rank;   //�÷��̾� ����
} Player;

typedef struct {
	int x; //������ġ x��ǥ
	int y; //������ġ y��ǥ
	char city_name[20]; //�����̸� 
	int owner; //���� ������ (1:player1���� 2:player2���� 3:player3���� 4:player4���� 6:������)
	int building; //�ǹ� ����������
	int price; //���� ����
	int take_over;//�ǹ� �μ�����
	char builsym;//���� �ɺ�
	char original[20];   //���� �Է°��̶� ��
	int bail;   //������
} CityInfo;

typedef struct {
	int dice1;
	int dice2;
}Dice;

typedef struct {
	int turn;
	int cnt;
}Play;

int menuDraw();
int keyControl();

//�Լ�����
void init_map();   //�� �ʱ�ȭ
void print_map(int map[68][68], int row, int col);   //�� ���
void gotoxy(int x, int y); //Ŀ���̵�
void Nocursor();   //Ŀ�� ���ֱ�
void Cursor();   //Ŀ�� ���
void textcolor(int font, int back);   //�ܼ� ��Ʈ �� ����
void clear();//ȭ�������
int Dice1(); //�ֻ���1
int Dice2();   //�ֻ���2
void init_city(CityInfo city[]);   //���� �ʱ�ȭ
void init_player(Player* pl, int pl_num);   //�÷��̾� �ʱ�ȭ
void move_player(CityInfo city[], int dice_num, Player* pl, int pl_num);  //�÷��̾� ������
////////////////////////////////////////////////////////////////////////////// 1�� ��ǥ

int Double(CityInfo city[], Player* pl, Dice dices, Play play, int pl_num, int limit);   //�ֻ��� ������ & ���� �Ǻ�
void Arrive(CityInfo city[], Player* pl, int num);   //���� ���� �� �� ����, �ǹ� ����
void player_update(CityInfo city[], Player* pl);   //�÷��̾� �ڻ� ����
void Ktx(CityInfo city[], Player* pl, int pl_num);   //ktx �̵�
void Sale_City(CityInfo city[], Player* pl, int num);   //���� �Ű�
void Buy_City(CityInfo city[], Player* pl, int pl_num);   //���� �μ�
void Prison(CityInfo city[], Player* pl, int pl_num);   //������
void Bankruptcy(CityInfo city[], Player* pl, int pl_num);
void GameOver(CityInfo city[], Player* pl);   //�� ���� ���ӿ���
int Choice_player();
void watingRoom(int i);

//�̴ϰ��� �Լ�
void game_rsp();   //����������
int printf_rsp(int rsp_user);
int printf_rsp2(int rsp_com);
int Chamchamcham();   //������
int Print_Cham(int);
////////////////////////////////////////////////////////////////////////////// 2�� ��ǥ

void GameOver(CityInfo city[], Player* pl);	//�� ���� ���ӿ���
void draw_box_end(int x1, int y1, int x2, int y2, char* ch);	//���� �Ѿ��
void Game_info();	//�������� ���������
void intro_color(); //��Ʈ�� 

//Ȳ�ݿ��� �Լ�
void Gold_key(Player* pl, CityInfo city[], int pl_num);
void G_Change(CityInfo city[], Player* pl, int pl_num);
void G_prison(CityInfo city[], Player* pl, int pl_num);
void G_start(CityInfo city[], Player* pl, int pl_num);
void G_hole(CityInfo city[], Player* pl, int pl_num);
void CityUpdate(CityInfo city[], Player* pl);
void SymbolUpdate(CityInfo city[], Player* pl);

void ServerSend(CityInfo city[], Player* pl);
void ServerRecv(CityInfo city[], Player* pl);
void Arrive2(CityInfo city[], Player* pl, Play play, int pl_num);

unsigned WINAPI sendChat(void* arg);
unsigned WINAPI recvChat(void* arg);

int pixed_pl_num; // �÷��̾�� ��������
int pl_cnt; //�Ļ� �� ��������
int StartFlag;

SOCKET Sock;
SOCKADDR_IN addr;
int addrSz;

HANDLE hMutex, sendThread, recvThread, mainThread;
char buf[1024] = { 0, };
int pl_num;
int readycnt = 0;
int call = 0;
//����
int main(void) {
	Dice dices;
	dices.dice1 = 0;
	dices.dice2 = 0;
	Play play;
	play.turn = 0;
	play.cnt = 40;

	int pl_count;
	int start = 5;
	char msg[100] = { 0, };

	hMutex = CreateMutex(NULL, FALSE, NULL);	//���ý� ����

	//WSAStartup
	WSADATA wsa; //���̺귯�� �ε�
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) //winsock �ʱ�ȭ �Լ�
		printf("WSAStartup Error\n");

	//socket 
	Sock = socket(PF_INET, SOCK_STREAM, 0);
	if (Sock == INVALID_SOCKET) {
		printf("socket error\n");
	}
	char ip[10] = { 0, };

	while (1) {
		system("mode con cols=86 lines=25");
		system("title KOMA");
		int menuCode = menuDraw();
		if (menuCode == 0) {//���� ����
			clear(); gotoxy(23, 10); printf("���� ip : "); scanf("%s", ip);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(12345);
			addr.sin_addr.s_addr = inet_addr(ip);
			if (connect(Sock, (SOCKADDR*)& addr, sizeof(addr)) == SOCKET_ERROR) {
				printf("connect error\n");
			}
			recv(Sock, (char*)& pixed_pl_num, sizeof(pixed_pl_num), 0);
			recv(Sock, (char*)& pl_num, sizeof(pl_num), 0);

			for (int i = pl_num; i < pixed_pl_num; i++) {
				recv(Sock, (char*)& pl_count, sizeof(pl_count), 0);
				watingRoom(pl_count);
			}
			recvThread = (HANDLE)_beginthreadex(NULL, 0, recvChat, (void*)& Sock, 0, NULL);
			sendThread = (HANDLE)_beginthreadex(NULL, 0, sendChat, (void*)& Sock, 0, NULL);
			WaitForSingleObject(recvThread, INFINITE);
			WaitForSingleObject(sendThread, INFINITE);
			CloseHandle(recvThread);
			CloseHandle(sendThread);

			while (start != 0) {
				start--;
				Sleep(1000);
				gotoxy(30, 2); printf("%d �� �ڿ� ������ ���۵˴ϴ�.", start);
			}
			system("mode con cols=114 lines=64");//�ܼ� ������ �Ƚ�
			Nocursor();
			CityInfo city[28];
			init_map();
			init_city(city);
			Player* pl = (Player*)malloc(sizeof(Player) * pixed_pl_num);
			init_player(pl, pixed_pl_num);
			int double_limit = 0;
			pl_cnt = 1;

			recv(Sock, msg, sizeof(msg), 0);   //�����忡 �����ִ� ������ �޾ƿ���
			while (play.cnt > 0) {
				recv(Sock, (char*)& play, sizeof(play), 0);
				if (pl_num == play.turn) {
					if (pl[pl_num].cash < 0) {
						break;
					}
					else {
						gotoxy(23, 15); printf("�Ƹ���� ��ͷο� ����� �����Դϴ�.");
						Double(city, pl, dices, play, pl_num, double_limit);
						clear();
						gotoxy(80, 50); textcolor(BLACK, WHITE); printf("���� �� �� : %d", play.cnt);
						call = 3;
						send(Sock, (char*)& call, sizeof(call), 0);
					}
				}
				else {
					if (pl[play.turn].cash < 0) {
						break;
					}
					else {
						gotoxy(23, 15); printf("�÷��̾� %d�� �����Դϴ�.", play.turn + 1);
						Double(city, pl, dices, play, pl_num, double_limit);
						clear();
					}
				}
			}
			GameOver(city, pl);
			system("pause>NULL");
			free(pl);
		}
		else if (menuCode == 2) {
			system("mode con cols=100 lines=35");
			Game_info();
			system("pause>NULL");
		}
		else if (menuCode == 4) {
			exit(1);
		}
		system("cls");
	}
	closesocket(Sock);

	WSACleanup();
}

void CityUpdate(CityInfo city[], Player* pl) {
	for (int i = 0; i < 28; i++) {
		if (city[i].owner == 6) {
			gotoxy(city[i].x, city[i].y - 1);
			textcolor(BLACK, WHITE);
			printf("%s", city[i].city_name);
		}
		else if (city[i].owner == 7) {
			gotoxy(city[i].x, city[i].y - 1);
			textcolor(BLACK, WHITE);
			printf("%s", city[i].city_name);
		}
		else {
			// ����
			gotoxy(city[i].x, city[i].y - 1);
			textcolor(pl[city[i].owner].textcolor, WHITE);
			printf("%s", city[i].city_name);
			textcolor(BLACK, WHITE);

			// �ǹ�
			if (city[i].building == 1) {
				gotoxy(city[i].x - 2, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("         ");
				textcolor(BLACK, WHITE);
			}
			if (city[i].building == 2) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("��");
				textcolor(BLACK, WHITE);
			}
			else if (city[i].building == 3) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("�֣�");
				textcolor(BLACK, WHITE);
			}
			else if (city[i].building == 4) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("�֣£�");
				textcolor(BLACK, WHITE);
			}
			else if (city[i].building == 5) {
				gotoxy(city[i].x, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("LAND MARK");
				textcolor(BLACK, WHITE);
			}
		}
	}
}

void SymbolUpdate(CityInfo city[], Player* pl) {
	for (int i = 0; i < pixed_pl_num; i++) {
		for (int j = 0; j < 28; j++) {
			gotoxy(city[j].x + i * 2, city[j].y + 1);
			printf("  ");
		}

		switch (i) {
		case 0:   //�÷��̾�1
			if (pl[i].cash < 0) {
				break;
			}
			gotoxy(city[pl[i].pos % 28].x + i * 2, city[pl[i].pos % 28].y + 1);
			printf("  ");
			gotoxy(city[pl[i].pos].x + i * 2, city[pl[i].pos].y + 1);
			textcolor(pl[i].textcolor, WHITE);
			printf("%s", pl[i].mysymbol);
			textcolor(BLACK, WHITE);
			break;
		case 1:   //�÷��̾�2
			if (pl[i].cash < 0) {
				break;
			}
			gotoxy(city[pl[i].pos % 28].x + i * 2, city[pl[i].pos % 28].y + 1);
			printf("  ");
			gotoxy(city[pl[i].pos].x + i * 2, city[pl[i].pos].y + 1);
			textcolor(pl[i].textcolor, WHITE);
			printf("%s", pl[i].mysymbol);
			textcolor(BLACK, WHITE);
			break;
		case 2:   //�÷��̾�3
			if (pl[i].cash < 0) {
				break;
			}
			gotoxy(city[pl[i].pos % 28].x + i * 2, city[pl[i].pos % 28].y + 1);
			printf("  ");
			gotoxy(city[pl[i].pos].x + i * 2, city[pl[i].pos].y + 1);
			textcolor(pl[i].textcolor, WHITE);
			printf("%s", pl[i].mysymbol);
			textcolor(BLACK, WHITE);
			break;
		case 3:   //�÷��̾�4
			if (pl[i].cash < 0) {
				break;
			}
			gotoxy(city[pl[i].pos % 28].x + i * 2, city[pl[i].pos % 28].y + 1);
			printf("  ");
			gotoxy(city[pl[i].pos].x + i * 2, city[pl[i].pos].y + 1);
			textcolor(pl[i].textcolor, WHITE);
			printf("%s", pl[i].mysymbol);
			textcolor(BLACK, WHITE);
			break;
		}
	}
}
//�Լ�
void textcolor(unsigned int text, unsigned int back) //text color
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void gotoxy(int x, int y)//Ŀ���̵�
{
	COORD Cur = { x,y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
}

void Nocursor() {
	CONSOLE_CURSOR_INFO CurInfo;
	CurInfo.dwSize = 1;
	CurInfo.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CurInfo);
}

void Cursor() {   //Ŀ�� ����
	CONSOLE_CURSOR_INFO CurInfo;
	CurInfo.dwSize = 100;
	CurInfo.bVisible = TRUE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CurInfo);
}

void clear()//��� �����ִ� �Լ� 
{
	gotoxy(16, 15);
	printf("                                                                                \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	printf("\t\t                                                                                 \n");
	gotoxy(80, 50); printf("                 ");//�����ϼ� �����ִ� ��
}

void clear2() {
	gotoxy(0, 0);
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");
	printf("                                                                                                                  \n");


}

int Double(CityInfo city[], Player* pl, Dice dices, Play play, int pl_num, int limit) {
	textcolor(BLACK, WHITE);
	int dice_sum = 0;
	int addrSz;
	Nocursor();
	if (pl_num == play.turn) {
		while (1) {

			dices.dice1 = Dice1(rand() % 6 + 1);
			dices.dice2 = Dice2(rand() % 6 + 1);

			if (_kbhit()) {//sendto
				call = 1;
				send(Sock, (char*)& call, sizeof(call), 0);
				send(Sock, (char*)& dices, sizeof(dices), 0);
				dice_sum = dices.dice1 + dices.dice2;
				if (dices.dice1 == dices.dice2) {
					gotoxy(23, 30); printf("����������������������������������\n");
					gotoxy(23, 31); printf("����������������������������������\n");
					gotoxy(23, 32); printf("����������������������������������\n");
					gotoxy(23, 33); printf("����������������������������������\n");
					gotoxy(23, 34); printf("����������������������������������\n");
					gotoxy(23, 35); printf("����������������������������������\n");
					gotoxy(23, 36); printf("����������������������������������\n");
					gotoxy(23, 37); printf("����������������������������������\n");
					gotoxy(23, 38); printf("����������������������������������\n");
					gotoxy(23, 39); printf("");
					limit++;
					if (pl[pl_num].pause_move > 0) {
						pl[pl_num].pause_move = 0;
						gotoxy(23, 17); printf("�������!!!�����Ϻ��� �̵��� �� �־��");
						system("pause>NULL");
					}
					else if (limit == 3) {
						gotoxy(23, 17); printf("������ 3��!");
						gotoxy(23, 19); printf("���ε��� ����\n"); gotoxy(50, 22);
						system("pause>NULL");

						gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
						printf("  "); //����������� ����
						pl[pl_num].pos = 7;
						gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
						textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
						printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
						textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
						Arrive(city, pl, pl_num);
					}
					else
					{
						move_player(city, dice_sum, pl, pl_num);
						Arrive(city, pl, pl_num);
						Double(city, pl, dices, play, pl_num, limit);
					}
				}
				else if (pl[pl_num].pause_move > 0) {
					Prison(city, pl, pl_num);
					if (pl[pl_num].pause_move == 0) {
						gotoxy(23, 15); printf("�����Ҹ� �����ϴ�.");
						system("pause>NULL");
						clear();
						move_player(city, dice_sum, pl, pl_num);
						Arrive(city, pl, pl_num);
					}
					else {
						gotoxy(23, 17); printf("��ƾƾӺ�...%d�� ���Ҿ��", pl[pl_num].pause_move - 1);
						system("pause>NULL");
					}
					pl[pl_num].pause_move--;
				}
				else {
					move_player(city, dice_sum, pl, pl_num); Arrive(city, pl, pl_num);
				}
				clear();
				break;
			}
		}
	}
	else {
		clear();
		recv(Sock, (char*)& dices, sizeof(dices), 0);
		dice_sum = dices.dice1 + dices.dice2;
		gotoxy(23, 15); printf("�÷��̾�%d %dĭ �̵�", play.turn + 1, dice_sum);
		if (dices.dice1 == dices.dice2) {
			gotoxy(23, 30); printf("����������������������������������\n");
			gotoxy(23, 31); printf("����������������������������������\n");
			gotoxy(23, 32); printf("����������������������������������\n");
			gotoxy(23, 33); printf("����������������������������������\n");
			gotoxy(23, 34); printf("����������������������������������\n");
			gotoxy(23, 35); printf("����������������������������������\n");
			gotoxy(23, 36); printf("����������������������������������\n");
			gotoxy(23, 37); printf("����������������������������������\n");
			gotoxy(23, 38); printf("����������������������������������\n");
			gotoxy(23, 39); printf("");
			limit++;
			if (pl[play.turn].pause_move > 0) {
				pl[play.turn].pause_move = 0;
			}
			else if (limit == 3) {
				gotoxy(23, 17); printf("������ 3��!");
				gotoxy(city[pl[pl_num].pos % 28].x + play.turn * 2, city[pl[play.turn].pos % 28].y + 1); //�÷��̾��� ������ġ
				printf("  "); //����������� ����
				pl[play.turn].pos = 7;
				gotoxy(city[pl[play.turn].pos].x + play.turn * 2, city[pl[pl_num].pos].y + 1);
				textcolor(pl[play.turn].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
				printf("%s", pl[play.turn].mysymbol); //�÷��̾� �ɺ� ���
				textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
				Arrive2(city, pl, play, pl_num);
				clear();
			}
			else
			{
				move_player(city, dice_sum, pl, play.turn);
				Arrive2(city, pl, play, pl_num);
				Double(city, pl, dices, play, pl_num, limit);
			}
		}
		else if (pl[pl_num].pause_move > 0) {
			Prison(city, pl, pl_num);
			if (pl[pl_num].pause_move == 0) {
				move_player(city, dice_sum, pl, play.turn);
				Arrive2(city, pl, play, pl_num);
			}
			else {
				pl[pl_num].pause_move--;
			}
		}
		else {
			move_player(city, dice_sum, pl, play.turn);
			Arrive2(city, pl, play, pl_num);
		}
		clear();
	}
}

//�ֻ��� ���� �ٲ�� �Լ�
int Dice1(int dice_num)
{
	if (dice_num == 1) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");
	}
	else if (dice_num == 2) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");
	}
	else if (dice_num == 3) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");

	}
	else if (dice_num == 4) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");

	}
	else if (dice_num == 5) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");

	}
	else if (dice_num == 6) {
		gotoxy(40, 44); printf("��������\n");
		gotoxy(40, 45); printf("��������\n");
		gotoxy(40, 46); printf("��������\n");
		gotoxy(40, 47); printf("��������\n");
		gotoxy(40, 48); printf("��������\n");
		gotoxy(40, 49); printf("��������\n");
		gotoxy(40, 50); printf("��������\n");
	}
	return dice_num;
}

int Dice2(int dice_num)
{
	if (dice_num == 1) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");

	}
	else if (dice_num == 2) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");
	}
	else if (dice_num == 3) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");
	}
	else if (dice_num == 4) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");
	}
	else if (dice_num == 5) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");
	}
	else if (dice_num == 6) {
		gotoxy(60, 44); printf("��������\n");
		gotoxy(60, 45); printf("��������\n");
		gotoxy(60, 46); printf("��������\n");
		gotoxy(60, 47); printf("��������\n");
		gotoxy(60, 48); printf("��������\n");
		gotoxy(60, 49); printf("��������\n");
		gotoxy(60, 50); printf("��������\n");
	}
	return dice_num;
}

void init_city(CityInfo city[]) //�� �̸� ��ǥ �ʱ�ȭ
{
	int i, k;
	char arr_city[28][20] = { {"��    ��"},{"��    ��"},{"�� ũ Ȧ"},{"��    õ"},{"�� �� ��"},{"��    ��"},{"õ    ��"},{"�� �� ��"},
							  {"�� �� ��"},{"��    õ"},{"Ȳ�ݿ���"},{"û    ��"},{"�� �� ��"},{"�� �� ��"},{"��������"},
							  {"��Ɽ��"},{"�� �� ��"},{"��    ��"},{"Ȳ�ݿ���"},{"��    ��"},{"��    ��"},{"K  T  X"},
							  {"��    ��"},{"��������"},{"Ȳ�ݿ���"},{"��    ��"},{"�� �� ��"},{"��    ��"} };

	int arr_cityprice[28] = { 0,5,0,8,10,12,14,0,10,15,0,18,10,20,0,30,10,34,0,36,38,0,42,10,0,48,0,50 };
	char tmp_city[28][20] = { {"���"},{"����"},{"��ũȦ"},{"��õ"},{"���ѻ�"},{"����"},{"õ��"},{"������"},
							 {"���̵�"},{"��õ"},{"Ȳ�ݿ���"},{"û��"},{"���ǻ�"},{"������"},{"��������"},
							 {"��Ɽ��"},{"������"},{"����"},{"Ȳ�ݿ���"},{"����"},{"����"},{"KTX"},
							 {"����"},{"��������"},{"Ȳ�ݿ���"},{"�λ�"},{"������"},{"����"} }; //���� �̸� �񱳿�

	for (i = 7, k = 0; i >= 0; i--) {      // <<���� �Ʒ���<< ���� ��ġ���� �� ǥ��, owner, ���� �ʱ�ȭ
		city[i].x = k + 4; //x =4 y =57   x 18 y = 57                                       
		city[i].y = 56;
		k = k + 14;
		gotoxy(city[i].x, city[i].y - 1);
		strcpy_s(city[i].city_name, sizeof(arr_city[i]) + 1, arr_city[i]);
		strcpy_s(city[i].original, sizeof(tmp_city[i]) + 1, tmp_city[i]);
		printf("%s", city[i].city_name);

		city[i].owner = 6;
		city[i].building = 0;
		city[i].price = arr_cityprice[i] * 10;
		city[i].take_over = 0;

	}
	for (i = 14, k = 0; i >= 8; i--) {   // <<���� ������<< ���� ��ġ���� �� ǥ��, owner, ���� �ʱ�ȭ
		city[i].x = 4;
		city[i].y = k + 7;
		k = k + 7;
		gotoxy(city[i].x, city[i].y - 1);
		strcpy_s(city[i].city_name, sizeof(arr_city[i]) + 1, arr_city[i]);
		strcpy_s(city[i].original, sizeof(tmp_city[i]) + 1, tmp_city[i]);
		printf("%s", city[i].city_name);

		city[i].owner = 6;
		city[i].building = 0;
		city[i].price = arr_cityprice[i] * 10;
		city[i].take_over = 0;
	}
	for (i = 21, k = 0; i >= 15; i--) {   // <<���� ����<< ���� ��ġ���� �� ǥ��, owner, ���� �ʱ�ȭ
		city[i].x = 102 - k;
		city[i].y = 7;
		k = k + 14;
		gotoxy(city[i].x, city[i].y - 1);
		strcpy_s(city[i].city_name, sizeof(arr_city[i]) + 1, arr_city[i]);
		strcpy_s(city[i].original, sizeof(tmp_city[i]) + 1, tmp_city[i]);
		printf("%s", city[i].city_name);

		city[i].owner = 6;
		city[i].building = 0;
		city[i].price = arr_cityprice[i] * 10;
		city[i].take_over = 0;
	}
	for (i = 27, k = 0; i >= 22; i--) {   // <<���� ��������<< ���� ��ġ���� �� ǥ��, owner, ���� �ʱ�ȭ
		city[i].x = 102;
		city[i].y = 49 - k;
		k = k + 7;
		gotoxy(city[i].x, city[i].y - 1);
		strcpy_s(city[i].city_name, sizeof(arr_city[i]) + 1, arr_city[i]);
		strcpy_s(city[i].original, sizeof(tmp_city[i]) + 1, tmp_city[i]);
		printf("%s", city[i].city_name);

		city[i].owner = 6;
		city[i].building = 0;
		city[i].price = arr_cityprice[i] * 10;
		city[i].take_over = 0;
	}
	city[0].owner = 7; city[7].owner = 7; city[14].owner = 7; city[21].owner = 7;
	city[2].owner = 7; city[10].owner = 7; city[18].owner = 7; city[24].owner = 7;
	city[26].owner = 7;
	city->bail = 0;
}

void init_map() {
	int map[63][57] = {
		// 8 * 8 6*6 �����
		//�÷��̾� 1 

		   //1 =  ��  8 = ��  9 = �� 10 = �� 11 = �� 12 = ��  13  ��  14 ����  15�� 16 �� 17 �� 18 ��
		{15,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,16,0,15,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,16},
		{9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9},
		{17,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,18,0,17,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,18},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,12,8,8,8,8,8,8,10,8,8,8,8,8,8,10,8,8,8,8,8,8,10,8,8,8,8,8,8,10,8,8,8,8,8,8,10,8,8,8,8,8,8,12,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,8,8,8,8,8,8,12,8,8,8,8,8,8,11,8,8,8,8,8,8,11,8,8,8,8,8,8,11,8,8,8,8,8,8,11,8,8,8,8,8,8,11,8,8,8,8,8,8,12,8,8,8,8,8,8,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,9,0,0,0,0,0,0,1},
		{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
		{15,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,16,0,15,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,16},
		{9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9},
		{17,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,18,0,17,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,18}
		//1 == �׵θ�, 8,9 == ��輱, 2,3,4,5 = �ǹ� 6==��,7==�̸�
	};

	int col = sizeof(map[0]) / sizeof(2);  //2���� �迭�� ���� ũ�� = ���� ������ ũ�� / ����� ũ��
	int row = sizeof(map) / sizeof(map[0]); // ���� ũ�� = �迭 ��ü���� / ���� ���� ũ��

	print_map(map, row, col);
}

void print_map(int map[63][57], int row, int col) { //�� �ʱ�ȭ

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {

			if (map[i][j] == 0) {
				printf("  ");
			}
			else if (map[i][j] == 1) {
				printf("��");
			}
			else if (map[i][j] == 8) {
				printf("����");
			}
			else if (map[i][j] == 9) {
				printf("�� ");
			}
			else if (map[i][j] == 10) {
				printf("�� ");
			}
			else if (map[i][j] == 11) {
				printf("�� ");
			}
			else if (map[i][j] == 12) {
				printf("�� ");
			}
			else if (map[i][j] == 14) {
				printf("�� ");
			}
			else if (map[i][j] == 13) {
				printf("�� ");
			}
			else if (map[i][j] == 15) {
				printf("�� ");
			}
			else if (map[i][j] == 16) {
				printf("�� ");
			}
			else if (map[i][j] == 17) {
				printf("�� ");
			}
			else if (map[i][j] == 18) {
				printf("�� ");
			}
		}
		printf("\n");
	}
}

void init_player(Player* pl, int pl_num)//�÷��̾� �ʱ�ȭ ���� ������ ��
{
	for (int i = 0; i < pl_num; i++) {

		pl[i].cash = 3000;
		pl[i].money = 0 + pl[i].cash;
		pl[i].pos = 0;
		pl[i].symbolnum = i;
		pl[i].pause_move = 0;
		pl[i].goldkey = 2;

		switch (i) {
		case 0:   //�÷��̾�1
			pl[i].textcolor = MAGENTA2;   //�÷� ����
			textcolor(pl[0].textcolor, WHITE);   //�÷� �ҷ�����
			strcpy(pl[i].mysymbol, "��");   //�ɺ�����
			gotoxy(102, 57);  printf("%s", pl[i].mysymbol);   //������� �ɺ����
			gotoxy(2, 1); printf("player 1  %s", pl[i].mysymbol);
			gotoxy(28, 1); printf("��  �� : %d �� ��", pl[i].cash);
			break;
		case 1:   //�÷��̾�2
			pl[i].textcolor = GREEN;
			textcolor(pl[1].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "��");
			gotoxy(104, 57);  printf("%s", pl[i].mysymbol);   //������� �ɺ����
			gotoxy(60, 1); printf("player 2  %s", pl[i].mysymbol);
			gotoxy(86, 1); printf("��  �� : %d �� ��", pl[i].cash);
			break;
		case 2:   //�÷��̾�3
			pl[i].textcolor = BLUE;
			textcolor(pl[2].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "��");
			gotoxy(106, 57);  printf("%s", pl[i].mysymbol);   //������� �ɺ����
			gotoxy(2, 61); printf("player 3  %s", pl[i].mysymbol);
			gotoxy(28, 61); printf("��  �� : %d �� ��", pl[i].cash);

			break;
		case 3:   //�÷��̾�4
			pl[i].textcolor = RED;
			textcolor(pl[3].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "��");
			gotoxy(108, 57);  printf("%s", pl[i].mysymbol);   //������� �ɺ����
			gotoxy(60, 61); printf("player 4  %s", pl[i].mysymbol);
			gotoxy(86, 61); printf("��  �� : %d �� ��", pl[i].cash);
			break;
		}
	}
	textcolor(BLACK, WHITE);
}

void move_player(CityInfo city[], int dice_num, Player* pl, int pl_num)   //city��ǥ��, �ֻ��� ��, �÷��̾� ��ǥ, �÷��̾� 1,2,3,4
{
	for (int i = 0; i < dice_num; i++) { //�ֻ��� ������ŭ for��
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
		printf("  "); //����������� ����
		gotoxy(city[(pl[pl_num].pos + 1) % 28].x + pl_num * 2, city[(pl[pl_num].pos + 1) % 28].y + 1); //�÷��̾� ������ġ
		textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
		printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
		pl[pl_num].pos++; //�÷��̾� ������ġ + 1 = �÷��̾� ������ġ

		Sleep(400);
	}
	textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
}

void Arrive(CityInfo city[], Player* pl, int pl_num) {

	Nocursor();
	int answer = 0;

	if (pl[pl_num].pos == 10 || pl[pl_num].pos == 18 || pl[pl_num].pos == 24) {
		gotoxy(23, 15);   printf("Ȳ�ݿ��迡 �����ϼ̽��ϴ�.");
		system("pause>NULL");
		clear();
		Nocursor();
		ServerSend(city, pl);
		return;
	}

	if (pl[pl_num].pos / 28 >= 1) //�������
	{
		gotoxy(23, 15);   printf("������� �������ϴ�. ���� 300������ ���޵˴ϴ�.");
		system("pause>null");
		clear();
		pl[pl_num].cash += 300;
		pl[pl_num].pos = pl[pl_num].pos % 28;
		if (pl[pl_num].pos == 0) //�������
		{
			char buildup[20];
			gotoxy(23, 15);   printf("������� �����Ͽ����ϴ�.");
			gotoxy(23, 17);   printf("�ǹ��� ���� ���ø� �Է��ϼ��� : ");
			scanf("%s", buildup);

			for (int i = 0; i < 28; i++) {
				if (!strcmp(buildup, city[i].original)) {
					clear();
					if (city[i].owner == pl_num) {
						if (city[i].building == 5) {
							gotoxy(23, 17);   printf("���̻� �Ǽ��� �� �����ϴ�.");
							system("pause>NULL");
							return;
						}
						if (city[i].building == 4) {
							gotoxy(23, 17);   printf("���帶ũ�� �Ǽ��Ͻðڽ��ϱ�??");
							gotoxy(23, 19);   printf("1. ��  2. �ƴϿ� : ");
							getch();
							scanf("%d", &answer);

							if (answer == 1) {
								if (pl[pl_num].cash - city[i].price * 2 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
									gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
									system("pause>NULL");
									return;
								}
								else {
									clear();
									gotoxy(23, 17);   printf("���帶ũ�� �Ǽ��մϴ�.");
									system("pause>NULL");
									city[i].building = 5;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 2;
									gotoxy(city[i].x - 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("LAND MARK");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear();
								}

							}
							else if (answer == 2) {
								gotoxy(23, 15);   printf("������ ������");
								system("pause>NULL");
								clear();
							}
							else {
								gotoxy(23, 15);   printf("�߸��Է��ϼ̱�����");
								system("pause>NULL");
								clear();

							}
						}
						else {
							gotoxy(23, 17); printf("���� �ǹ��� �������ּ���.");
							gotoxy(23, 19);   printf("1. ���� 2. ���� 3. ȣ�� 4. �Ǽ�����! : ");
							system("pause>NULL");
							scanf("%d", &answer);
							switch (answer) {
							case 1:
								if (city[i].building == 2) {
									clear();
									gotoxy(23, 15);   printf("�̹� ������ �������ֽ��ϴ�. �ٽ� �������ּ���!");
									system("pause>NULL");
									clear();
									break;
								}
								if (pl[pl_num].cash - city[i].price * 0.5 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
									gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("������ �������ϴ�!!");
									system("pause>NULL");
									city[i].building = 2;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 0.5;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("��");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear();
									break;
								}
							case 2:
								if (city[i].building == 3) {
									clear();
									gotoxy(23, 15);   printf("�̹� ������ �������ֽ��ϴ�. �ٽ� �������ּ���!");
									system("pause>NULL");
									clear();
									break;
								}
								if (pl[pl_num].cash - city[i].price * 1.2 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
									gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("������ �������ϴ�!!");
									system("pause>NULL");
									city[i].building = 3;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 1.2;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("�֣�");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear(); break;
								}
							case 3:
								if (pl[pl_num].cash - city[i].price * 1.8 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
									gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("ȣ���� �������ϴ�!!");
									system("pause>NULL");
									city[i].building = 4;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 1.8;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("�֣£�");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear();
									break;
								}
							case 4:
								clear();
								gotoxy(23, 15);   printf("�Ǽ����մϴ�!!");
								system("pause>NULL");
								clear();
								break;
							default:
								clear();
								gotoxy(23, 15);   printf("�߸��Է��ϼ̱�����");
								system("pause>NULL");
								clear();
								break;
							}

						}
					}
					else {
						clear();
						gotoxy(23, 15);   printf("������ �����ڰ� �ƴմϴ�. ��ȸ�� �����̽��ϴ�.");
						system("pause>NULL");
						return;
					}
				}
			}
			ServerSend(city, pl);
			return;
		}
		else {
			ServerSend(city, pl);
			return;
		}
	}

	if (pl[pl_num].pos == 2) {
		gotoxy(29, 15);   printf("��ũȦ�̴�!!!!!!");
		gotoxy(29, 17);   printf("���ΰ��ɰž�!!!!!!!!!!!");
		system("pause>NULL");
		clear();
		srand(time(NULL));
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
		printf("  "); //����������� ����
		pl[pl_num].pos = rand() % 28;
		gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
		textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
		printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
		textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
		ServerSend(city, pl);
		Arrive(city, pl, pl_num);
		return;
	}

	if (pl[pl_num].pos == 21) { //KTX
		Ktx(city, pl, pl_num);
		ServerSend(city, pl);
		return;
	}

	if (pl[pl_num].pos == 14) {
		int game_sel;
		gotoxy(23, 15); printf("�������忡 �����Ͽ����ϴ�.");
		gotoxy(23, 17); printf("���� ������� 100���� �Դϴ�. ���� �Ͻðڽ��ϱ�?");
		gotoxy(23, 19); printf("1. �����ϰڽ��ϴ�.");
		gotoxy(23, 21); printf("2. �ƴϿ�. �������� �ʰڽ��ϴ�.");
		gotoxy(23, 23); printf("���� >>> ");
		getch();
		gotoxy(32, 23); scanf("%d", &game_sel);
		clear();
		while (1) {
			switch (game_sel) {
			case 1:
				gotoxy(23, 17); printf("���ϴ� ������ �����ϼ��� : ");
				gotoxy(23, 19); printf("1.����������");
				gotoxy(23, 21); printf("2.������");
				gotoxy(50, 17); scanf("%d", &game_sel);
				clear();
				while (1) {
					switch (game_sel) {
					case 1:
						game_rsp(city, pl, pl_num); break;
					case 2:
						Chamchamcham(city, pl, pl_num); break;
					default:
						gotoxy(23, 23); printf("�߸� �����ϼ̽��ϴ�. �ٽ� �����ϼ���.");
					}
					clear();
					return;
				}
			case 2:
				return;
			default:
				gotoxy(23, 23); printf("�߸� �����ϼ̽��ϴ�. �ٽ� �����ϼ���.");
			}
		}
	}

	if (pl[pl_num].pos == 7) {
		int prison_choice;
		gotoxy(23, 15); printf("�����ҿ� �����Ͽ����ϴ�.");
		if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17);
			printf("������ Ż��ī�带 �����ϰ��ʴϴ�. ����Ͻðٽ��ϱ�?");
			gotoxy(23, 19);
			printf("1. �� 2. �ƴϿ� : ");
			gotoxy(42, 19);
			getch();
			scanf("%d", &prison_choice);
			clear();
			switch (prison_choice)
			{
			case 1:
				gotoxy(23, 15); printf("������ Ż����� ����մϴ�...");
				gotoxy(23, 17); printf("�����Ϻ��� Ż���մϴ�.");
				system("pause>NULL");
				pl[pl_num].pause_move = 0;
				pl[pl_num].goldkey = 0;
				clear();
				break;
			case 2:
				gotoxy(23, 16); printf("������ Ż����� ��������ʽ��ϴ�...");
				clear();
				break;
			default:
				printf("�߸��Է��ϼ̽��ϴ�. ����������.....");
			}
		}
		else {
			gotoxy(23, 17); printf("3�ϰ� �̵��� ���ѵ˴ϴ�.");
			gotoxy(23, 19); printf("Ż��õ��� ���� �� ���� �����մϴ�.");
			system("pause>NULL");
			clear();
			pl[pl_num].pause_move = 3;
			return;
		}
		return;
	}

	if (pl[pl_num].pos == 26) {
		if (city->bail == 0) {
			gotoxy(23, 15); printf("������ �������� �����ϴ�.");
			system("pause>NULL");
			clear();
			return;
		}
		else {
			clear();
			gotoxy(23, 15);   printf("�������� �޽��ϴ�.");
			gotoxy(23, 17);   printf("�����ݾ��� %d���� �Դϴ�.", city->bail);   //�����ݾ� ���� ����
			pl[pl_num].cash += city->bail;
			system("pause>NULL");
			player_update(city, pl);
			city->bail = 0;
			clear();
			return;
		}
	}

	if (city[pl[pl_num].pos].owner == 6) { // ���õ��� �� ���� ������
		gotoxy(23, 15);   printf("\"%s\"�� �����Ͽ����ϴ�. \"%s\"�� ������ %d���� �Դϴ�.", city[pl[pl_num].pos].city_name, city[pl[pl_num].pos].city_name, city[pl[pl_num].pos].price);
		gotoxy(23, 17);   printf("���ø� �����Ͻðڽ��ϱ�?");
		gotoxy(23, 19);   printf("1. �� 2. �ƴϿ� : ");
		_getch();
		scanf("%d", &answer);

		if (answer == 1) { //������
			if (pl[pl_num].cash - city[pl[pl_num].pos].price < 0) {
				clear();
				gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
				gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
				system("pause>NULL");
			}
			else {
				clear();
				pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price;
				player_update(city, pl);
				gotoxy(23, 15);   printf("���ø� �����Ͽ����ϴ�!!");
				gotoxy(23, 17);   system("pause>null");
				//�÷��̾� ���� ������Ʈ.
				city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
				city[pl[pl_num].pos].building = 1;
				gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
				textcolor(pl[pl_num].textcolor, WHITE);
				printf("%s", city[pl[pl_num].pos].city_name);
				textcolor(BLACK, WHITE);
				clear();
			}
		}

		else if (answer == 2) {
			clear();
			gotoxy(23, 15);   printf("�ǹ��� �������� �ʴٴ� ���������̱�..");
			system("pause>null");
			clear();
		}

		else {
			clear();
			gotoxy(23, 15);   printf("�߸��Է��ϼ̱�����");
			system("pause>null");
			clear();
		}
		ServerSend(city, pl);
	}

	if (city[pl[pl_num].pos].owner == pl[pl_num].symbolnum) {  //�� �����϶�
		if (pl[pl_num].pos == 4 || pl[pl_num].pos == 8 || pl[pl_num].pos == 12 || pl[pl_num].pos == 16 || pl[pl_num].pos == 23) {
			return;
		}

		gotoxy(23, 15);   printf("\"%s\"�� ����� ���Դϴ�.", city[pl[pl_num].pos].city_name);
		if (city[pl[pl_num].pos].building == 5) {
			gotoxy(23, 17);   printf("���̻� �Ǽ��� �� �����ϴ�.");
			system("pause>NULL");
			return;
		}

		if (city[pl[pl_num].pos].building == 4) {
			gotoxy(23, 17);   printf("���帶ũ�� �Ǽ��Ͻðڽ��ϱ�??");
			gotoxy(23, 19);   printf("1. ��  2. �ƴϿ� :");
			getch();
			scanf("%d", &answer);
			if (answer == 1) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 2 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					return;
				}
				else {
					clear();
					gotoxy(23, 17);   printf("���帶ũ�� �Ǽ��մϴ�.");
					system("pause>NULL");
					city[pl[pl_num].pos].building = 5;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 2;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("LAND MARK");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear();
				}

			}
			else if (answer == 2) {
				gotoxy(23, 15);   printf("������ ������");
				system("pause>NULL");
				clear();
			}
			else {
				gotoxy(23, 15);   printf("�߸��Է��ϼ̱�����");
				system("pause>NULL");
				clear();

			}
		}
		else {
			gotoxy(23, 17); printf("���� �ǹ��� �������ּ���.");
			gotoxy(23, 19);   printf("1. ���� 2. ���� 3. ȣ�� 4. �Ǽ�����! : ");
			scanf("%d", &answer);
			switch (answer) {
			case 1:
				if (city[pl[pl_num].pos].building == 2) {
					clear();
					gotoxy(23, 15);   printf("�̹� ������ �������ֽ��ϴ�. �ٽ� �������ּ���!");
					system("pause>NULL");
					clear();
					break;
				}
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 0.5 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("������ �������ϴ�!!");
					system("pause>NULL");
					city[pl[pl_num].pos].building = 2;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 0.5;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("��");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear();
					break;
				}
			case 2:
				if (city[pl[pl_num].pos].building == 3) {
					clear();
					gotoxy(23, 15);   printf("�̹� ������ �������ֽ��ϴ�. �ٽ� �������ּ���!");
					system("pause>NULL");
					clear();
					break;
				}
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 1.2 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("������ �������ϴ�!!");
					gotoxy(23, 17);   system("pause>null");
					city[pl[pl_num].pos].building = 3;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 1.2;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("�֣�");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear(); break;
				}
			case 3:
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 1.8 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. ���Ű�  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("ȣ���� �������ϴ�!!");
					system("pause>NULL");
					city[pl[pl_num].pos].building = 4;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 1.8;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("�֣£�");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear();
					break;
				}
			case 4:
				clear();
				gotoxy(23, 15);   printf("�Ǽ����մϴ�!!");
				system("pause>NULL");
				clear();
				break;
			default:
				clear();
				gotoxy(23, 15);   printf("�߸��Է��ϼ̱�����");
				system("pause>NULL");
				clear();
				break;
			}
		}
		ServerSend(city, pl);
		clear();
		return;
	}

	if (city[pl[pl_num].pos].owner != pl[pl_num].symbolnum && city[pl[pl_num].pos].owner != 6) { // �ٸ��÷��̾� ���϶�
		gotoxy(23, 15);   printf("����Ḧ �����ؾ��մϴ�.");
		system("pause>NULL");
		clear();
		if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 15);   printf("õ��ī�带 �����ϰ� ��ʴϴ�. ����Ͻðڽ��ϱ��?");
			gotoxy(23, 17);   printf("1. ��   2. �ƴϿ� : ");
			scanf("%d", &answer);
			clear();
			if (answer == 1) {
				gotoxy(23, 15);   printf("õ��ī�带 ����մϴ�. ����ᰡ �����˴ϴ�.");
				system("pause>NULL");
				pl[pl_num].goldkey = 0;
				Buy_City(city, pl, pl_num);
				return;
			}
			else {
				clear();
				gotoxy(23, 15);   printf("õ��ī�带 ������� �ʽ��ϴ�.");
				gotoxy(23, 17);   printf("����Ḧ �����մϴ�.");
				system("pause>NULL");
			}
		}

		else {
			switch (city[pl[pl_num].pos].building) {
			case 1:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price / 10) >= 0) {

					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price / 10;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price / 10;
					player_update(city, pl);
					gotoxy(23, 15);   printf("�����(%d����)�� �����Ͽ����ϴ�.", city[pl[pl_num].pos].price / 10);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("����Ḧ �� ���� �����ϴ�. ������ �Ű��ؾ��մϴ�.");
					system("pause>NULL");
					clear();
					Sale_City(city, pl, pl_num);
				}
				break;
			case 2:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price / 2) >= 0) {

					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price / 2;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price / 2;
					player_update(city, pl);
					gotoxy(23, 15);   printf("�����(%d����)�� �����Ͽ����ϴ�.", city[pl[pl_num].pos].price / 2);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("����Ḧ �� ���� �����ϴ�. ������ �Ű��ؾ��մϴ�.");
					system("pause>NULL");
					clear();
					Sale_City(city, pl, pl_num);
				}
				break;
			case 3:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price * 2) >= 0) {

					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 2;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price * 2;
					player_update(city, pl);
					gotoxy(23, 15);   printf("�����(%d����)�� �����Ͽ����ϴ�.", (city[pl[pl_num].pos].price * 2));
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("����Ḧ �� ���� �����ϴ�. ������ �Ű��ؾ��մϴ�.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			case 4:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price * 3) >= 0) {

					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 3;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price * 3;
					player_update(city, pl);
					gotoxy(23, 15); printf("�����(%d����)�� �����Ͽ����ϴ�.", city[pl[pl_num].pos].price * 3);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("����Ḧ �� ���� �����ϴ�. ������ �Ű��ؾ��մϴ�.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			case 5:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price * 4) >= 0) {
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 4;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price * 4;
					player_update(city, pl);
					gotoxy(23, 15); printf("�����(%d����)�� �����Ͽ����ϴ�.", city[pl[pl_num].pos].price * 4);
					system("pause>NULL");
				}
				else {
					gotoxy(23, 15);   printf("����Ḧ �� ���� �����ϴ�. ������ �Ű��ؾ��մϴ�.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			default:
				gotoxy(23, 15);   printf("����");
				system("pause>NULL");
				clear();
			}
		}
		ServerSend(city, pl);
		clear();
	}
	clear();
	return;
}

void Arrive2(CityInfo city[], Player* pl, Play play, int pl_num) {
	clear();
	gotoxy(23, 15); printf("�÷��̾�%d ������...", play.turn + 1);
	clear();
	if (pl[play.turn].pos == 10 || pl[play.turn].pos == 18 || pl[play.turn].pos == 24) {
		ServerRecv(city, pl);
		return;
	}

	if (pl[play.turn].pos == 2) {
		ServerRecv(city, pl);
		Arrive2(city, pl, play, pl_num);
		return;
	}
	if (pl[play.turn].pos == 7 || pl[play.turn].pos == 14 || pl[play.turn].pos == 26) {
		return;
	}

	if (pl[play.turn].pos / 28 >= 1) {
		ServerRecv(city, pl);
		return;
	}

	if (pl[play.turn].pos == 21) {
		ServerRecv(city, pl);
		Arrive2(city, pl, play, pl_num);
		return;
	}

	if (city[pl[play.turn].pos].owner == 6) {
		ServerRecv(city, pl);
	}

	if (city[pl[play.turn].pos].owner == pl[play.turn].symbolnum) {
		if (pl[play.turn].pos == 4 || pl[play.turn].pos == 8 || pl[play.turn].pos == 12 || pl[play.turn].pos == 16 || pl[play.turn].pos == 23) {
			return;
		}
		else {
			if (city[pl[play.turn].pos].building == 5) {
				return;
			}
			ServerRecv(city, pl);
			return;
		}

	}
	if (city[pl[play.turn].pos].owner != pl[play.turn].symbolnum && city[pl[play.turn].pos].owner != 6) {
		ServerRecv(city, pl);
		return;
	}
}

void Buy_City(CityInfo city[], Player* pl, int pl_num) {
	if (pl[pl_num].pos == 4 || pl[pl_num].pos == 8 || pl[pl_num].pos == 12 || pl[pl_num].pos == 16 || pl[pl_num].pos == 23)
		return;
�μ�:
	if (city[pl[pl_num].pos].building == 5) {
		gotoxy(23, 15);   printf("���帶ũ�� �μ��� �� �����ϴ�.");
		system("pause>NULL");
		clear();
		return;
	}
	else {
		int answer = 0;
		clear();
		gotoxy(23, 15);
		printf("���ø� �μ��մϴ�. ���ø� �μ��Ͻðڽ��ϱ�??");
		gotoxy(23, 17);
		if (city[pl[pl_num].pos].building == 1) {
			printf("�� ������ �μ������ %d���� �Դϴ�.", city[pl[pl_num].pos].price * 1);
		}
		else if (city[pl[pl_num].pos].building == 2) {
			printf("�� ������ �μ������ %d���� �Դϴ�.", city[pl[pl_num].pos].price * 2);
		}
		else if (city[pl[pl_num].pos].building == 3) {
			printf("�� ������ �μ������ %d���� �Դϴ�.", city[pl[pl_num].pos].price * 3);
		}
		else if (city[pl[pl_num].pos].building == 4) {
			printf("�� ������ �μ������ %d���� �Դϴ�.", city[pl[pl_num].pos].price * 4);
		}
		else {
			gotoxy(23, 15);   printf("���帶ũ�� �μ��� �� �����ϴ�.");
			system("pause>NULL");
			clear();
		}
		gotoxy(23, 19);
		printf("1. ��   2.�ƴϿ� : ");
		scanf("%d", &answer);
		clear();
		switch (answer) {
		case 1:

			gotoxy(23, 15);	printf("���ø� �μ��մϴ�.");
			system("pause>null");
			clear();
			if (city[pl[pl_num].pos].building == 1) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. �μ���  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("�μ���� (%d����)�� �����ϰ� ���ø� �μ��մϴ�.", city[pl[pl_num].pos].price);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price;
					pl[pl_num].cash -= city[pl[pl_num].pos].price;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 1;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					gotoxy(23, 15);
					clear();
					Arrive(city, pl, pl_num);
					break;
				}
			}
			else if (city[pl[pl_num].pos].building == 2) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 2 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. �μ���  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("�μ���� (%d����)�� �����ϰ� ���ø� �μ��մϴ�.", city[pl[pl_num].pos].price * 2);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 2;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 2;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 2;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("��");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					gotoxy(23, 15);
					clear();
					Arrive(city, pl, pl_num);
					break;
				}
			}
			else if (city[pl[pl_num].pos].building == 3) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 3 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. �μ���  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("�μ���� (%d����)�� �����ϰ� ���ø� �μ��մϴ�.", city[pl[pl_num].pos].price * 3);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 3;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 3;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 3;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("�֣�");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					gotoxy(23, 15);
					clear();
					Arrive(city, pl, pl_num);
					break;
				}
			}
			else if (city[pl[pl_num].pos].building == 4) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 4 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money�� �����մϴ�. �μ���  Impossible...T^T..");
					gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
					system("pause>NULL");
					break;
				}

				else {
					gotoxy(23, 15);   printf("�μ���� (%d����)�� �����ϰ� ���ø� �μ��մϴ�.", city[pl[pl_num].pos].price * 4);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 4;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 4;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 4;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("�֣£�");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					gotoxy(23, 15);
					clear();
					Arrive(city, pl, pl_num);
					break;
				}
			}
			else {
				clear();
				gotoxy(23, 15);   printf("�߸��� �����Դϴ�.");
				gotoxy(23, 17);   printf("���������� �Ѿ�ϴ�.");
				system("pause>NULL");
				break;
			}
		case 2:
			gotoxy(23, 15);   printf("���ø� �μ����� �ʽ��ϴ�.");
			system("pause>NULL");
			clear();
			break;
		default:
			gotoxy(23, 15);   printf("�߸� �����ϼ̽��ϴ�. �ٽ� �Է����ּ���.");
			system("pause>NULL");
			clear();
			goto �μ�;
		}
		return;
	}
}

void Sale_City(CityInfo city[], Player* pl, int pl_num) {

	clear();
	int count = 0;
	char mg[20] = { 0, };
maegak:
	clear(); gotoxy(23, 15); printf("(�Ļ� �Է½� �Ļ�, �׸� �Է½� �Ű�����)");
	gotoxy(23, 17); printf("�Ű��� ���ø� �Է��ϼ��� : "); scanf("%s", mg);
	if (!strcmp(mg, "�׸�")) {
		gotoxy(23, 15); printf("�Ű��� �����մϴ�.");
		system("pause>NULL");
		Arrive(city, pl, pl_num);
		return;
	}
	if (!strcmp(mg, "�Ļ�")) {
		pl[pl_num].cash = -999;
		Bankruptcy(city, pl, pl_num);
		return;
	}
	for (int i = 0; i < 28; i++) {
		if (!strcmp(mg, city[i].original)) {
			if (city[i].owner == pl[pl_num].symbolnum) {
				clear();
				gotoxy(23, 15); printf("������ ���� %s�� �Ű��մϴ�.", city[i].original);
				system("pause>NULL");
				switch (city[i].building) {
				case 1:
					pl[pl_num].cash += city[i].price; break;
				case 2:
					pl[pl_num].cash += city[i].price * 0.5; break;
				case 3:
					pl[pl_num].cash += city[i].price * 1.2; break;
				case 4:
					pl[pl_num].cash += city[i].price * 1.8; break;
				case 5:
					pl[pl_num].cash += city[i].price * 2; break;
				}
				city[i].building = 0;
				city[i].owner = 6;
				gotoxy(city[i].x, city[i].y - 1);
				textcolor(BLACK, WHITE);
				printf("%s", city[i].city_name);
				gotoxy(city[i].x - 1, city[i].y - 3);
				printf("          ");
				player_update(city, pl);
				goto maegak;
			}
			else {
				gotoxy(23, 15);
				printf("\"%s\"�� �� ���ð� �ƴմϴ�.", mg);
				gotoxy(23, 17);
				system("pause>null");
				goto maegak;
			}
			return;
		}
	}
}

void player_update(CityInfo city[], Player* pl) {//�÷��̾� 1234

	for (int i = 0; i < pixed_pl_num; i++) {
		switch (i) {
		case 0:   //�÷��̾�1
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);   //�÷� �ҷ�����
			gotoxy(2, 1); printf("player 1  %s", pl[i].mysymbol);   //���߿� ������Ʈ �Լ��� ����
			gotoxy(28, 1); printf("��  �� : %d �� ��", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 1:   //�÷��̾�2
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(60, 1); printf("player 2  %s", pl[i].mysymbol);
			gotoxy(86, 1); printf("��  �� : %d �� ��", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 2:   //�÷��̾�3
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(2, 61); printf("player 3  %s", pl[i].mysymbol);
			gotoxy(28, 61); printf("��  �� : %d �� ��", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 3:   //�÷��̾�4
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(60, 61); printf("player 4  %s", pl[i].mysymbol);
			gotoxy(86, 61); printf("��  �� : %d �� ��", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		}
	}
}

void Ktx(CityInfo city[], Player* pl, int pl_num)   //city��ǥ��, ������ city, ������ player
{
	int i;
	char destination[20];

	if (pl[pl_num].pos == 21) {   //pos�� KTX(=21)�� ���ǹ� ����
	KTX:
		clear(); gotoxy(23, 15); printf("KTX�� �����ϴ�. ���� ���� �Է��ϼ��� : "); _getch();
		Cursor(); scanf("%s", destination); clear();
		if (!strcmp(destination, "KTX")) { //KTX ��� �̿� ����
			gotoxy(23, 17); printf("KTX�� �������� ������ �� �����ϴ�."); getch();
			goto KTX;
		}
		for (i = 0; i < 28; i++) {
			if (!strcmp(destination, city[i].original)) {
				if (i == 10) {
					int sel;
					Cursor(); clear();
					gotoxy(23, 15); printf("�̵��� Ȳ�ݿ��� ������ �����ϼ���.");
					gotoxy(23, 17);   printf("1.���� 2.���� 3.������ : ");
					scanf("%d", &sel);
					Nocursor();
					if (sel == 1)
						move_player(city, 28 - (pl[pl_num].pos) + i, pl, pl_num);
					else if (sel == 2)
						move_player(city, 28 - (pl[pl_num].pos) + 18, pl, pl_num);
					else if (sel == 3)
						move_player(city, 24 - (pl[pl_num].pos), pl, pl_num);
					break;
				}
				else {
					Nocursor();
					if (pl[pl_num].pos < i)
						move_player(city, i - (pl[pl_num].pos), pl, pl_num);   //KTX ������ ���ö��
					else
						move_player(city, 28 - (pl[pl_num].pos) + i, pl, pl_num);   //KTX ������ ���ö��
					break;
				}
			}
		}
		if (i == 28) {   //�߸��Է����� ��
			clear();
			gotoxy(23, 15);
			printf("�������� �߸� ���ؼ� KTX�� ���ƽ��ϴ�.");
			gotoxy(23, 17);
			printf("���͸� ������ ���� ������ �����մϴ�...");
			getch();
			goto KTX;
		}
		gotoxy(23, 17);   printf("KTX�� Ÿ�� %s�� �Խ��ϴ�.", city[pl[pl_num].pos % 28].city_name);
		system("pause>NULL");
		clear();
		Arrive(city, pl, pl_num);
		Nocursor();
	}
}

void Prison(CityInfo city[], Player* pl, int pl_num) {
	int bail_sel;
	int dice_num;
	getchar();
	gotoxy(23, 15); printf("������ �ƴϳ׿�. ������ 150������ ���� �̵��Ͻðڽ��ϱ�? ");
	gotoxy(23, 17); printf("1. ��. �����ϰڽ��ϴ�.");
	gotoxy(23, 19); printf("2. �ƴϿ�.");
	gotoxy(23, 21); printf("���� >>> ");
	gotoxy(32, 21); scanf("%d", &bail_sel);
	clear();
	while (1) {
		switch (bail_sel) {
		case 1:
			pl[pl_num].pause_move = 0;
			pl[pl_num].cash -= 150;
			player_update(city, pl, pl_num);
			city->bail += 150;
			break;
		case 2:
			gotoxy(23, 15); printf("������ ������ �����Ҹ� ������ �� �ֽ��ϴ�."); //ȭ�鿡 ��µǴ��� Ȯ��
			gotoxy(23, 17); printf("�ֻ����� �����ּ���.");
			system("pause>NULL");
			break;
		default:
			printf(23, 15); printf("�߸� �����ϼ̽��ϴ�. �ٽ� �����ϼ���.");
			system("pause>NULL");
		}
		clear();
		return;
	}
}

void Bankruptcy(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("���� �����ؼ� �Ļ� ó�� �˴ϴ�.");
	system("pause>NULL");
	for (int i = 0; i < 28; i++) {
		if (city[i].owner == pl_num) {
			clear();
			city[i].building = 0;
			city[i].owner = 6;
			gotoxy(city[i].x, city[i].y - 1);
			textcolor(BLACK, WHITE);
			printf("%s", city[i].city_name);
			gotoxy(city[i].x - 1, city[i].y - 3);
			printf("        ");
			gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1); //�÷��̾��� ������ġ
			printf("  "); //����������� ����
		}
	}
	switch (pl_num) {
	case 0:
		gotoxy(2, 1); printf("                      ");
		gotoxy(28, 1); printf("                    ");
		break;
	case 1:
		gotoxy(60, 1); printf("                     ");
		gotoxy(86, 1); printf("                    ");
		break;
	case 2:
		gotoxy(2, 61); printf("                     ");
		gotoxy(28, 61); printf("                   ");
		break;
	case 3:
		gotoxy(60, 61); printf("                    ");
		gotoxy(86, 61); printf("                   ");
		break;
	}
	pl_cnt++;
}

//�������� �̴ϰ��� ���� �Լ�
void game_rsp(CityInfo city[], Player* pl, int pl_num) {
	int user, com;
	srand(time(NULL));
	gotoxy(26, 15); printf("### ���� ���� �� ���� ###\n");
	gotoxy(26, 17); printf("�����ϼ���. (1.���� 2.���� 3.��) : ");
�ٽ��Է�:
	scanf("%d", &user);
	Nocursor();
	while (1) {
		printf_rsp(user);
		com = printf_rsp2(rand() % 3 + 1);
		if (_kbhit()) {
			if (user < 1 || user>3) {
				gotoxy(43, 45); printf("�߸� �Է��ϼ̽��ϴ�.\n"); goto �ٽ��Է�;
			}
			else if (com - user == 0) {
				gotoxy(43, 45); printf("������ϱ� ���ٰ� ��!!!"); break;
			}
			else if (com - user == 1 || com - user == -2) {
				gotoxy(39, 45); printf("100���� �ܲ� ���̵� �ƾ�~�� ���ܶ�!!!");
				pl[pl_num].cash -= 100;
				player_update(city, pl);
				break;
			}
			else {
				gotoxy(43, 45); printf("���� ����.. ������...��");
				pl[pl_num].cash += 500;
				player_update(city, pl);
				break;
			}
			clear();
		}
		Sleep(70);
	}
	system("pause>NULL");
	clear();
}

int printf_rsp(int rsp_user) {
	gotoxy(29, 38);
	printf("[���� �ǹ���]");
	textcolor(BLUE, WHITE);
	if (rsp_user == 1) {
		gotoxy(23, 20); printf("         ###         ##              ");
		gotoxy(23, 21); printf("        #  :#      #   #             ");
		gotoxy(23, 22); printf("        #   #:    #   #              ");
		gotoxy(23, 23); printf("       :#   :#  :#   #               ");
		gotoxy(23, 24); printf("       :#   :#: #  :#                ");
		gotoxy(23, 25); printf("        #    # #    #                ");
		gotoxy(23, 26); printf("        #    ##    #                 ");
		gotoxy(23, 27); printf(" ..:#########....  :#                ");
		gotoxy(23, 28); printf("####.   #     ####### #              ");
		gotoxy(23, 29); printf("#   #    #.   ##.       #            ");
		gotoxy(23, 30); printf("#.  #     #     ###      #           ");
		gotoxy(23, 31); printf("#    #    #.     ##      #           ");
		gotoxy(23, 32); printf(" #    #    #      #     #            ");
		gotoxy(23, 33); printf("  ###########:####.    #             ");
		gotoxy(23, 34); printf("   #                  #              ");
		gotoxy(23, 35); printf("    #               #                ");
		gotoxy(23, 36); printf("     #             #                 ");
	}
	else if (rsp_user == 2) {
		gotoxy(23, 20); printf("\t   ############:                   ");
		gotoxy(23, 21); printf("  #####   ###    #######             ");
		gotoxy(23, 22); printf("##    #: ###    ##      ##           ");
		gotoxy(23, 23); printf("#:   #:    ##########   ##           ");
		gotoxy(23, 24); printf("#    #    ###        #####           ");
		gotoxy(23, 25); printf("#    #    ###:         ###           ");
		gotoxy(23, 26); printf("#    #    :###::        :##          ");
		gotoxy(23, 27); printf(":#   :#    #########     ##          ");
		gotoxy(23, 28); printf("##:   #:    ###  ##:      #          ");
		gotoxy(23, 29); printf("##### #############       #          ");
		gotoxy(23, 30); printf("##### ###########         #          ");
		gotoxy(23, 31); printf("#            ###         #           ");
		gotoxy(23, 32); printf("#             #        :#            ");
		gotoxy(23, 33); printf(" #                    :#             ");
		gotoxy(23, 34); printf("  #                  :#              ");
		gotoxy(23, 35); printf("   #                :#               ");
		gotoxy(23, 36); printf("    #              :#                ");
	}
	else {

		gotoxy(18, 20); printf("                  ###               ");
		gotoxy(18, 21); printf("         ##      #   #      :##     ");
		gotoxy(18, 22); printf("        #  #:    #   #    :#   #    ");
		gotoxy(18, 23); printf("        #  #:   :#   #:  :#   #     ");
		gotoxy(18, 24); printf(" ##:    #   #   #    #: #    #      ");
		gotoxy(18, 25); printf("#   #   #   #   #    # #    #       ");
		gotoxy(18, 26); printf("#   #   #   #: :#    ##    #   :##  ");
		gotoxy(18, 27); printf(" #   #  #   :# #      #    #  #   # ");
		gotoxy(18, 28); printf(" :#   # :#    #           #  #   :# ");
		gotoxy(18, 29); printf("   #   :#                  ##   :#  ");
		gotoxy(18, 30); printf("    #:                         :#   ");
		gotoxy(18, 31); printf("     #:                      :#     ");
		gotoxy(18, 32); printf("      #:                    :#:     ");
		gotoxy(18, 33); printf("       #:                  :#       ");
		gotoxy(18, 34); printf("       :#:                :#        ");
		gotoxy(18, 35); printf("        :#:              :#         ");
		gotoxy(18, 36); printf("          :#             #:         ");

	}
	textcolor(BLACK, WHITE);
}

int printf_rsp2(int rsp_com) {
	gotoxy(70, 38);
	printf("[���� �Ǹ�]");
	textcolor(RED, WHITE);
	if (rsp_com == 1) {
		gotoxy(60, 20); printf("         ###         ##              ");
		gotoxy(60, 21); printf("        #  :#      #   #             ");
		gotoxy(60, 22); printf("        #   #:    #   #              ");
		gotoxy(60, 23); printf("       :#   :#  :#   #               ");
		gotoxy(60, 24); printf("       :#   :#: #  :#                ");
		gotoxy(60, 25); printf("        #    # #    #                ");
		gotoxy(60, 26); printf("        #    ##    #                 ");
		gotoxy(60, 27); printf(" ..:#########....  :#                ");
		gotoxy(60, 28); printf("####.   #     ####### #              ");
		gotoxy(60, 29); printf("#   #    #.   ##.       #            ");
		gotoxy(60, 30); printf("#.  #     #     ###      #           ");
		gotoxy(60, 31); printf("#    #    #.     ##      #           ");
		gotoxy(60, 32); printf(" #    #    #      #     #            ");
		gotoxy(60, 33); printf("  ###########:####.    #             ");
		gotoxy(60, 34); printf("   #                  #              ");
		gotoxy(60, 35); printf("    #               #                ");
		gotoxy(60, 36); printf("     #             #                 ");
	}
	else if (rsp_com == 2) {
		gotoxy(60, 20); printf("     ############:                   ");
		gotoxy(60, 21); printf("  #####   ###    #######             ");
		gotoxy(60, 22); printf("##    #: ###    ##      ##           ");
		gotoxy(60, 23); printf("#:   #:    ##########   ##           ");
		gotoxy(60, 24); printf("#    #    ###        #####           ");
		gotoxy(60, 25); printf("#    #    ###:         ###           ");
		gotoxy(60, 26); printf("#    #    :###::        :##          ");
		gotoxy(60, 27); printf(":#   :#    #########     ##          ");
		gotoxy(60, 28); printf("##:   #:    ###  ##:      #          ");
		gotoxy(60, 29); printf("##### #############       #          ");
		gotoxy(60, 30); printf("##### ###########         #          ");
		gotoxy(60, 31); printf("#            ###         #           ");
		gotoxy(60, 32); printf("#             #        :#            ");
		gotoxy(60, 33); printf(" #                    :#             ");
		gotoxy(60, 34); printf("  #                  :#              ");
		gotoxy(60, 35); printf("   #                :#               ");
		gotoxy(60, 36); printf("    #              :#                ");
	}
	else {
		gotoxy(60, 20); printf("                  ###               ");
		gotoxy(60, 21); printf("         ##      #   #      :##     ");
		gotoxy(60, 22); printf("        #  #:    #   #    :#   #    ");
		gotoxy(60, 23); printf("        #  #:   :#   #:  :#   #     ");
		gotoxy(60, 24); printf(" ##:    #   #   #    #: #    #      ");
		gotoxy(60, 25); printf("#   #   #   #   #    # #    #       ");
		gotoxy(60, 26); printf("#   #   #   #: :#    ##    #   :##  ");
		gotoxy(60, 27); printf(" #   #  #   :# #      #    #  #   # ");
		gotoxy(60, 28); printf(" :#   # :#    #           #  #   :# ");
		gotoxy(60, 29); printf("   #   :#                  ##   :#  ");
		gotoxy(60, 30); printf("    #:                         :#   ");
		gotoxy(60, 31); printf("     #:                      :#     ");
		gotoxy(60, 32); printf("      #:                    :#:     ");
		gotoxy(60, 33); printf("       #:                  :#       ");
		gotoxy(60, 34); printf("       :#:                :#        ");
		gotoxy(60, 35); printf("        :#:              :#         ");
		gotoxy(60, 36); printf("          :#             #:         ");
	}
	textcolor(BLACK, WHITE);
	return rsp_com;
}

int Chamchamcham(CityInfo city[], Player* pl, int pl_num) {
	int user, com;
	srand(time(NULL));
�ٽ��Է�:
	gotoxy(26, 15); printf("### ������ ###\n");
	gotoxy(26, 17); printf("�����ϼ���. (1. ��   2. ��) : ");
	scanf("%d", &user);
	Nocursor();
	while (1) {
		com = Print_Cham(rand() % 2 + 1);
		if (_kbhit()) {
			if (user < 1 || user>2) {
				gotoxy(23, 19); printf("�߸��Է��ϼ̽��ϴ�.\n");   goto �ٽ��Է�;
			}
			if (user == com) {
				gotoxy(43, 45); printf("�޿���������~~~Ǫ����������");
				pl[pl_num].cash -= 100;
				player_update(city, pl);
				break;
			}
			else {
				gotoxy(33, 45);   printf("�ż��� �̰ܺ��� ���� ��..��...��....((#  ..��) ~ <3   ");
				pl[pl_num].cash += 500;
				player_update(city, pl);
				break;
			}
		}
		Sleep(70);
		clear();
	}
	system("pause>NULL");
	clear();
}

int Print_Cham(int com) {
	if (com == 1) {
		gotoxy(20, 18); printf("                                     ");
		gotoxy(20, 19); printf("                             ####    ");
		gotoxy(20, 20); printf(" ########################  ##/   #   ");
		gotoxy(20, 21); printf("#                        ##  /    #  ");
		gotoxy(20, 22); printf("#                  #       ///    #  ");
		gotoxy(20, 23); printf(" ############    ##   #     //    #  ");
		gotoxy(20, 24); printf("             #..#   ##..    //    #  ");
		gotoxy(20, 25); printf("             ###  #     /   //    #  ");
		gotoxy(20, 26); printf("             #  ##       /  //    #  ");
		gotoxy(20, 27); printf("             #........'''/  //    #  ");
		gotoxy(20, 28); printf("             #           / / /    #  ");
		gotoxy(20, 29); printf("             ##...'''''''##  /   #   ");
		gotoxy(20, 30); printf("             ##          # ##    #   ");
		gotoxy(20, 31); printf("               #########     ####    ");
		gotoxy(20, 32); printf("                                     ");
		gotoxy(20, 33); printf("                                     ");
	}
	else {
		gotoxy(60, 18); printf("                                     ");
		gotoxy(60, 19); printf("   ###                               ");
		gotoxy(60, 20); printf(" #   /##  #########################  ");
		gotoxy(60, 21); printf("#    /  ##                         # ");
		gotoxy(60, 22); printf("#    / /       #                   # ");
		gotoxy(60, 23); printf("#    //     #   ##    #############  ");
		gotoxy(60, 24); printf("#    //    ..##   #..#               ");
		gotoxy(60, 25); printf("#    //   /     #  ###               ");
		gotoxy(60, 26); printf("#    //   /      ##  #               ");
		gotoxy(60, 27); printf("#    //  /'''........#               ");
		gotoxy(60, 28); printf("##   / / /           #               ");
		gotoxy(60, 29); printf("##   /  ## /''......##               ");
		gotoxy(60, 30); printf(" #   /## #  /       ##               ");
		gotoxy(60, 31); printf("  ####     #########                 ");
		gotoxy(60, 32); printf("                                     ");
		gotoxy(60, 33); printf("                                     ");
	}
	return com;
}

//Ȳ�ݿ��� ���� �Լ�
void Gold_key(Player* pl, CityInfo city[], int pl_num) {
	int event_num; //������ ���� ����
	int num;   //����ڸ޴��� �Էº���
	int angel = 1, outland = 2, //1.õ��ī�� 2.���ε�Ż���
		choice = 3, Go_ktx = 4, Go_start = 5, Go_hole = 6;   //3.���������̵� 4.KTX 5.��������̵� 6.��ũȦ
	Nocursor();
	srand((unsigned)time(NULL));
	event_num = rand() % 6 + 1;
	if (event_num == 1) {
		gotoxy(23, 15); printf("õ��ī�尡 ���Խ��ϴ�.");
		if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 17); printf("õ��ī�带 �����ϰ��ʴϴ�.");
			gotoxy(23, 19); printf("õ��ī��� 1�常 ���������մϴ�.");
		}
		else if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17); printf("������ī�带 �����ϰ��ʴϴ�.");
			gotoxy(23, 19); printf("1.��ü 2.���� : ");
			gotoxy(38, 19); scanf("%d", &num);
			clear();
			switch (num)
			{
			case 1:
				gotoxy(23, 15); printf("������ī��� ��ü�մϴ�.");
				pl[pl_num].goldkey = 2; break;
			case 2:
				gotoxy(23, 15); printf("ī�带 �����ϴ�."); break;
			}
		}
	}
	else if (event_num == 2) {
		gotoxy(23, 15); printf("������ī�尡 ���Խ��ϴ�.");
		if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17); printf("������ī�带 �����ϰ��ʴϴ�.");
			return;
		}
		else if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 17); printf("õ��ī�带 �����ϰ��ʴϴ�.");
			gotoxy(23, 19); printf("1.��ü 2.���� : ");
			gotoxy(38, 19);  scanf("%d", &num);
			clear();
			switch (num)
			{
			case 1:
				gotoxy(23, 15); printf("õ��ī��� ��ü�մϴ�.");
				pl[pl_num].goldkey = 1; break;
			case 2:
				gotoxy(23, 15); printf("ī�带 �����ϴ�."); break;
			}
		}
	}
	else if (event_num == 3) {
		G_prison(city, pl, pl_num); //���ε�
	}
	else if (event_num == 4) {
		gotoxy(23, 15); printf("KTX�� �̵��մϴ�.");
		system("pause>null");
		clear();
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
		printf("  "); //����������� ����
		pl[pl_num].pos = 21;
		gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
		textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
		printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
		textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
		Nocursor();
		Arrive(city, pl, pl_num);
	}
	else if (event_num == 5) {
		G_start(city, pl, pl_num); //������� �̵�
	}
	else if (event_num == 6) {
		G_hole(city, pl, pl_num); //��ũȦ
	}
	system("pause>NULL"); clear();
}

void G_prison(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("�����ҷ� �̵��մϴ�....");
	system("pause>null");
	clear();
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
	printf("  "); //����������� ����
	pl[pl_num].pos = 7;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
	printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
	textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
	Nocursor();
	Arrive(city, pl, pl_num);
}

void G_start(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("������� �̵��մϴ�.");
	system("pause>NULL");
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
	printf("  "); //����������� ����
	pl[pl_num].pos = 0;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
	printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
	textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
	Nocursor();
	Arrive(city, pl, pl_num);
}

void G_hole(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(29, 15);   printf("��ũȦ�̴�!!!!!!");
	gotoxy(29, 17);   printf("���ΰ��ɰž�!!!!!!!!!!!");
	system("pause>NULL");
	srand(time(NULL));
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //�÷��̾��� ������ġ
	printf("  "); //����������� ����
	pl[pl_num].pos = rand() % 28;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //�÷��̾� ���� �ҷ�����
	printf("%s", pl[pl_num].mysymbol); //�÷��̾� �ɺ� ���
	textcolor(BLACK, WHITE); //��Ʈ �ʱ�ȭ
	system("pause>NULL");
	clear();
	Arrive(city, pl, pl_num);
}

void GameOver(CityInfo city[], Player* pl) {
	clear();
	for (int i = 0; i < pixed_pl_num; i++) {
		pl[i].rank = 1;
		for (int j = 0; j < 28; j++) {
			if (city[j].owner == i) {   //���� ������ (0:������ 1:player1���� 2:player2���� 3:player3���� 4:player4����)
				switch (city[j].building) {
				case 1:
					pl[i].cash += city[j].price; break;   //��
				case 2:
					pl[i].cash += city[j].price * 0.5; break;   //����
				case 3:
					pl[i].cash += city[j].price * 1.2; break;   //����
				case 4:
					pl[i].cash += city[j].price * 1.8; break;   //ȣ��
				case 5:
					pl[i].cash += city[j].price * 2; break;   //���帶ũ
				}
			}
		}
	}
	player_update(city, pl); gotoxy(23, 15); printf("Game Over...");
	gotoxy(23, 17); system("pause>null"); clear();
	for (int i = 0; i <= 27; i++) {
		draw_box_end(2 + i * 2, i + 4, 110 - i * 2, 58 - i, "��"); Sleep(20);
	}
	for (int i = 0; i <= 27; i++) {
		draw_box_end(2 + i * 2, i + 4, 110 - i * 2, 58 - i, "��"); Sleep(20);
	}

	for (int i = 0; i < pixed_pl_num; i++) {
		for (int j = i + 1; j < pixed_pl_num; j++) {
			if (pl[i].cash > pl[j].cash) {
				pl[j].rank++;
			}
			else {
				pl[i].rank++;
			}
		}
	}
	int pl_rank;
	for (int i = 0; i < 4; i++) {
		if (pl[i].rank == 1) {
			pl_rank = pl[i].symbolnum;
		}
	}

	system("mode con cols=86 lines=25");
	while (1) {
		if (_kbhit()) {
			break;
		}
		intro_color();
		textcolor(pl[pl_rank].textcolor, WHITE);
		gotoxy(35, 18); printf("Player%d �̱�!!!", pl_rank + 1);
	}
}

void draw_box_end(int x1, int y1, int x2, int y2, char* ch)
{
	int x, y;

	for (x = x1; x <= x2; x = x + 2) {
		gotoxy(x, y1);	printf("%s", ch);
		gotoxy(x, y2);	printf("%s", ch);
	}
	for (y = y1; y <= y2; y++) {
		gotoxy(x1, y);	printf("%s", ch);
		gotoxy(x2, y);	printf("%s", ch);
	}
}

int keyControl() {
	int temp = getch();

	if (temp == 72) {
		return UP;
	}
	else if (temp == 75) {
		return LEFT;
	}
	else if (temp == 80) {
		return DOWN;
	}
	else if (temp == 77) {
		return RIGHT;
	}
	else if (temp == 32) {
		return SUBMIT;
	}
}

int menuDraw()
{
	Nocursor();
	int x = 36;
	int y = 17;

	while (1) {
		if (_kbhit())
			break;
		intro_color();
	}
	getch();

	gotoxy(5, 1); textcolor(RED, WHITE); printf("���      ����    �����        ����          ����        ���   \n");
	gotoxy(5, 2); printf("���     ����   ���    ���      ����          ����       ����\n");
	gotoxy(5, 3); printf("���    ����  ���        ���    �����      �����      �����\n");
	gotoxy(5, 4); printf("���   ����  ���          ���   �����      �����     ������   \n");
	gotoxy(5, 5); printf("���  ����  ���            ���  ��� ���    ��� ���     ���  ���   \n");
	gotoxy(5, 6); printf("��� ����   ����   ���� ���  ���  ���  ���  ���    ���    ���   \n");
	gotoxy(5, 7); textcolor(BLUE, WHITE); printf("������    ��� ����   ����  ���  ���  ���  ���    ���    ���       \n");
	gotoxy(5, 8); printf("��� ����   ���            ���  ���  ���  ���  ���   ��������   \n");
	gotoxy(5, 9); printf("���  ����  ���            ���  ���   �����   ���   ��������   \n");
	gotoxy(5, 10); printf("���   ����  ���          ���   ���   �����   ���  ���        ���   \n");
	gotoxy(5, 11); printf("���    ����  ���        ���    ���     ���     ���  ���        ���   \n");
	gotoxy(5, 12); printf("���     ����   ���    ���      ���     ���     ��� ���          ���   \n");
	gotoxy(5, 13); printf("���      ����    �����        ���      ��      ��� ���          ���   \n");

	gotoxy(x - 2, y); textcolor(BLACK, WHITE); printf("�� �� �� �� ��");
	gotoxy(x, y + 2);  printf(" �� �� �� ��");
	gotoxy(x, y + 4);  printf("  ��    �� ");

	while (1) {
		int n = keyControl();
		switch (n) {
		case UP: {
			if (y > 17) {
				gotoxy(x - 2, y);
				printf("  ");
				gotoxy(x - 2, y -= 2);
				printf("��");
			}
			break;
		}
		case DOWN: {
			if (y < 21) {
				gotoxy(x - 2, y);
				printf("  ");
				gotoxy(x - 2, y += 2);
				printf("��");
			}
			break;
		}
		case SUBMIT:
			return y - 17;
		}
	}
}

void intro_color() {
	int komarand = rand() % 14 + 1;

	textcolor(komarand, WHITE);
	gotoxy(5, 1); printf("���      ����    �����        ����          ����        ���   \n");
	gotoxy(5, 2); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���     ����   ���    ���      ����          ����       ����\n");
	gotoxy(5, 3); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���    ����  ���        ���    �����      �����      �����\n");
	gotoxy(5, 4); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���   ����  ���          ���   �����      �����     ������   \n");
	gotoxy(5, 5); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���  ����  ���            ���  ��� ���    ��� ���     ���  ���   \n");
	gotoxy(5, 6); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("��� ����   ���            ���  ��� ���    ��� ���    ���    ���   \n");
	gotoxy(5, 7); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("������    ���            ���  ���  ���  ���  ���    ���    ���   \n");
	gotoxy(5, 8); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("��� ����   ���            ���  ���  ���  ���  ���   ��������   \n");
	gotoxy(5, 9); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���  ����  ���            ���  ���   �����   ���   ��������   \n");
	gotoxy(5, 10); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���   ����  ���          ���   ���   �����   ���  ���        ���   \n");
	gotoxy(5, 11); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���    ����  ���        ���    ���     ���     ���  ���        ���   \n");
	gotoxy(5, 12); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���     ����   ���    ���      ���     ���     ��� ���          ���   \n");
	gotoxy(5, 13); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("���      ����    �����        ���      ��      ��� ���          ���   \n");
	Sleep(50);
	system("cls");

}

int Choice_player() {
	system("cls");
	Nocursor();
	int x = 28;
	int y = 16;

	gotoxy(29, 7); printf("�� K O R E A   M A R V E L ��\n");
	gotoxy(30, 10); printf("Choose The Number Of Player\n");
	gotoxy(28, 14); printf("2P             3P             4P\n");
	gotoxy(x, y);	 printf("��");

	while (1) {
		int n = keyControl();
		switch (n) {
		case RIGHT: {
			if (x < 53) {
				gotoxy(x, y); printf("  ");
				gotoxy(x += 15, y); printf("��");
			}
			break;
		}
		case LEFT: {
			if (x > 29) {
				gotoxy(x, y); printf("  ");
				gotoxy(x -= 15, y); printf("��");
			}
			break;
		}
		case SUBMIT:
			if (x == 28)
				return x - 26;
			else if (x == 43)
				return x - 40;
			else
				return x - 54;
		}
	}
}

void Game_info() {
	FILE* fp = fopen("gameinfo.txt", "w+");
	char str[3000] = { 0, };
	int count = 0;
	int rcvSz;

	rcvSz = sizeof(addr);
	recv(Sock, str, sizeof(str), 0);
	fwrite((void*)str, 1, count, fp);

	fseek(fp, 0, SEEK_SET);
	fread(str, sizeof(str), 1, fp);
	printf("%s\n", str);

	fclose(fp);

	return 0;
}

void watingRoom(int i) {
	system("mode con cols=86 lines=45");
	textcolor(BLACK, WHITE);

	Nocursor();   //         78      
	gotoxy(4, 2); printf("����������������������������������������������������������������������������������������������������������������������������������������������������������");
	gotoxy(4, 3); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 4); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 5); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 6); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 7); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 8); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 9); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 10); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 11); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 12); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 13); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 14); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 15); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 16); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 17); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 18); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 19); printf("��                  ��                  ��                  ��                  ��");
	gotoxy(4, 20); printf("����������������������������������������������������������������������������������������������������������������������������������������������������������");
	gotoxy(4, 21); printf("����������������������������������������������������������������������������������������������������������������������������������������������������������");
	gotoxy(4, 22); printf("��                                                                           ��");
	gotoxy(4, 23); printf("��                                                                           ��");
	gotoxy(4, 24); printf("��                                                                           ��");
	gotoxy(4, 25); printf("��                                                                           ��");
	gotoxy(4, 26); printf("��                                                                           ��");
	gotoxy(4, 27); printf("��                                                                           ��");
	gotoxy(4, 28); printf("��                                                                           ��");
	gotoxy(4, 29); printf("��                                                                           ��");
	gotoxy(4, 30); printf("��                                                                           ��");
	gotoxy(4, 31); printf("��                                                                           ��");
	gotoxy(4, 32); printf("��                                                                           ��");
	gotoxy(4, 33); printf("��                                                                           ��");
	gotoxy(4, 34); printf("��                                                                           ��");
	gotoxy(4, 35); printf("��                                                                           ��");
	gotoxy(4, 36); printf("��                                                                           ��");
	gotoxy(4, 37); printf("����������������������������������������������������������������������������������������������������������������������������������������������������������");
	gotoxy(4, 38); printf("��                                                                           ��");
	gotoxy(4, 39); printf("��                                                                           ��");
	gotoxy(4, 40); printf("����������������������������������������������������������������������������������������������������������������������������������������������������������");
	textcolor(GRAY2, WHITE); gotoxy(8, 22); printf("ä��â�� Ready!�� �Է��ϸ� �غ�Ϸᰡ �˴ϴ�.");
	textcolor(GRAY2, WHITE); gotoxy(8, 23); printf("��� �÷��̾ Ready! ���°� �Ǹ� 5�� �� ������ ���۵˴ϴ�.");
	textcolor(GRAY2, WHITE); gotoxy(69, 42); printf("KoreaMarble");
	textcolor(GRAY2, WHITE); gotoxy(29, 43); printf("Copyright �� 2020. BoramSamJo Co.all rights reserved");
	gotoxy(6, 38); printf("                                          "); gotoxy(6, 38);
	switch (i) {
	case 3:
		textcolor(RED, WHITE);
		gotoxy(71, 8);      printf("��");
		gotoxy(68, 11);   printf("Player 4");
		textcolor(BLACK, WHITE);
	case 2:
		textcolor(BLUE, WHITE);
		gotoxy(52, 8);      printf("��");
		gotoxy(49, 11);   printf("Player 3");
		textcolor(BLACK, WHITE);
	case 1:
		textcolor(GREEN, WHITE);
		gotoxy(33, 8);      printf("��");
		gotoxy(30, 11);   printf("Player 2");
		textcolor(BLACK, WHITE);
	case 0:
		textcolor(MAGENTA2, WHITE);
		gotoxy(14, 8);      printf("��");
		gotoxy(11, 11);   printf("Player 1");
		textcolor(BLACK, WHITE);
	}
}

void ServerSend(CityInfo city[], Player* pl) {
	call = 2;
	send(Sock, (char*)& call, sizeof(call), 0);
	for (int i = 0; i < pixed_pl_num; i++) {
		send(Sock, (char*)& pl[i], sizeof(pl[i]), 0);
	}
	for (int i = 0; i < 28; i++) {
		send(Sock, (char*)& city[i], sizeof(city[i]), 0);
	}
	player_update(city, pl);
}

void ServerRecv(CityInfo city[], Player* pl) {
	for (int i = 0; i < pixed_pl_num; i++) {
		recv(Sock, (char*)& pl[i], sizeof(pl[i]), 0);
	}
	for (int i = 0; i < 28; i++) {
		recv(Sock, (char*)& city[i], sizeof(city[i]), 0);
	}
	player_update(city, pl);
	SymbolUpdate(city, pl);
	CityUpdate(city, pl);
}

unsigned WINAPI sendChat(void* arg) {
	char name[] = "Player";
	char sendMsg[1024] = { 0, };
	while (1) {
		WaitForSingleObject(hMutex, INFINITE);
		gotoxy(6, 38);
		ReleaseMutex(hMutex);
		getchar();
		scanf("%[^\n]s", buf);
		if (!strcmp(buf, "Ready!")) {
			WaitForSingleObject(hMutex, INFINITE);
			gotoxy(6, 38);
			printf("                                ");
			gotoxy(6, 38);
			ReleaseMutex(hMutex);
			send(Sock, buf, strlen(buf), 0);
			return 0;
		}
		else {
			sprintf(sendMsg, "%s %d >> %s", name, pl_num + 1, buf);
			send(Sock, sendMsg, strlen(sendMsg), 0);
			memset(sendMsg, 0, sizeof(sendMsg));
		}
		WaitForSingleObject(hMutex, INFINITE);
		gotoxy(10, 38);
		ReleaseMutex(hMutex);
	}
}

unsigned WINAPI recvChat(void* arg) {
	int recvret = 0;
	char recvMsg[1024] = { 0, };
	char tmp[13][1024] = { 0, };
	int y = 24, i = 0;

	while (1) {
		recvret = recv(Sock, recvMsg, sizeof(recvMsg) - 1, 0);
		if (recvret == -1)
			return 0;
		recvMsg[recvret] = '\0';
		WaitForSingleObject(hMutex, INFINITE);
		if (i < 13) {
			strcpy(tmp[i], recvMsg);
			gotoxy(10, y + i); printf("%s", tmp[i]);
			gotoxy(6, 38);
			printf("                                ");
			gotoxy(6, 38);
			i++;
		}
		else {
			for (int j = 1; j < i; j++)
				strcpy(tmp[j - 1], tmp[j]);
			strcpy(tmp[12], recvMsg);
			for (int j = 0; j < i; j++) {
				gotoxy(10, y + j);
				printf("                                ");
				gotoxy(10, y + j);
				printf("%s", tmp[j]);
			}
		}
		if (!strcmp(recvMsg, "Start!")) {
			return 0;
		}
		ReleaseMutex(hMutex);
	}
}

