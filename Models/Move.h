#pragma once
#include <stdlib.h>

// Тип для хранения координат на доске (8-битное целое число со знаком)
typedef int8_t POS_T;

// Структура для представления хода в игре
struct move_pos
{
    POS_T x, y;             // Начальные координаты шашки (откуда ходим)
    POS_T x2, y2;          // Конечные координаты шашки (куда ходим)
    POS_T xb = -1, yb = -1; // Координаты побитой шашки (-1, если нет взятия)

    // Конструктор для обычного хода без взятия
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }

    // Конструктор для хода со взятием
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Оператор сравнения ходов (без учета взятия)
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }

    // Оператор неравенства ходов
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
