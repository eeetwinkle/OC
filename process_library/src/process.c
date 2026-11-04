#include "process.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

//  структура для хранения процесс айди дочернего процесса
// и состояния ожидания его завершения
struct Process {
    pid_t pid;
    int waited;
};

// Запускает программу в дочернем процессе.
// два указателя на процесс - функция передаст информацию о созданном процессе обратно вызывающему коду
int process_start(Process** process, const char* program, char* const args[]) {
    // Проверяем корректность переданных аргументов.
    if (process == NULL || program == NULL || args == NULL) {
        return -1;
    }
//указатель ноль - процесс не создан
    *process = NULL;

    // выделяем память для процесса
    Process* p = malloc(sizeof(Process));
    if (p == NULL) {
        return -1;
    }

    // создаётся дочерний процесс возвр пид 
    // если пид меньше 0, то ошибка, равен нулю - выполняется дочерний процесс
    // больше нуля - родительский выполняется, а число это пид дочернего 
    pid_t pid = fork();

    if (pid < 0) {
        free(p);
        return -1;
    }

    if (pid == 0) {
        // запуск указанной программы в дочернем процессе
        execvp(program, args);

        // 52, если запуск не удался
        _exit(52);
    }

    // сохраняем пид дочернего процесса
    p->pid = pid;
    p->waited = 0;

    *process = p;
    return 0;
}

// Ожидает завершения процесса и получает его код возврата.
int process_wait(Process* process, int* exit_code) {

    if (process == NULL || exit_code == NULL || process->waited) {
        return -1;
    }

    int status;
    pid_t result;

    // Ожидаем завершения процесса, повторяя вызов при прерывании.
    do {
        result = waitpid(process->pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        return -1;
    }

    // Получаем код завершения процесса.
    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        // Если процесс завершён сигналом, формируем код 128 + номер сигнала.
        *exit_code = 128 + WTERMSIG(status);
    } else {
        return -1;
    }

    process->waited = 1;
    return 0;
}

// Освобождает память, занятую структурой процесса.
void process_destroy(Process* process) {
    if (process == NULL) {
        return;
    }

    // Если процесс ещё не завершён, дожидаемся его окончания.
    if (!process->waited) {
        int exit_code;
        process_wait(process, &exit_code);
    }

    free(process);
}