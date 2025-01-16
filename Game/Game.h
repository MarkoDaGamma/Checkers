#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Основная функция игры, управляет игровым процессом
    int play()
    {
        // Засекаем время начала игры
        auto start = chrono::steady_clock::now();
        
        // Если это повторная игра, перезагружаем настройки и доску
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // Первичная отрисовка доски
            board.start_draw();
        }
        is_replay = false;

        // Основной игровой цикл
        int turn_num = -1;  // Номер текущего хода
        bool is_quit = false;  // Флаг выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns");  // Максимальное количество ходов
        
        while (++turn_num < Max_turns)
        {
            beat_series = 0;  // Сброс серии взятий
            logic.find_turns(turn_num % 2);  // Поиск возможных ходов для текущего игрока
            
            // Если ходов нет - игра окончена
            if (logic.turns.empty())
                break;
                
            // Установка уровня сложности бота для текущего игрока
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            
            // Проверка, является ли текущий игрок человеком или ботом
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ход игрока-человека
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)  // Отмена хода
                {
                    // Отмена двух ходов, если предыдущий ход был сделан ботом
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2);  // Ход бота
        }
        
        // Записываем время игры в лог
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // Обработка завершения игры
        if (is_replay)
            return play();
        if (is_quit)
            return 0;
            
        // Определение результата игры
        int res = 2;  // По умолчанию - ничья
        if (turn_num == Max_turns)
        {
            res = 0;  // Превышено максимальное количество ходов
        }
        else if (turn_num % 2)
        {
            res = 1;  // Победа одного из игроков
        }
        
        // Показ финального экрана и ожидание действия игрока
        board.show_final(res);
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    }

  private:
    // Обработка хода бота
    // Параметр color: true - черные, false - белые
    void bot_turn(const bool color)
    {
        // Засекаем время начала хода
        auto start = chrono::steady_clock::now();

        // Получаем задержку хода из настроек
        auto delay_ms = config("Bot", "BotDelayMS");
        
        // Создаем отдельный поток для обеспечения минимальной задержки хода
        // Это нужно, чтобы бот не делал ходы слишком быстро
        thread th(SDL_Delay, delay_ms);
        
        // Находим лучшую последовательность ходов для текущего состояния
        auto turns = logic.find_best_turns(color);
        th.join();  // Ждем окончания задержки
        
        bool is_first = true;
        // Выполняем все ходы из найденной последовательности
        // (может быть несколько ходов при серии взятий)
        for (auto turn : turns)
        {
            // Добавляем задержку между ходами в серии взятий
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            
            // Увеличиваем счетчик серии взятий, если текущий ход - взятие
            beat_series += (turn.xb != -1);
            
            // Выполняем ход на доске
            board.move_piece(turn, beat_series);
        }

        // Записываем время хода в лог
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    // Обработка хода игрока
    // Параметр color: true - черные, false - белые
    // Возвращает Response::OK при успешном ходе,
    // Response::QUIT для выхода из игры,
    // Response::REPLAY для начала новой игры,
    // Response::BACK для отмены хода
    Response player_turn(const bool color)
    {
        // Создаем список клеток с возможными ходами
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);  // Подсвечиваем возможные ходы
        
        move_pos pos = {-1, -1, -1, -1};  // Структура для хранения хода (начальная и конечная позиции)
        POS_T x = -1, y = -1;  // Координаты выбранной фишки
        
        // Ожидаем выбора фишки и клетки для хода
        while (true)
        {
            auto resp = hand.get_cell();  // Получаем клетку, выбранную игроком
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);  // Если получена не клетка, возвращаем полученный ответ
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};  // Координаты выбранной клетки

            // Проверяем корректность выбранной клетки
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)  // Если выбрана фишка с возможным ходом
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})  // Если выбрана клетка для хода
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)  // Если ход найден, выходим из цикла
                break;
                
            if (!is_correct)  // Если выбрана некорректная клетка
            {
                if (x != -1)  // Если была выбрана фишка, очищаем подсветку
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            
            // Запоминаем выбранную фишку и показываем возможные ходы для нее
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }
        
        // Выполняем ход
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        
        if (pos.xb == -1)  // Если это не взятие
            return Response::OK;
            
        // Если это взятие, проверяем возможность продолжения серии взятий
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);  // Ищем возможные взятия с новой позиции
            if (!logic.have_beats)  // Если взятий больше нет, завершаем ход
                break;

            // Подсвечиваем возможные клетки для продолжения взятия
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            
            // Ожидаем выбора клетки для продолжения взятия
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                // Проверяем корректность выбранной клетки
                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                // Выполняем взятие
                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
