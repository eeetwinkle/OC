#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    long long counter;
} SharedData;

typedef intptr_t ChildProcess;

// работа с общим счетчиком
int shared_open(SharedData** data);
void shared_lock(void);
void shared_unlock(void);
void shared_close(void);

// выбор главного процесса
int shared_try_become_coordinator(void);
void shared_release_coordinator(void);

// запуск дочерних процессов
void set_program_path(const char* path);
int spawn_copy(ChildProcess* child, const char* mode);
int check_child(ChildProcess* child);

// общие функции
void sleep_ms(unsigned int milliseconds);
long long get_time_ms(void);
int read_char(void);
void get_time(char* buffer, size_t size);
int get_pid(void);
void write_log(const char* message);

#endif