#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/stat.h"

// исправлено(добавлено)
#pragma pack(push, 1)

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

// исправлено(добавлено)
#pragma pack(pop)

int check_file(char filename[]) {
  if (access(filename, F_OK) != 0) {
    printf("File does not exist\n"); //добавлено
    return 2;
  }
  FILE * fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Cannot open file\n"); //добавлено
    return -1;
  }
  struct stat statbuf;
  stat(filename, &statbuf);
  struct file_header Header;
  if (statbuf.st_size == 0) {
    printf("File is empty\n"); //добавлено
    return 2;
  }
  fread(&Header, sizeof(struct file_header), 1, fp);
  int records_in_file = (statbuf.st_size - sizeof(struct file_header)) / sizeof(struct file_record);
  fclose(fp);
  if (Header.active + Header.deleted == records_in_file) return 1;
  else if (statbuf.st_size != 0) return 0;
}

int create_empty_file(char filename[]) {
  printf("Creating empty file: %s\n", filename); //добавлено
  struct file_header Header = {0, 0, -1, -1, -1};
  FILE * fp = fopen(filename, "wb");
  fwrite(&Header, sizeof(Header), 1, fp);
  if (ferror(fp)) {
    return 0;
  }
  fclose(fp);
  printf("Empty file created successfully\n");//добавлено
  return 1;
}

//исправленная функция вставки
int insert_record(char filename[], char data[]) {
  FILE * fp = fopen(filename, "rb+");
  if (!fp){
    printf("Cannot open file %s\n", filename);//добавлено
    return 0;
  }
  struct file_header Header;
  fread(&Header, sizeof(struct file_header), 1, fp);
  
  // исправлено: проверка уникальности среди активных
  int pos = Header.offset_active;
  struct file_record read;
  while (pos != -1) {
      fseek(fp, pos, SEEK_SET);
      fread(&read, sizeof(struct file_record), 1, fp);
      if (read.is_deleted == 0 && strncmp(read.name, data, 20) == 0) {
          fclose(fp);
          return 0;  // Уже существует
      }
      pos = read.offset_next;
  }

  // добавлена проверка наличия удаленных
  struct file_record Record;
  Record.is_deleted = 0;
  strncpy(Record.name, data, 19);  // исправлено
  Record.name[19] = '\0';          // исправлено
  Record.offset_next = Header.offset_active;

  int new_offset;
    
  if (Header.offset_first_deleted != -1) {
      // если есть удаленные то используем место первого удаленного
      new_offset = Header.offset_first_deleted;
        
      // читаем первый удаленный элемент
      fseek(fp, new_offset, SEEK_SET);
      struct file_record deleted_rec;
      fread(&deleted_rec, sizeof(struct file_record), 1, fp);
      
      // обновляем заголовок списка удаленных
      Header.offset_first_deleted = deleted_rec.offset_next;
      Header.deleted--;
        
      // если список удаленных стал пуст
      if (Header.offset_first_deleted == -1) {
          Header.offset_last_deleted = -1;
      }
        
      // записываем новую запись на место удаленной
      fseek(fp, new_offset, SEEK_SET);
      fwrite(&Record, sizeof(struct file_record), 1, fp);
      printf("New record written at offset %d (over the deleted record)\n", new_offset);
        
  } else {
      // еасли нет удаленных то добавляем в конец файла
      fseek(fp, 0, SEEK_END);
      new_offset = ftell(fp);
      fwrite(&Record, sizeof(struct file_record), 1, fp);
      printf("New record appended to end of file\n");
  }
    
  // обновляем заголовок, добавляем в начало активного списка 
  Header.active++;
  Header.offset_active = new_offset;
    
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
    perror("Bad File!\n");
    return 1;
  }
  else if (check_result == -1) {
    return 1;
  }
  
  if (check_result == 2) {
    create_empty_file(filename);
  }
  if (!insert_record(filename, data)) {
    printf("%s already in %s, ignoring!\n", data, filename);
    return 1;
  }
  
  return 0;
}
