#include "shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile sig_atomic_t running = 1;

// обработка сигнала завершения
void handle_signal(int signal) {
    (void)signal;
    running = 0;
}

// изменение общего счетчика
void change_counter(SharedData* data, int operation, long long value) {
    shared_lock();

    if (operation == 1) {
        data->counter += value;
    } else if (operation == 2) {
        data->counter *= value;
    } else if (operation == 3) {
        data->counter /= value;
    } else {
        data->counter = value;
    }

    shared_unlock();
}

// выполнение первой или второй копии
void run_copy(SharedData* data, int mode) {
    char time[64];
    char message[256];

    get_time(time, sizeof(time));

    snprintf(message, sizeof(message),
            "%s | PID=%d | Копия запущена",
            time, get_pid());

    write_log(message);

    if (mode == 1) {
        change_counter(data, 1, 10);
    } else {
        change_counter(data, 2, 2);

        sleep_ms(2000);

        change_counter(data, 3, 2);
    }

    get_time(time, sizeof(time));

    snprintf(message, sizeof(message),
            "%s | PID=%d | Копия завершена",
            time, get_pid());

    write_log(message);
}

// чтение нового значения счетчика
void check_input(SharedData* data) {
    static char input[64];
    static int position = 0;

    int ch = read_char();

    if (ch == -1) {
        return;
    }

    if (ch == '\n' || ch == '\r') {
        input[position] = '\0';

        if (position > 0) {
            char* end;
            long long value = strtoll(input, &end, 10);

            if (*end == '\0') {
                change_counter(data, 0, value);
                printf("\nСчетчик изменен: %lld\n", value);
            } else {
                printf("\nВведите целое число\n");
            }
        }

        position = 0;
        return;
    }

    if ((ch >= '0' && ch <= '9') ||
        (ch == '-' && position == 0)) {

        if (position < 63) {
            input[position++] = (char)ch;
            putchar(ch);
            fflush(stdout);
        }
    }
}

int main(int argc, char* argv[]) {
    SharedData* data = NULL;

    if (shared_open(&data) != 0) {
        printf("Ошибка общей памяти\n");
        return 1;
    }

    // запуск дочерней копии
    if (argc > 1) {
        if (strcmp(argv[1], "copy1") == 0) {
            run_copy(data, 1);
        } else if (strcmp(argv[1], "copy2") == 0) {
            run_copy(data, 2);
        }

        shared_close();
        return 0;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    set_program_path(argv[0]);

    char time[64];
    char message[256];

    // запись информации о запуске
    get_time(time, sizeof(time));

    snprintf(message, sizeof(message),
            "%s | PID=%d | Программа запущена",
            time, get_pid());

    write_log(message);

    printf("Программа запущена\n");
    printf("Введите новое значение счетчика и нажмите Enter\n");

    ChildProcess copy1 = 0;
    ChildProcess copy2 = 0;

    long long next_tick = get_time_ms() + 300;
    long long next_log = get_time_ms() + 1000;
    long long next_spawn = get_time_ms() + 3000;

    int coordinator = 0;

    while (running) {
        long long now = get_time_ms();

        // чтение команды пользователя
        check_input(data);

        // увеличение счетчика каждые 300 мс
        if (now >= next_tick) {
            change_counter(data, 1, 1);
            next_tick += 300;
        }

        // попытка стать главным процессом
        if (!coordinator) {
            coordinator = shared_try_become_coordinator();
        }

        // запись счетчика в лог каждую секунду
        if (coordinator && now >= next_log) {
            shared_lock();
            long long value = data->counter;
            shared_unlock();

            get_time(time, sizeof(time));

            snprintf(message, sizeof(message),
                    "%s | PID=%d | Счетчик=%lld",
                    time, get_pid(), value);

            write_log(message);

            next_log += 1000;
        }

        // запуск двух копий каждые три секунды
        if (coordinator && now >= next_spawn) {
            if (copy1 == 0) {
                if (spawn_copy(&copy1, "copy1") != 0) {
                    write_log("Ошибка запуска копии 1");
                }
            } else {
                write_log("Копия 1 еще работает. Запуск пропущен");
            }

            if (copy2 == 0) {
                if (spawn_copy(&copy2, "copy2") != 0) {
                    write_log("Ошибка запуска копии 2");
                }
            } else {
                write_log("Копия 2 еще работает. Запуск пропущен");
            }

            next_spawn += 3000;
        }

        // проверка завершения дочерних процессов
        check_child(&copy1);
        check_child(&copy2);

        sleep_ms(10);
    }

    shared_release_coordinator();
    shared_close();

    printf("\nПрограмма завершена\n");

    return 0;
}