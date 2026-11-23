#include "shared.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define LOG_FILE "program.log"

#define SHARED_NAME "Local\\Lab3SharedMemory"
#define LOCK_NAME "Local\\Lab3CounterLock"
#define COORDINATOR_NAME "Local\\Lab3Coordinator"
#define LOG_LOCK_NAME "Local\\Lab3LogLock"

static HANDLE mapping_handle = NULL;
static HANDLE lock_handle = NULL;
static HANDLE coordinator_handle = NULL;

static SharedData* shared_data = NULL;
static char program_path[MAX_PATH];

static int coordinator_owned = 0;

// открытие общей памяти
int shared_open(SharedData** data) {
    lock_handle = CreateMutexA(NULL, FALSE, LOCK_NAME);

    if (lock_handle == NULL) {
        return -1;
    }

    WaitForSingleObject(lock_handle, INFINITE);

    mapping_handle = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        SHARED_NAME
    );

    if (mapping_handle == NULL) {
        ReleaseMutex(lock_handle);
        return -1;
    }

    int initialize = GetLastError() != ERROR_ALREADY_EXISTS;

    shared_data = MapViewOfFile(
        mapping_handle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    );

    if (shared_data == NULL) {
        ReleaseMutex(lock_handle);
        return -1;
    }

    if (initialize) {
        shared_data->counter = 0;
    }

    *data = shared_data;

    ReleaseMutex(lock_handle);

    return 0;
}

// блокировка общего счетчика
void shared_lock(void) {
    WaitForSingleObject(lock_handle, INFINITE);
}

// снятие блокировки
void shared_unlock(void) {
    ReleaseMutex(lock_handle);
}

// закрытие общей памяти
void shared_close(void) {
    if (shared_data != NULL) {
        UnmapViewOfFile(shared_data);
        shared_data = NULL;
    }

    if (mapping_handle != NULL) {
        CloseHandle(mapping_handle);
        mapping_handle = NULL;
    }

    if (lock_handle != NULL) {
        CloseHandle(lock_handle);
        lock_handle = NULL;
    }
}

// попытка стать главным процессом
int shared_try_become_coordinator(void) {
    if (coordinator_handle == NULL) {
        coordinator_handle = CreateMutexA(
            NULL,
            FALSE,
            COORDINATOR_NAME
        );
    }

    if (coordinator_handle == NULL || coordinator_owned) {
        return coordinator_owned;
    }

    DWORD result = WaitForSingleObject(coordinator_handle, 0);

    if (result == WAIT_OBJECT_0 || result == WAIT_ABANDONED) {
        coordinator_owned = 1;
    }

    return coordinator_owned;
}

// освобождение роли главного процесса
void shared_release_coordinator(void) {
    if (coordinator_owned) {
        ReleaseMutex(coordinator_handle);
        coordinator_owned = 0;
    }

    if (coordinator_handle != NULL) {
        CloseHandle(coordinator_handle);
        coordinator_handle = NULL;
    }
}

// получение пути к программе
void set_program_path(const char* path) {
    (void)path;
    GetModuleFileNameA(NULL, program_path, MAX_PATH);
}

// запуск дочерней копии
int spawn_copy(ChildProcess* child, const char* mode) {
    char command[1024];

    snprintf(command, sizeof(command),
            "\"%s\" %s", program_path, mode);

    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};

    startup.cb = sizeof(startup);

    if (!CreateProcessA(
        program_path,
        command,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startup,
        &process
    )) {
        return -1;
    }

    CloseHandle(process.hThread);

    *child = (ChildProcess)process.hProcess;

    return 0;
}

// проверка завершения дочерней копии
int check_child(ChildProcess* child) {
    if (*child == 0) {
        return 0;
    }

    HANDLE process = (HANDLE)*child;

    DWORD result = WaitForSingleObject(process, 0);

    if (result == WAIT_TIMEOUT) {
        return 1;
    }

    if (result == WAIT_OBJECT_0) {
        CloseHandle(process);
        *child = 0;
        return 0;
    }

    return -1;
}

// пауза в миллисекундах
void sleep_ms(unsigned int milliseconds) {
    Sleep(milliseconds);
}

// получение времени для таймера
long long get_time_ms(void) {
    return (long long)GetTickCount64();
}

// чтение символа из терминала
int read_char(void) {
    if (_kbhit()) {
        return _getch();
    }

    return -1;
}

// получение текущего времени
void get_time(char* buffer, size_t size) {
    SYSTEMTIME time;

    GetLocalTime(&time);

    snprintf(buffer, size,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            time.wYear,
            time.wMonth,
            time.wDay,
            time.wHour,
            time.wMinute,
            time.wSecond,
            time.wMilliseconds);
}

// получение идентификатора процесса
int get_pid(void) {
    return (int)GetCurrentProcessId();
}

// запись сообщения в лог
void write_log(const char* message) {
    HANDLE log_lock = CreateMutexA(NULL, FALSE, LOG_LOCK_NAME);

    if (log_lock == NULL) {
        return;
    }

    WaitForSingleObject(log_lock, INFINITE);

    FILE* file = fopen(LOG_FILE, "a");

    if (file != NULL) {
        fprintf(file, "%s\n", message);
        fclose(file);
    }

    ReleaseMutex(log_lock);
    CloseHandle(log_lock);
}