// MongoDB schema validation setup for project_management
const appDb = db.getSiblingDB(process.env.MONGO_INITDB_DATABASE || "project_management");

function createOrUpdateValidation(collectionName, validator) {
  const exists = appDb.getCollectionNames().includes(collectionName);
  if (!exists) {
    appDb.createCollection(collectionName, {
      validator,
      validationLevel: "strict",
      validationAction: "error"
    });
    return;
  }

  appDb.runCommand({
    collMod: collectionName,
    validator,
    validationLevel: "strict",
    validationAction: "error"
  });
}

createOrUpdateValidation("users", {
  $jsonSchema: {
    bsonType: "object",
    required: ["id", "login", "password", "firstName", "role", "createdAt"],
    properties: {
      id: { bsonType: "int", minimum: 1 },
      login: { bsonType: "string", minLength: 3, maxLength: 50, pattern: "^[a-zA-Z0-9_]+$" },
      password: { bsonType: "string", minLength: 6 },
      firstName: { bsonType: "string", minLength: 1, maxLength: 100 },
      lastName: { bsonType: "string", maxLength: 100 },
      email: { bsonType: "string", pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$" },
      role: { enum: ["GUEST", "WORKER", "TRACKER", "ADMINISTRATOR"] },
      createdAt: { bsonType: "date" }
    }
  }
});

createOrUpdateValidation("tasks", {
  $jsonSchema: {
    bsonType: "object",
    required: ["id", "code", "title", "projectId", "reporterId", "status", "createdAt", "updatedAt"],
    properties: {
      id: { bsonType: "int", minimum: 1 },
      code: { bsonType: "string", pattern: "^PROJ-[0-9]+-TASK-[0-9]+$" },
      title: { bsonType: "string", minLength: 3, maxLength: 200 },
      description: { bsonType: "string" },
      projectId: { bsonType: "int", minimum: 1 },
      assigneeId: { bsonType: "int", minimum: 0 },
      reporterId: { bsonType: "int", minimum: 1 },
      status: { enum: ["NEW", "IN_PROGRESS", "REVIEW", "DONE", "CANCELLED"] },
      priority: { enum: ["LOW", "MEDIUM", "HIGH", "CRITICAL"] },
      dueDate: { bsonType: "date" },
      tags: {
        bsonType: "array",
        items: { bsonType: "string" }
      },
      comments: {
        bsonType: "array",
        items: {
          bsonType: "object",
          required: ["userId", "text", "createdAt"],
          properties: {
            userId: { bsonType: "int", minimum: 1 },
            text: { bsonType: "string", minLength: 1 },
            createdAt: { bsonType: "date" }
          }
        }
      },
      history: {
        bsonType: "array",
        items: {
          bsonType: "object",
          required: ["field", "oldValue", "newValue", "changedBy", "changedAt"],
          properties: {
            field: { bsonType: "string" },
            oldValue: { bsonType: "string" },
            newValue: { bsonType: "string" },
            changedBy: { bsonType: "int" },
            changedAt: { bsonType: "date" }
          }
        }
      },
      createdAt: { bsonType: "date" },
      updatedAt: { bsonType: "date" }
    }
  }
});

// Validation smoke test (expected failures are caught)
try {
  appDb.users.insertOne({
    id: 999,
    login: "ab",
    password: "123456",
    firstName: "Bad",
    role: "WORKER",
    createdAt: new Date()
  });
  print("ERROR: short login validation did not trigger");
} catch (e) {
  print("OK: invalid user rejected (short login)");
}

try {
  appDb.tasks.insertOne({
    id: 999,
    code: "BAD-CODE",
    title: "x",
    projectId: 1,
    reporterId: 1,
    status: "UNKNOWN",
    createdAt: new Date(),
    updatedAt: new Date()
  });
  print("ERROR: task validation did not trigger");
} catch (e) {
  print("OK: invalid task rejected");
}
