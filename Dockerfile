# ===== Builder Stage =====
FROM ubuntu:24.04 AS builder

# Устанавливаем зависимости для сборки
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    ca-certificates \
    libssl-dev \
    libsasl2-dev \
    zlib1g-dev \
    libzstd-dev \
    libsnappy-dev \
    libicu-dev \
    libpq-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Собираем POCO из исходников (версия 1.15.0)
ARG POCO_VERSION=1.15.0
ARG MONGO_C_DRIVER_VERSION=1.29.1
ARG MONGO_CXX_DRIVER_VERSION=r3.10.2
WORKDIR /build
RUN git clone --depth 1 --branch poco-${POCO_VERSION}-release https://github.com/pocoproject/poco.git

WORKDIR /build/poco
RUN mkdir cmake-build && cd cmake-build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DENABLE_DATA=ON \
        -DENABLE_DATA_POSTGRESQL=ON \
        -DENABLE_DATA_MYSQL=OFF \
    && cmake --build . --target install -j$(nproc)

# Собираем Mongo C driver
WORKDIR /build
RUN git clone --depth 1 --branch ${MONGO_C_DRIVER_VERSION} https://github.com/mongodb/mongo-c-driver.git
WORKDIR /build/mongo-c-driver
RUN mkdir cmake-build && cd cmake-build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
        -DENABLE_EXAMPLES=OFF \
        -DENABLE_TESTS=OFF \
    && cmake --build . --target install -j$(nproc)

# Собираем Mongo C++ driver
WORKDIR /build
RUN git clone --depth 1 --branch ${MONGO_CXX_DRIVER_VERSION} https://github.com/mongodb/mongo-cxx-driver.git
WORKDIR /build/mongo-cxx-driver/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
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
    libsasl2-2 \
    libsnappy1v5 \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

# Копируем библиотеки POCO из builder
COPY --from=builder /usr/local/lib/libPoco*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libmongoc*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbson*.so* /usr/local/lib/

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