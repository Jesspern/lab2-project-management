# Project Management API

REST сервер на C++ с использованием библиотеки POCO.

📖 [Расширенная документация](docs/README.md) — структура проекта, POCO, DTO, JWT, ролевая модель, тестирование.

## Endpoints

- `POST /api/users` — регистрация пользователя
- `POST /api/auth` — логин (получение JWT токена)
- `GET /api/users/login/{login}` — поиск по логину
- `GET /api/users/search?firstName=&lastName=` — поиск по имени (требует токен)
- `POST /api/projects` — создание проекта (требует TRACKER/ADMIN)
- `GET /api/projects` — список всех проектов
- `GET /api/projects/search/{name}` — поиск проекта по имени
- `POST /api/projects/{projectId}/tasks` — создание задачи
- `GET /api/projects/{projectId}/tasks` — список задач проекта

## Переменные окружения

- `PORT` — порт сервера (по умолчанию: 8080)
- `JWT_SECRET` — секрет для подписи JWT токенов (по умолчанию: dev-secret-key)
- `LOG_LEVEL` — уровень логирования: trace, debug, information, notice, warning, error, critical, fatal, none
- `DATABASE_DSN` — строка подключения POCO Data к PostgreSQL (формат `host=... port=... dbname=... user=... password=...`); см. `docker-compose.yml`

---

## Сборка (CMake + Conan)

Из корня `lab2`:

```bash
conan install . -of=build --build=missing -s build_type=Release -s compiler.cppstd=17
cmake --preset conan-default
cmake --build build --config Release
```

`CMakeUserPresets.json` подключает `build/generators/conan_toolchain.cmake` после `conan install`.

В `CMakeLists.txt` линкуется `Poco::DataPostgreSQL` (коннектор PostgreSQL для POCO Data).

### Windows (MSVC)

Пакет Conan `libpq` при отсутствии готового бинарника собирает исходники PostgreSQL и генерирует проекты под **набор инструментов v143 (Visual Studio 2022)**. Если у вас только VS 2025/2026 (v145) без v143, установите в Visual Studio Installer компонент **«MSVC v143 — VS 2022 C++ x64/x86 build tools»** (или полный workload «Разработка классических приложений на C++» для VS 2022), затем повторите `conan install`.

Альтернатива: сборка в **WSL2 (Ubuntu)** или только через **Docker** (см. ниже).

### Конфигурации MSVC и RelWithDebInfo

Для много конфигурационных генераторов (Visual Studio) в `CMakeLists.txt` задано сопоставление `RelWithDebInfo`/`MinSizeRel` к библиотекам Release из Conan; предпочтительно собирать с **`--config Release`**.

## PostgreSQL schema

- SQL схема: `sql/schema.sql`
- Тестовые данные: `sql/data.sql`
- Набор API-запросов: `sql/queries.sql`
- Оптимизация: `docs/optimization.md`

Основные сущности:

- `users` (роль через ENUM `user_role`)
- `projects`
- `tasks` (статус через ENUM `task_status`)

## Запуск через Docker

Убедитесь, что **Docker Desktop** запущен и демон доступен.

Сборка только образа API (без БД в контейнере; нужен доступный PostgreSQL и `DATABASE_DSN`):

```bash
docker build -t lab2-jira .
```

Полный стек (API + PostgreSQL + init SQL):

```bash
docker compose up --build
```

Проверка API с хоста:

```bash
curl http://localhost:8080/api/health
```

Сервисы:

- `postgres` — PostgreSQL 15 с автоинициализацией схемы и тестовых данных
- `api` — C++ API, подключается к БД через `DATABASE_DSN`

## Автоматические тесты (PowerShell)

Скрипт `tests/test_api.ps1` проверяет основные сценарии API. Запуск (при работающем сервере на `localhost:8080`):

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_api.ps1
```

Пароли в сценарии совпадают с пользователями из `sql/data.sql` (`worker1` / `worker123`, `tracker1` / `tracker123`). Если регистрация возвращает 409, тесты продолжают работу с уже существующими учётными записями из seed.

## SQL для API операций

См. `sql/queries.sql`, где собраны:

- CRUD для users/projects/tasks
- Поиск с фильтрами
- JOIN-запросы
- Агрегации `COUNT` + `GROUP BY`
