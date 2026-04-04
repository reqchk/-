#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

typedef struct {
    int activeCount;
    int deletedCount;
    int firstActive;
    int firstDeleted;
    int lastDeleted;
} Header;

// Структура записи-элемента
typedef struct {
    char deletedFlag; 
    char name[20];
    int next;
} Record;

// Вывод справки
void printHelp(const char* programName) {
    printf("Программа поиска элементов по началу наименования\n\n");
    printf("Использование:\n");
    printf("  %s <имя_файла> <подстрока>\n\n", programName);
    printf("Параметры:\n");
    printf("  <имя_файла>  - путь к файлу с данными\n");
    printf("  <подстрока>  - подстрока для поиска (ищет элементы,\n");
    printf("                 наименование которых начинается с этой подстроки)\n\n");
    printf("Пример:\n");
    printf("  %s data.dat \"АБВ\"\n", programName);
}

// Чтение заголовка из файла
int readHeader(FILE* file, Header* header) {
    rewind(file);

    if (fread(&header->activeCount, sizeof(int), 1, file) != 1) return 0;
    if (fread(&header->deletedCount, sizeof(int), 1, file) != 1) return 0;
    if (fread(&header->firstActive, sizeof(int), 1, file) != 1) return 0;
    if (fread(&header->firstDeleted, sizeof(int), 1, file) != 1) return 0;
    if (fread(&header->lastDeleted, sizeof(int), 1, file) != 1) return 0;

    return 1;
}

// Чтение записи по смещению
int readRecord(FILE* file, int offset, Record* record) {
    if (offset <= 0) return 0;

    fseek(file, offset, SEEK_SET);

    if (fread(&record->deletedFlag, sizeof(char), 1, file) != 1) return 0;
    if (fread(record->name, sizeof(char), 20, file) != 20) return 0;
    if (fread(&record->next, sizeof(int), 1, file) != 1) return 0;

    return 1;
}

// Поиск элементов, начинающихся с подстроки
void searchByPrefix(const char* filename, const char* prefix) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Ошибка: не удалось открыть файл '%s'\n", filename);
        return;
    }

    Header header;
    if (!readHeader(file, &header)) {
        printf("Ошибка: не удалось прочитать заголовок файла\n");
        fclose(file);
        return;
    }
    // ИСПРАВЛЕНО: проверяем на -1, а не на 0
    if (header.firstActive == -1) {
        printf("Список активных элементов пуст\n");
        fclose(file);
        return;
    }

    printf("\n=== Поиск элементов, начинающихся с \"%s\" ===\n\n", prefix);

    int currentOffset = header.firstActive;
    int orderNumber = 0;
    int foundCount = 0;
    // ИСПРАВЛЕНО: проверяем на -1
    while (currentOffset != -1) {
        Record record;
        if (!readRecord(file, currentOffset, &record)) {
            printf("Ошибка: не удалось прочитать запись по смещению %d\n", currentOffset);
            break;
        }

        // Увеличиваем порядковый номер только для активных элементов
        orderNumber++;

        // Проверяем, начинается ли наименование с подстроки
        if (record.deletedFlag == 0 && strncmp(record.name, prefix, strlen(prefix)) == 0) {
            printf("%d. %s\n", orderNumber, record.name);
            foundCount++;
        }

        currentOffset = record.next;
    }

    if (foundCount == 0) {
        printf("Элементы, начинающиеся с \"%s\", не найдены\n", prefix);
    }
    else {
        printf("\nНайдено элементов: %d\n", foundCount);
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    // Проверка параметров командной строки
    // SetConsoleCP(1251);
    // SetConsoleOutputCP(1251);
    setlocale(LC_ALL,"ru_RU.UTF-8");
    if (argc != 3) {
        printHelp(argv[0]);
        return 1;
    }

    // Проверка на пустую подстроку
    if (strlen(argv[2]) == 0) {
        printf("Ошибка: подстрока не может быть пустой\n");
        printHelp(argv[0]);
        return 1;
    }

    // Выполнение поиска
    searchByPrefix(argv[1], argv[2]);

    return 0;
}
