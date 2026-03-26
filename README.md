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

## Сборка

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

##Docker

```
docker build -t lab2-jira .
docker run -p 8080:8080 -e JWT_SECRET=your-secret-key lab2-jira
```
Или через docker-compose
```
docker compose up --build
```

##Тестирование

```
.\tests\test_api.ps1
```
