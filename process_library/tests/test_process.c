#include "process.h"

#include <stdio.h>

int main(void) {
    Process* process = NULL;
    int exit_code;

#ifdef _WIN32
    // Команда для Windows.
    char* args[] = {"cmd", "/C", "exit 67", NULL};
    const char* program = "cmd";
#else
    // Команда для macOS и Linux.
    char* args[] = {"sh", "-c", "exit 67", NULL};
    const char* program = "sh";
#endif

    // Запускаем дочерний процесс.
    if (process_start(&process, program, args) != 0) {
        printf("Ошибка запуска процесса\n");
        return 1;
    }

    // Ожидаем завершения процесса.
    if (process_wait(process, &exit_code) != 0) {
        printf("Ошибка ожидания процесса\n");
        process_destroy(process);
        return 1;
    }

    printf("Код завершения процесса: %d\n", exit_code);

    // Освобождаем ресурсы.
    process_destroy(process);

    return 0;
}