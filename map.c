#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
#include <WS2tcpip.h>
#include <process.h>

//콘솔 속성 - 글꼴: 돋움체 굵게, 검은색 / 배경 흰색 
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
	int pos; //위치
	int money;   //돈
	int cash;    //현금
	int bank;   //부동산값
	char mysymbol[4]; //말  건민 1 ★ 해린 2 ♣ 현우 3 ◑ 범준 4 ▩
	int symbolnum; //player1 이면 1, player2 이면 2
	int pause_move; //무인도 남은 턴 값
	char textcolor;  //define으로 플레이어 수에 맞춰서 색받아오기
	int goldkey; //무인도 탈출권
	int rank;   //플레이어 순위
} Player;

typedef struct {
	int x; //도시위치 x좌표
	int y; //도시위치 y좌표
	char city_name[20]; //도시이름 
	int owner; //도시 소유자 (1:player1소유 2:player2소유 3:player3소유 4:player4소유 6:무소유)
	int building; //건물 지어졌는지
	int price; //도시 가격
	int take_over;//건물 인수가격
	char builsym;//빌딩 심볼
	char original[20];   //도시 입력값이랑 비교
	int bail;   //보석금
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

//함수선언
void init_map();   //맵 초기화
void print_map(int map[68][68], int row, int col);   //맵 출력
void gotoxy(int x, int y); //커서이동
void Nocursor();   //커서 없애기
void Cursor();   //커서 출력
void textcolor(int font, int back);   //콘솔 폰트 색 변경
void clear();//화면지우기
int Dice1(); //주사위1
int Dice2();   //주사위2
void init_city(CityInfo city[]);   //도시 초기화
void init_player(Player* pl, int pl_num);   //플레이어 초기화
void move_player(CityInfo city[], int dice_num, Player* pl, int pl_num);  //플레이어 움직임
////////////////////////////////////////////////////////////////////////////// 1차 발표

int Double(CityInfo city[], Player* pl, Dice dices, Play play, int pl_num, int limit);   //주사위 굴리기 & 더블 판별
void Arrive(CityInfo city[], Player* pl, int num);   //소유 여부 및 땅 구매, 건물 선택
void player_update(CityInfo city[], Player* pl);   //플레이어 자산 가감
void Ktx(CityInfo city[], Player* pl, int pl_num);   //ktx 이동
void Sale_City(CityInfo city[], Player* pl, int num);   //도시 매각
void Buy_City(CityInfo city[], Player* pl, int pl_num);   //도시 인수
void Prison(CityInfo city[], Player* pl, int pl_num);   //교도소
void Bankruptcy(CityInfo city[], Player* pl, int pl_num);
void GameOver(CityInfo city[], Player* pl);   //턴 종료 게임오버
int Choice_player();
void watingRoom(int i);

//미니게임 함수
void game_rsp();   //가위바위보
int printf_rsp(int rsp_user);
int printf_rsp2(int rsp_com);
int Chamchamcham();   //참참참
int Print_Cham(int);
////////////////////////////////////////////////////////////////////////////// 2차 발표

void GameOver(CityInfo city[], Player* pl);	//턴 종료 게임오버
void draw_box_end(int x1, int y1, int x2, int y2, char* ch);	//엔딩 넘어갈때
void Game_info();	//게임정보 파일입출력
void intro_color(); //인트로 

//황금열쇠 함수
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

int pixed_pl_num; // 플레이어수 전역변수
int pl_cnt; //파산 비교 전역변수
int StartFlag;

SOCKET Sock;
SOCKADDR_IN addr;
int addrSz;

HANDLE hMutex, sendThread, recvThread, mainThread;
char buf[1024] = { 0, };
int pl_num;
int readycnt = 0;
int call = 0;
//메인
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

	hMutex = CreateMutex(NULL, FALSE, NULL);	//뮤택스 생성

	//WSAStartup
	WSADATA wsa; //라이브러리 로드
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) //winsock 초기화 함수
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
		if (menuCode == 0) {//게임 시작
			clear(); gotoxy(23, 10); printf("서버 ip : "); scanf("%s", ip);
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
				gotoxy(30, 2); printf("%d 초 뒤에 게임이 시작됩니다.", start);
			}
			system("mode con cols=114 lines=64");//콘솔 사이즈 픽스
			Nocursor();
			CityInfo city[28];
			init_map();
			init_city(city);
			Player* pl = (Player*)malloc(sizeof(Player) * pixed_pl_num);
			init_player(pl, pixed_pl_num);
			int double_limit = 0;
			pl_cnt = 1;

			recv(Sock, msg, sizeof(msg), 0);   //쓰레드에 남아있던 데이터 받아오기
			while (play.cnt > 0) {
				recv(Sock, (char*)& play, sizeof(play), 0);
				if (pl_num == play.turn) {
					if (pl[pl_num].cash < 0) {
						break;
					}
					else {
						gotoxy(23, 15); printf("아름답고 고귀로운 당신의 차례입니다.");
						Double(city, pl, dices, play, pl_num, double_limit);
						clear();
						gotoxy(80, 50); textcolor(BLACK, WHITE); printf("남은 턴 수 : %d", play.cnt);
						call = 3;
						send(Sock, (char*)& call, sizeof(call), 0);
					}
				}
				else {
					if (pl[play.turn].cash < 0) {
						break;
					}
					else {
						gotoxy(23, 15); printf("플레이어 %d의 차례입니다.", play.turn + 1);
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
			// 도시
			gotoxy(city[i].x, city[i].y - 1);
			textcolor(pl[city[i].owner].textcolor, WHITE);
			printf("%s", city[i].city_name);
			textcolor(BLACK, WHITE);

			// 건물
			if (city[i].building == 1) {
				gotoxy(city[i].x - 2, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("         ");
				textcolor(BLACK, WHITE);
			}
			if (city[i].building == 2) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("Ｖ");
				textcolor(BLACK, WHITE);
			}
			else if (city[i].building == 3) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("ＶＢ");
				textcolor(BLACK, WHITE);
			}
			else if (city[i].building == 4) {
				gotoxy(city[i].x + 1, city[i].y - 3);
				textcolor(pl[city[i].owner].textcolor, WHITE);
				printf("ＶＢＨ");
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
		case 0:   //플레이어1
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
		case 1:   //플레이어2
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
		case 2:   //플레이어3
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
		case 3:   //플레이어4
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
//함수
void textcolor(unsigned int text, unsigned int back) //text color
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void gotoxy(int x, int y)//커서이동
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

void Cursor() {   //커서 띄우기
	CONSOLE_CURSOR_INFO CurInfo;
	CurInfo.dwSize = 100;
	CurInfo.bVisible = TRUE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &CurInfo);
}

void clear()//가운데 지워주는 함수 
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
	gotoxy(80, 50); printf("                 ");//남은턴수 지워주는 거
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
					gotoxy(23, 30); printf("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
					gotoxy(23, 31); printf("■□□□■■■■■■■■■■■■■□■■■□□□■■■■■■■■■\n");
					gotoxy(23, 32); printf("■□■■□■■■■■■■■■■■■□■■■□■□■■■■■■■■■\n");
					gotoxy(23, 33); printf("■□■■□■■■■■■■■■■■■□■■■■■□■■■■□□□■■\n");
					gotoxy(23, 34); printf("■□■■□■■□□■■□■■□■■□□□□■■□■■■□■■■□■\n");
					gotoxy(23, 35); printf("■□■■□■□■■□■□■■□■■□■■■□■□■■■□□□□□■\n");
					gotoxy(23, 36); printf("■□■■□■□■■□■□■■□■■□■■■□■□■□■□■■■■■\n");
					gotoxy(23, 37); printf("■□□□■■■□□■■■□□■□■□□□□■■□□□■■□□□□■\n");
					gotoxy(23, 38); printf("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
					gotoxy(23, 39); printf("");
					limit++;
					if (pl[pl_num].pause_move > 0) {
						pl[pl_num].pause_move = 0;
						gotoxy(23, 17); printf("떠어어어블!!!다음턴부터 이동할 수 있어요");
						system("pause>NULL");
					}
					else if (limit == 3) {
						gotoxy(23, 17); printf("더블이 3번!");
						gotoxy(23, 19); printf("무인도로 슝슝\n"); gotoxy(50, 22);
						system("pause>NULL");

						gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
						printf("  "); //공백삽입으로 지움
						pl[pl_num].pos = 7;
						gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
						textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
						printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
						textcolor(BLACK, WHITE); //폰트 초기화
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
						gotoxy(23, 15); printf("교도소를 나갑니다.");
						system("pause>NULL");
						clear();
						move_player(city, dice_sum, pl, pl_num);
						Arrive(city, pl, pl_num);
					}
					else {
						gotoxy(23, 17); printf("까아아앙비...%d번 남았어요", pl[pl_num].pause_move - 1);
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
		gotoxy(23, 15); printf("플레이어%d %d칸 이동", play.turn + 1, dice_sum);
		if (dices.dice1 == dices.dice2) {
			gotoxy(23, 30); printf("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
			gotoxy(23, 31); printf("■□□□■■■■■■■■■■■■■□■■■□□□■■■■■■■■■\n");
			gotoxy(23, 32); printf("■□■■□■■■■■■■■■■■■□■■■□■□■■■■■■■■■\n");
			gotoxy(23, 33); printf("■□■■□■■■■■■■■■■■■□■■■■■□■■■■□□□■■\n");
			gotoxy(23, 34); printf("■□■■□■■□□■■□■■□■■□□□□■■□■■■□■■■□■\n");
			gotoxy(23, 35); printf("■□■■□■□■■□■□■■□■■□■■■□■□■■■□□□□□■\n");
			gotoxy(23, 36); printf("■□■■□■□■■□■□■■□■■□■■■□■□■□■□■■■■■\n");
			gotoxy(23, 37); printf("■□□□■■■□□■■■□□■□■□□□□■■□□□■■□□□□■\n");
			gotoxy(23, 38); printf("■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■\n");
			gotoxy(23, 39); printf("");
			limit++;
			if (pl[play.turn].pause_move > 0) {
				pl[play.turn].pause_move = 0;
			}
			else if (limit == 3) {
				gotoxy(23, 17); printf("더블이 3번!");
				gotoxy(city[pl[pl_num].pos % 28].x + play.turn * 2, city[pl[play.turn].pos % 28].y + 1); //플레이어의 현재위치
				printf("  "); //공백삽입으로 지움
				pl[play.turn].pos = 7;
				gotoxy(city[pl[play.turn].pos].x + play.turn * 2, city[pl[pl_num].pos].y + 1);
				textcolor(pl[play.turn].textcolor, WHITE); //플레이어 색깔 불러오기
				printf("%s", pl[play.turn].mysymbol); //플레이어 심볼 출력
				textcolor(BLACK, WHITE); //폰트 초기화
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

//주사위 숫자 바뀌는 함수
int Dice1(int dice_num)
{
	if (dice_num == 1) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■■■□■■■\n");
		gotoxy(40, 46); printf("■■■□■■■\n");
		gotoxy(40, 47); printf("■■■□■■■\n");
		gotoxy(40, 48); printf("■■■□■■■\n");
		gotoxy(40, 49); printf("■■■□■■■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 2) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■□□□□□■\n");
		gotoxy(40, 46); printf("■■■■■□■\n");
		gotoxy(40, 47); printf("■□□□□□■\n");
		gotoxy(40, 48); printf("■□■■■■■\n");
		gotoxy(40, 49); printf("■□□□□□■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 3) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■□□□□□■\n");
		gotoxy(40, 46); printf("■■■■■□■\n");
		gotoxy(40, 47); printf("■□□□□□■\n");
		gotoxy(40, 48); printf("■■■■■□■\n");
		gotoxy(40, 49); printf("■□□□□□■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");

	}
	else if (dice_num == 4) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■□■■□■■\n");
		gotoxy(40, 46); printf("■□■■□■■\n");
		gotoxy(40, 47); printf("■□□□□□■\n");
		gotoxy(40, 48); printf("■■■■□■■\n");
		gotoxy(40, 49); printf("■■■■□■■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");

	}
	else if (dice_num == 5) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■□□□□□■\n");
		gotoxy(40, 46); printf("■□■■■■■\n");
		gotoxy(40, 47); printf("■□□□□□■\n");
		gotoxy(40, 48); printf("■■■■■□■\n");
		gotoxy(40, 49); printf("■□□□□□■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");

	}
	else if (dice_num == 6) {
		gotoxy(40, 44); printf("■■■■■■■\n");
		gotoxy(40, 45); printf("■□□□□□■\n");
		gotoxy(40, 46); printf("■□■■■■■\n");
		gotoxy(40, 47); printf("■□□□□□■\n");
		gotoxy(40, 48); printf("■□■■■□■\n");
		gotoxy(40, 49); printf("■□□□□□■\n");
		gotoxy(40, 50); printf("■■■■■■■\n");
	}
	return dice_num;
}

int Dice2(int dice_num)
{
	if (dice_num == 1) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■■■□■■■\n");
		gotoxy(60, 46); printf("■■■□■■■\n");
		gotoxy(60, 47); printf("■■■□■■■\n");
		gotoxy(60, 48); printf("■■■□■■■\n");
		gotoxy(60, 49); printf("■■■□■■■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");

	}
	else if (dice_num == 2) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■□□□□□■\n");
		gotoxy(60, 46); printf("■■■■■□■\n");
		gotoxy(60, 47); printf("■□□□□□■\n");
		gotoxy(60, 48); printf("■□■■■■■\n");
		gotoxy(60, 49); printf("■□□□□□■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 3) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■□□□□□■\n");
		gotoxy(60, 46); printf("■■■■■□■\n");
		gotoxy(60, 47); printf("■□□□□□■\n");
		gotoxy(60, 48); printf("■■■■■□■\n");
		gotoxy(60, 49); printf("■□□□□□■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 4) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■□■■□■■\n");
		gotoxy(60, 46); printf("■□■■□■■\n");
		gotoxy(60, 47); printf("■□□□□□■\n");
		gotoxy(60, 48); printf("■■■■□■■\n");
		gotoxy(60, 49); printf("■■■■□■■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 5) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■□□□□□■\n");
		gotoxy(60, 46); printf("■□■■■■■\n");
		gotoxy(60, 47); printf("■□□□□□■\n");
		gotoxy(60, 48); printf("■■■■■□■\n");
		gotoxy(60, 49); printf("■□□□□□■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");
	}
	else if (dice_num == 6) {
		gotoxy(60, 44); printf("■■■■■■■\n");
		gotoxy(60, 45); printf("■□□□□□■\n");
		gotoxy(60, 46); printf("■□■■■■■\n");
		gotoxy(60, 47); printf("■□□□□□■\n");
		gotoxy(60, 48); printf("■□■■■□■\n");
		gotoxy(60, 49); printf("■□□□□□■\n");
		gotoxy(60, 50); printf("■■■■■■■\n");
	}
	return dice_num;
}

void init_city(CityInfo city[]) //땅 이름 좌표 초기화
{
	int i, k;
	char arr_city[28][20] = { {"출    발"},{"강    릉"},{"싱 크 홀"},{"춘    천"},{"북 한 산"},{"경    주"},{"천    안"},{"교 도 소"},
							  {"오 이 도"},{"인    천"},{"황금열쇠"},{"청    주"},{"설 악 산"},{"의 정 부"},{"강원랜드"},
							  {"경기광주"},{"지 리 산"},{"제    주"},{"황금열쇠"},{"포    항"},{"여    수"},{"K  T  X"},
							  {"전    주"},{"에버랜드"},{"황금열쇠"},{"부    산"},{"보 석 금"},{"서    울"} };

	int arr_cityprice[28] = { 0,5,0,8,10,12,14,0,10,15,0,18,10,20,0,30,10,34,0,36,38,0,42,10,0,48,0,50 };
	char tmp_city[28][20] = { {"출발"},{"강릉"},{"싱크홀"},{"춘천"},{"북한산"},{"경주"},{"천안"},{"교도소"},
							 {"오이도"},{"인천"},{"황금열쇠"},{"청주"},{"설악산"},{"의정부"},{"강원랜드"},
							 {"경기광주"},{"지리산"},{"제주"},{"황금열쇠"},{"포항"},{"여수"},{"KTX"},
							 {"전주"},{"에버랜드"},{"황금열쇠"},{"부산"},{"보석금"},{"서울"} }; //도시 이름 비교용

	for (i = 7, k = 0; i >= 0; i--) {      // <<가로 아래줄<< 도시 위치설정 및 표기, owner, 가격 초기화
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
	for (i = 14, k = 0; i >= 8; i--) {   // <<세로 왼쪽줄<< 도시 위치설정 및 표기, owner, 가격 초기화
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
	for (i = 21, k = 0; i >= 15; i--) {   // <<가로 윗줄<< 도시 위치설정 및 표기, owner, 가격 초기화
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
	for (i = 27, k = 0; i >= 22; i--) {   // <<세로 오른쪽줄<< 도시 위치설정 및 표기, owner, 가격 초기화
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
		// 8 * 8 6*6 만들어
		//플레이어 1 

		   //1 =  ■  8 = ─  9 = │ 10 = ┴ 11 = ┬ 12 = ┼  13  ┤  14 ├┤  15┌ 16 ┐ 17 └ 18 ┘
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
		//1 == 테두리, 8,9 == 경계선, 2,3,4,5 = 건물 6==말,7==이름
	};

	int col = sizeof(map[0]) / sizeof(2);  //2차원 배열의 가로 크기 = 가로 한줄의 크기 / 요소의 크기
	int row = sizeof(map) / sizeof(map[0]); // 세로 크기 = 배열 전체공간 / 가로 한줄 크기

	print_map(map, row, col);
}

void print_map(int map[63][57], int row, int col) { //맵 초기화

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {

			if (map[i][j] == 0) {
				printf("  ");
			}
			else if (map[i][j] == 1) {
				printf("■");
			}
			else if (map[i][j] == 8) {
				printf("──");
			}
			else if (map[i][j] == 9) {
				printf("│ ");
			}
			else if (map[i][j] == 10) {
				printf("┴ ");
			}
			else if (map[i][j] == 11) {
				printf("┬ ");
			}
			else if (map[i][j] == 12) {
				printf("┼ ");
			}
			else if (map[i][j] == 14) {
				printf("├ ");
			}
			else if (map[i][j] == 13) {
				printf("┤ ");
			}
			else if (map[i][j] == 15) {
				printf("┌ ");
			}
			else if (map[i][j] == 16) {
				printf("┐ ");
			}
			else if (map[i][j] == 17) {
				printf("└ ");
			}
			else if (map[i][j] == 18) {
				printf("┘ ");
			}
		}
		printf("\n");
	}
}

void init_player(Player* pl, int pl_num)//플레이어 초기화 게임 시작할 때
{
	for (int i = 0; i < pl_num; i++) {

		pl[i].cash = 3000;
		pl[i].money = 0 + pl[i].cash;
		pl[i].pos = 0;
		pl[i].symbolnum = i;
		pl[i].pause_move = 0;
		pl[i].goldkey = 2;

		switch (i) {
		case 0:   //플레이어1
			pl[i].textcolor = MAGENTA2;   //컬러 정의
			textcolor(pl[0].textcolor, WHITE);   //컬러 불러오기
			strcpy(pl[i].mysymbol, "▩");   //심볼지정
			gotoxy(102, 57);  printf("%s", pl[i].mysymbol);   //출발지에 심볼출력
			gotoxy(2, 1); printf("player 1  %s", pl[i].mysymbol);
			gotoxy(28, 1); printf("현  금 : %d 만 원", pl[i].cash);
			break;
		case 1:   //플레이어2
			pl[i].textcolor = GREEN;
			textcolor(pl[1].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "◑");
			gotoxy(104, 57);  printf("%s", pl[i].mysymbol);   //출발지에 심볼출력
			gotoxy(60, 1); printf("player 2  %s", pl[i].mysymbol);
			gotoxy(86, 1); printf("현  금 : %d 만 원", pl[i].cash);
			break;
		case 2:   //플레이어3
			pl[i].textcolor = BLUE;
			textcolor(pl[2].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "♣");
			gotoxy(106, 57);  printf("%s", pl[i].mysymbol);   //출발지에 심볼출력
			gotoxy(2, 61); printf("player 3  %s", pl[i].mysymbol);
			gotoxy(28, 61); printf("현  금 : %d 만 원", pl[i].cash);

			break;
		case 3:   //플레이어4
			pl[i].textcolor = RED;
			textcolor(pl[3].textcolor, WHITE);
			strcpy(pl[i].mysymbol, "★");
			gotoxy(108, 57);  printf("%s", pl[i].mysymbol);   //출발지에 심볼출력
			gotoxy(60, 61); printf("player 4  %s", pl[i].mysymbol);
			gotoxy(86, 61); printf("현  금 : %d 만 원", pl[i].cash);
			break;
		}
	}
	textcolor(BLACK, WHITE);
}

void move_player(CityInfo city[], int dice_num, Player* pl, int pl_num)   //city좌표들, 주사위 수, 플레이어 좌표, 플레이어 1,2,3,4
{
	for (int i = 0; i < dice_num; i++) { //주사위 갯수만큼 for문
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
		printf("  "); //공백삽입으로 지움
		gotoxy(city[(pl[pl_num].pos + 1) % 28].x + pl_num * 2, city[(pl[pl_num].pos + 1) % 28].y + 1); //플레이어 다음위치
		textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
		printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
		pl[pl_num].pos++; //플레이어 현재위치 + 1 = 플레이어 다음위치

		Sleep(400);
	}
	textcolor(BLACK, WHITE); //폰트 초기화
}

void Arrive(CityInfo city[], Player* pl, int pl_num) {

	Nocursor();
	int answer = 0;

	if (pl[pl_num].pos == 10 || pl[pl_num].pos == 18 || pl[pl_num].pos == 24) {
		gotoxy(23, 15);   printf("황금열쇠에 도착하셨습니다.");
		system("pause>NULL");
		clear();
		Nocursor();
		ServerSend(city, pl);
		return;
	}

	if (pl[pl_num].pos / 28 >= 1) //출발지점
	{
		gotoxy(23, 15);   printf("출발지를 지났습니다. 월급 300만원이 지급됩니다.");
		system("pause>null");
		clear();
		pl[pl_num].cash += 300;
		pl[pl_num].pos = pl[pl_num].pos % 28;
		if (pl[pl_num].pos == 0) //출발지점
		{
			char buildup[20];
			gotoxy(23, 15);   printf("출발지에 도착하였습니다.");
			gotoxy(23, 17);   printf("건물을 지을 도시를 입력하세요 : ");
			scanf("%s", buildup);

			for (int i = 0; i < 28; i++) {
				if (!strcmp(buildup, city[i].original)) {
					clear();
					if (city[i].owner == pl_num) {
						if (city[i].building == 5) {
							gotoxy(23, 17);   printf("더이상 건설할 수 없습니다.");
							system("pause>NULL");
							return;
						}
						if (city[i].building == 4) {
							gotoxy(23, 17);   printf("랜드마크를 건설하시겠습니까??");
							gotoxy(23, 19);   printf("1. 예  2. 아니오 : ");
							getch();
							scanf("%d", &answer);

							if (answer == 1) {
								if (pl[pl_num].cash - city[i].price * 2 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
									gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
									system("pause>NULL");
									return;
								}
								else {
									clear();
									gotoxy(23, 17);   printf("랜드마크를 건설합니다.");
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
								gotoxy(23, 15);   printf("싫으면 말아유");
								system("pause>NULL");
								clear();
							}
							else {
								gotoxy(23, 15);   printf("잘못입력하셨구먼유");
								system("pause>NULL");
								clear();

							}
						}
						else {
							gotoxy(23, 17); printf("지을 건물을 선택해주세요.");
							gotoxy(23, 19);   printf("1. 별장 2. 빌딩 3. 호텔 4. 건설안해! : ");
							system("pause>NULL");
							scanf("%d", &answer);
							switch (answer) {
							case 1:
								if (city[i].building == 2) {
									clear();
									gotoxy(23, 15);   printf("이미 별장이 지어져있습니다. 다시 선택해주세요!");
									system("pause>NULL");
									clear();
									break;
								}
								if (pl[pl_num].cash - city[i].price * 0.5 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
									gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("별장을 지었습니다!!");
									system("pause>NULL");
									city[i].building = 2;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 0.5;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("Ｖ");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear();
									break;
								}
							case 2:
								if (city[i].building == 3) {
									clear();
									gotoxy(23, 15);   printf("이미 빌딩이 지어져있습니다. 다시 선택해주세요!");
									system("pause>NULL");
									clear();
									break;
								}
								if (pl[pl_num].cash - city[i].price * 1.2 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
									gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("빌딩을 지었습니다!!");
									system("pause>NULL");
									city[i].building = 3;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 1.2;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("ＶＢ");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear(); break;
								}
							case 3:
								if (pl[pl_num].cash - city[i].price * 1.8 < 0) {
									clear();
									gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
									gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
									system("pause>NULL");
									break;
								}
								else {
									clear();
									gotoxy(23, 15);   printf("호텔을 지었습니다!!");
									system("pause>NULL");
									city[i].building = 4;
									pl[pl_num].cash = pl[pl_num].cash - city[i].price * 1.8;
									gotoxy(city[i].x + 1, city[i].y - 3);
									textcolor(pl[pl_num].textcolor, WHITE);
									printf("ＶＢＨ");
									textcolor(BLACK, WHITE);
									player_update(city, pl);
									clear();
									break;
								}
							case 4:
								clear();
								gotoxy(23, 15);   printf("건설안합니다!!");
								system("pause>NULL");
								clear();
								break;
							default:
								clear();
								gotoxy(23, 15);   printf("잘못입력하셨구먼유");
								system("pause>NULL");
								clear();
								break;
							}

						}
					}
					else {
						clear();
						gotoxy(23, 15);   printf("도시의 소유자가 아닙니다. 기회를 잃으셨습니다.");
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
		gotoxy(29, 15);   printf("씽크홀이당!!!!!!");
		gotoxy(29, 17);   printf("어디로가능거야!!!!!!!!!!!");
		system("pause>NULL");
		clear();
		srand(time(NULL));
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
		printf("  "); //공백삽입으로 지움
		pl[pl_num].pos = rand() % 28;
		gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
		textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
		printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
		textcolor(BLACK, WHITE); //폰트 초기화
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
		gotoxy(23, 15); printf("강원랜드에 도착하였습니다.");
		gotoxy(23, 17); printf("게임 참가비는 100만원 입니다. 게임 하시겠습니까?");
		gotoxy(23, 19); printf("1. 참가하겠습니다.");
		gotoxy(23, 21); printf("2. 아니요. 참가하지 않겠습니다.");
		gotoxy(23, 23); printf("선택 >>> ");
		getch();
		gotoxy(32, 23); scanf("%d", &game_sel);
		clear();
		while (1) {
			switch (game_sel) {
			case 1:
				gotoxy(23, 17); printf("원하는 게임을 선택하세요 : ");
				gotoxy(23, 19); printf("1.가위바위보");
				gotoxy(23, 21); printf("2.참참참");
				gotoxy(50, 17); scanf("%d", &game_sel);
				clear();
				while (1) {
					switch (game_sel) {
					case 1:
						game_rsp(city, pl, pl_num); break;
					case 2:
						Chamchamcham(city, pl, pl_num); break;
					default:
						gotoxy(23, 23); printf("잘못 선택하셨습니다. 다시 선택하세요.");
					}
					clear();
					return;
				}
			case 2:
				return;
			default:
				gotoxy(23, 23); printf("잘못 선택하셨습니다. 다시 선택하세요.");
			}
		}
	}

	if (pl[pl_num].pos == 7) {
		int prison_choice;
		gotoxy(23, 15); printf("교도소에 도착하였습니다.");
		if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17);
			printf("교도소 탈출카드를 소유하고계십니다. 사용하시겟습니까?");
			gotoxy(23, 19);
			printf("1. 네 2. 아니오 : ");
			gotoxy(42, 19);
			getch();
			scanf("%d", &prison_choice);
			clear();
			switch (prison_choice)
			{
			case 1:
				gotoxy(23, 15); printf("교도소 탈출권을 사용합니다...");
				gotoxy(23, 17); printf("다음턴부터 탈출합니다.");
				system("pause>NULL");
				pl[pl_num].pause_move = 0;
				pl[pl_num].goldkey = 0;
				clear();
				break;
			case 2:
				gotoxy(23, 16); printf("교도소 탈출권을 사용하지않습니다...");
				clear();
				break;
			default:
				printf("잘못입력하셨습니다. 다음턴으로.....");
			}
		}
		else {
			gotoxy(23, 17); printf("3턴간 이동이 제한됩니다.");
			gotoxy(23, 19); printf("탈출시도는 다음 턴 부터 가능합니다.");
			system("pause>NULL");
			clear();
			pl[pl_num].pause_move = 3;
			return;
		}
		return;
	}

	if (pl[pl_num].pos == 26) {
		if (city->bail == 0) {
			gotoxy(23, 15); printf("누적된 보석금이 없습니다.");
			system("pause>NULL");
			clear();
			return;
		}
		else {
			clear();
			gotoxy(23, 15);   printf("보석금을 받습니다.");
			gotoxy(23, 17);   printf("누적금액은 %d만원 입니다.", city->bail);   //누적금액 변수 변경
			pl[pl_num].cash += city->bail;
			system("pause>NULL");
			player_update(city, pl);
			city->bail = 0;
			clear();
			return;
		}
	}

	if (city[pl[pl_num].pos].owner == 6) { // 도시도착 땅 주인 없을때
		gotoxy(23, 15);   printf("\"%s\"에 도착하였습니다. \"%s\"의 가격은 %d만원 입니다.", city[pl[pl_num].pos].city_name, city[pl[pl_num].pos].city_name, city[pl[pl_num].pos].price);
		gotoxy(23, 17);   printf("도시를 구매하시겠습니까?");
		gotoxy(23, 19);   printf("1. 네 2. 아니오 : ");
		_getch();
		scanf("%d", &answer);

		if (answer == 1) { //땅구매
			if (pl[pl_num].cash - city[pl[pl_num].pos].price < 0) {
				clear();
				gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
				gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
				system("pause>NULL");
			}
			else {
				clear();
				pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price;
				player_update(city, pl);
				gotoxy(23, 15);   printf("도시를 구매하였습니다!!");
				gotoxy(23, 17);   system("pause>null");
				//플레이어 정보 업데이트.
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
			gotoxy(23, 15);   printf("건물을 구매하지 않다니 좋은선택이군..");
			system("pause>null");
			clear();
		}

		else {
			clear();
			gotoxy(23, 15);   printf("잘못입력하셨구먼유");
			system("pause>null");
			clear();
		}
		ServerSend(city, pl);
	}

	if (city[pl[pl_num].pos].owner == pl[pl_num].symbolnum) {  //내 도시일때
		if (pl[pl_num].pos == 4 || pl[pl_num].pos == 8 || pl[pl_num].pos == 12 || pl[pl_num].pos == 16 || pl[pl_num].pos == 23) {
			return;
		}

		gotoxy(23, 15);   printf("\"%s\"는 당신의 땅입니다.", city[pl[pl_num].pos].city_name);
		if (city[pl[pl_num].pos].building == 5) {
			gotoxy(23, 17);   printf("더이상 건설할 수 없습니다.");
			system("pause>NULL");
			return;
		}

		if (city[pl[pl_num].pos].building == 4) {
			gotoxy(23, 17);   printf("랜드마크를 건설하시겠습니까??");
			gotoxy(23, 19);   printf("1. 예  2. 아니오 :");
			getch();
			scanf("%d", &answer);
			if (answer == 1) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 2 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					return;
				}
				else {
					clear();
					gotoxy(23, 17);   printf("랜드마크를 건설합니다.");
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
				gotoxy(23, 15);   printf("싫으면 말아유");
				system("pause>NULL");
				clear();
			}
			else {
				gotoxy(23, 15);   printf("잘못입력하셨구먼유");
				system("pause>NULL");
				clear();

			}
		}
		else {
			gotoxy(23, 17); printf("지을 건물을 선택해주세요.");
			gotoxy(23, 19);   printf("1. 별장 2. 빌딩 3. 호텔 4. 건설안해! : ");
			scanf("%d", &answer);
			switch (answer) {
			case 1:
				if (city[pl[pl_num].pos].building == 2) {
					clear();
					gotoxy(23, 15);   printf("이미 별장이 지어져있습니다. 다시 선택해주세요!");
					system("pause>NULL");
					clear();
					break;
				}
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 0.5 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("별장을 지었습니다!!");
					system("pause>NULL");
					city[pl[pl_num].pos].building = 2;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 0.5;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("Ｖ");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear();
					break;
				}
			case 2:
				if (city[pl[pl_num].pos].building == 3) {
					clear();
					gotoxy(23, 15);   printf("이미 빌딩이 지어져있습니다. 다시 선택해주세요!");
					system("pause>NULL");
					clear();
					break;
				}
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 1.2 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("빌딩을 지었습니다!!");
					gotoxy(23, 17);   system("pause>null");
					city[pl[pl_num].pos].building = 3;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 1.2;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("ＶＢ");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear(); break;
				}
			case 3:
				if (pl[pl_num].cash - city[pl[pl_num].pos].price * 1.8 < 0) {
					clear();
					gotoxy(23, 15);   printf("Money가 부족합니다. 구매가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					clear();
					gotoxy(23, 15);   printf("호텔을 지었습니다!!");
					system("pause>NULL");
					city[pl[pl_num].pos].building = 4;
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 1.8;
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("ＶＢＨ");
					textcolor(BLACK, WHITE);
					player_update(city, pl);
					clear();
					break;
				}
			case 4:
				clear();
				gotoxy(23, 15);   printf("건설안합니다!!");
				system("pause>NULL");
				clear();
				break;
			default:
				clear();
				gotoxy(23, 15);   printf("잘못입력하셨구먼유");
				system("pause>NULL");
				clear();
				break;
			}
		}
		ServerSend(city, pl);
		clear();
		return;
	}

	if (city[pl[pl_num].pos].owner != pl[pl_num].symbolnum && city[pl[pl_num].pos].owner != 6) { // 다른플레이어 땅일때
		gotoxy(23, 15);   printf("통행료를 지불해야합니다.");
		system("pause>NULL");
		clear();
		if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 15);   printf("천사카드를 소유하고 계십니다. 사용하시겠습니까요?");
			gotoxy(23, 17);   printf("1. 네   2. 아니오 : ");
			scanf("%d", &answer);
			clear();
			if (answer == 1) {
				gotoxy(23, 15);   printf("천사카드를 사용합니다. 통행료가 면제됩니다.");
				system("pause>NULL");
				pl[pl_num].goldkey = 0;
				Buy_City(city, pl, pl_num);
				return;
			}
			else {
				clear();
				gotoxy(23, 15);   printf("천사카드를 사용하지 않습니다.");
				gotoxy(23, 17);   printf("통행료를 지불합니다.");
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
					gotoxy(23, 15);   printf("통행료(%d만원)을 지불하였습니다.", city[pl[pl_num].pos].price / 10);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("통행료를 낼 돈이 없습니다. 지역을 매각해야합니다.");
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
					gotoxy(23, 15);   printf("통행료(%d만원)을 지불하였습니다.", city[pl[pl_num].pos].price / 2);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("통행료를 낼 돈이 없습니다. 지역을 매각해야합니다.");
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
					gotoxy(23, 15);   printf("통행료(%d만원)을 지불하였습니다.", (city[pl[pl_num].pos].price * 2));
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("통행료를 낼 돈이 없습니다. 지역을 매각해야합니다.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			case 4:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price * 3) >= 0) {

					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 3;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price * 3;
					player_update(city, pl);
					gotoxy(23, 15); printf("통행료(%d만원)을 지불하였습니다.", city[pl[pl_num].pos].price * 3);
					system("pause>NULL");
					Buy_City(city, pl, pl_num);
				}
				else {
					gotoxy(23, 15);   printf("통행료를 낼 돈이 없습니다. 지역을 매각해야합니다.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			case 5:
				if (pl[pl_num].cash - (city[pl[pl_num].pos].price * 4) >= 0) {
					pl[pl_num].cash = pl[pl_num].cash - city[pl[pl_num].pos].price * 4;
					pl[city[pl[pl_num].pos].owner].cash = pl[city[pl[pl_num].pos].owner].cash + city[pl[pl_num].pos].price * 4;
					player_update(city, pl);
					gotoxy(23, 15); printf("통행료(%d만원)을 지불하였습니다.", city[pl[pl_num].pos].price * 4);
					system("pause>NULL");
				}
				else {
					gotoxy(23, 15);   printf("통행료를 낼 돈이 없습니다. 지역을 매각해야합니다.");
					system("pause>NULL");
					Sale_City(city, pl, pl_num);
				}
				break;
			default:
				gotoxy(23, 15);   printf("오류");
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
	gotoxy(23, 15); printf("플레이어%d 선택중...", play.turn + 1);
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
인수:
	if (city[pl[pl_num].pos].building == 5) {
		gotoxy(23, 15);   printf("랜드마크는 인수할 수 없습니다.");
		system("pause>NULL");
		clear();
		return;
	}
	else {
		int answer = 0;
		clear();
		gotoxy(23, 15);
		printf("도시를 인수합니다. 도시를 인수하시겠습니까??");
		gotoxy(23, 17);
		if (city[pl[pl_num].pos].building == 1) {
			printf("이 도시의 인수비용은 %d만원 입니다.", city[pl[pl_num].pos].price * 1);
		}
		else if (city[pl[pl_num].pos].building == 2) {
			printf("이 도시의 인수비용은 %d만원 입니다.", city[pl[pl_num].pos].price * 2);
		}
		else if (city[pl[pl_num].pos].building == 3) {
			printf("이 도시의 인수비용은 %d만원 입니다.", city[pl[pl_num].pos].price * 3);
		}
		else if (city[pl[pl_num].pos].building == 4) {
			printf("이 도시의 인수비용은 %d만원 입니다.", city[pl[pl_num].pos].price * 4);
		}
		else {
			gotoxy(23, 15);   printf("랜드마크는 인수할 수 없습니다.");
			system("pause>NULL");
			clear();
		}
		gotoxy(23, 19);
		printf("1. 예   2.아니오 : ");
		scanf("%d", &answer);
		clear();
		switch (answer) {
		case 1:

			gotoxy(23, 15);	printf("도시를 인수합니다.");
			system("pause>null");
			clear();
			if (city[pl[pl_num].pos].building == 1) {
				if (pl[pl_num].cash - city[pl[pl_num].pos].price < 0) {
					clear();
					gotoxy(23, 15);   printf("Money가 부족합니다. 인수가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("인수비용 (%d만원)을 지불하고 도시를 인수합니다.", city[pl[pl_num].pos].price);
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
					gotoxy(23, 15);   printf("Money가 부족합니다. 인수가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("인수비용 (%d만원)을 지불하고 도시를 인수합니다.", city[pl[pl_num].pos].price * 2);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 2;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 2;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 2;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("Ｖ");
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
					gotoxy(23, 15);   printf("Money가 부족합니다. 인수가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}
				else {
					gotoxy(23, 15);   printf("인수비용 (%d만원)을 지불하고 도시를 인수합니다.", city[pl[pl_num].pos].price * 3);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 3;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 3;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 3;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("ＶＢ");
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
					gotoxy(23, 15);   printf("Money가 부족합니다. 인수가  Impossible...T^T..");
					gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
					system("pause>NULL");
					break;
				}

				else {
					gotoxy(23, 15);   printf("인수비용 (%d만원)을 지불하고 도시를 인수합니다.", city[pl[pl_num].pos].price * 4);
					system("pause>NULL");
					pl[city[pl[pl_num].pos].owner].cash += city[pl[pl_num].pos].price * 4;
					pl[pl_num].cash -= city[pl[pl_num].pos].price * 4;
					city[pl[pl_num].pos].owner = pl[pl_num].symbolnum;
					city[pl[pl_num].pos].building = 4;
					gotoxy(city[pl[pl_num].pos].x, city[pl[pl_num].pos].y - 1);
					textcolor(pl[pl_num].textcolor, WHITE);
					printf("%s", city[pl[pl_num].pos].city_name);
					gotoxy(city[pl[pl_num].pos].x + 1, city[pl[pl_num].pos].y - 3);
					printf("ＶＢＨ");
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
				gotoxy(23, 15);   printf("잘못된 접근입니다.");
				gotoxy(23, 17);   printf("다음턴으로 넘어갑니다.");
				system("pause>NULL");
				break;
			}
		case 2:
			gotoxy(23, 15);   printf("도시를 인수하지 않습니다.");
			system("pause>NULL");
			clear();
			break;
		default:
			gotoxy(23, 15);   printf("잘못 선택하셨습니다. 다시 입력해주세요.");
			system("pause>NULL");
			clear();
			goto 인수;
		}
		return;
	}
}

void Sale_City(CityInfo city[], Player* pl, int pl_num) {

	clear();
	int count = 0;
	char mg[20] = { 0, };
maegak:
	clear(); gotoxy(23, 15); printf("(파산 입력시 파산, 그만 입력시 매각중지)");
	gotoxy(23, 17); printf("매각할 도시를 입력하세요 : "); scanf("%s", mg);
	if (!strcmp(mg, "그만")) {
		gotoxy(23, 15); printf("매각을 중지합니다.");
		system("pause>NULL");
		Arrive(city, pl, pl_num);
		return;
	}
	if (!strcmp(mg, "파산")) {
		pl[pl_num].cash = -999;
		Bankruptcy(city, pl, pl_num);
		return;
	}
	for (int i = 0; i < 28; i++) {
		if (!strcmp(mg, city[i].original)) {
			if (city[i].owner == pl[pl_num].symbolnum) {
				clear();
				gotoxy(23, 15); printf("선택한 도시 %s를 매각합니다.", city[i].original);
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
				printf("\"%s\"는 내 도시가 아닙니다.", mg);
				gotoxy(23, 17);
				system("pause>null");
				goto maegak;
			}
			return;
		}
	}
}

void player_update(CityInfo city[], Player* pl) {//플레이어 1234

	for (int i = 0; i < pixed_pl_num; i++) {
		switch (i) {
		case 0:   //플레이어1
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);   //컬러 불러오기
			gotoxy(2, 1); printf("player 1  %s", pl[i].mysymbol);   //나중에 업데이트 함수로 빼기
			gotoxy(28, 1); printf("현  금 : %d 만 원", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 1:   //플레이어2
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(60, 1); printf("player 2  %s", pl[i].mysymbol);
			gotoxy(86, 1); printf("현  금 : %d 만 원", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 2:   //플레이어3
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(2, 61); printf("player 3  %s", pl[i].mysymbol);
			gotoxy(28, 61); printf("현  금 : %d 만 원", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		case 3:   //플레이어4
			if (pl[i].cash < 0) {
				break;
			}
			textcolor(pl[i].textcolor, WHITE);
			gotoxy(60, 61); printf("player 4  %s", pl[i].mysymbol);
			gotoxy(86, 61); printf("현  금 : %d 만 원", pl[i].cash);
			textcolor(BLACK, WHITE);
			break;
		}
	}
}

void Ktx(CityInfo city[], Player* pl, int pl_num)   //city좌표들, 도착할 city, 움직일 player
{
	int i;
	char destination[20];

	if (pl[pl_num].pos == 21) {   //pos가 KTX(=21)면 조건문 실행
	KTX:
		clear(); gotoxy(23, 15); printf("KTX를 탔습니다. 내릴 곳을 입력하세요 : "); _getch();
		Cursor(); scanf("%s", destination); clear();
		if (!strcmp(destination, "KTX")) { //KTX 계속 이용 방지
			gotoxy(23, 17); printf("KTX를 목적지로 설정할 수 없습니다."); getch();
			goto KTX;
		}
		for (i = 0; i < 28; i++) {
			if (!strcmp(destination, city[i].original)) {
				if (i == 10) {
					int sel;
					Cursor(); clear();
					gotoxy(23, 15); printf("이동할 황금열쇠 지역을 선택하세요.");
					gotoxy(23, 17);   printf("1.왼쪽 2.위쪽 3.오른쪽 : ");
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
						move_player(city, i - (pl[pl_num].pos), pl, pl_num);   //KTX 다음의 도시라면
					else
						move_player(city, 28 - (pl[pl_num].pos) + i, pl, pl_num);   //KTX 이전의 도시라면
					break;
				}
			}
		}
		if (i == 28) {   //잘못입력했을 떄
			clear();
			gotoxy(23, 15);
			printf("목적지를 잘못 말해서 KTX를 놓쳤습니다.");
			gotoxy(23, 17);
			printf("엔터를 누르면 다음 열차가 도착합니다...");
			getch();
			goto KTX;
		}
		gotoxy(23, 17);   printf("KTX를 타고 %s에 왔습니다.", city[pl[pl_num].pos % 28].city_name);
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
	gotoxy(23, 15); printf("더블이 아니네요. 보석금 150만원을 내고 이동하시겠습니까? ");
	gotoxy(23, 17); printf("1. 네. 지불하겠습니다.");
	gotoxy(23, 19); printf("2. 아니요.");
	gotoxy(23, 21); printf("선택 >>> ");
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
			gotoxy(23, 15); printf("더블이 나오면 교도소를 나가실 수 있습니다."); //화면에 출력되는지 확인
			gotoxy(23, 17); printf("주사위를 던져주세요.");
			system("pause>NULL");
			break;
		default:
			printf(23, 15); printf("잘못 선택하셨습니다. 다시 선택하세요.");
			system("pause>NULL");
		}
		clear();
		return;
	}
}

void Bankruptcy(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("돈이 부족해서 파산 처리 됩니다.");
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
			gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1); //플레이어의 현재위치
			printf("  "); //공백삽입으로 지움
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

//강원랜드 미니게임 관련 함수
void game_rsp(CityInfo city[], Player* pl, int pl_num) {
	int user, com;
	srand(time(NULL));
	gotoxy(26, 15); printf("### 가위 바위 보 게임 ###\n");
	gotoxy(26, 17); printf("선택하세요. (1.가위 2.바위 3.보) : ");
다시입력:
	scanf("%d", &user);
	Nocursor();
	while (1) {
		printf_rsp(user);
		com = printf_rsp2(rand() % 3 + 1);
		if (_kbhit()) {
			if (user < 1 || user>3) {
				gotoxy(43, 45); printf("잘못 입력하셨습니다.\n"); goto 다시입력;
			}
			else if (com - user == 0) {
				gotoxy(43, 45); printf("비겼으니까 봐줄게 흥!!!"); break;
			}
			else if (com - user == 1 || com - user == -2) {
				gotoxy(39, 45); printf("100만원 꿀꺽 개이득 아앙~♡ 갸꿀띠!!!");
				pl[pl_num].cash -= 100;
				player_update(city, pl);
				break;
			}
			else {
				gotoxy(43, 45); printf("운이 좋군.. 후후후...★");
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
	printf("[나는 건민이]");
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
	printf("[나는 악마]");
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
다시입력:
	gotoxy(26, 15); printf("### 참참참 ###\n");
	gotoxy(26, 17); printf("선택하세요. (1. 좌   2. 우) : ");
	scanf("%d", &user);
	Nocursor();
	while (1) {
		com = Print_Cham(rand() % 2 + 1);
		if (_kbhit()) {
			if (user < 1 || user>2) {
				gotoxy(23, 19); printf("잘못입력하셨습니다.\n");   goto 다시입력;
			}
			if (user == com) {
				gotoxy(43, 45); printf("붸에에에에롱~~~푸에에에에롱");
				pl[pl_num].cash -= 100;
				player_update(city, pl);
				break;
			}
			else {
				gotoxy(33, 45);   printf("거세게 이겨봐라 나는 돌..덩...힝....((#  ..ㅜ) ~ <3   ");
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

//황금열쇠 관련 함수
void Gold_key(Player* pl, CityInfo city[], int pl_num) {
	int event_num; //랜덤값 담을 변수
	int num;   //사용자메뉴얼 입력변수
	int angel = 1, outland = 2, //1.천사카드 2.무인도탈출권
		choice = 3, Go_ktx = 4, Go_start = 5, Go_hole = 6;   //3.감옥으로이동 4.KTX 5.출발지로이동 6.싱크홀
	Nocursor();
	srand((unsigned)time(NULL));
	event_num = rand() % 6 + 1;
	if (event_num == 1) {
		gotoxy(23, 15); printf("천사카드가 나왔습니다.");
		if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 17); printf("천사카드를 소유하고계십니다.");
			gotoxy(23, 19); printf("천사카드는 1장만 소유가능합니다.");
		}
		else if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17); printf("교도소카드를 소유하고계십니다.");
			gotoxy(23, 19); printf("1.교체 2.버림 : ");
			gotoxy(38, 19); scanf("%d", &num);
			clear();
			switch (num)
			{
			case 1:
				gotoxy(23, 15); printf("교도소카드로 교체합니다.");
				pl[pl_num].goldkey = 2; break;
			case 2:
				gotoxy(23, 15); printf("카드를 버립니다."); break;
			}
		}
	}
	else if (event_num == 2) {
		gotoxy(23, 15); printf("교도소카드가 나왔습니다.");
		if (pl[pl_num].goldkey == 2) {
			gotoxy(23, 17); printf("교도소카드를 소유하고계십니다.");
			return;
		}
		else if (pl[pl_num].goldkey == 1) {
			gotoxy(23, 17); printf("천사카드를 소유하고계십니다.");
			gotoxy(23, 19); printf("1.교체 2.버림 : ");
			gotoxy(38, 19);  scanf("%d", &num);
			clear();
			switch (num)
			{
			case 1:
				gotoxy(23, 15); printf("천사카드로 교체합니다.");
				pl[pl_num].goldkey = 1; break;
			case 2:
				gotoxy(23, 15); printf("카드를 버립니다."); break;
			}
		}
	}
	else if (event_num == 3) {
		G_prison(city, pl, pl_num); //무인도
	}
	else if (event_num == 4) {
		gotoxy(23, 15); printf("KTX로 이동합니다.");
		system("pause>null");
		clear();
		gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
		printf("  "); //공백삽입으로 지움
		pl[pl_num].pos = 21;
		gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
		textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
		printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
		textcolor(BLACK, WHITE); //폰트 초기화
		Nocursor();
		Arrive(city, pl, pl_num);
	}
	else if (event_num == 5) {
		G_start(city, pl, pl_num); //출발지로 이동
	}
	else if (event_num == 6) {
		G_hole(city, pl, pl_num); //싱크홀
	}
	system("pause>NULL"); clear();
}

void G_prison(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("교도소로 이동합니다....");
	system("pause>null");
	clear();
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
	printf("  "); //공백삽입으로 지움
	pl[pl_num].pos = 7;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
	printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
	textcolor(BLACK, WHITE); //폰트 초기화
	Nocursor();
	Arrive(city, pl, pl_num);
}

void G_start(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(23, 15); printf("출발지로 이동합니다.");
	system("pause>NULL");
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
	printf("  "); //공백삽입으로 지움
	pl[pl_num].pos = 0;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
	printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
	textcolor(BLACK, WHITE); //폰트 초기화
	Nocursor();
	Arrive(city, pl, pl_num);
}

void G_hole(CityInfo city[], Player* pl, int pl_num) {
	clear();
	gotoxy(29, 15);   printf("씽크홀이당!!!!!!");
	gotoxy(29, 17);   printf("어디로가능거야!!!!!!!!!!!");
	system("pause>NULL");
	srand(time(NULL));
	gotoxy(city[pl[pl_num].pos % 28].x + pl_num * 2, city[pl[pl_num].pos % 28].y + 1); //플레이어의 현재위치
	printf("  "); //공백삽입으로 지움
	pl[pl_num].pos = rand() % 28;
	gotoxy(city[pl[pl_num].pos].x + pl_num * 2, city[pl[pl_num].pos].y + 1);
	textcolor(pl[pl_num].textcolor, WHITE); //플레이어 색깔 불러오기
	printf("%s", pl[pl_num].mysymbol); //플레이어 심볼 출력
	textcolor(BLACK, WHITE); //폰트 초기화
	system("pause>NULL");
	clear();
	Arrive(city, pl, pl_num);
}

void GameOver(CityInfo city[], Player* pl) {
	clear();
	for (int i = 0; i < pixed_pl_num; i++) {
		pl[i].rank = 1;
		for (int j = 0; j < 28; j++) {
			if (city[j].owner == i) {   //도시 소유자 (0:무소유 1:player1소유 2:player2소유 3:player3소유 4:player4소유)
				switch (city[j].building) {
				case 1:
					pl[i].cash += city[j].price; break;   //땅
				case 2:
					pl[i].cash += city[j].price * 0.5; break;   //별장
				case 3:
					pl[i].cash += city[j].price * 1.2; break;   //빌딩
				case 4:
					pl[i].cash += city[j].price * 1.8; break;   //호텔
				case 5:
					pl[i].cash += city[j].price * 2; break;   //랜드마크
				}
			}
		}
	}
	player_update(city, pl); gotoxy(23, 15); printf("Game Over...");
	gotoxy(23, 17); system("pause>null"); clear();
	for (int i = 0; i <= 27; i++) {
		draw_box_end(2 + i * 2, i + 4, 110 - i * 2, 58 - i, "■"); Sleep(20);
	}
	for (int i = 0; i <= 27; i++) {
		draw_box_end(2 + i * 2, i + 4, 110 - i * 2, 58 - i, "□"); Sleep(20);
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
		gotoxy(35, 18); printf("Player%d 이깅!!!", pl_rank + 1);
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

	gotoxy(5, 1); textcolor(RED, WHITE); printf("■■      ■■■    ■■■■        ■■■          ■■■        ■■   \n");
	gotoxy(5, 2); printf("■■     ■■■   ■■    ■■      ■■■          ■■■       ■■■\n");
	gotoxy(5, 3); printf("■■    ■■■  ■■        ■■    ■■■■      ■■■■      ■■■■\n");
	gotoxy(5, 4); printf("■■   ■■■  ■■          ■■   ■■■■      ■■■■     ■■■■■   \n");
	gotoxy(5, 5); printf("■■  ■■■  ■■            ■■  ■■ ■■    ■■ ■■     ■■  ■■   \n");
	gotoxy(5, 6); printf("■■ ■■■   ■■■   ■■■ ■■  ■■  ■■  ■■  ■■    ■■    ■■   \n");
	gotoxy(5, 7); textcolor(BLUE, WHITE); printf("■■■■■    ■■ ■■■   ■■■  ■■  ■■  ■■  ■■    ■■    ■■       \n");
	gotoxy(5, 8); printf("■■ ■■■   ■■            ■■  ■■  ■■  ■■  ■■   ■■■■■■■   \n");
	gotoxy(5, 9); printf("■■  ■■■  ■■            ■■  ■■   ■■■■   ■■   ■■■■■■■   \n");
	gotoxy(5, 10); printf("■■   ■■■  ■■          ■■   ■■   ■■■■   ■■  ■■        ■■   \n");
	gotoxy(5, 11); printf("■■    ■■■  ■■        ■■    ■■     ■■     ■■  ■■        ■■   \n");
	gotoxy(5, 12); printf("■■     ■■■   ■■    ■■      ■■     ■■     ■■ ■■          ■■   \n");
	gotoxy(5, 13); printf("■■      ■■■    ■■■■        ■■      ■      ■■ ■■          ■■   \n");

	gotoxy(x - 2, y); textcolor(BLACK, WHITE); printf("▶ 게 임 시 작");
	gotoxy(x, y + 2);  printf(" 게 임 정 보");
	gotoxy(x, y + 4);  printf("  종    료 ");

	while (1) {
		int n = keyControl();
		switch (n) {
		case UP: {
			if (y > 17) {
				gotoxy(x - 2, y);
				printf("  ");
				gotoxy(x - 2, y -= 2);
				printf("▶");
			}
			break;
		}
		case DOWN: {
			if (y < 21) {
				gotoxy(x - 2, y);
				printf("  ");
				gotoxy(x - 2, y += 2);
				printf("▶");
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
	gotoxy(5, 1); printf("■■      ■■■    ■■■■        ■■■          ■■■        ■■   \n");
	gotoxy(5, 2); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■     ■■■   ■■    ■■      ■■■          ■■■       ■■■\n");
	gotoxy(5, 3); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■    ■■■  ■■        ■■    ■■■■      ■■■■      ■■■■\n");
	gotoxy(5, 4); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■   ■■■  ■■          ■■   ■■■■      ■■■■     ■■■■■   \n");
	gotoxy(5, 5); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■  ■■■  ■■            ■■  ■■ ■■    ■■ ■■     ■■  ■■   \n");
	gotoxy(5, 6); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■ ■■■   ■■            ■■  ■■ ■■    ■■ ■■    ■■    ■■   \n");
	gotoxy(5, 7); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■■■■    ■■            ■■  ■■  ■■  ■■  ■■    ■■    ■■   \n");
	gotoxy(5, 8); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■ ■■■   ■■            ■■  ■■  ■■  ■■  ■■   ■■■■■■■   \n");
	gotoxy(5, 9); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■  ■■■  ■■            ■■  ■■   ■■■■   ■■   ■■■■■■■   \n");
	gotoxy(5, 10); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■   ■■■  ■■          ■■   ■■   ■■■■   ■■  ■■        ■■   \n");
	gotoxy(5, 11); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■    ■■■  ■■        ■■    ■■     ■■     ■■  ■■        ■■   \n");
	gotoxy(5, 12); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■     ■■■   ■■    ■■      ■■     ■■     ■■ ■■          ■■   \n");
	gotoxy(5, 13); komarand = rand() % 14 + 1; textcolor(komarand, WHITE); printf("■■      ■■■    ■■■■        ■■      ■      ■■ ■■          ■■   \n");
	Sleep(50);
	system("cls");

}

int Choice_player() {
	system("cls");
	Nocursor();
	int x = 28;
	int y = 16;

	gotoxy(29, 7); printf("★ K O R E A   M A R V E L ★\n");
	gotoxy(30, 10); printf("Choose The Number Of Player\n");
	gotoxy(28, 14); printf("2P             3P             4P\n");
	gotoxy(x, y);	 printf("▲");

	while (1) {
		int n = keyControl();
		switch (n) {
		case RIGHT: {
			if (x < 53) {
				gotoxy(x, y); printf("  ");
				gotoxy(x += 15, y); printf("▲");
			}
			break;
		}
		case LEFT: {
			if (x > 29) {
				gotoxy(x, y); printf("  ");
				gotoxy(x -= 15, y); printf("▲");
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
	gotoxy(4, 2); printf("┌──────────────────┬──────────────────┬──────────────────┬──────────────────┐");
	gotoxy(4, 3); printf("│                  │                  │                  │                  │");
	gotoxy(4, 4); printf("│                  │                  │                  │                  │");
	gotoxy(4, 5); printf("│                  │                  │                  │                  │");
	gotoxy(4, 6); printf("│                  │                  │                  │                  │");
	gotoxy(4, 7); printf("│                  │                  │                  │                  │");
	gotoxy(4, 8); printf("│                  │                  │                  │                  │");
	gotoxy(4, 9); printf("│                  │                  │                  │                  │");
	gotoxy(4, 10); printf("│                  │                  │                  │                  │");
	gotoxy(4, 11); printf("│                  │                  │                  │                  │");
	gotoxy(4, 12); printf("│                  │                  │                  │                  │");
	gotoxy(4, 13); printf("│                  │                  │                  │                  │");
	gotoxy(4, 14); printf("│                  │                  │                  │                  │");
	gotoxy(4, 15); printf("│                  │                  │                  │                  │");
	gotoxy(4, 16); printf("│                  │                  │                  │                  │");
	gotoxy(4, 17); printf("│                  │                  │                  │                  │");
	gotoxy(4, 18); printf("│                  │                  │                  │                  │");
	gotoxy(4, 19); printf("│                  │                  │                  │                  │");
	gotoxy(4, 20); printf("└──────────────────┴──────────────────┴──────────────────┴──────────────────┘");
	gotoxy(4, 21); printf("┌───────────────────────────────────────────────────────────────────────────┐");
	gotoxy(4, 22); printf("│                                                                           │");
	gotoxy(4, 23); printf("│                                                                           │");
	gotoxy(4, 24); printf("│                                                                           │");
	gotoxy(4, 25); printf("│                                                                           │");
	gotoxy(4, 26); printf("│                                                                           │");
	gotoxy(4, 27); printf("│                                                                           │");
	gotoxy(4, 28); printf("│                                                                           │");
	gotoxy(4, 29); printf("│                                                                           │");
	gotoxy(4, 30); printf("│                                                                           │");
	gotoxy(4, 31); printf("│                                                                           │");
	gotoxy(4, 32); printf("│                                                                           │");
	gotoxy(4, 33); printf("│                                                                           │");
	gotoxy(4, 34); printf("│                                                                           │");
	gotoxy(4, 35); printf("│                                                                           │");
	gotoxy(4, 36); printf("│                                                                           │");
	gotoxy(4, 37); printf("├───────────────────────────────────────────────────────────────────────────┤");
	gotoxy(4, 38); printf("│                                                                           │");
	gotoxy(4, 39); printf("│                                                                           │");
	gotoxy(4, 40); printf("└───────────────────────────────────────────────────────────────────────────┘");
	textcolor(GRAY2, WHITE); gotoxy(8, 22); printf("채팅창에 Ready!를 입력하면 준비완료가 됩니다.");
	textcolor(GRAY2, WHITE); gotoxy(8, 23); printf("모든 플레이어가 Ready! 상태가 되면 5초 후 게임이 시작됩니다.");
	textcolor(GRAY2, WHITE); gotoxy(69, 42); printf("KoreaMarble");
	textcolor(GRAY2, WHITE); gotoxy(29, 43); printf("Copyright ⓒ 2020. BoramSamJo Co.all rights reserved");
	gotoxy(6, 38); printf("                                          "); gotoxy(6, 38);
	switch (i) {
	case 3:
		textcolor(RED, WHITE);
		gotoxy(71, 8);      printf("★");
		gotoxy(68, 11);   printf("Player 4");
		textcolor(BLACK, WHITE);
	case 2:
		textcolor(BLUE, WHITE);
		gotoxy(52, 8);      printf("♣");
		gotoxy(49, 11);   printf("Player 3");
		textcolor(BLACK, WHITE);
	case 1:
		textcolor(GREEN, WHITE);
		gotoxy(33, 8);      printf("◑");
		gotoxy(30, 11);   printf("Player 2");
		textcolor(BLACK, WHITE);
	case 0:
		textcolor(MAGENTA2, WHITE);
		gotoxy(14, 8);      printf("▩");
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

