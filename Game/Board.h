#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

// Условная компиляция для разных операционных систем
#ifdef __APPLE__
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>
#else
    #include <SDL.h>
    #include <SDL_image.h>
#endif

using namespace std;

// Класс, представляющий игровую доску и её графическое отображение
class Board
{
public:
    Board() = default;
    // Конструктор с заданными размерами окна
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // Инициализация SDL и создание начального состояния доски
    // Возвращает 0 при успехе, 1 при ошибке
    int start_draw()
    {
        // Инициализация SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }

        // Если размеры окна не заданы, устанавливаем их автоматически
        if (W == 0 || H == 0)
        {
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm))
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h);
            W -= W / 15;
            H = W;
        }

        // Создание окна
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE);
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }

        // Создание рендерера
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }

        // Загрузка текстур
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay)
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }

        // Получение реальных размеров окна
        SDL_GetRendererOutputSize(ren, &W, &H);
        
        // Инициализация начального состояния доски
        make_start_mtx();
        rerender();
        return 0;
    }

    // Сброс доски к начальному состоянию
    void redraw()
    {
        game_results = -1;  // Сброс результата игры
        history_mtx.clear();  // Очистка истории ходов
        history_beat_series.clear();  // Очистка истории серий взятий
        make_start_mtx();  // Создание начальной расстановки
        clear_active();  // Очистка активной шашки
        clear_highlight();  // Очистка подсветки
    }

    // Перемещение шашки с учетом взятия
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1)  // Если есть взятие
        {
            mtx[turn.xb][turn.yb] = 0;  // Удаляем побитую шашку
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series);
    }

    // Перемещение шашки из позиции (i,j) в позицию (i2,j2)
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        if (mtx[i2][j2])  // Проверка, что целевая клетка пуста
        {
            throw runtime_error("final position is not empty, can't move");
        }
        if (!mtx[i][j])  // Проверка, что начальная клетка не пуста
        {
            throw runtime_error("begin position is empty, can't move");
        }
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7))  // Проверка, что шашка достигла края доски
            mtx[i][j] += 2;
        mtx[i2][j2] = mtx[i][j];  // Перемещение шашки
        drop_piece(i, j);  // Удаление шашки из начальной позиции
        add_history(beat_series);  // Добавление хода в историю
    }

    // Удаление шашки из позиции (i,j)
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0;  // Удаление шашки
        rerender();  // Перерисовка доски
    }

    // Превращение шашки в дамку
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2)  // Проверка, что шашка не пуста и не является дамкой
        {
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2;  // Превращение шашки в дамку
        rerender();  // Перерисовка доски
    }

    // Возвращает текущее состояние доски
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    // Подсветка клеток
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1;  // Подсветка клетки
        }
        rerender();  // Перерисовка доски
    }

    // Удаление подсветки
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0);  // Удаление подсветки
        }
        rerender();  // Перерисовка доски
    }

    // Установка активной шашки
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x;  // Установка активной шашки
        active_y = y;
        rerender();  // Перерисовка доски
    }

    // Удаление активной шашки
    void clear_active()
    {
        active_x = -1;  // Удаление активной шашки
        active_y = -1;
        rerender();  // Перерисовка доски
    }

    // Проверка, подсвечена ли клетка
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y];  // Возвращает true, если клетка подсвечена
    }

    // Отмена последнего хода
    void rollback()
    {
        auto beat_series = max(1, *(history_beat_series.rbegin()));  // Получение последней серии взятий
        while (beat_series-- && history_mtx.size() > 1)  // Отмена последнего хода
        {
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin());  // Восстановление предыдущего состояния доски
        clear_highlight();  // Удаление подсветки
        clear_active();  // Удаление активной шашки
    }

    // Показ результата игры
    void show_final(const int res)
    {
        game_results = res;  // Установка результата игры
        rerender();  // Перерисовка доски
    }

    // Обновление размеров окна
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H);  // Получение реальных размеров окна
        rerender();  // Перерисовка доски
    }

    // Выход из игры
    void quit()
    {
        SDL_DestroyTexture(board);  // Удаление текстур
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren);  // Удаление рендерера
        SDL_DestroyWindow(win);  // Удаление окна
        SDL_Quit();  // Выход из SDL
    }

    // Деструктор
    ~Board()
    {
        if (win)  // Если окно существует
            quit();  // Выход из игры
    }

private:
    // Добавление хода в историю
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx);  // Добавление текущего состояния доски в историю
        history_beat_series.push_back(beat_series);  // Добавление серии взятий в историю
    }

    // Создание начальной расстановки шашек
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0;  // Очистка доски
                if (i < 3 && (i + j) % 2 == 1)  // Расстановка белых шашек
                    mtx[i][j] = 2;
                if (i > 4 && (i + j) % 2 == 1)  // Расстановка черных шашек
                    mtx[i][j] = 1;
            }
        }
        add_history();  // Добавление начальной расстановки в историю
    }

    // Перерисовка доски
    void rerender()
    {
        // Очистка экрана
        SDL_RenderClear(ren);

        // Рисование доски
        SDL_RenderCopy(ren, board, NULL, NULL);

        // Рисование шашек
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])  // Если клетка пуста
                    continue;
                int wpos = W * (j + 1) / 10 + W / 120;  // Вычисление координаты шашки
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 };

                SDL_Texture* piece_texture;  // Выбор текстуры шашки
                if (mtx[i][j] == 1)
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect);  // Рисование шашки
            }
        }

        // Рисование подсветки
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0);
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale);
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])  // Если клетка не подсвечена
                    continue;
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell);  // Рисование подсветки
            }
        }

        // Рисование активной шашки
        if (active_x != -1)
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0);
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell);  // Рисование активной шашки
        }
        SDL_RenderSetScale(ren, 1, 1);

        // Рисование кнопок
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // Рисование результата игры
        if (game_results != -1)
        {
            string result_path = draw_path;
            if (game_results == 1)
                result_path = white_path;
            else if (game_results == 2)
                result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect);
            SDL_DestroyTexture(result_texture);
        }

        SDL_RenderPresent(ren);  // Обновление экрана
        // next rows for mac os
        SDL_Delay(10);
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent);
    }

    // Вывод ошибки в лог
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Error: " << text << ". "<< SDL_GetError() << endl;
        fout.close();
    }

public:
    int W = 0;  // Ширина окна
    int H = 0;  // Высота окна
    // История ходов
    vector<vector<vector<POS_T>>> history_mtx;

private:
    SDL_Window *win = nullptr;  // Окно
    SDL_Renderer *ren = nullptr;  // Рендерер
    // Текстуры
    SDL_Texture *board = nullptr;
    SDL_Texture *w_piece = nullptr;
    SDL_Texture *b_piece = nullptr;
    SDL_Texture *w_queen = nullptr;
    SDL_Texture *b_queen = nullptr;
    SDL_Texture *back = nullptr;
    SDL_Texture *replay = nullptr;
    // Пути к текстурам
    const string textures_path = project_path + "Textures/";
    const string board_path = textures_path + "board.png";
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png";
    const string queen_white_path = textures_path + "queen_white.png";
    const string queen_black_path = textures_path + "queen_black.png";
    const string white_path = textures_path + "white_wins.png";
    const string black_path = textures_path + "black_wins.png";
    const string draw_path = textures_path + "draw.png";
    const string back_path = textures_path + "back.png";
    const string replay_path = textures_path + "replay.png";
    // Координаты активной шашки
    int active_x = -1;
    int active_y = -1;
    // Результат игры
    int game_results = -1;
    // Матрица подсветки
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0));
    // Матрица доски
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));
    // История серий взятий
    vector<int> history_beat_series;
};
