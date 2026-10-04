#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  printf("Parent process started. Forking...\n");

  pid_t pid = fork();

  if (pid == -1) {
    // Ошибка при создании процесса
    perror("Fork failed");
    return 1;
  } 
  else if (pid == 0) {
    // МЫ В ДОЧЕРНЕМ ПРОЦЕССЕ

    // Подготавливаем аргументы для новой программы.
    // По правилам, первый аргумент (args[0]) — это имя самой программы.
    // Последний аргумент обязательно должен быть NULL.
    char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};

    // Вызываем execv. 
    // Первый параметр — путь к программе, второй — массив аргументов.
    execv(args[0], args);

    // ВАЖНО: Если execv отработал успешно, код ниже НИКОГДА не выполнится,
    // потому что процесс заменится на sequential_min_max.
    // Если мы дошли до этой строчки, значит произошла ошибка (например, файл не скомпилирован).
    perror("execv failed"); 
    return 1;
  } 
  else {
    // МЫ В РОДИТЕЛЬСКОМ ПРОЦЕССЕ
    
    int status;
    // Ждем, пока дочерний процесс (наш запущенный sequential_min_max) завершит работу
    wait(&status); 
    printf("Child process finished. Parent exiting.\n");
  }

  return 0;
}