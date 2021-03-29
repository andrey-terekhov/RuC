#include "memory.h"

void delete_free_block(const int *pointer) {
    int *pointer_first = *((int **) (pointer + 1));
    int *pointer_second = *((int **) (pointer + 3));
    *((int **) (pointer_first + 3)) = pointer_second;
    *((int **) (pointer_second + 1)) = pointer_first;
}

void *my_malloc(size_t size) {
    // Увеличиваем размер с учетом блоков с информацией
    size += 8;
    int *pointer = *((int **) (end_mem + 1));
    while (pointer != end_mem) // Иду по свободным ячейкам, пока не дойду до конца
    {
        int pointer_size = -(*pointer);
        // Если ячейка больше либо равна нужному размеру, то берем ее
        if (pointer_size > 0 && (size_t) pointer_size >= size) {
            int *L;
            size_t K = -(*pointer) - size;
            if (K <= 26) // 1)Если размер ячейки чуть больше нужного
            {
                L = pointer;
                // Помечаем ячейку как занятую
                *L = pointer_size; // Сохранаю информацию о размере в начало
                *(L + pointer_size - 1) = pointer_size; // Сохранаю информацию о размере в конец
                delete_free_block(pointer); //Удаляю ячейку из свободных
            } else  // 2)Если размер ячейки больше нужного нам размера
            {
                *pointer = -K;
                *(pointer + K - 1) = -K;
                L = pointer + K;
                // Помечаем ячейку как занятую
                *L = size; // Сохранаю информацию о размере в начало
                *(L + size - 1) = size; // Сохранаю информацию о размере в конец
            }
            return L + 1;
        }
        // Если размер ячейки не подошел
        pointer = *((int **) (pointer + 1));
    }
    return NULL;
}

void my_free(void *ptr) {
    // Перемащаю указатель на начало
    int zero = 0;
    int *pointer_zero = (int *) ptr - 1;
    int *pointer_left = &zero;
    int *pointer_right = &zero;
    *pointer_zero = -*pointer_zero;
    *(pointer_zero - *pointer_zero - 1) = *pointer_zero;
    if (*(pointer_zero - 1) < 0)  // Свободна ли левая ячейка
    {
        pointer_left = pointer_zero + *(pointer_zero - 1);
    }
    if (*(pointer_zero - *pointer_zero) < 0) // Свободна ли правая ячейка
    {
        pointer_right = pointer_zero - *pointer_zero;
    }
    if (*pointer_left < 0 && *pointer_right == 0) // Если свободна левая, а правая занята
    {
        *(pointer_zero - *pointer_zero - 1) += *pointer_left;
        *pointer_left = *(pointer_zero - *pointer_zero - 1);
        return;
    } else if (*pointer_left == 0 && *pointer_right < 0) // Если свободна правая, а левая занаята
    {
        int *before_right = *((int **) (pointer_right + 3));
        int *after_right = *((int **) (pointer_right + 1));
        //Изменяю размер ячеек
        *(pointer_right - *pointer_right - 1) += *pointer_zero;
        *pointer_zero = *(pointer_right - *pointer_right - 1);
        // Изменяю указатели
        *((int **) (pointer_zero + 1)) = *((int **) (pointer_right + 1));
        *((int **) (pointer_zero + 3)) = *((int **) (pointer_right + 3));
        *((int **) (before_right + 1)) = pointer_zero;
        *((int **) (after_right + 3)) = pointer_zero;
        return;
    } else if (*pointer_left < 0 && *pointer_right < 0) // Если свободна левая и правая
    {
        delete_free_block(pointer_right); //Удаляю правую ячейку из свободных
        // Изменяю размер и указатели
        *pointer_zero += *pointer_right;
        *(pointer_zero - *pointer_zero - 1) = *pointer_left + *pointer_zero;
        *pointer_left = *(pointer_zero - *pointer_zero - 1);
        return;
    } else // Если обе заняты
    {
        int *tmp_AVAIL = *((int **) (end_mem + 1));
        if (end_mem == *((int **) (end_mem + 1))) // Если только 1 свободная ячейка
        {
            *((int **) (end_mem + 1)) = pointer_zero;
            *((int **) (tmp_AVAIL + 3)) = pointer_zero;
            *((int **) (pointer_zero + 1)) = tmp_AVAIL;
            *((int **) (pointer_zero + 3)) = end_mem;
            return;
        } else // Добавляю новую ячейку к свободным
        {
            *((int **) (pointer_zero + 1)) = tmp_AVAIL;
            *((int **) (pointer_zero + 3)) = end_mem;
            *((int **) (tmp_AVAIL + 3)) = pointer_zero;
            *((int **) (end_mem + 1)) = pointer_zero;
            return;
        }
    }
}