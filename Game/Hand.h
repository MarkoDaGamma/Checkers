#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс для обработки пользовательского ввода (мышь, окно)
class Hand
{
  public:
    // Конструктор принимает указатель на игровую доску
    Hand(Board *board) : board(board)
    {
    }

    // Получает координаты выбранной клетки и тип действия
    // Возвращает кортеж из:
    // - Response: тип действия (QUIT, BACK, REPLAY, CELL)
    // - POS_T: координата x выбранной клетки (-1 если не выбрана)
    // - POS_T: координата y выбранной клетки (-1 если не выбрана)
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;      // Координаты клика мыши
        int xc = -1, yc = -1;    // Координаты клетки на доске

        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Нажатие на крестик окна
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN:  // Клик мышью
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    // Преобразование координат экрана в координаты доски
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    
                    // Проверка специальных зон клика
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)  // Кнопка "назад"
                    {
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8)  // Кнопка "новая игра"
                    {
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)  // Клетка на доске
                    {
                        resp = Response::CELL;
                    }
                    else  // Клик вне доски и кнопок
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:  // Событие окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)  // Изменение размера окна
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return {resp, xc, yc};
    }

    // Ожидает действия пользователя в конце игры
    // Возвращает:
    // - Response::QUIT при закрытии окна
    // - Response::REPLAY при нажатии кнопки "новая игра"
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:  // Закрытие окна
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:  // Изменение размера окна
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {  // Клик мышью
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    // Преобразование координат экрана в координаты доски
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)  // Клик по кнопке "новая игра"
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

  private:
    Board *board;  // Указатель на игровую доску
};
