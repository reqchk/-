#include <cstdio>
#include <cstring>
#include <iostream>

using std::cout;
using std::endl;

constexpr int NAME_SIZE = 20;
constexpr int EMPTY_PTR = -1;

// Упаковываем структуры, чтобы гарантировать фиксированный размер и отсутствие паддинга между полями
#pragma pack(push, 1)

// Структура заголовка файла
struct Header {
	int activeCount;
	int deletedCount;
	int firstActive;
	int firstDeleted;
	int lastDeleted;
};

// Структура записи
struct Record {
	unsigned char deleted;
	char name[NAME_SIZE];
	int next;
};
// Восстанавливаем стандартное выравнивание
#pragma pack(pop)

// Вычисляем размеры структур для удобства работы с файлами
constexpr int HEADER_SIZE = sizeof(Header);
// Размер каждой записи в байтах
constexpr int RECORD_SIZE = sizeof(Record);

// Вычисляем смещение записи по её индексу (0-based)
int recordOffsetByIndex(int index) {
	return HEADER_SIZE + index * RECORD_SIZE;
}

// Удобная функция для создания записи
Record makeRecord(unsigned char deleted, const char* name, int next) {
	Record r{};
	r.deleted = deleted;
	std::strncpy(r.name, name, NAME_SIZE - 1);
	r.name[NAME_SIZE - 1] = '\0';
	r.next = next;
	return r;
}

// Функции для чтения и записи заголовка и записей в файл
bool readHeader(FILE* file, Header& header) {
	if (std::fseek(file, 0, SEEK_SET) != 0) {
		return false;
	}
	return std::fread(&header, sizeof(Header), 1, file) == 1;
}

// Чтение записи по смещению
bool writeHeader(FILE* file, const Header& header) {
	if (std::fseek(file, 0, SEEK_SET) != 0) {
		return false;
	}
	return std::fwrite(&header, sizeof(Header), 1, file) == 1;
}

// Чтение записи по смещению
bool readRecordAt(FILE* file, int offset, Record& record) {
	if (std::fseek(file, offset, SEEK_SET) != 0) {
		return false;
	}
	return std::fread(&record, sizeof(Record), 1, file) == 1;
}

// Запись записи по смещению
bool writeRecordAt(FILE* file, int offset, const Record& record) {
	if (std::fseek(file, offset, SEEK_SET) != 0) {
		return false;
	}
	return std::fwrite(&record, sizeof(Record), 1, file) == 1;
}

// Функция для генерации тестового файла с предопределёнными данными
int generateTestFile(const char* filename) {
	FILE* file = std::fopen(filename, "wb");
	if (!file) {
		return -1;
	}

	const int r0 = recordOffsetByIndex(0);
	const int r1 = recordOffsetByIndex(1);
	const int r2 = recordOffsetByIndex(2);
	const int r3 = recordOffsetByIndex(3);
	const int r4 = recordOffsetByIndex(4);

	Header header{};
	header.activeCount = 3;
	header.deletedCount = 2;
	header.firstActive = r0;
	header.firstDeleted = r3;
	header.lastDeleted = r4;

	const Record records[] = {
			makeRecord(0, "Alpha", r1),
			makeRecord(0, "Beta", r2),
			makeRecord(0, "Gamma", EMPTY_PTR),
			makeRecord(1, "DeletedOld1", r4),
			makeRecord(1, "DeletedOld2", EMPTY_PTR)};

	const bool ok =
			std::fwrite(&header, sizeof(Header), 1, file) == 1 &&
			std::fwrite(records, sizeof(Record), 5, file) == 5;

	std::fclose(file);
	return ok ? 1 : -1;
}

// Функция для сравнения имён с учётом ограниченного размера
bool namesEqual(const char* lhs, const char* rhs) {
	return std::strncmp(lhs, rhs, NAME_SIZE) == 0;
}

// Функция для удаления записи по имени. Возвращает 1 при успешном удалении, 0 если имя не найдено, и -1 при ошибке.
int removeByName(const char* filename, const char* name) {
	FILE* file = std::fopen(filename, "rb+");
	if (!file) {
		return -1;
	}

	Header header{};
	if (!readHeader(file, header)) {
		std::fclose(file);
		return -1;
	}

	int prevOffset = EMPTY_PTR;
	int curOffset = header.firstActive;

	while (curOffset != EMPTY_PTR) {
		Record cur{};
		if (!readRecordAt(file, curOffset, cur)) {
			std::fclose(file);
			return -1;
		}

		if (cur.deleted == 0 && namesEqual(cur.name, name)) {
			if (prevOffset == EMPTY_PTR) {
				header.firstActive = cur.next;
			} else {
				Record prev{};
				if (!readRecordAt(file, prevOffset, prev)) {
					std::fclose(file);
					return -1;
				}
				prev.next = cur.next;
				if (!writeRecordAt(file, prevOffset, prev)) {
					std::fclose(file);
					return -1;
				}
			}

			cur.deleted = 1;
			cur.next = EMPTY_PTR;
			if (!writeRecordAt(file, curOffset, cur)) {
				std::fclose(file);
				return -1;
			}

			if (header.deletedCount == 0) {
				header.firstDeleted = curOffset;
				header.lastDeleted = curOffset;
			} else {
				Record lastDeleted{};
				if (!readRecordAt(file, header.lastDeleted, lastDeleted)) {
					std::fclose(file);
					return -1;
				}
				lastDeleted.next = curOffset;
				if (!writeRecordAt(file, header.lastDeleted, lastDeleted)) {
					std::fclose(file);
					return -1;
				}
				header.lastDeleted = curOffset;
			}

			header.activeCount--;
			header.deletedCount++;

			if (!writeHeader(file, header)) {
				std::fclose(file);
				return -1;
			}

			std::fflush(file);
			std::fclose(file);
			return 1;
		}

		prevOffset = curOffset;
		curOffset = cur.next;
	}

	std::fclose(file);
	return 0;
}

// Вспомогательная функция для печати списка записей, начиная с заданного смещения
void printList(FILE* file, int firstOffset) {
	int ptr = firstOffset;
	while (ptr != EMPTY_PTR) {
		Record r{};
		if (!readRecordAt(file, ptr, r)) {
			cout << "Broken pointer " << ptr << endl;
			return;
		}
		cout << "{" << r.name << ", off=" << ptr << ", del=" << static_cast<int>(r.deleted)
			 << ", next=" << r.next << "} ";
		ptr = r.next;
	}
}

// Функция для печати текущего состояния файла: заголовок, активные записи и удалённые запис
void printState(const char* filename) {
	FILE* file = std::fopen(filename, "rb");
	if (!file) {
		cout << "Failed to open file: " << filename << endl;
		return;
	}

	Header h{};
	if (!readHeader(file, h)) {
		cout << "Failed to read header" << endl;
		std::fclose(file);
		return;
	}

	cout << "Header: active=" << h.activeCount
		 << ", deleted=" << h.deletedCount
		 << ", firstActive=" << h.firstActive
		 << ", firstDeleted=" << h.firstDeleted
		 << ", lastDeleted=" << h.lastDeleted << endl;

	cout << "Active list: ";
	printList(file, h.firstActive);
	cout << endl;

	cout << "\nDeleted queue:" << endl;
	printList(file, h.firstDeleted);
	cout << endl;
	std::fclose(file);
}

int main(int argc, char* argv[]) {
	// generateTestFile("test.dat");

	if (argc < 3) {
		cout << "Invalid number of arguments. Provide a filename and at least one name to delete." << endl;
		return 1;
	}

	const char* filename = argv[1];

	cout << "State before deletion:" << endl;
	printState(filename);

	bool hadErrors = false;

	for (int i = 2; i < argc; ++i) {
		const char* nameToRemove = argv[i];
		const int result = removeByName(filename, nameToRemove);

		if (result == 1) {
			cout << "Deleted name: " << nameToRemove << endl;
		} else if (result == 0) {
			cout << "Name not found: " << nameToRemove << endl;
		} else {
			cout << "Error deleting name: " << nameToRemove << endl;
			hadErrors = true;
		}
	}

	cout << "\nState after deletion:" << endl;
	printState(filename);

	return hadErrors ? 1 : 0;
}

