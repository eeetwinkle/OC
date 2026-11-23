#define _DEFAULT_SOURCE
#define _DARWIN_C_SOURCE

#include "shared.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <time.h>
#include <limits.h>


#define SHARED_FILE "/tmp/lab3_shared.dat"
#define LOCK_FILE "/tmp/lab3_counter.lock"
#define COORDINATOR_FILE "/tmp/lab3_coordinator.lock"
#define LOG_LOCK_FILE "/tmp/lab3_log.lock"
#define LOG_FILE "program.log"

static int shared_fd = -1;
static int lock_fd = -1;
static int coordinator_fd = -1;

static SharedData* shared_data = NULL;
static char program_path[PATH_MAX];

// открытие общей памяти
int shared_open(SharedData** data) {
    lock_fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);

    if (lock_fd == -1) {
        return -1;
    }

    flock(lock_fd, LOCK_EX);

    shared_fd = open(SHARED_FILE, O_CREAT | O_RDWR, 0666);

    if (shared_fd == -1) {
        flock(lock_fd, LOCK_UN);
        return -1;
    }

    struct stat info;

    if (fstat(shared_fd, &info) == -1) {
        flock(lock_fd, LOCK_UN);
        return -1;
    }

    int initialize = info.st_size != sizeof(SharedData);

    if (initialize && ftruncate(shared_fd, sizeof(SharedData)) == -1) {
        flock(lock_fd, LOCK_UN);
        return -1;
    }

    shared_data = mmap(NULL, sizeof(SharedData),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, shared_fd, 0);

    if (shared_data == MAP_FAILED) {
        shared_data = NULL;
        flock(lock_fd, LOCK_UN);
        return -1;
    }

    if (initialize) {
        shared_data->counter = 0;
    }

    *data = shared_data;

    flock(lock_fd, LOCK_UN);

    return 0;
}

// блокировка общего счетчика
void shared_lock(void) {
    flock(lock_fd, LOCK_EX);
}

// снятие блокировки
void shared_unlock(void) {
    flock(lock_fd, LOCK_UN);
}

// закрытие общей памяти
void shared_close(void) {
    if (shared_data != NULL) {
        munmap(shared_data, sizeof(SharedData));
        shared_data = NULL;
    }

    if (shared_fd != -1) {
        close(shared_fd);
        shared_fd = -1;
    }

    if (lock_fd != -1) {
        close(lock_fd);
        lock_fd = -1;
    }
}

// попытка стать главным процессом
int shared_try_become_coordinator(void) {
    if (coordinator_fd == -1) {
        coordinator_fd = open(
            COORDINATOR_FILE,
            O_CREAT | O_RDWR,
            0666
        );
    }

    if (coordinator_fd == -1) {
        return 0;
    }

    return flock(coordinator_fd, LOCK_EX | LOCK_NB) == 0;
}

// освобождение роли главного процесса
void shared_release_coordinator(void) {
    if (coordinator_fd != -1) {
        flock(coordinator_fd, LOCK_UN);
        close(coordinator_fd);
        coordinator_fd = -1;
    }
}

// получение пути к программе
void set_program_path(const char* path) {
    if (realpath(path, program_path) == NULL) {
        snprintf(program_path, sizeof(program_path), "%s", path);
    }
}

// запуск дочерней копии
int spawn_copy(ChildProcess* child, const char* mode) {
    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        execl(program_path, program_path, mode, NULL);
        _exit(127);
    }

    *child = (ChildProcess)pid;

    return 0;
}

// проверка завершения дочерней копии
int check_child(ChildProcess* child) {
    if (*child == 0) {
        return 0;
    }

    int status;

    pid_t result = waitpid((pid_t)*child, &status, WNOHANG);

    if (result == 0) {
        return 1;
    }

    if (result > 0) {
        *child = 0;
        return 0;
    }

    return -1;
}

// пауза в миллисекундах
void sleep_ms(unsigned int milliseconds) {
    struct timespec time;

    time.tv_sec = milliseconds / 1000;
    time.tv_nsec = (milliseconds % 1000) * 1000000L;

    nanosleep(&time, NULL);
}

// получение времени для таймера
long long get_time_ms(void) {
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);

    return (long long)time.tv_sec * 1000 +
        time.tv_nsec / 1000000;
}

// чтение символа из терминала
int read_char(void) {
    fd_set set;
    struct timeval timeout = {0, 0};

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        char ch;

        if (read(STDIN_FILENO, &ch, 1) == 1) {
            return ch;
        }
    }

    return -1;
}

// получение текущего времени
void get_time(char* buffer, size_t size) {
    struct timespec ts;
    struct tm time_info;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &time_info);

    snprintf(buffer, size,
            "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
            time_info.tm_year + 1900,
            time_info.tm_mon + 1,
            time_info.tm_mday,
            time_info.tm_hour,
            time_info.tm_min,
            time_info.tm_sec,
            ts.tv_nsec / 1000000);
}

// получение идентификатора процесса
int get_pid(void) {
    return (int)getpid();
}

// запись сообщения в лог
void write_log(const char* message) {
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    int log_lock = open(LOG_LOCK_FILE, O_CREAT | O_RDWR, 0666);

    if (fd == -1 || log_lock == -1) {
        if (fd != -1) close(fd);
        if (log_lock != -1) close(log_lock);
        return;
    }

    flock(log_lock, LOCK_EX);

    char line[512];
    int length = snprintf(line, sizeof(line), "%s\n", message);

    if (length > 0 && length < (int)sizeof(line)) {
        write(fd, line, (size_t)length);
    }

    flock(log_lock, LOCK_UN);

    close(log_lock);
    close(fd);
}