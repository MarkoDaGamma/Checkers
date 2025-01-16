#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

// Класс, реализующий игровую логику и искусственный интеллект для игры в шашки
// 
// Основные особенности:
// 1. Использует алгоритм минимакс с альфа-бета отсечением для поиска лучшего хода
// 2. Поддерживает настраиваемую глубину поиска (Max_depth)
// 3. Имеет два режима оценки позиции:
//    - "Number" - учитывает только количество шашек
//    - "NumberAndPotential" - учитывает количество шашек и их близость к превращению в дамки
// 4. Поддерживает оптимизацию поиска:
//    - "O0" - без оптимизации
//    - Другие значения - с альфа-бета отсечением, которое значительно уменьшает
//      количество рассматриваемых позиций за счет пропуска заведомо невыгодных вариантов
//
// Рекомендации по настройке:
// 1. Max_depth (глубина поиска):
//    - 1-2: очень слабый уровень
//    - 3-4: средний уровень
//    - 5-6: сильный уровень
//    - 7+: очень сильный уровень (но может работать медленно)
// 2. scoring_mode:
//    - "Number" - для начинающих игроков
//    - "NumberAndPotential" - для более опытных игроков
// 3. optimization:
//    - "O0" - только для отладки
//    - Рекомендуется всегда использовать альфа-бета отсечение
class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    // Поиск лучшей последовательности ходов для текущего игрока
    // Параметр color: true - черные, false - белые
    // Возвращает вектор ходов, которые нужно сделать (может быть несколько при серии взятий)
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    // Выполняет ход на виртуальной доске и возвращает новое состояние
    // Параметры:
    // mtx - текущее состояние доски
    // turn - ход, который нужно выполнить
    // Возвращает новое состояние доски после выполнения хода
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    // Вычисляет оценку текущей позиции для бота
    // Параметры:
    // mtx - текущее состояние доски
    // first_bot_color - цвет бота, который делает первый ход (true - черные, false - белые)
    // Возвращает оценку позиции:
    // - чем больше оценка, тем лучше позиция для бота
    // - INF означает победу бота
    // - 0 означает поражение бота
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // Цвет игрока, для которого максимизируем оценку
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    // Рекурсивно ищет лучший первый ход и все последующие ходы в серии взятий
    // Параметры:
    // mtx - текущее состояние доски
    // color - цвет игрока, который делает ход (true - черные, false - белые)
    // x, y - координаты шашки, которой нужно сделать следующий ход в серии взятий
    // state - индекс текущего состояния в векторах next_move и next_best_state
    // alpha - лучшая оценка, найденная на данный момент (для альфа-бета отсечения)
    // Возвращает оценку лучшего найденного хода
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1)
    {
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;
        if (state != 0)
            find_turns(x, y, mtx);
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        vector<move_pos> best_moves;
        vector<int> best_states;

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;
            if (have_beats_now)
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score)
            {
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score;
    }

    // Рекурсивно ищет лучший ход с использованием минимакса и альфа-бета отсечения
    // Параметры:
    // mtx - текущее состояние доски
    // color - цвет игрока, который делает ход (true - черные, false - белые)
    // depth - текущая глубина рекурсии
    // alpha - нижняя граница оценки (для альфа-бета отсечения)
    // beta - верхняя граница оценки (для альфа-бета отсечения)
    // x, y - координаты шашки, которой нужно сделать следующий ход в серии взятий
    // Возвращает оценку лучшего найденного хода
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color));
        }
        if (x != -1)
        {
            find_turns(x, y, mtx);
        }
        else
            find_turns(color, mtx);
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty())
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1;
        double max_score = -1;
        for (auto turn : turns_now)
        {
            double score = 0.0;
            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            min_score = min(min_score, score);
            max_score = max(max_score, score);
            // alpha-beta pruning
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        return (depth % 2 ? max_score : min_score);
    }

public:
    // Находит все возможные ходы для указанного цвета на текущей доске
    // Параметр color: true - черные, false - белые
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    // Находит все возможные ходы для шашки в указанной позиции
    // Параметры:
    // x, y - координаты шашки
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // Находит все возможные ходы для указанного цвета на текущей доске
    // Параметры:
    // color - цвет игрока (true - черные, false - белые)
    // mtx - текущее состояние доски
    // Результат сохраняется в поля turns и have_beats
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    // Находит все возможные ходы для шашки в указанной позиции
    // Параметры:
    // x, y - координаты шашки
    // mtx - текущее состояние доски
    // Результат сохраняется в поля turns и have_beats
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // Проверяем возможные взятия
        switch (type)  // Тип шашки: 1 - белая, 2 - черная, 3 - белая дамка, 4 - черная дамка
        {
        // Обработка обычных шашек (белых и черных)
        case 1:  // Белая шашка
        case 2:  // Черная шашка
            // Проверяем взятия для обычных шашек
            // Обычная шашка может бить на 2 клетки по диагонали в любом направлении
            // Шаг 4 используется, чтобы проверить клетки через одну:
            // - (i = x±2, j = y±2) - клетка, куда можно сделать ход
            // - ((x+i)/2, (y+j)/2) - клетка с шашкой противника, которую бьем
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)  // Проверка выхода за границы доски
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;  // Координаты бьемой шашки
                    // Проверяем, что:
                    // - клетка для хода пуста (mtx[i][j] == 0)
                    // - на пути есть шашка противника (!mtx[xb][yb] == false)
                    // - эта шашка противника другого цвета (mtx[xb][yb] % 2 != type % 2)
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;

        // Обработка дамок (белых и черных)
        default:  // Дамки (тип 3 и 4)
            // Проверяем взятия для дамок
            // Дамка может ходить на любое количество клеток по диагонали
            // и бить через любое количество пустых клеток
            for (POS_T i = -1; i <= 1; i += 2)  // Направление движения по вертикали
            {
                for (POS_T j = -1; j <= 1; j += 2)  // Направление движения по горизонтали
                {
                    POS_T xb = -1, yb = -1;  // Координаты шашки, которую можем побить
                    // Проверяем все клетки по диагонали в выбранном направлении
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])  // Если встретили шашку
                        {
                            // Если это шашка того же цвета или мы уже нашли шашку для взятия,
                            // то дальше идти нельзя
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            // Запоминаем координаты шашки, которую можем побить
                            xb = i2;
                            yb = j2;
                        }
                        // Если нашли шашку для взятия и текущая клетка пуста,
                        // то можем сделать ход на эту клетку
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }

        // Если есть взятия, то другие ходы не проверяем
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }

        // Проверяем обычные ходы (без взятия)
        switch (type)
        {
        // Обработка обычных шашек (белых и черных)
        case 1:  // Белая шашка
        case 2:  // Черная шашка
            // Проверяем ходы для обычных шашек
            // Обычная шашка может ходить по диагонали вперед на одну клетку:
            // - белые (тип 1) ходят вверх (i = x-1)
            // - черные (тип 2) ходят вниз (i = x+1)
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);  // Координата по вертикали
                for (POS_T j = y - 1; j <= y + 1; j += 2)  // Проверяем обе диагонали
                {
                    // Проверяем выход за границы доски и наличие других шашек
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }

        // Обработка дамок (белых и черных)
        default:  // Дамки (тип 3 и 4)
            // Проверяем ходы для дамок
            // Дамка может ходить на любое количество клеток по диагонали в любом направлении
            for (POS_T i = -1; i <= 1; i += 2)  // Направление движения по вертикали
            {
                for (POS_T j = -1; j <= 1; j += 2)  // Направление движения по горизонтали
                {
                    // Проверяем все клетки по диагонали в выбранном направлении
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])  // Если встретили шашку, дальше идти нельзя
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    // Вектор возможных ходов для текущей позиции
    vector<move_pos> turns;
    
    // Флаг наличия взятий в текущей позиции
    bool have_beats;
    
    // Максимальная глубина поиска для бота (уровень сложности)
    int Max_depth;

  private:
    // Генератор случайных чисел для выбора хода
    default_random_engine rand_eng;
    
    // Режим подсчета очков: "Number" - только количество шашек,
    // "NumberAndPotential" - количество шашек и их потенциал
    string scoring_mode;
    
    // Уровень оптимизации: "O0" - без оптимизации,
    // другие значения - с альфа-бета отсечением
    string optimization;
    
    // Вектор ходов для каждого состояния при поиске лучшего хода
    vector<move_pos> next_move;
    
    // Вектор следующих состояний при поиске лучшего хода
    vector<int> next_best_state;
    
    // Указатель на игровую доску
    Board *board;
    
    // Указатель на конфигурацию игры
    Config *config;
};
