# 🧪 Тесты и демонстрационные скрипты

## Структура папки

```
tests/
├── images/           # Тестовые изображения
│   ├── img1.jpg     # Основные тестовые изображения  
│   ├── img2.jpg
│   ├── test_640_*.jpg      # 640x480 тестовые изображения
│   ├── test_real_*.jpg     # Изображения с текстурами
│   └── test_*.png/.jpg     # Различные тестовые файлы
├── benchmark_dc.sh         # Тест производительности DC-only режима
├── benchmark_filesize.sh   # Тест производительности file-size режима
├── create_test_images.sh   # Создание тестовых изображений
├── example_pipeline.sh     # Демонстрация 3-ступенчатого конвейера
└── test_motion.sh         # Базовые тесты детекции движения
```

## Запуск тестов

### Из корневой папки проекта:
```bash
cd tests
./test_motion.sh          # Основные тесты
./benchmark_filesize.sh   # Тест производительности
./example_pipeline.sh     # Умный конвейер
```

### Прямое тестирование:
```bash
# Из папки tests/
../motion-detector images/img1.jpg images/img2.jpg -v

# Тест DC-only совместимости  
../motion-detector images/test_640_1.jpg images/test_640_2.jpg --dc-strict -v

# Тест с оптимальными параметрами для 640x480
../motion-detector images/test_640_1.jpg images/test_640_2.jpg -s 2 -m 0.5 -g -d -v
```

## Создание собственных тестов

### Добавление новых изображений:
1. Поместите изображения в папку `images/`
2. Тестируйте: `../motion-detector images/ваш_файл1.jpg images/ваш_файл2.jpg -v`

### Тест DC-only совместимости:
```bash
../motion-detector images/ваш_файл1.jpg images/ваш_файл2.jpg --dc-strict -v
```

## Ожидаемые результаты

- **Идентичные изображения**: 0-5% движения
- **Разные изображения**: 10-90% движения  
- **Время обработки 640x480**: ~1-3ms
- **DC-only ускорение**: 2-3x когда совместимо 