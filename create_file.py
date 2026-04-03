import struct
import os

# Константы
EMPTY_PTR = -1
ACTIVE_BIT = 0
NAME_SIZE = 20

def create_list_file(filename, words):
    """
    Создаёт бинарный файл с заданной структурой
    
    Структура заголовка (20 байт):
    - active_count: 4 байта (int32)
    - deleted_count: 4 байта (int32)
    - first_active_ptr: 4 байта (int32)
    - first_deleted_ptr: 4 байта (int32)
    - last_deleted_ptr: 4 байта (int32)
    
    Структура записи (25 байт):
    - deleted_bit: 1 байт (unsigned char)
    - name: 20 байт (string, null-terminated)
    - next_ptr: 4 байта (int32)
    """
    
    with open(filename, 'wb') as f:
        # Временно пишем заглушку для заголовка (потом перезапишем)
        f.write(b'\x00' * 20)
        
        # Позиции записей
        positions = []
        
        # Создаём записи для каждого слова
        for word in words:
            positions.append(f.tell())
            
            # 1. deleted_bit (активен)
            f.write(struct.pack('B', ACTIVE_BIT))
            
            # 2. name (20 байт, дополняем нулями)
            name_bytes = word.encode('ascii')
            f.write(name_bytes)
            f.write(b'\x00' * (NAME_SIZE - len(name_bytes)))
            
            # 3. next_ptr (пока временно пишем 0)
            f.write(struct.pack('<i', 0))
        
        # Заполняем указатели next_ptr
        for i, pos in enumerate(positions):
            if i < len(positions) - 1:
                next_ptr = positions[i + 1]
            else:
                next_ptr = EMPTY_PTR
            
            # Записываем next_ptr по нужному смещению
            f.seek(pos + 1 + NAME_SIZE)  # пропускаем deleted_bit и name
            f.write(struct.pack('<i', next_ptr))
        
        # Заполняем заголовок
        f.seek(0)
        header = struct.pack('<iiiii', 
            len(words),        # active_count
            0,                 # deleted_count
            positions[0],      # first_active_ptr
            EMPTY_PTR,         # first_deleted_ptr
            EMPTY_PTR          # last_deleted_ptr
        )
        f.write(header)
    
    print(f"Файл '{filename}' успешно создан!")
    print(f"Размер файла: {os.path.getsize(filename)} байт")
    print(f"Добавлено слов: {len(words)}")
    for i, (word, pos) in enumerate(zip(words, positions)):
        print(f"  {i+1}. '{word}' - позиция: {pos}")


# Создаём файл с нужными словами
words = ['nstu', 'house', 'world', 'hello', 'cat']
create_list_file('list.dat', words)
