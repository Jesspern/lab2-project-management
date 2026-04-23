// Mongo init seed for project_management
const dbName = process.env.MONGO_INITDB_DATABASE || "project_management";
const appDb = db.getSiblingDB(dbName);

appDb.users.deleteMany({});
appDb.projects.deleteMany({});
appDb.tasks.deleteMany({});
appDb.counters.deleteMany({});

const users = [
  { id: 1, login: "admin", password: "admin123", firstName: "Alice", lastName: "Admin", email: "alice.admin@example.com", role: "ADMINISTRATOR", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 2, login: "tracker1", password: "tracker123", firstName: "Tom", lastName: "Tracker", email: "tom.tracker@example.com", role: "TRACKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 3, login: "tracker2", password: "tracker123", firstName: "Tracy", lastName: "Lead", email: "tracy.lead@example.com", role: "TRACKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 4, login: "worker1", password: "worker123", firstName: "Will", lastName: "One", email: "will.one@example.com", role: "WORKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 5, login: "worker2", password: "worker123", firstName: "Wendy", lastName: "Two", email: "wendy.two@example.com", role: "WORKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 6, login: "worker3", password: "worker123", firstName: "Bob", lastName: "Builder", email: "bob.builder@example.com", role: "WORKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 7, login: "worker4", password: "worker123", firstName: "Eve", lastName: "Worker", email: "eve.worker@example.com", role: "WORKER", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 8, login: "guest1", password: "guest123", firstName: "Gary", lastName: "Guest", email: "gary.guest@example.com", role: "GUEST", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 9, login: "guest2", password: "guest123", firstName: "Gina", lastName: "Guest", email: "gina.guest@example.com", role: "GUEST", createdAt: new Date("2026-01-01T00:00:00Z") },
  { id: 10, login: "worker5", password: "worker123", firstName: "Nina", lastName: "Dev", email: "nina.dev@example.com", role: "WORKER", createdAt: new Date("2026-01-01T00:00:00Z") }
];

const projects = [
  { id: 1, name: "Core API", description: "Main backend API development", code: "PROJ-1", ownerId: 2, memberIds: [2, 4, 5, 6], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 2, name: "Mobile App", description: "Android and iOS app", code: "PROJ-2", ownerId: 3, memberIds: [3, 4, 7], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 3, name: "Web Portal", description: "Frontend dashboard", code: "PROJ-3", ownerId: 2, memberIds: [2, 6, 7], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 4, name: "Infrastructure", description: "CI/CD and deployment", code: "PROJ-4", ownerId: 1, memberIds: [1, 5, 6, 7], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 5, name: "Monitoring", description: "Observability and alerting", code: "PROJ-5", ownerId: 1, memberIds: [1, 4, 5], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 6, name: "Billing", description: "Payment and invoices", code: "PROJ-6", ownerId: 3, memberIds: [3, 4, 5], active: false, createdAt: new Date(), updatedAt: new Date() },
  { id: 7, name: "Notifications", description: "Email/SMS notifications", code: "PROJ-7", ownerId: 2, memberIds: [2, 6, 7], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 8, name: "Auth Service", description: "Authentication and authorization", code: "PROJ-8", ownerId: 1, memberIds: [1, 5, 6], active: true, createdAt: new Date(), updatedAt: new Date() },
  { id: 9, name: "Data Warehouse", description: "Analytics pipeline", code: "PROJ-9", ownerId: 3, memberIds: [3, 4, 5, 6], active: false, createdAt: new Date(), updatedAt: new Date() },
  { id: 10, name: "Internal Tools", description: "Admin/internal features", code: "PROJ-10", ownerId: 1, memberIds: [1, 4, 7], active: true, createdAt: new Date(), updatedAt: new Date() }
];

const statuses = ["NEW", "IN_PROGRESS", "REVIEW", "DONE", "CANCELLED"];
const priorities = ["LOW", "MEDIUM", "HIGH", "CRITICAL"];
const tagsPool = [["backend"], ["frontend"], ["urgent"], ["api", "auth"], ["ops"], ["mobile"], ["analytics"], ["billing"], ["qa"], ["docs"]];
const tasks = [];

for (let i = 1; i <= 30; i += 1) {
  const projectId = ((i - 1) % 10) + 1;
  const assigneeId = ((i - 1) % 7) + 1;
  const reporterId = ((i + 1) % 3) + 1;
  tasks.push({
    id: i,
    code: `PROJ-${projectId}-TASK-${i}`,
    title: `Task ${i}`,
    description: `Task ${i} description`,
    projectId,
    assigneeId,
    reporterId,
    status: statuses[i % statuses.length],
    priority: priorities[i % priorities.length],
    dueDate: new Date(Date.now() + i * 24 * 60 * 60 * 1000),
    tags: tagsPool[i % tagsPool.length],
    comments: [
      {
        userId: reporterId,
        text: `Comment for task ${i}`,
        createdAt: new Date()
      }
    ],
    history: [
      {
        field: "status",
        oldValue: "NEW",
        newValue: statuses[i % statuses.length],
        changedBy: reporterId,
        changedAt: new Date()
      }
    ],
    createdAt: new Date(),
    updatedAt: new Date()
  });
}

appDb.users.insertMany(users);
appDb.projects.insertMany(projects);
appDb.tasks.insertMany(tasks);

appDb.counters.insertMany([
  { _id: "users", seq: users.length },
  { _id: "projects", seq: projects.length },
  { _id: "tasks", seq: tasks.length }
]);

print(`Seed completed for DB '${dbName}': users=${users.length}, projects=${projects.length}, tasks=${tasks.length}`);
