#ifndef PROCESS_H
#define PROCESS_H

typedef struct Process Process;

/* Запускает дочерний процесс */
int process_start(Process** process, const char* program, char* const args[]);

/* Ожидает завершения процесса и получает код возврата */
int process_wait(Process* process, int* exit_code);

/* Освобождает ресурсы процесса */
void process_destroy(Process* process);

#endif