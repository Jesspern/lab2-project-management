# Project Management API (MongoDB)

REST сервер на C++ с использованием POCO + MongoDB C++ Driver.

## Документная модель

Реализована hybrid-модель:
- `users` — отдельная коллекция;
- `projects` — отдельная коллекция;
- `tasks` — отдельная коллекция;
- `comments` и `history` — embedded массивы внутри `tasks`.

Подробный дизайн: `schema_design.md`.

## MongoDB файлы задания

- `schema_design.md` — описание модели и обоснование
- `data.js` — тестовые данные (10+ users, 10+ projects, 30 tasks)
- `queries.js` — CRUD + операторы `$eq`, `$ne`, `$gt`, `$lt`, `$in`, `$and`, `$or`, `$regex`, `$push`, `$pull`, `$addToSet`
- `validation.js` — `$jsonSchema` валидация для `users` и `tasks`

## Переменные окружения

- `PORT` — порт API (по умолчанию `8080`)
- `JWT_SECRET` — секрет JWT
- `LOG_LEVEL` — уровень логирования
- `MONGODB_URI` — строка подключения MongoDB
- `MONGODB_DATABASE` — имя БД (по умолчанию `project_management`)

## Запуск через Docker

```bash
docker compose up --build
```

Сервисы:
- `mongodb` — MongoDB 7 c init-скриптами (`validation.js`, `data.js`)
- `api` — C++ API, подключается к MongoDB

Проверка:

```bash
curl http://localhost:8080/api/health
```

## Тесты API

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_api.ps1
```

## Endpoints

- `POST /api/users`
- `POST /api/auth`
- `GET /api/users/login/{login}`
- `GET /api/users/search?firstName=&lastName=`
- `DELETE /api/users/{id}`
- `POST /api/projects`
- `GET /api/projects`
- `GET /api/projects/search/{name}`
- `POST /api/projects/{projectId}/tasks`
- `GET /api/projects/{projectId}/tasks`
- `GET /api/tasks/{taskCode}`
