-- USERS CRUD
INSERT INTO users (login, password, first_name, last_name, email, role)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING id, login, first_name, last_name, email, role, created_at;

SELECT id, login, password, first_name, last_name, email, role, created_at
FROM users
WHERE login = $1;

SELECT id, login, first_name, last_name, email, role, created_at
FROM users
WHERE ($1 = '' OR first_name ILIKE '%' || $1 || '%')
  AND ($2 = '' OR last_name ILIKE '%' || $2 || '%')
ORDER BY id;

UPDATE users
SET first_name = $2,
    last_name = $3,
    email = $4,
    role = $5
WHERE id = $1
RETURNING id, login, first_name, last_name, email, role, created_at;

DELETE FROM users WHERE id = $1;

-- PROJECTS CRUD
INSERT INTO projects (name, description, code, owner_id, active)
VALUES ($1, $2, $3, $4, TRUE)
RETURNING id, name, description, code, owner_id, active, created_at;

SELECT id, name, description, code, owner_id, active, created_at
FROM projects
WHERE active = TRUE
ORDER BY id DESC;

SELECT id, name, description, code, owner_id, active, created_at
FROM projects
WHERE name ILIKE '%' || $1 || '%'
ORDER BY id DESC;

UPDATE projects
SET name = $2, description = $3, active = $4
WHERE id = $1
RETURNING id, name, description, code, owner_id, active, created_at;

DELETE FROM projects WHERE id = $1;

-- TASKS CRUD
INSERT INTO tasks (code, title, description, project_id, assignee_id, reporter_id, status)
VALUES ($1, $2, $3, $4, $5, $6, $7)
RETURNING id, code, title, description, project_id, assignee_id, reporter_id, status, created_at, updated_at;

SELECT id, code, title, description, project_id, assignee_id, reporter_id, status, created_at, updated_at
FROM tasks
WHERE code = $1;

SELECT id, code, title, description, project_id, assignee_id, reporter_id, status, created_at, updated_at
FROM tasks
WHERE project_id = $1
ORDER BY created_at DESC;

SELECT id, code, title, description, project_id, assignee_id, reporter_id, status, created_at, updated_at
FROM tasks
WHERE ($1::task_status IS NULL OR status = $1)
  AND ($2::INTEGER IS NULL OR assignee_id = $2)
ORDER BY updated_at DESC;

UPDATE tasks
SET title = $2,
    description = $3,
    assignee_id = $4,
    status = $5,
    updated_at = NOW()
WHERE id = $1
RETURNING id, code, title, description, project_id, assignee_id, reporter_id, status, created_at, updated_at;

DELETE FROM tasks WHERE id = $1;

-- JOIN queries
SELECT t.id,
       t.code,
       t.title,
       t.status,
       p.name AS project_name,
       a.login AS assignee_login,
       r.login AS reporter_login
FROM tasks t
JOIN projects p ON p.id = t.project_id
LEFT JOIN users a ON a.id = t.assignee_id
JOIN users r ON r.id = t.reporter_id
WHERE t.project_id = $1
ORDER BY t.created_at DESC;

-- Aggregations
SELECT project_id, status, COUNT(*) AS total
FROM tasks
GROUP BY project_id, status
ORDER BY project_id, status;

SELECT u.id, u.login, COUNT(t.id) AS assigned_tasks
FROM users u
LEFT JOIN tasks t ON t.assignee_id = u.id
GROUP BY u.id, u.login
ORDER BY assigned_tasks DESC, u.id;
