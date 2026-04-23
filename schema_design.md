# MongoDB Schema Design (Hybrid Model)

## Collections and entity placement

- `users`: user account and auth data.
- `projects`: project metadata and project-level membership.
- `tasks`: task lifecycle data, references to users/projects, embedded comments/history.
- `counters`: technical collection for integer IDs (`users`, `projects`, `tasks`) used by existing API contracts.

## Document structure

### users

```javascript
{
  _id: ObjectId("..."),
  id: 4, // integer API id for backward compatibility
  login: "worker1",
  password: "worker123",
  firstName: "Will",
  lastName: "One",
  email: "will.one@example.com",
  role: "WORKER",
  createdAt: ISODate("2026-01-01T00:00:00Z")
}
```

### projects

```javascript
{
  _id: ObjectId("..."),
  id: 1,
  name: "Core API",
  description: "Main backend API development",
  code: "PROJ-1",
  ownerId: 2,           // users.id reference
  memberIds: [2, 4, 5], // users.id references
  active: true,
  createdAt: ISODate("2026-01-01T00:00:00Z"),
  updatedAt: ISODate("2026-01-01T00:00:00Z")
}
```

### tasks

```javascript
{
  _id: ObjectId("..."),
  id: 1,
  code: "PROJ-1-TASK-1",
  title: "Create users endpoint",
  description: "Implement CRUD users",
  projectId: 1,   // projects.id reference
  assigneeId: 4,  // users.id reference
  reporterId: 2,  // users.id reference
  status: "NEW",
  priority: "HIGH",
  tags: ["backend", "api"],
  dueDate: ISODate("2026-02-15T00:00:00Z"),
  comments: [
    {
      userId: 2,
      text: "Initial requirements added",
      createdAt: ISODate("2026-01-10T08:00:00Z")
    }
  ],
  history: [
    {
      field: "status",
      oldValue: "NEW",
      newValue: "IN_PROGRESS",
      changedBy: 2,
      changedAt: ISODate("2026-01-12T10:00:00Z")
    }
  ],
  createdAt: ISODate("2026-01-01T00:00:00Z"),
  updatedAt: ISODate("2026-01-12T10:00:00Z")
}
```

## Embedded vs references rationale

- **References** for `users`, `projects`, `tasks`:
  - entities are reused by many operations and endpoints;
  - avoids heavy duplication of user/project data across tasks;
  - supports independent writes and scaling.
- **Embedded** for `comments` and `history` inside `tasks`:
  - task activity log is naturally scoped to one task;
  - fetched together in task details;
  - append-heavy and read-together access pattern.

## Relationship map

- `projects.ownerId` -> `users.id` (1:N)
- `projects.memberIds[]` -> `users.id` (N:M)
- `tasks.projectId` -> `projects.id` (1:N)
- `tasks.assigneeId` -> `users.id` (N:1, optional)
- `tasks.reporterId` -> `users.id` (N:1)

## Indexing strategy

- `users`: unique `id`, unique `login`, unique `email` (sparse allowed)
- `projects`: unique `id`, unique `code`, index on `active`, `ownerId`
- `tasks`: unique `id`, unique `code`, index on `projectId`, `status`, `assigneeId`

This model preserves current REST API payload contracts and keeps query patterns efficient.
