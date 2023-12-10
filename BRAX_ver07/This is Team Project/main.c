#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <conio.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") // mmsystem.h

// 색상 정의
#define BLACK	0
#define BLUE1	1
#define GREEN1	2
#define CYAN1	3
#define RED1	4
#define MAGENTA1 5
#define YELLOW1	6
#define GRAY1	7
#define GRAY2	8
#define BLUE2	9
#define GREEN2	10
#define CYAN2	11
#define RED2	12
#define MAGENTA2 13
#define YELLOW2	14
#define WHITE	15

#define PLAYER "<-*->" // player1 표시
#define BLANK	' ' 
#define BRICK	"□" // 벽돌. 특수문자
#define BULLET	"│" // 총알
#define BOMB    "@" // 폭탄

#define ESC 0x1b //  ESC 누르면 종료

#define UP		'w' // WASD로 이동
#define DOWN	's'
#define LEFT	'a'
#define RIGHT	'd'
#define SPACE	' ' // 스페이스바로 발사
#define ITEM    'e'

#define WIDTH 50
#define HEIGHT 24

int Delay;// msec
int life; // 기회
int score; // 점수
int bombCount;
int brick_count;
int brick[WIDTH / 2][HEIGHT - 2] = { 0 }; // 1이면 벽돌이 있다는 뜻, 벽돌이 2byte 문자열이므로, 행을 반으로하고 이후에 2를 곱함
int bullet_count;
int bullet[WIDTH][HEIGHT - 2] = { 0 }; // 1이면 총알이 있다는 뜻
int bomb[WIDTH][HEIGHT - 2] = { 0 }; // 1이면 폭탄이 있다는 뜻
int bomb_count;
int called;

int frame_count; // 전체 frame
int brick_create_frame_sync; // 벽돌 생성 간격
int brick_frame_sync; //  frame 마다 한번씩 brick를 이동
int player_frame_sync; // 처음 시작은 10 frame 마다 이동, 즉, 100msec 마다 이동
int bullet_frame_sync; // 1 frame 마다 한번씩 bullet을 이동
int bomb_frame_sync; //1 frame 마다 한번씩 bomb을 이동

int p_oldx = 40, p_oldy = 20, p_newx = 40, p_newy = 20; //플레이어의 좌표

int crash; // 벽돌과 플레이어가 충돌했는지 판단하기 위한 변수
int breakBrick; // 벽돌을 부쉈는지 판단하는 변수

int randomColor;

typedef struct {
	char playerName[50];
	int playerScore;
	int rank;
} Player;

Player currentPlayer;

//커서 숨김
void removeCursor(void) { 
	CONSOLE_CURSOR_INFO curInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curInfo.bVisible = 0;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

//내가 원하는 위치로 커서 이동
void gotoxy(int x, int y)
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);// WIN32API 함수
}

//플레이어 기체 설정
void putplayer(int x, int y)
{
	gotoxy(p_newx - 2, p_newy);

	if (p_newx == 2) {
		gotoxy(p_newx, p_newy);
		printf("*->");
	}
	else if (p_newx == 3) {
		gotoxy(p_newx - 1, p_newy);
		printf("-*->");
	}
	else if (p_newx == 76) {
		printf("<-*-");
	}
	else if (p_newx == 77) {
		printf("<-*");
	}
	else {
		printf(PLAYER);
	}
}

void erasestar(int x, int y)
{
	gotoxy(x, y);
	putchar(BLANK);
}

void eraseplayer(int x, int y) {
	gotoxy(p_oldx - 2, p_oldy);

	if (p_oldx == 2) {
		gotoxy(p_oldx, p_oldy);
		printf("   ");
	}

	else if (p_oldx == 3) {
		gotoxy(p_oldx - 1, p_oldy);
		printf("    ");
	}

	else if (p_oldx == 76) {
		printf("    ");
	}

	else if (p_oldx == 77) {
		printf("   ");
	}

	else {
		printf("     ");
	}
}

//색상 설정
void textcolor(int fg_color, int bg_color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fg_color | bg_color << 4);
}

//화면 지우고 원하는 배경색으로 설정
void cls(int bg_color, int text_color)
{
	char cmd[100];
	system("cls");
	sprintf(cmd, "COLOR %x%x", bg_color, text_color);
	system(cmd);
}

//점수 표시
void showscore()
{
	textcolor(BLUE1, GRAY2);
	gotoxy(2, 0);
	printf("SCORE : %d", score);
	textcolor(WHITE, BLACK);
}

//폭탄 표시
void showbomb()
{
	textcolor(RED1, GRAY2);
	gotoxy(18, 0);
	printf("BOMB : %d", bombCount);
	textcolor(WHITE, BLACK);
}

//목숨 표시
void showLife()
{
	int i = 0;
	textcolor(YELLOW2, GRAY2);
	gotoxy(31, 0);
	printf("남은 생명 : ");
	for (; i < life; i++) {
		printf("♥");
	}
	for (; i < 5; i++) {
		printf("♡");
	}
	textcolor(WHITE, BLACK);
}

//벽돌 생성
void show_brick() {
	int x = rand() % (WIDTH / 2 - 3) + 1;
	int y = rand() % (HEIGHT / 5) + 2;
	gotoxy(2 * x, y);

	do {
		randomColor = rand() % 16;
	} while (randomColor == BLACK);

	textcolor(randomColor, BLACK);
	printf(BRICK);
	brick[x][y] = randomColor;  // 색 정보 저장
	++brick_count;
}

//벽돌 이동
void move_brick() {
	int x, y, newy = 0;
	int newbricks[WIDTH / 2][HEIGHT - 2] = { 0 };
	static int call_count = 0;

	if (brick_count == 0)
		return;

	for (x = 0; x < WIDTH / 2; x++) {
		for (y = 0; y < HEIGHT - 1; y++) {
			if (brick[x][y]) {
				newy = y + 1;
				if (newy < HEIGHT - 1) {
					gotoxy(2 * x, y);
					printf("  ");
					gotoxy(2 * x, newy);
					if ((2 * x >= p_newx - 2 && 2 * x <= p_newx + 2) && newy == p_newy) {
						// 깜빡임 효과
						for (int blink_count = 0; blink_count < 4; blink_count++) {
							textcolor(YELLOW1, BLACK);
							printf(BRICK);
							Sleep(250);  // 250밀리초 동안 대기
							gotoxy(2 * x, newy);
							printf("  ");
							Sleep(250);
						}
						textcolor(brick[x][y], BLACK);  // 깜빡임 이후 원래 색으로 설정
					}
					else {
						textcolor(brick[x][y], BLACK);  // 이동된 위치에서 이전 색으로 설정
						printf(BRICK);
					}
					newbricks[x][newy] = brick[x][y];  // 색 정보 이동
				}
				else {
					gotoxy(2 * x, y);
					printf("  ");
					--life;
					score = score > 10 ? score - 10 : 0;
				}
				if ((2 * x >= p_newx - 2 && 2 * x <= p_newx + 2) && newy == p_newy) {
					if (crash == 0) {
						gotoxy(2 * x, y);
						printf("  ");
						--life;
						score = score > 5 ? score - 5 : 0;
						brick[x][y] = 0;
						--brick_count;
					}
					else crash = 0;
				}
			}
		}
	}
	memcpy(brick, newbricks, sizeof(newbricks));
}

void flush_key()
{
	while (_kbhit())
		_getch();
}

//게임 박스 세팅
void draw_box(int x1, int y1, int x2, int y2, char* ch)
{
	for (int x = x1; x <= x2; x += 2) {
		gotoxy(x, y1);
		textcolor(WHITE, BLACK);
		printf("%s", ch);
		gotoxy(x, y2);
		textcolor(WHITE, BLACK);
		printf("%s", ch);
	}
	for (int y = y1; y < y2; ++y) {
		gotoxy(x1, y);
		textcolor(WHITE, BLACK);
		printf("%s", ch);
		gotoxy(x2, y);
		textcolor(WHITE, BLACK);
		printf("%s", ch);
	}
}

void draw_hline(int y, int x1, int x2, char ch)
{
	gotoxy(x1, y);
	for (; x1 <= x2; x1++)
		putchar(ch);
}

//플레이어 이동 세팅
void player(unsigned char ch)
{
	int move_flag = 0;
	static unsigned char last_ch = 0;

	if (!called) { // 처음 또는 Restart
		p_oldx = 40, p_oldy = 20, p_newx = 40, p_newy = 20;
		putplayer(p_oldx - 2, p_oldy);
		called = 1;
		last_ch = 0;
		ch = 0;
	}

	switch (ch) {
	case UP:
		if (p_oldy > 2) // 0 은 Status Line
			p_newy = p_oldy - 1;
		else { // 벽에 부딛치면 방향을 반대로 이동
			p_newy = p_oldy + 1;
		}
		move_flag = 1;
		break;
	case DOWN:
		if (p_oldy < HEIGHT - 2)
			p_newy = p_oldy + 1;
		else {
			p_newy = p_oldy - 1;
		}
		move_flag = 1;
		break;
	case LEFT:
		if (p_oldx > 2)
			p_newx = p_oldx - 1;
		else {
			p_newx = p_oldx + 1;
		}
		move_flag = 1;
		break;
	case RIGHT:
		if (p_oldx < WIDTH - 4)
			p_newx = p_oldx + 1;
		else {
			p_newx = p_oldx - 1;
		}
		move_flag = 1;
		break;
	}
	if (move_flag) {
		eraseplayer(p_oldx, p_oldy); // 이전 위치의 
		if (p_newx & 1) {// 홀수, 벽돌을 같이 지우기 위함
			erasestar(p_newx - 1, p_newy);
			erasestar(p_newx, p_newy);
		}
		else {
			erasestar(p_newx + 1, p_newy);
			erasestar(p_newx, p_newy);
		}
		putplayer(p_newx - 2, p_newy); // 새로운 위치에서 player를 표시
		p_oldx = p_newx; // 마지막 위치를 기억
		p_oldy = p_newy;

		if (brick[p_newx / 2][p_newy]) { // 부딪힘 체크
			if (crash == 0) {
				score = score > 5 ? score - 5 : 0;
				brick[p_newx / 2][p_newy] = 0;
				--brick_count;
				showscore();
				--life;
			}
			else crash = 0;
		}
	}
}

//초기화
void init_game()  
{
	int x, y;
	char cmd[100];

	srand(time(NULL));

	brick_count = 0; // 벽돌 초기화
	for (x = 0; x < WIDTH / 2; x++)
		for (y = 0; y < HEIGHT; y++)
			brick[x][y] = 0;

	bullet_count = 0; // 총알 초기화
	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++)
			bullet[x][y] = 0;

	bomb_count = 0;
	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++)
			bomb[x][y] = 0;

	// 변수 초기화
	score = 0;
	life = 5;
	bombCount = 1;
	Delay = 1;
	frame_count = 0;
	brick_create_frame_sync = 25;
	brick_frame_sync = 100;
	player_frame_sync = 1;
	bullet_frame_sync = 1;
	bomb_frame_sync = 2;
	crash = 0;
	breakBrick = 0;

	cls(BLACK, WHITE);
	sprintf(cmd, "mode con cols=%d lines=%d", WIDTH, HEIGHT);
	system(cmd);

	called = 0; // 플레이어 설정
	player(0);
}

//타이틀 화면 (BRAX -> 자작 이름)
void firstWindows(int sec) {
	if (sec & 1) {
		gotoxy(3, 2);
		textcolor(RED1, BLACK);
		printf("   ▣▣▣▣▣     ");
		textcolor(GREEN1, BLACK);
		printf("▣ ▣▣     ");
		textcolor(MAGENTA1, BLACK);
		printf("  ▣▣");
		textcolor(YELLOW1, BLACK);
		printf("      ▣     ▣");



		gotoxy(3, 3);
		textcolor(RED1, BLACK);
		printf("   ▣     ▣   ");
		textcolor(GREEN1, BLACK);
		printf("▣    ▣");
		textcolor(MAGENTA1, BLACK);
		printf("   ▣    ▣  ");
		textcolor(YELLOW1, BLACK);
		printf("   ▣   ▣");


		gotoxy(3, 4);
		textcolor(RED1, BLACK);
		printf("   ▣     ▣   ");
		textcolor(GREEN1, BLACK);
		printf("▣    ▣");
		textcolor(MAGENTA1, BLACK);
		printf("  ▣      ▣ ");
		textcolor(YELLOW1, BLACK);
		printf("    ▣ ▣");


		gotoxy(3, 5);
		textcolor(RED1, BLACK);
		printf("   ▣ ▣▣▣     ");
		textcolor(GREEN1, BLACK);
		printf("▣ ▣▣▣");
		textcolor(MAGENTA1, BLACK);
		printf("   ▣ ▣ ▣ ▣ ▣");
		textcolor(YELLOW1, BLACK);
		printf("     ▣");


		gotoxy(3, 6);
		textcolor(RED1, BLACK);
		printf("   ▣     ▣   ");
		textcolor(GREEN1, BLACK);
		printf("▣ ▣");
		textcolor(MAGENTA1, BLACK);
		printf("     ▣       ▣");
		textcolor(YELLOW1, BLACK);
		printf("    ▣ ▣");


		gotoxy(3, 7);
		textcolor(RED1, BLACK);
		printf("   ▣     ▣   ");
		textcolor(GREEN1, BLACK);
		printf("▣   ▣");
		textcolor(MAGENTA1, BLACK);
		printf("   ▣       ▣");
		textcolor(YELLOW1, BLACK);
		printf("   ▣   ▣");


		gotoxy(3, 8);
		textcolor(RED1, BLACK);
		printf("   ▣▣▣▣▣     ");
		textcolor(GREEN1, BLACK);
		printf("▣     ▣");
		textcolor(MAGENTA1, BLACK);
		printf(" ▣       ▣");
		textcolor(YELLOW1, BLACK);
		printf("  ▣     ▣");


	}
	else {
		textcolor(WHITE, BLACK);
		for (int y = 2; y <= 10; ++y) {
			gotoxy(3, y);
			printf("                                             ");
		}
	}
	textcolor(WHITE, BLACK);
}

//총알 발사
void shotBullet() {
	if (p_newy > 2) {
		bullet[p_newx - 2][p_newy - 1] = 1;
		bullet[p_newx][p_newy - 1] = 1;
		bullet[p_newx + 2][p_newy - 1] = 1;
		bullet_count += 3;
		gotoxy(p_newx - 2, p_newy - 1);
		textcolor(RED1, BLACK);
		printf(BULLET);
		gotoxy(p_newx, p_newy - 1);
		textcolor(RED1, BLACK);
		printf(BULLET);
		gotoxy(p_newx - 2, p_newy - 1);
		textcolor(RED1, BLACK);
		printf(BULLET);
	}
}

//총알 이동
void moveBullet() {
	int newy = 0;
	int newbullet[WIDTH][HEIGHT - 2] = { 0 };

	if (bullet_count == 0) return; // 총알이 없다면 그냥 return

	for (int x = 0; x < WIDTH; ++x) {
		for (int y = 0; y < HEIGHT - 2; ++y) {
			if (bullet[x][y]) {
				newy = y - 1;
				if (newy > 1) {
					if (brick[x / 2][newy]) {
						if (x & 1) { // 홀수면
							erasestar(x - 1, newy);
							erasestar(x, newy);
						}
						else {
							erasestar(x + 1, newy);
							erasestar(x, newy);
						}
						brick[x / 2][newy] = 0;
						--brick_count;
						bullet[x][y] = 0;
						--bullet_count;
						erasestar(x, y);
						++score;
						breakBrick = 1;
					}
					else {
						gotoxy(x, y);
						putchar(BLANK);
						gotoxy(x, newy);
						textcolor(RED1, BLACK);
						printf(BULLET);
						newbullet[x][newy] = 1;
						++bullet_count;
					}
				}
				else {
					gotoxy(x, y);
					putchar(BLANK);
					newbullet[x][y] = 0;
					--bullet_count;
				}
			}
		}
	}
	memcpy(bullet, newbullet, sizeof(newbullet));
}

//폭탄 폭발
void explodeBomb()
{
	int color1, color2;

	if (p_newy > 2) {
		bomb[p_newx][p_newy - 1] = 1;
		bomb_count += 1;
		gotoxy(p_newx, p_newy - 1);

		do { // 색을 변경하면서 출력
			color1 = rand() % 16;
		} while (color1 == BLACK);

		textcolor(color1, BLACK);
		printf(BOMB);
	}
}

//폭탄 이동
void moveBomb()
{
	int newy = 0;
	int newbomb[WIDTH][HEIGHT - 2] = { 0 };
	int color2;

	if (bomb_count == 0) return; // 폭탄이 없다면 그냥 return

	for (int x = 0; x < WIDTH; ++x) {
		for (int y = 0; y < HEIGHT - 2; ++y) {
			if (bomb[x][y]) {
				newy = y - 1;
				if (newy > 1) {
					if (brick[x / 2][newy]) {
						// 벽돌 부딪혔을 때 10x10 범위 안의 벽돌 모두 깨뜨리기
						for (int i = -5; i <= 5; ++i) {
							for (int j = -5; j <= 5; ++j) {
								int brickX = (x / 2) + i;
								int brickY = newy + j;
								if (brickX >= 0 && brickX < WIDTH / 2 && brickY >= 0 && brickY < HEIGHT - 2) {
									if (brick[brickX][brickY]) {
										erasestar(brickX * 2, brickY);
										erasestar(brickX * 2 + 1, brickY);
										brick[brickX][brickY] = 0;
										--brick_count;
										++score; // 부딪힌 벽돌마다 점수 추가
									}
								}
							}
						}
						// 부딪힌 벽돌 처리
						if (x & 1) { // 홀수면
							erasestar(x - 1, newy);
							erasestar(x, newy);
						}
						else {
							erasestar(x + 1, newy);
							erasestar(x, newy);
						}
						brick[x / 2][newy] = 0;
						--brick_count;
						bomb[x][y] = 0;
						--bomb_count;
						erasestar(x, y);
						++score; // 부딪힌 벽돌마다 점수 추가
						breakBrick = 1;
					}
					else {
						gotoxy(x, y);
						putchar(BLANK);
						gotoxy(x, newy);
						do { // 색을 변경하면서 출력
							color2 = rand() % 16;
						} while (color2 == BLACK);
						textcolor(color2, BLACK);
						printf(BOMB);
						newbomb[x][newy] = 1;
						++bomb_count;
					}
				}
				else {
					gotoxy(x, y);
					putchar(BLANK);
					newbomb[x][y] = 0;
					--bomb_count;
				}
			}
		}
	}
	memcpy(bomb, newbomb, sizeof(newbomb));
}

// 스코어보드를 초기화하는 함수
void resetScoreboard() {
	FILE* file = fopen("scoreboard.txt", "w");
	if (file == NULL) {
		printf("파일 열기 오류\n");
		return;
	}
	fclose(file);

	printf("스코어보드가 초기화되었습니다.\n\n");
}

//플레이어 이름 저장
void getPlayerName(char playerName[50]) {
	while (1) {
		printf("플레이어의 이름을 입력하세요(최대 10글자): ");
		scanf("%s", playerName);

		// '/reset'을 입력한 경우 스코어보드 초기화
		if (strcmp(playerName, "/reset") == 0) {
			resetScoreboard();
			continue;
		}

		// 이름이 10글자 이하인지 확인
		if (strlen(playerName) <= 10) {
			break;  // 조건을 만족하면 반복문 탈출
		}
		else {
			printf("이름은 ");
			textcolor(RED1, BLACK);
			printf("10글자 이하");
			textcolor(WHITE, BLACK);
			printf("여야 합니다.다시 입력해주세요.\n\n");
		}
	}

	strcpy(currentPlayer.playerName, playerName);
}

//점수 비교
int comparePlayers(const void* a, const void* b) {
	return ((Player*)b)->playerScore - ((Player*)a)->playerScore;
}

// 등수를 계산하여 저장하는 함수
void calculateRanks(Player players[], int numPlayers) {
	for (int i = 0; i < numPlayers; i++) {
		players[i].rank = i + 1;
	}
}

//스코어보드 출력
void displayScoreboard() {
	FILE* file = fopen("scoreboard.txt", "r");
	if (file == NULL) {
		printf("파일 열기 오류\n");
		return;
	}

	textcolor(MAGENTA1, WHITE);
	printf("=== scoreboard ===\n");

	char playerName[50];
	int playerScore;
	int playerRank = 0;
	int scoreboardStart = 16;

	while (fscanf(file, "%d %s %d", &playerRank, playerName, &playerScore) != EOF && scoreboardStart < HEIGHT - 2) {
		gotoxy(15, scoreboardStart);
		printf("%d. %-10s: %3d", playerRank, playerName, playerScore);

		scoreboardStart++;
	}

	fclose(file);
}

//점수 저장
void saveScore() {
	// 읽기 모드로 파일 열기
	FILE* fileRead = fopen("scoreboard.txt", "r");
	if (fileRead == NULL) {
		// 파일이 없는 경우, 쓰기 모드로 새로 파일 생성
		fileRead = fopen("scoreboard.txt", "w");
		if (fileRead == NULL) {
			printf("파일 생성 오류.\n");
			return;
		}
		fclose(fileRead);
	}

	// 다시 읽기 모드로 파일 열기
	fileRead = fopen("scoreboard.txt", "r");
	if (fileRead == NULL) {
		printf("파일 열기 오류.\n");
		return;
	}

	Player players[100];  // 최대 100명의 플레이어를 가정
	int numPlayers = 0;

	// 파일이 있는 경우 기존 내용을 읽어옴
	Player player;
	while (fscanf(fileRead, "%d %s %d", &player.rank, player.playerName, &player.playerScore) != EOF) {
		players[numPlayers++] = player;
	}

	fclose(fileRead);  // 파일 닫기

	// 추가된 내용을 배열에 저장
	int found = 0;
	for (int i = 0; i < numPlayers; i++) {
		if (strcmp(players[i].playerName, currentPlayer.playerName) == 0) {
			// 플레이어가 이미 존재할 경우
			found = 1;
			if (currentPlayer.playerScore > players[i].playerScore) {
				// 현재 플레이어의 점수가 더 높으면 갱신
				players[i] = currentPlayer;
			}
			break;
		}
	}

	// 플레이어가 존재하지 않을 경우 배열에 추가
	if (!found) {
		players[numPlayers++] = currentPlayer;
	}

	// 스코어를 내림차순으로 정렬
	qsort(players, numPlayers, sizeof(Player), comparePlayers);

	// 정렬된 배열을 파일에 쓰기 전에 등수 계산
	calculateRanks(players, numPlayers);


	// 쓰기 모드로 파일 열기
	FILE* fileWrite = fopen("scoreboard.txt", "w");
	if (fileWrite == NULL) {
		printf("파일 열기 오류.\n");
		return;
	}

	// 정렬된 배열을 파일에 쓰기
	for (int i = 0; i < numPlayers && i < HEIGHT - 2; i++) {
		fprintf(fileWrite, "%d %s %d\n", players[i].rank, players[i].playerName, players[i].playerScore);
	}

	fclose(fileWrite);  // 파일 닫기
}

//메인 함수
void main()
{
	unsigned char ch;
	int firstWindow_CallCount;
	int tempScore = 0;

	currentPlayer.playerScore = 0;

	SetConsoleTitleA("BRAX");
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX font;
	GetCurrentConsoleFontEx(hConsole, FALSE, &font);
	font.dwFontSize.X = 160;
	font.dwFontSize.Y = 240;
	SetCurrentConsoleFontEx(hConsole, FALSE, &font);

	while (1)
	{
		// 초기화
		cls(BLACK, WHITE);
		getPlayerName(currentPlayer.playerName);
		removeCursor();
		cls(BLACK, WHITE);
		firstWindow_CallCount = 0;

		draw_box(0, 0, WIDTH - 2, HEIGHT - 1, "■");

		while (1)
		{
			firstWindows(firstWindow_CallCount);
			gotoxy(17, HEIGHT - 3);

			if (firstWindow_CallCount++ & 1) {
				printf("press to start");
				gotoxy(12, 15);
				textcolor(CYAN2, BLACK);
				printf("C프로그래밍응용[001] ");
				textcolor(YELLOW2, BLACK);
				printf("F조");
			}
			else {
				printf("                       ");
				gotoxy(12, 15);
				printf("                       ");
			}

			Sleep(300);

			if (_kbhit()) {
				ch = _getch();
				if (ch == ESC) break;
				else break;  // 초기화 루프 탈출
			}
		}

		flush_key(); // 버퍼 한번 비우기
		init_game();
		showscore();
		showLife();
		showbomb();

		// 메인 루프
		while (1)
		{
			draw_box(0, 1, WIDTH - 2, HEIGHT - 1, "■");
			showscore();
			showLife();
			showbomb();

			if (life <= 0) break;
			if (frame_count % brick_create_frame_sync == 0)
				show_brick();

			if (_kbhit()) {
				ch = _getch();
				if (ch == ESC) break;

				switch (ch) {
				case UP: // 이동
				case DOWN:
				case LEFT:
				case RIGHT:
					if (frame_count % player_frame_sync == 0)
						player(ch);
					break;
				case SPACE: // 발사
					shotBullet();
					break;
				case ITEM:
					if (bombCount >= 1) {
						explodeBomb();
						bombCount -= 1;
						break;
					}
					else if (bombCount == 0)
						break;
				default:// 방향 전환이 아니면
					player(0);
				}
			}

			if (frame_count % brick_frame_sync == 0)
				move_brick(); // 벽돌의 위치를 변경
			if (frame_count % bullet_frame_sync == 0)
				moveBullet();
			if (frame_count % bomb_frame_sync == 0)
				moveBomb();

			Sleep(Delay);
			++frame_count;

			if (tempScore >= 5 && brick_frame_sync >= 37) {
				brick_frame_sync = 100 - score / 5 > brick_frame_sync ? brick_frame_sync : 100 - score / 5; // 속도 조정
				tempScore = 0;
				
			}
			else if (breakBrick) {
				++tempScore;
				breakBrick = 0;
			}
			if (score != 0 && score % 15 == 0) {
				bombCount++;
				score++;
			}
			//gotoxy(30, 22); // 진행 현황을 보기 위한 코드
			//printf("%d %d %d", frame_count, brick_frame_sync, tempScore);
		}

		// 끝
		flush_key();

		currentPlayer.playerScore = score;

		gotoxy(14, 11);
		textcolor(GREEN1, WHITE);
		printf("   YOUR SCORE : %2d   ", currentPlayer.playerScore);

		saveScore();

		gotoxy(15, 15);
		displayScoreboard();

		while (1)
		{
			int c1, c2;

			do { // 색을 변경하면서 출력
				c1 = rand() % 16;
				c2 = rand() % 16;
			} while (c1 == c2);

			textcolor(c1, c2);
			gotoxy(17, 10);
			printf("** Game Over **");
			gotoxy(8, 13);
			textcolor(BLACK, WHITE);
			printf("  Hit (R) to Restart (Q) to Quit  ");
			Sleep(300);

			if (_kbhit()) {
				ch = _getch();
				if (ch == 'r') break;  // 게임 루프 탈출
				else if (ch == 'q') {  //프로그램 종료
					cls(BLACK, WHITE);
					return;
				}
			}
		}
		textcolor(WHITE, BLACK);
	}
}

