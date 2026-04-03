#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#pragma pack(push, 1)  // ВАЖНО: отключаем выравнивание

struct file_header {
    int active;
    int deleted;
    int offset_active;
    int offset_first_deleted;
    int offset_last_deleted;
};

struct file_record {
    char is_deleted;
    char name[20];
    int offset_next;
};

#pragma pack(pop)

int check_file(char filename[]) {
    if (access(filename, F_OK) != 0) {
        return 2;
    }
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return -1;
    }
    struct stat statbuf;
    stat(filename, &statbuf);
    struct file_header Header;
    if (statbuf.st_size == 0) {
        fclose(fp);
        return 2;
    }
    fread(&Header, sizeof(struct file_header), 1, fp);
    int records_in_file = (statbuf.st_size - sizeof(struct file_header)) / sizeof(struct file_record);
    fclose(fp);
    if (Header.active + Header.deleted == records_in_file) return 1;
    else if (statbuf.st_size != 0) return 0;
    return 0;
}

int create_empty_file(char filename[]) {
    struct file_header Header = {0, 0, -1, -1, -1};
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;
    fwrite(&Header, sizeof(Header), 1, fp);
    if (ferror(fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

int insert_record(char filename[], char data[]) {
    // Открываем файл для чтения и записи
    FILE *fp = fopen(filename, "rb+");
    if (!fp) return 0;
    
    // Читаем заголовок
    struct file_header Header;
    fread(&Header, sizeof(struct file_header), 1, fp);
    
    // Проверяем уникальность среди АКТИВНЫХ записей
    int pos = Header.offset_active;
    struct file_record read;
    
    while (pos != -1) {
        fseek(fp, pos, SEEK_SET);
        if (fread(&read, sizeof(struct file_record), 1, fp) != 1) break;
        
        if (read.is_deleted == 0 && strncmp(read.name, data, 20) == 0) {
            fclose(fp);
            return 0;  // Уже существует
        }
        pos = read.offset_next;
    }
    
    // Создаем новую запись
    struct file_record Record;
    Record.is_deleted = 0;
    strcpy(Record.name, data);
    Record.offset_next = Header.offset_active;
    
    // Перемещаемся в конец файла
    fseek(fp, 0, SEEK_END);
    int new_offset = ftell(fp);
    
    // Записываем новую запись
    fwrite(&Record, sizeof(struct file_record), 1, fp);
    
    // Обновляем заголовок
    Header.active++;
    Header.offset_active = new_offset;
    
    // Возвращаемся в начало и перезаписываем заголовок
    fseek(fp, 0, SEEK_SET);
    fwrite(&Header, sizeof(struct file_header), 1, fp);
    
    fclose(fp);
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s [FILENAME] [DATA]\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];
    char *data = argv[2];
    int check_result = check_file(filename);
    
    if (check_result == 0) {
        printf("Bad File!\n");
        return 1;
    } else if (check_result == -1) {
        return 1;
    }
    
    if (check_result == 2) {
        if (!create_empty_file(filename)) {
            printf("Cannot create file\n");
            return 1;
        }
    }
    
    if (!insert_record(filename, data)) {
        printf("%s already in %s, ignoring!\n", data, filename);
        return 1;
    }
    
    printf("Successfully inserted '%s' into %s\n", data, filename);
    return 0;
}
