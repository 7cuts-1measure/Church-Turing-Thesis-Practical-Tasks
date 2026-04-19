#ifndef TURING_MACHINE_H
#define TURING_MACHINE_H

#define ACCEPT 0   // Успешное завершение
#define REJECT -1  // Ошибка выполнения

// Структура для описания одного правила перехода
typedef struct {
    char current_state;    // Текущее состояние
    char read_symbol;      // Символ, считанный с ленты
    char write_symbol;     // Символ, который будет записан
    char direction;        // Направление: 'L', 'R' или 'N'
    char next_state;       // Новое состояние
} Transition;

// Структура машины Тьюринга
typedef struct {
    char *tape;             // Лента (динамический массив)
    int tape_size;          // Текущий размер ленты
    int head;               // Позиция головки
    char current_state;     // Текущее состояние
    Transition *transitions; // Массив правил
    int num_transitions;    // Количество правил
    int transitions_capacity; // Ёмкость массива правил
} TuringMachine;

/*
 * Функция: create_machine
 * ------------------------
 * Создаёт экземпляр машины Тьюринга.
 * 
 * Параметры:
 *   tape_initial – строка, содержащая исходное содержимое ленты
 * 
 * Возвращает:
 *   Указатель на созданную машину или NULL при ошибке
 */
TuringMachine* create_machine(const char *tape_initial);

/*
 * Функция: add_transition
 * ------------------------
 * Добавляет новое правило перехода в машину.
 */
void add_transition(TuringMachine *tm, char current_state, char read_symbol, 
                    char write_symbol, char direction, char next_state);

/*
 * Функция: run_machine
 * ---------------------
 * Симулирует работу машины Тьюринга, последовательно применяя правила переходов,
 * пока не будет достигнуто финальное состояние (A или R) или не кончатся правила.
 */
void run_machine(TuringMachine *tm);

/*
 * Функция: destroy_machine
 * -------------------------
 * Освобождает всю память, выделенную под машину Тьюринга.
 */
void destroy_machine(TuringMachine *tm);

/*
 * Функция: process_string
 * ------------------------
 * Преобразует входную строку в формат "строка#строка" с помощью машины Тьюринга.
 * 
 * Параметры:
 *   input – входная строка из символов 'a' и 'b'
 * 
 * Возвращает:
 *   Указатель на динамически выделенную строку-результат
 *   NULL в случае ошибки
 */
 char* process_string(const char *input);

#endif