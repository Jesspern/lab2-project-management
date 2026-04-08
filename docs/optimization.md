# Query Optimization Report

Ниже приведены примеры оптимизации для PostgreSQL запросов Project Management API.

## 1) Получение задач проекта

Запрос:

```sql
SELECT * FROM tasks WHERE project_id = 5 ORDER BY created_at DESC;
```

До индекса:

```sql
EXPLAIN ANALYZE SELECT * FROM tasks WHERE project_id = 5 ORDER BY created_at DESC;
-- Seq Scan on tasks
-- Filter: (project_id = 5)
-- Execution Time: 1.97 ms
```

После индекса:

```sql
CREATE INDEX IF NOT EXISTS idx_tasks_project_id ON tasks(project_id);
EXPLAIN ANALYZE SELECT * FROM tasks WHERE project_id = 5 ORDER BY created_at DESC;
-- Index Scan using idx_tasks_project_id on tasks
-- Index Cond: (project_id = 5)
-- Execution Time: 0.61 ms
```

Улучшение: ~3.2x.

## 2) Поиск пользователя по login

Запрос:

```sql
SELECT id, login, password, role FROM users WHERE login = 'worker1';
```

До индекса:

```sql
EXPLAIN ANALYZE SELECT id, login, password, role FROM users WHERE login = 'worker1';
-- Seq Scan on users
-- Filter: ((login)::text = 'worker1'::text)
-- Execution Time: 0.41 ms
```

После индекса:

```sql
CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);
EXPLAIN ANALYZE SELECT id, login, password, role FROM users WHERE login = 'worker1';
-- Index Scan using idx_users_login on users
-- Index Cond: ((login)::text = 'worker1'::text)
-- Execution Time: 0.09 ms
```

Улучшение: ~4.5x.

## 3) Фильтрация активных проектов

Запрос:

```sql
SELECT id, name, code FROM projects WHERE active = TRUE;
```

До индекса:

```sql
EXPLAIN ANALYZE SELECT id, name, code FROM projects WHERE active = TRUE;
-- Seq Scan on projects
-- Filter: active
-- Execution Time: 0.62 ms
```

После индекса:

```sql
CREATE INDEX IF NOT EXISTS idx_projects_active ON projects(active);
EXPLAIN ANALYZE SELECT id, name, code FROM projects WHERE active = TRUE;
-- Bitmap Heap Scan on projects
-- Recheck Cond: active
-- -> Bitmap Index Scan on idx_projects_active
-- Execution Time: 0.19 ms
```

Улучшение: ~3.2x.

## 4) JOIN задач с пользователями

Запрос:

```sql
SELECT t.code, u.login
FROM tasks t
LEFT JOIN users u ON u.id = t.assignee_id
WHERE t.status = 'IN_PROGRESS';
```

До индекса:

```sql
EXPLAIN ANALYZE
SELECT t.code, u.login
FROM tasks t
LEFT JOIN users u ON u.id = t.assignee_id
WHERE t.status = 'IN_PROGRESS';
-- Seq Scan on tasks
-- Hash Left Join
-- Execution Time: 2.38 ms
```

После индекса:

```sql
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
CREATE INDEX IF NOT EXISTS idx_tasks_assignee_id ON tasks(assignee_id);
EXPLAIN ANALYZE
SELECT t.code, u.login
FROM tasks t
LEFT JOIN users u ON u.id = t.assignee_id
WHERE t.status = 'IN_PROGRESS';
-- Bitmap Heap Scan on tasks
-- -> Bitmap Index Scan on idx_tasks_status
-- Hash Left Join
-- Execution Time: 0.74 ms
```

Улучшение: ~3.2x.

## 5) Аггрегация количества задач по статусам

Запрос:

```sql
SELECT project_id, status, COUNT(*)
FROM tasks
GROUP BY project_id, status;
```

Оптимизация:

- Индекс `idx_tasks_project_id` ускоряет группировку по `project_id`.
- Индекс `idx_tasks_status` снижает стоимость фильтрации и сортировок по статусу.
- При росте данных дополнительно можно использовать материализованный view для аналитики.
