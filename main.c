#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Player yapısı
typedef struct
{
    int score;
    int captured_pieces[5]; // Her harften kaç tane toplandığını saklamak için (A, B, C, D, E)
} Player;

// Move yapısı
typedef struct
{
    int from_x, from_y, to_x, to_y;
    char captured[10]; // Yakalanan taşları saklamak için (en fazla 10 taş yakalanabilir)
    int num_captured;
} Move;

// MoveStack yapısı
typedef struct
{
    Move *moves;
    int top;
    int capacity;
} MoveStack;

typedef struct
{
    int from_row, from_col;
    int to_row, to_col;
    int captured;
} BestMove;

// MoveStack oluşturma fonksiyonu
MoveStack *create_move_stack(int capacity)
{
    MoveStack *stack = (MoveStack *)malloc(sizeof(MoveStack));
    stack->capacity = capacity;
    stack->top = -1;
    stack->moves = (Move *)malloc(capacity * sizeof(Move));
    return stack;
}

// MoveStack'i genişletme fonksiyonu
void expand_move_stack(MoveStack *stack)
{
    stack->capacity *= 2;
    stack->moves = (Move *)realloc(stack->moves, stack->capacity * sizeof(Move));
}

// MoveStack'e hamle ekleme fonksiyonu
void push_move(MoveStack *stack, int from_x, int from_y, int to_x, int to_y, char *captured, int num_captured)
{
    if (stack->top == stack->capacity - 1)
    {
        expand_move_stack(stack);
    }
    stack->top++;
    stack->moves[stack->top].from_x = from_x;
    stack->moves[stack->top].from_y = from_y;
    stack->moves[stack->top].to_x = to_x;
    stack->moves[stack->top].to_y = to_y;
    stack->moves[stack->top].num_captured = num_captured;
    for (int i = 0; i < num_captured; i++)
    {
        stack->moves[stack->top].captured[i] = captured[i];
    }
}

// MoveStack'ten hamle çıkarma fonksiyonu
Move pop_move(MoveStack *stack)
{
    if (stack->top == -1)
    {
        Move empty_move = {-1, -1, -1, -1, {'.'}, 0};
        return empty_move;
    }
    return stack->moves[stack->top--];
}

// Tahta oluşturma fonksiyonu
char **create_board(int size)
{
    char **board = (char **)malloc(size * sizeof(char *));
    for (int i = 0; i < size; i++)
    {
        board[i] = (char *)malloc(size * sizeof(char));
        for (int j = 0; j < size; j++)
        {
            board[i][j] = '.'; // Başlangıçta boş hücreler
        }
    }
    return board;
}

// Tahtayı ekrana yazdırma fonksiyonu
void print_board(char **board, int size)
{
    // Üst sütun numaralarını yazdır
    printf("   ");
    for (int i = 0; i < size; i++)
    {
        printf("%2d ", i);
    }
    printf("\n");

    // Tahtayı yazdır
    for (int i = 0; i < size; i++)
    {
        // Sol satır numarasını yazdır
        printf("%2d ", i);

        for (int j = 0; j < size; j++)
        {
            printf("%2c ", board[i][j]);
        }
        printf("\n");
    }
}

// Taşları rastgele dizme fonksiyonu
void initialize_pieces(char **board, int size)
{
    char pieces[] = "ABCDE"; // Sadece beş harf kullanılır
    int num_pieces = 5;
    srand(time(NULL));

    int total_cells = size * size;
    int pieces_to_place = total_cells - 4; // Ortadaki 4 kare boş bırakılacak

    for (int i = 0; i < pieces_to_place; i++)
    {
        int x, y;
        do
        {
            x = rand() % size;
            y = rand() % size;
        } while ((x == size / 2 || x == size / 2 - 1) && (y == size / 2 || y == size / 2 - 1) || board[x][y] != '.'); // Ortadaki 4 kare ve dolu hücreleri atla

        int random_index = rand() % num_pieces;
        board[x][y] = pieces[random_index];
    }
}

int can_capture_more(char **board, int size, int row, int col)
{
    char piece = board[row][col];
    if (piece == '.')
        return 0;

    int directions[4][2] = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}};
    for (int i = 0; i < 4; i++)
    {
        int new_row = row + directions[i][0];
        int new_col = col + directions[i][1];
        int between_row = row + directions[i][0] / 2;
        int between_col = col + directions[i][1] / 2;

        if (new_row >= 0 && new_row < size && new_col >= 0 && new_col < size && board[new_row][new_col] == '.' && board[between_row][between_col] != '.' && board[between_row][between_col] != piece)
        {
            return 1;
        }
    }

    return 0;
}

// Hamle yapma fonksiyonu
int make_move(char **board, int size, int from_row, int from_col, int to_row, int to_col, Player *player, MoveStack *move_stack)
{
    // Hamle geçerlilik kontrolü
    if (from_row < 0 || from_row >= size || from_col < 0 || from_col >= size ||
        to_row < 0 || to_row >= size || to_col < 0 || to_col >= size)
    {
        printf("Geçersiz koordinatlar!\n");
        return 0;
    }

    if (board[from_row][from_col] == '.')
    {
        printf("Başlangıç noktasında taş yok!\n");
        return 0;
    }

    if (board[to_row][to_col] != '.')
    {
        printf("Hedef nokta dolu!\n");
        return 0;
    }

    int dx = (to_row - from_row) / (abs(to_row - from_row) > 0 ? abs(to_row - from_row) : 1);
    int dy = (to_col - from_col) / (abs(to_col - from_col) > 0 ? abs(to_col - from_col) : 1);

    // Hamlenin geçerliliğini kontrol et (2 adım ilerleme ve arada taş olması)
    if (abs(to_row - from_row) != 2 && abs(to_col - from_col) != 2)
    {
        printf("Geçersiz hamle! Taş sadece iki kare ilerleyebilir.\n");
        return 0;
    }

    int mid_row = from_row + dx;
    int mid_col = from_col + dy;

    if (board[mid_row][mid_col] == '.')
    {
        printf("Geçersiz hamle! Arada taş olmalı.\n");
        return 0;
    }

    // Taşların yakalanması
    char captured[10]; // Yakalanan taşları saklamak için (en fazla 10 taş yakalanabilir)
    int num_captured = 0;

    if (board[mid_row][mid_col] != '.')
    {
        player->score++;
        player->captured_pieces[board[mid_row][mid_col] - 'A']++;
        captured[num_captured++] = board[mid_row][mid_col];
        board[mid_row][mid_col] = '.';
    }

    // Hamleyi gerçekleştir
    board[to_row][to_col] = board[from_row][from_col];
    board[from_row][from_col] = '.';

    // Hamleyi move_stack'e ekle
    push_move(move_stack, from_row, from_col, to_row, to_col, captured, num_captured);

    // Eğer devam edilebilecek bir hamle varsa return 2 döndür
    if (can_capture_more(board, size, to_row, to_col))
    {
        return 2;
    }

    return 1;
}

BestMove find_best_move(char **board, int size)
{
    BestMove best_move = {-1, -1, -1, -1, -1};
    char piece;
    int directions[4][2] = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}};

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (board[i][j] == '.')
                continue;
            piece = board[i][j];

            for (int d = 0; d < 4; d++)
            {
                int to_row = i + directions[d][0];
                int to_col = j + directions[d][1];
                int between_row = i + directions[d][0] / 2;
                int between_col = j + directions[d][1] / 2;

                if (to_row >= 0 && to_row < size && to_col >= 0 && to_col < size &&
                    board[to_row][to_col] == '.' && board[between_row][between_col] != '.' &&
                    board[between_row][between_col] != piece)
                {
                    int captured = 1;

                    if (can_capture_more(board, size, to_row, to_col))
                    {
                        captured++;
                    }

                    if (captured > best_move.captured)
                    {
                        best_move.from_row = i;
                        best_move.from_col = j;
                        best_move.to_row = to_row;
                        best_move.to_col = to_col;
                        best_move.captured = captured;
                    }
                }
            }
        }
    }
    return best_move;
}

// Undo hamlesi fonksiyonu
int undo_move(char **board, Player *player, MoveStack *move_stack, MoveStack *redo_stack)
{
    Move move = pop_move(move_stack);
    if (move.from_x == -1)
    {
        printf("Geri alınacak hamle yok!\n");
        return 0;
    }

    // Hamleyi geri al
    board[move.from_x][move.from_y] = board[move.to_x][move.to_y];
    board[move.to_x][move.to_y] = '.';

    // Yakalanan taşları geri ekle
    int dx = (move.to_x - move.from_x) / (abs(move.to_x - move.from_x) > 0 ? abs(move.to_x - move.from_x) : 1);
    int dy = (move.to_y - move.from_y) / (abs(move.to_y - move.from_y) > 0 ? abs(move.to_y - move.from_y) : 1);

    for (int i = 0; i < move.num_captured; i++)
    {
        int x = move.from_x + dx * (i + 1);
        int y = move.from_y + dy * (i + 1);
        board[x][y] = move.captured[i];
        player->score--;
        player->captured_pieces[move.captured[i] - 'A']--;
    }

    // Geri alınan hamleyi redo_stack'e ekle
    push_move(redo_stack, move.from_x, move.from_y, move.to_x, move.to_y, move.captured, move.num_captured);

    return 1;
}

int redo_move(char **board, Player *player, MoveStack *redo_stack, MoveStack *move_stack)
{
    Move move = pop_move(redo_stack);
    if (move.from_x == -1)
    {
        printf("Yeniden yapılacak hamle yok!\n");
        return 0;
    }

    // Yakalanan taşları kaldır
    int dx = (move.to_x - move.from_x) / (abs(move.to_x - move.from_x) > 0 ? abs(move.to_x - move.from_x) : 1);
    int dy = (move.to_y - move.from_y) / (abs(move.to_y - move.from_y) > 0 ? abs(move.to_y - move.from_y) : 1);

    for (int i = 0; i < move.num_captured; i++)
    {
        int x = move.from_x + dx * (i + 1);
        int y = move.from_y + dy * (i + 1);
        board[x][y] = '.';
        player->score++;
        player->captured_pieces[move.captured[i] - 'A']++;
    }

    // Hamleyi gerçekleştir
    board[move.to_x][move.to_y] = board[move.from_x][move.from_y];
    board[move.from_x][move.from_y] = '.';

    // Yeniden yapılan hamleyi move_stack'e ekle
    push_move(move_stack, move.from_x, move.from_y, move.to_x, move.to_y, move.captured, move.num_captured);

    return 1;
}

// AI hamlesi fonksiyonu (rastgele hamle)
void make_ai_move(char **board, int size, Player *ai_player, MoveStack *move_stack)
{
    BestMove best_move = find_best_move(board, size);

    if (best_move.from_row == -1)
    {
        printf("Bilgisayar hamle yapamıyor.\n");
        return;
    }

    int move_result = make_move(board, size, best_move.from_row, best_move.from_col, best_move.to_row, best_move.to_col, ai_player, move_stack);
    printf("Bilgisayar hamlesi: (%d, %d) -> (%d, %d)\n", best_move.from_row, best_move.from_col, best_move.to_row, best_move.to_col);

    // Ek hamle kontrolü
    while (move_result == 2)
    {
        best_move = find_best_move(board, size);
        move_result = make_move(board, size, best_move.from_row, best_move.from_col, best_move.to_row, best_move.to_col, ai_player, move_stack);
        printf("Bilgisayar ek hamlesi: (%d, %d) -> (%d, %d)\n", best_move.from_row, best_move.from_col, best_move.to_row, best_move.to_col);
    }
}

void save_game(char **board, int size, Player player1, Player player2, MoveStack *move_stack, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("Dosya açılamadı!\n");
        return;
    }

    fprintf(file, "%d\n", size);
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            fprintf(file, "%c ", board[i][j]);
        }
        fprintf(file, "\n");
    }

    fprintf(file, "%d %d\n", player1.score, player2.score);
    for (int i = 0; i < 5; i++)
    {
        fprintf(file, "%d %d ", player1.captured_pieces[i], player2.captured_pieces[i]);
    }
    fprintf(file, "\n");

    fprintf(file, "%d\n", move_stack->top);
    for (int i = 0; i <= move_stack->top; i++)
    {
        Move move = move_stack->moves[i];
        fprintf(file, "%d %d %d %d %d ", move.from_x, move.from_y, move.to_x, move.to_y, move.num_captured);
        for (int j = 0; j < move.num_captured; j++)
        {
            fprintf(file, "%c ", move.captured[j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    printf("Oyun kaydedildi.\n");
}

void load_game(char **board, int *size, Player *player1, Player *player2, MoveStack *move_stack, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Dosya açılamadı!\n");
        return;
    }

    fscanf(file, "%d", size);
    for (int i = 0; i < *size; i++)
    {
        for (int j = 0; j < *size; j++)
        {
            fscanf(file, " %c", &board[i][j]);
        }
    }

    fscanf(file, "%d %d", &player1->score, &player2->score);
    for (int i = 0; i < 5; i++)
    {
        fscanf(file, "%d %d", &player1->captured_pieces[i], &player2->captured_pieces[i]);
    }

    fscanf(file, "%d", &move_stack->top);
    for (int i = 0; i <= move_stack->top; i++)
    {
        Move *move = &move_stack->moves[i];
        fscanf(file, "%d %d %d %d %d", &move->from_x, &move->from_y, &move->to_x, &move->to_y, &move->num_captured);
        for (int j = 0; j < move->num_captured; j++)
        {
            fscanf(file, " %c", &move->captured[j]);
        }
    }

    fclose(file);
    printf("Oyun yüklendi.\n");
}

void print_scores(Player player1, Player player2)
{
    printf("Oyuncu 1 Puan: %d\n", player1.score);
    printf("Oyuncu 2 Puan: %d\n", player2.score);
}

int check_game_end(char **board, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (board[i][j] != '.' && can_capture_more(board, size, i, j))
            {
                return 0; // Oyun bitmedi
            }
        }
    }
    return 1; // Oyun bitti
}

void print_winner(Player player1, Player player2)
{
    printf("Oyun bitti!\n");
    printf("Oyuncu 1 Puan: %d\n", player1.score);
    printf("Oyuncu 2 Puan: %d\n", player2.score);

    if (player1.score > player2.score)
    {
        printf("Oyuncu 1 kazandı!\n");
    }
    else if (player2.score > player1.score)
    {
        printf("Oyuncu 2 kazandı!\n");
    }
    else
    {
        printf("Oyun berabere!\n");
    }
}

int main()
{
    int size, mode;
    char choice;
    char **board;
    Player player1 = {0}, player2 = {0};
    MoveStack *move_stack = create_move_stack(10);
    MoveStack *redo_stack = create_move_stack(10);
    int turn = 0; // 0 - Player 1, 1 - Player 2

    printf("Oyun yüklemek istiyor musunuz? (e/h): ");
    scanf(" %c", &choice);

    if (choice == 'e')
    {
        char filename[100];
        printf("Dosya adını girin: ");
        scanf("%s", filename);

        board = create_board(20); // Maksimum boyutta oluştur, yüklendikten sonra boyutu güncelleriz
        load_game(board, &size, &player1, &player2, move_stack, filename);
        print_board(board, size);

        printf("Oyun modu seçin: 1 - İki oyunculu, 2 - Bilgisayara karşı: ");
        scanf("%d", &mode);
    }
    else
    {
        printf("Tahta boyutunu girin (1-20 arası bir sayı): ");
        scanf("%d", &size);

        // Geçerli boyut kontrolü
        if (size < 1 || size > 20)
        {
            printf("Geçersiz boyut! Lütfen 1 ile 20 arasında bir sayı girin.\n");
            return 1;
        }

        board = create_board(size);
        initialize_pieces(board, size);
        print_board(board, size);

        printf("Oyun modu seçin: 1 - İki oyunculu, 2 - Bilgisayara karşı: ");
        scanf("%d", &mode);
    }

    int from_row, from_col, to_row, to_col;
    int move_result;

    while (1)
    {
        print_scores(player1, player2);
        if (mode == 1 || (mode == 2 && turn == 0))
        {
            printf("Oyuncu %d hamle yapmak için başlangıç ve hedef koordinatları girin (from_row from_col to_row to_col) veya 'u' ile geri al, 'r' ile yeniden yap, 's' ile kaydet, 'q' ile çık: ", turn + 1);
            scanf(" %c", &choice);

            if (choice == 'u')
            {
                if (undo_move(board, turn == 0 ? &player1 : &player2, move_stack, redo_stack))
                {
                    printf("Hamle geri alındı!\n");
                    turn = 1 - turn; // Sıra değiştir
                }
            }
            else if (choice == 'r')
            {
                if (redo_move(board, turn == 0 ? &player1 : &player2, redo_stack, move_stack))
                {
                    printf("Hamle yeniden yapıldı!\n");
                }
            }
            else if (choice == 's')
            {
                char filename[100];
                printf("Dosya adını girin: ");
                scanf("%s", filename);
                save_game(board, size, player1, player2, move_stack, filename);
            }
            else if (choice == 'q')
            {
                break;
            }
            else
            {
                ungetc(choice, stdin); // Girdiği karakteri geri al
                scanf("%d %d %d %d", &from_row, &from_col, &to_row, &to_col);
                move_result = make_move(board, size, from_row, from_col, to_row, to_col, turn == 0 ? &player1 : &player2, move_stack);
                if (move_result)
                {
                    printf("Hamle başarılı!\n");
                    redo_stack->top = -1; // Yeni hamle yapıldığında redo_stack'i temizle
                    if (move_result == 1)
                    {
                        turn = 1 - turn; // Sıra değiştir
                    }

                    // Oyun bitiş kontrolü
                    if (check_game_end(board, size))
                    {
                        print_board(board, size);
                        print_winner(player1, player2);
                        break;
                    }
                }
                else
                {
                    printf("Hamle başarısız!\n");
                }
            }
        }
        else if (mode == 2 && turn == 1)
        {
            make_ai_move(board, size, &player2, move_stack);
            redo_stack->top = -1; // Yeni hamle yapıldığında redo_stack'i temizle
            turn = 0;             // Sıra oyuncuya geçer

            // Oyun bitiş kontrolü
            if (check_game_end(board, size))
            {
                print_board(board, size);
                print_winner(player1, player2);
                break;
            }
        }

        print_board(board, size);
    }

    // Bellek temizliği
    for (int i = 0; i < size; i++)
    {
        free(board[i]);
    }
    free(board);
    free(move_stack->moves);
    free(move_stack);
    free(redo_stack->moves);
    free(redo_stack);

    return 0;
}
