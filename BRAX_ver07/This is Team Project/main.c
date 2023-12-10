#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <conio.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") // mmsystem.h

// ���� ����
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

#define PLAYER "<-*->" // player1 ǥ��
#define BLANK	' ' 
#define BRICK	"��" // ����. Ư������
#define BULLET	"��" // �Ѿ�
#define BOMB    "@" // ��ź

#define ESC 0x1b //  ESC ������ ����

#define UP		'w' // WASD�� �̵�
#define DOWN	's'
#define LEFT	'a'
#define RIGHT	'd'
#define SPACE	' ' // �����̽��ٷ� �߻�
#define ITEM    'e'

#define WIDTH 50
#define HEIGHT 24

int Delay;// msec
int life; // ��ȸ
int score; // ����
int bombCount;
int brick_count;
int brick[WIDTH / 2][HEIGHT - 2] = { 0 }; // 1�̸� ������ �ִٴ� ��, ������ 2byte ���ڿ��̹Ƿ�, ���� �������ϰ� ���Ŀ� 2�� ����
int bullet_count;
int bullet[WIDTH][HEIGHT - 2] = { 0 }; // 1�̸� �Ѿ��� �ִٴ� ��
int bomb[WIDTH][HEIGHT - 2] = { 0 }; // 1�̸� ��ź�� �ִٴ� ��
int bomb_count;
int called;

int frame_count; // ��ü frame
int brick_create_frame_sync; // ���� ���� ����
int brick_frame_sync; //  frame ���� �ѹ��� brick�� �̵�
int player_frame_sync; // ó�� ������ 10 frame ���� �̵�, ��, 100msec ���� �̵�
int bullet_frame_sync; // 1 frame ���� �ѹ��� bullet�� �̵�
int bomb_frame_sync; //1 frame ���� �ѹ��� bomb�� �̵�

int p_oldx = 40, p_oldy = 20, p_newx = 40, p_newy = 20; //�÷��̾��� ��ǥ

int crash; // ������ �÷��̾ �浹�ߴ��� �Ǵ��ϱ� ���� ����
int breakBrick; // ������ �ν����� �Ǵ��ϴ� ����

int randomColor;

typedef struct {
	char playerName[50];
	int playerScore;
	int rank;
} Player;

Player currentPlayer;

//Ŀ�� ����
void removeCursor(void) { 
	CONSOLE_CURSOR_INFO curInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curInfo.bVisible = 0;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

//���� ���ϴ� ��ġ�� Ŀ�� �̵�
void gotoxy(int x, int y)
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);// WIN32API �Լ�
}

//�÷��̾� ��ü ����
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

//���� ����
void textcolor(int fg_color, int bg_color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fg_color | bg_color << 4);
}

//ȭ�� ����� ���ϴ� �������� ����
void cls(int bg_color, int text_color)
{
	char cmd[100];
	system("cls");
	sprintf(cmd, "COLOR %x%x", bg_color, text_color);
	system(cmd);
}

//���� ǥ��
void showscore()
{
	textcolor(BLUE1, GRAY2);
	gotoxy(2, 0);
	printf("SCORE : %d", score);
	textcolor(WHITE, BLACK);
}

//��ź ǥ��
void showbomb()
{
	textcolor(RED1, GRAY2);
	gotoxy(18, 0);
	printf("BOMB : %d", bombCount);
	textcolor(WHITE, BLACK);
}

//��� ǥ��
void showLife()
{
	int i = 0;
	textcolor(YELLOW2, GRAY2);
	gotoxy(31, 0);
	printf("���� ���� : ");
	for (; i < life; i++) {
		printf("��");
	}
	for (; i < 5; i++) {
		printf("��");
	}
	textcolor(WHITE, BLACK);
}

//���� ����
void show_brick() {
	int x = rand() % (WIDTH / 2 - 3) + 1;
	int y = rand() % (HEIGHT / 5) + 2;
	gotoxy(2 * x, y);

	do {
		randomColor = rand() % 16;
	} while (randomColor == BLACK);

	textcolor(randomColor, BLACK);
	printf(BRICK);
	brick[x][y] = randomColor;  // �� ���� ����
	++brick_count;
}

//���� �̵�
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
						// ������ ȿ��
						for (int blink_count = 0; blink_count < 4; blink_count++) {
							textcolor(YELLOW1, BLACK);
							printf(BRICK);
							Sleep(250);  // 250�и��� ���� ���
							gotoxy(2 * x, newy);
							printf("  ");
							Sleep(250);
						}
						textcolor(brick[x][y], BLACK);  // ������ ���� ���� ������ ����
					}
					else {
						textcolor(brick[x][y], BLACK);  // �̵��� ��ġ���� ���� ������ ����
						printf(BRICK);
					}
					newbricks[x][newy] = brick[x][y];  // �� ���� �̵�
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

//���� �ڽ� ����
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

//�÷��̾� �̵� ����
void player(unsigned char ch)
{
	int move_flag = 0;
	static unsigned char last_ch = 0;

	if (!called) { // ó�� �Ǵ� Restart
		p_oldx = 40, p_oldy = 20, p_newx = 40, p_newy = 20;
		putplayer(p_oldx - 2, p_oldy);
		called = 1;
		last_ch = 0;
		ch = 0;
	}

	switch (ch) {
	case UP:
		if (p_oldy > 2) // 0 �� Status Line
			p_newy = p_oldy - 1;
		else { // ���� �ε�ġ�� ������ �ݴ�� �̵�
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
		eraseplayer(p_oldx, p_oldy); // ���� ��ġ�� 
		if (p_newx & 1) {// Ȧ��, ������ ���� ����� ����
			erasestar(p_newx - 1, p_newy);
			erasestar(p_newx, p_newy);
		}
		else {
			erasestar(p_newx + 1, p_newy);
			erasestar(p_newx, p_newy);
		}
		putplayer(p_newx - 2, p_newy); // ���ο� ��ġ���� player�� ǥ��
		p_oldx = p_newx; // ������ ��ġ�� ���
		p_oldy = p_newy;

		if (brick[p_newx / 2][p_newy]) { // �ε��� üũ
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

//�ʱ�ȭ
void init_game()  
{
	int x, y;
	char cmd[100];

	srand(time(NULL));

	brick_count = 0; // ���� �ʱ�ȭ
	for (x = 0; x < WIDTH / 2; x++)
		for (y = 0; y < HEIGHT; y++)
			brick[x][y] = 0;

	bullet_count = 0; // �Ѿ� �ʱ�ȭ
	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++)
			bullet[x][y] = 0;

	bomb_count = 0;
	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++)
			bomb[x][y] = 0;

	// ���� �ʱ�ȭ
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

	called = 0; // �÷��̾� ����
	player(0);
}

//Ÿ��Ʋ ȭ�� (BRAX -> ���� �̸�)
void firstWindows(int sec) {
	if (sec & 1) {
		gotoxy(3, 2);
		textcolor(RED1, BLACK);
		printf("   �ââââ�     ");
		textcolor(GREEN1, BLACK);
		printf("�� �â�     ");
		textcolor(MAGENTA1, BLACK);
		printf("  �â�");
		textcolor(YELLOW1, BLACK);
		printf("      ��     ��");



		gotoxy(3, 3);
		textcolor(RED1, BLACK);
		printf("   ��     ��   ");
		textcolor(GREEN1, BLACK);
		printf("��    ��");
		textcolor(MAGENTA1, BLACK);
		printf("   ��    ��  ");
		textcolor(YELLOW1, BLACK);
		printf("   ��   ��");


		gotoxy(3, 4);
		textcolor(RED1, BLACK);
		printf("   ��     ��   ");
		textcolor(GREEN1, BLACK);
		printf("��    ��");
		textcolor(MAGENTA1, BLACK);
		printf("  ��      �� ");
		textcolor(YELLOW1, BLACK);
		printf("    �� ��");


		gotoxy(3, 5);
		textcolor(RED1, BLACK);
		printf("   �� �ââ�     ");
		textcolor(GREEN1, BLACK);
		printf("�� �ââ�");
		textcolor(MAGENTA1, BLACK);
		printf("   �� �� �� �� ��");
		textcolor(YELLOW1, BLACK);
		printf("     ��");


		gotoxy(3, 6);
		textcolor(RED1, BLACK);
		printf("   ��     ��   ");
		textcolor(GREEN1, BLACK);
		printf("�� ��");
		textcolor(MAGENTA1, BLACK);
		printf("     ��       ��");
		textcolor(YELLOW1, BLACK);
		printf("    �� ��");


		gotoxy(3, 7);
		textcolor(RED1, BLACK);
		printf("   ��     ��   ");
		textcolor(GREEN1, BLACK);
		printf("��   ��");
		textcolor(MAGENTA1, BLACK);
		printf("   ��       ��");
		textcolor(YELLOW1, BLACK);
		printf("   ��   ��");


		gotoxy(3, 8);
		textcolor(RED1, BLACK);
		printf("   �ââââ�     ");
		textcolor(GREEN1, BLACK);
		printf("��     ��");
		textcolor(MAGENTA1, BLACK);
		printf(" ��       ��");
		textcolor(YELLOW1, BLACK);
		printf("  ��     ��");


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

//�Ѿ� �߻�
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

//�Ѿ� �̵�
void moveBullet() {
	int newy = 0;
	int newbullet[WIDTH][HEIGHT - 2] = { 0 };

	if (bullet_count == 0) return; // �Ѿ��� ���ٸ� �׳� return

	for (int x = 0; x < WIDTH; ++x) {
		for (int y = 0; y < HEIGHT - 2; ++y) {
			if (bullet[x][y]) {
				newy = y - 1;
				if (newy > 1) {
					if (brick[x / 2][newy]) {
						if (x & 1) { // Ȧ����
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

//��ź ����
void explodeBomb()
{
	int color1, color2;

	if (p_newy > 2) {
		bomb[p_newx][p_newy - 1] = 1;
		bomb_count += 1;
		gotoxy(p_newx, p_newy - 1);

		do { // ���� �����ϸ鼭 ���
			color1 = rand() % 16;
		} while (color1 == BLACK);

		textcolor(color1, BLACK);
		printf(BOMB);
	}
}

//��ź �̵�
void moveBomb()
{
	int newy = 0;
	int newbomb[WIDTH][HEIGHT - 2] = { 0 };
	int color2;

	if (bomb_count == 0) return; // ��ź�� ���ٸ� �׳� return

	for (int x = 0; x < WIDTH; ++x) {
		for (int y = 0; y < HEIGHT - 2; ++y) {
			if (bomb[x][y]) {
				newy = y - 1;
				if (newy > 1) {
					if (brick[x / 2][newy]) {
						// ���� �ε����� �� 10x10 ���� ���� ���� ��� ���߸���
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
										++score; // �ε��� �������� ���� �߰�
									}
								}
							}
						}
						// �ε��� ���� ó��
						if (x & 1) { // Ȧ����
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
						++score; // �ε��� �������� ���� �߰�
						breakBrick = 1;
					}
					else {
						gotoxy(x, y);
						putchar(BLANK);
						gotoxy(x, newy);
						do { // ���� �����ϸ鼭 ���
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

// ���ھ�带 �ʱ�ȭ�ϴ� �Լ�
void resetScoreboard() {
	FILE* file = fopen("scoreboard.txt", "w");
	if (file == NULL) {
		printf("���� ���� ����\n");
		return;
	}
	fclose(file);

	printf("���ھ�尡 �ʱ�ȭ�Ǿ����ϴ�.\n\n");
}

//�÷��̾� �̸� ����
void getPlayerName(char playerName[50]) {
	while (1) {
		printf("�÷��̾��� �̸��� �Է��ϼ���(�ִ� 10����): ");
		scanf("%s", playerName);

		// '/reset'�� �Է��� ��� ���ھ�� �ʱ�ȭ
		if (strcmp(playerName, "/reset") == 0) {
			resetScoreboard();
			continue;
		}

		// �̸��� 10���� �������� Ȯ��
		if (strlen(playerName) <= 10) {
			break;  // ������ �����ϸ� �ݺ��� Ż��
		}
		else {
			printf("�̸��� ");
			textcolor(RED1, BLACK);
			printf("10���� ����");
			textcolor(WHITE, BLACK);
			printf("���� �մϴ�.�ٽ� �Է����ּ���.\n\n");
		}
	}

	strcpy(currentPlayer.playerName, playerName);
}

//���� ��
int comparePlayers(const void* a, const void* b) {
	return ((Player*)b)->playerScore - ((Player*)a)->playerScore;
}

// ����� ����Ͽ� �����ϴ� �Լ�
void calculateRanks(Player players[], int numPlayers) {
	for (int i = 0; i < numPlayers; i++) {
		players[i].rank = i + 1;
	}
}

//���ھ�� ���
void displayScoreboard() {
	FILE* file = fopen("scoreboard.txt", "r");
	if (file == NULL) {
		printf("���� ���� ����\n");
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

//���� ����
void saveScore() {
	// �б� ���� ���� ����
	FILE* fileRead = fopen("scoreboard.txt", "r");
	if (fileRead == NULL) {
		// ������ ���� ���, ���� ���� ���� ���� ����
		fileRead = fopen("scoreboard.txt", "w");
		if (fileRead == NULL) {
			printf("���� ���� ����.\n");
			return;
		}
		fclose(fileRead);
	}

	// �ٽ� �б� ���� ���� ����
	fileRead = fopen("scoreboard.txt", "r");
	if (fileRead == NULL) {
		printf("���� ���� ����.\n");
		return;
	}

	Player players[100];  // �ִ� 100���� �÷��̾ ����
	int numPlayers = 0;

	// ������ �ִ� ��� ���� ������ �о��
	Player player;
	while (fscanf(fileRead, "%d %s %d", &player.rank, player.playerName, &player.playerScore) != EOF) {
		players[numPlayers++] = player;
	}

	fclose(fileRead);  // ���� �ݱ�

	// �߰��� ������ �迭�� ����
	int found = 0;
	for (int i = 0; i < numPlayers; i++) {
		if (strcmp(players[i].playerName, currentPlayer.playerName) == 0) {
			// �÷��̾ �̹� ������ ���
			found = 1;
			if (currentPlayer.playerScore > players[i].playerScore) {
				// ���� �÷��̾��� ������ �� ������ ����
				players[i] = currentPlayer;
			}
			break;
		}
	}

	// �÷��̾ �������� ���� ��� �迭�� �߰�
	if (!found) {
		players[numPlayers++] = currentPlayer;
	}

	// ���ھ ������������ ����
	qsort(players, numPlayers, sizeof(Player), comparePlayers);

	// ���ĵ� �迭�� ���Ͽ� ���� ���� ��� ���
	calculateRanks(players, numPlayers);


	// ���� ���� ���� ����
	FILE* fileWrite = fopen("scoreboard.txt", "w");
	if (fileWrite == NULL) {
		printf("���� ���� ����.\n");
		return;
	}

	// ���ĵ� �迭�� ���Ͽ� ����
	for (int i = 0; i < numPlayers && i < HEIGHT - 2; i++) {
		fprintf(fileWrite, "%d %s %d\n", players[i].rank, players[i].playerName, players[i].playerScore);
	}

	fclose(fileWrite);  // ���� �ݱ�
}

//���� �Լ�
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
		// �ʱ�ȭ
		cls(BLACK, WHITE);
		getPlayerName(currentPlayer.playerName);
		removeCursor();
		cls(BLACK, WHITE);
		firstWindow_CallCount = 0;

		draw_box(0, 0, WIDTH - 2, HEIGHT - 1, "��");

		while (1)
		{
			firstWindows(firstWindow_CallCount);
			gotoxy(17, HEIGHT - 3);

			if (firstWindow_CallCount++ & 1) {
				printf("press to start");
				gotoxy(12, 15);
				textcolor(CYAN2, BLACK);
				printf("C���α׷�������[001] ");
				textcolor(YELLOW2, BLACK);
				printf("F��");
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
				else break;  // �ʱ�ȭ ���� Ż��
			}
		}

		flush_key(); // ���� �ѹ� ����
		init_game();
		showscore();
		showLife();
		showbomb();

		// ���� ����
		while (1)
		{
			draw_box(0, 1, WIDTH - 2, HEIGHT - 1, "��");
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
				case UP: // �̵�
				case DOWN:
				case LEFT:
				case RIGHT:
					if (frame_count % player_frame_sync == 0)
						player(ch);
					break;
				case SPACE: // �߻�
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
				default:// ���� ��ȯ�� �ƴϸ�
					player(0);
				}
			}

			if (frame_count % brick_frame_sync == 0)
				move_brick(); // ������ ��ġ�� ����
			if (frame_count % bullet_frame_sync == 0)
				moveBullet();
			if (frame_count % bomb_frame_sync == 0)
				moveBomb();

			Sleep(Delay);
			++frame_count;

			if (tempScore >= 5 && brick_frame_sync >= 37) {
				brick_frame_sync = 100 - score / 5 > brick_frame_sync ? brick_frame_sync : 100 - score / 5; // �ӵ� ����
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
			//gotoxy(30, 22); // ���� ��Ȳ�� ���� ���� �ڵ�
			//printf("%d %d %d", frame_count, brick_frame_sync, tempScore);
		}

		// ��
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

			do { // ���� �����ϸ鼭 ���
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
				if (ch == 'r') break;  // ���� ���� Ż��
				else if (ch == 'q') {  //���α׷� ����
					cls(BLACK, WHITE);
					return;
				}
			}
		}
		textcolor(WHITE, BLACK);
	}
}

