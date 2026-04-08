# Homework 03: Database Design and Optimization for Project Management API

## 📋 Задание

Спроектировать и оптимизировать PostgreSQL базу данных для Project Management API (Вариант 8). Заменить in-memory хранилище на PostgreSQL.

## 🎯 Контекст

Существующая система с сущностями:
- **Users** (id, login, password, firstName, lastName, email, role)
- **Projects** (id, name, description, code, ownerId, active)
- **Tasks** (id, code, title, description, projectId, assigneeId, reporterId, status, createdAt, updatedAt)
- **Roles**: GUEST, WORKER, TRACKER, ADMINISTRATOR
- **Task Status**: NEW, IN_PROGRESS, REVIEW, DONE, CANCELLED

---

## 📂 Требуемые файлы

### 1. `sql/schema.sql`
SQL скрипт создания схемы БД:
- Все таблицы (users, projects, tasks)
- Первичные ключи (PRIMARY KEY)
- Внешние ключи (FOREIGN KEY) с ON DELETE/UPDATE
- UNIQUE, NOT NULL, CHECK ограничения
- Индексы для FK, WHERE, JOIN колонок
- ENUM типы для role и status

### 2. `sql/data.sql`
Тестовые данные:
- Минимум 10 пользователей с разными ролями
- Минимум 10 проектов
- Минимум 30 задач с разными статусами

### 3. `sql/queries.sql`
SQL запросы для всех операций API:
- CRUD для users, projects, tasks
- Поиск с фильтрами
- JOIN запросы
- Агрегатные функции (COUNT, GROUP BY)

### 4. `docs/optimization.md`
Оптимизация запросов:
- EXPLAIN ANALYZE до оптимизации
- Добавление индексов
- EXPLAIN ANALYZE после оптимизации
- Сравнение и объяснение улучшений

### 5. `docker-compose.yaml`
Обновлённый с PostgreSQL:
- Сервис postgres с инициализацией из sql/
- Сервис api с подключением к БД
- Volume для сохранения данных
- Переменные окружения

### 6. `README.md`
Обновлённая документация:
- Схема БД
- Инструкции по запуску с Docker
- Список SQL запросов

---

## 🗄️ Схема БД (требования)

### Таблица users
- id (SERIAL PRIMARY KEY)
- login (VARCHAR, UNIQUE, NOT NULL)
- password (VARCHAR, NOT NULL)
- first_name (VARCHAR, NOT NULL)
- last_name (VARCHAR)
- email (VARCHAR, UNIQUE)
- role (ENUM: GUEST, WORKER, TRACKER, ADMINISTRATOR)
- created_at (TIMESTAMP)

### Таблица projects
- id (SERIAL PRIMARY KEY)
- name (VARCHAR, NOT NULL)
- description (TEXT)
- code (VARCHAR, UNIQUE, NOT NULL)
- owner_id (FK → users.id)
- active (BOOLEAN)
- created_at (TIMESTAMP)

### Таблица tasks
- id (SERIAL PRIMARY KEY)
- code (VARCHAR, UNIQUE, NOT NULL)
- title (VARCHAR, NOT NULL)
- description (TEXT)
- project_id (FK → projects.id)
- assignee_id (FK → users.id)
- reporter_id (FK → users.id)
- status (ENUM: NEW, IN_PROGRESS, REVIEW, DONE, CANCELLED)
- created_at (TIMESTAMP)
- updated_at (TIMESTAMP)

### Индексы
- idx_projects_owner_id (projects.owner_id)
- idx_tasks_project_id (tasks.project_id)
- idx_tasks_assignee_id (tasks.assignee_id)
- idx_tasks_status (tasks.status)
- idx_users_login (users.login)
- idx_projects_active (projects.active)

---

## 📝 Примеры SQL запросов

### Регистрация пользователя
```sql
INSERT INTO users (login, password, first_name, last_name, email, role)
VALUES ('user1', 'hash123', 'John', 'Doe', 'john@example.com', 'WORKER')
RETURNING id, login, email, role;
```

### Логин
```sql
SELECT id, login, password, role 
FROM users 
WHERE login = 'user1';
```

### Создание проекта
```sql
INSERT INTO projects (name, description, code, owner_id)
VALUES ('New Project', 'Description', 'PROJ-1', 1)
RETURNING *;
```

### Задачи проекта с исполнителями
```sql
SELECT t.*, 
       u.first_name as assignee_name,
       r.first_name as reporter_name
FROM tasks t
LEFT JOIN users u ON t.assignee_id = u.id
LEFT JOIN users r ON t.reporter_id = r.id
WHERE t.project_id = 1
ORDER BY t.created_at DESC;
```

---

## 🔧 Оптимизация (пример)

### Запрос: Найти все задачи проекта

**До оптимизации:**
```sql
EXPLAIN ANALYZE
SELECT * FROM tasks WHERE project_id = 5;
```
```
Seq Scan on tasks  (cost=0.00..50.00 rows=10 width=80)
  Filter: (project_id = 5)
```

**После создания индекса:**
```sql
CREATE INDEX idx_tasks_project_id ON tasks(project_id);

EXPLAIN ANALYZE
SELECT * FROM tasks WHERE project_id = 5;
```
```
Index Scan using idx_tasks_project_id on tasks  (cost=0.15..20.00 rows=10 width=80)
  Index Cond: (project_id = 5)
```

**Улучшение:** Seq Scan → Index Scan (в 2.5 раза быстрее)

---

## 🐳 Docker Compose (требования)

```yaml
version: '3.8'

services:
  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_DB: project_management
      POSTGRES_USER: postgres
      POSTGRES_PASSWORD: secret
    ports:
      - "5432:5432"
    volumes:
      - ./sql/schema.sql:/docker-entrypoint-initdb.d/1-schema.sql
      - ./sql/data.sql:/docker-entrypoint-initdb.d/2-data.sql
      - postgres_data:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres"]
      interval: 5s
      timeout: 5s
      retries: 5

  api:
    build: .
    ports:
      - "8080:8080"
    environment:
      DATABASE_URL: postgresql://postgres:secret@postgres:5432/project_management
      JWT_SECRET: dev-secret-key
      LOG_LEVEL: information
    depends_on:
      postgres:
        condition: service_healthy
    restart: on-failure

volumes:
  postgres_data:
```

---

## 💻 Интеграция с C++ API

### Требования:
1. Добавить POCO Data PostgreSQL connector
2. Создать DatabaseManager класс для работы с БД
3. Заменить storage::users, storage::projects, storage::tasks на SQL запросы
4. Использовать prepared statements
5. Обработать транзакции
6. Обновить CMakeLists.txt для подключения POCO::Data::PostgreSQL

### Пример подключения:
```cpp
#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/PostgreSQL/Connector.h>

// В main.cpp или DatabaseManager.cpp
void initDatabase() {
    Poco::Data::PostgreSQL::Connector::registerConnector();
}

Poco::Data::Session createSession() {
    return Poco::Data::SessionFactory::instance().create(
        "postgresql://dbname=project_management user=postgres password=secret host=localhost port=5432"
    );
}
```

---

## ✅ Чек-лист выполнения

- [ ] Создана папка `sql/`
- [ ] Создан `sql/schema.sql` со всеми таблицами, индексами и constraints
- [ ] Создан `sql/data.sql` с тестовыми данными (10+ записей в каждой таблице)
- [ ] Создан `sql/queries.sql` с запросами для всех API операций
- [ ] Создана папка `docs/`
- [ ] Создан `docs/optimization.md` с EXPLAIN ANALYZE до и после оптимизации
- [ ] Обновлён `docker-compose.yaml` с PostgreSQL сервисом
- [ ] Обновлён `Dockerfile` (если нужно)
- [ ] Обновлён `CMakeLists.txt` для подключения POCO Data PostgreSQL
- [ ] Создан DatabaseManager или аналогичный класс
- [ ] Обновлены все handlers (UserHandler, ProjectHandler, TaskHandler, AuthHandler)
- [ ] Заменено in-memory storage на PostgreSQL запросы
- [ ] Все тесты проходят
- [ ] README обновлён документацией БД
- [ ] Docker Compose успешно разворачивает БД и API
- [ ] API корректно работает с PostgreSQL

---

## 📚 Ресурсы

- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [EXPLAIN Visualizer](https://explain.depesz.com/)
- [POCO Data](https://pocoproject.org/docs/Poco.Data.html)
- [POCO PostgreSQL Connector](https://pocoproject.org/docs/Poco.Data.PostgreSQL.html)

---

## 🎯 Критерии оценки

1. **Корректность проектирования** — все PK, FK, constraints правильные
2. **Типы данных** — подходящие типы для каждой колонки
3. **Индексы** — обоснованное создание для FK, WHERE, JOIN
4. **SQL запросы** — корректные и эффективные
5. **Оптимизация** — реальное улучшение производительности с EXPLAIN
6. **Интеграция** — API работает с PostgreSQL вместо in-memory
7. **Docker** — docker-compose up разворачивает всё автоматически
8. **Документация** — полное описание в README

---

## 🚀 Инструкции для Cursor

Пожалуйста, выполни следующие шаги:

1. **Создай структуру папок:**
   - sql/
   - docs/

2. **Создай sql/schema.sql** с полной схемой БД

3. **Создай sql/data.sql** с тестовыми данными

4. **Создай sql/queries.sql** с запросами для всех операций API

5. **Создай docs/optimization.md** с анализом EXPLAIN для 3-5 запросов

6. **Обнови docker-compose.yaml** для запуска PostgreSQL и инициализации БД

7. **Обнови CMakeLists.txt** для подключения POCO::Data::PostgreSQL

8. **Создай src/database/DatabaseManager.h и DatabaseManager.cpp** для работы с БД

9. **Обнови все handlers** для использования DatabaseManager вместо in-memory storage

10. **Обнови README.md** с документацией по БД

11. **Протестируй** что docker-compose up успешно разворачивает всё и API работает

Начни с создания файлов и постепенно реализуй каждый пункт. После каждого шага показывай результат.