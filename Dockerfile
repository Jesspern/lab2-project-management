# ===== Builder Stage =====
FROM ubuntu:24.04 AS builder

# Устанавливаем зависимости для сборки
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libssl-dev \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

# Собираем POCO из исходников (версия 1.15.0)
ARG POCO_VERSION=1.15.0
WORKDIR /build
RUN git clone --depth 1 --branch poco-${POCO_VERSION}-release https://github.com/pocoproject/poco.git

WORKDIR /build/poco
RUN mkdir cmake-build && cd cmake-build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DENABLE_DATA=ON \
        -DENABLE_DATA_POSTGRESQL=ON \
        -DENABLE_DATA_MYSQL=OFF \
    && cmake --build . --target install -j$(nproc)

# Собираем приложение
WORKDIR /build/app
COPY CMakeLists.txt ./
COPY src/ ./src/
RUN mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local \
    && cmake --build . -j$(nproc)

# ===== Runner Stage =====
FROM ubuntu:24.04 AS runner

# Устанавливаем минимальные зависимости
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

# Копируем библиотеки POCO из builder
COPY --from=builder /usr/local/lib/libPoco*.so* /usr/local/lib/

# Копируем бинарник приложения
COPY --from=builder /build/app/build/jira /usr/local/bin/jira

# Настраиваем поиск библиотек
ENV LD_LIBRARY_PATH=/usr/local/lib

# Переменные окружения
ENV PORT=8080
ENV LOG_LEVEL=information

# Открываем порт
EXPOSE 8080

# Запускаем сервер
CMD ["jira"]