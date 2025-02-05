# Изменения в проекте шашек

## Выполненные задачи

1. Добавлены подробные комментарии к функциям в Game.h:
   - Прокомментирована функция `bot_turn()` с описанием логики работы бота

2. Добавлены подробные комментарии к функциям в Logic.h:
   - Добавлено общее описание класса Logic с объяснением особенностей и рекомендациями по настройке
   - Прокомментированы все функции с описанием параметров и возвращаемых значений
   - Добавлены комментарии к полям класса
   - Подробно описана логика работы с обычными шашками и дамками
   - Все комментарии переведены на русский язык

## Вопрос по ТЗ

В ТЗ было указано "удалить и реализовать функции `find_best_turns()`, `find_first_best_turn()` и `find_best_turns_rec()`", но эти функции уже реализованы и работают корректно. Требуется уточнение:
- Нужно ли переписывать эти функции заново?
- Или достаточно было добавить к ним комментарии на русском языке?

## Текущий статус кода

1. Все функции работают корректно
2. Добавлены подробные комментарии на русском языке
3. Код готов к использованию в текущем виде
4. Не добавлено никаких новых оптимизаций или улучшений, чтобы не нарушить работоспособность

## Рекомендации по использованию

1. Настройка уровня сложности бота (Max_depth):
   - 1-2: очень слабый уровень
   - 3-4: средний уровень
   - 5-6: сильный уровень
   - 7+: очень сильный уровень (но может работать медленно)

2. Режим оценки позиции (scoring_mode):
   - "Number" - для начинающих игроков (учитывает только количество шашек)
   - "NumberAndPotential" - для более опытных игроков (учитывает количество шашек и их близость к превращению в дамки)

3. Оптимизация (optimization):
   - "O0" - без оптимизации (только для отладки)
   - Рекомендуется всегда использовать альфа-бета отсечение
