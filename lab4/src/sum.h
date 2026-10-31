#ifndef SUM_H
#define SUM_H

/* Описание отрезка массива, сумму которого считает один поток. */
struct SumArgs {
  const int *array;
  unsigned int begin; /* включительно */
  unsigned int end;   /* не включительно */
};

/* Сумма array[begin .. end-1].
   Тип long long, потому что rand() даёт числа до 2^31-1 и сумма
   нескольких миллионов таких чисел не помещается в int. */
long long Sum(const struct SumArgs *args);

#endif
