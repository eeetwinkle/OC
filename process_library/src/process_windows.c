#include "process.h"
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

struct Process {
    HANDLE process_handle;
    int waited;
};

// Запускает программу в отдельном процессе Windows
int process_start(Process** process, const char* program, char* const args[]) {
    if (process == NULL || program == NULL || args == NULL) {
        return -1;
    }

    *process = NULL;

    // Формируем командную строку для CreateProcess
    char command[4096];
    snprintf(command, sizeof(command), "\"%s\"", program);

    for (int i = 1; args[i] != NULL; i++) {
        strcat(command, " ");
        strcat(command, args[i]);
    }

    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};

    startup_info.cb = sizeof(startup_info);

    // Создаём дочерний процесс.
    if (!CreateProcessA(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startup_info,
        &process_info
    )) {
        return -1;
    }

    Process* p = malloc(sizeof(Process));

    if (p == NULL) {
        TerminateProcess(process_info.hProcess, 1);
        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        return -1;
    }

    p->process_handle = process_info.hProcess;
    p->waited = 0;

    CloseHandle(process_info.hThread);

    *process = p;
    return 0;
}

// Ожидает завершения процесса и получает его код возврата.
int process_wait(Process* process, int* exit_code) {
    DWORD code;
    if (!GetExitCodeProcess(process->process_handle, &code)) {
        return -1;
    }

    *exit_code = (int)code;

    if (WaitForSingleObject(process->process_handle, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }

    if (!GetExitCodeProcess(process->process_handle, (DWORD*)exit_code)) {
        return -1;
    }

    process->waited = 1;
    return 0;
}

// Освобождает ресурсы процесса.
void process_destroy(Process* process) {
    if (process == NULL) {
        return;
    }

    if (!process->waited) {
        int exit_code;
        process_wait(process, &exit_code);
    }

    CloseHandle(process->process_handle);
    free(process);
}