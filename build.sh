# Останавливаем выполнение при ошибке
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

cd "$PROJECT_DIR"

echo "Обновление исходников из Git..."

# Получаем последние изменения из GitHub
git pull --ff-only origin main

echo "Настройка проекта..."

# Определяем операционную систему
if [ "$(uname)" = "Darwin" ]; then
    C_COMPILER="/opt/homebrew/bin/gcc-16"

    cmake -S . -B build \
        -DCMAKE_C_COMPILER="$C_COMPILER" \
        -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"
else
    C_COMPILER="gcc"

    cmake -S . -B build \
        -DCMAKE_C_COMPILER="$C_COMPILER"
fi

echo "Компиляция проекта..."

# Собираем программу
cmake --build build

echo "Сборка успешно завершена!"