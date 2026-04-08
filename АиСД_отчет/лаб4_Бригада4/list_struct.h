#ifndef LIST_STRUCT_H
#define LIST_STRUCT_H

#pragma pack(push, 1)

// Заголовочная запись (20 байт: 5 чисел по 4 байта)
typedef struct {
    int activeCount;      // количество активных элементов
    int deletedCount;     // количество удалённых
    int firstActive;      // смещение на первый активный элемент
    int firstDeleted;     // смещение на первый удалённый элемент
    int lastDeleted;      // смещение на последний удалённый элемент
} Header;

// Запись-элемент (25 байт: 1 байт + 20 байт + 4 байта)
typedef struct {
    unsigned char deletedBit;  // 0 - активен, 1 - удалён
    char name[20];              // наименование (строка, 20 байт)
    int next;                   // указатель на следующий элемент (смещение)
} Record;

#pragma pack(pop)

#endif