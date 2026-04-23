// CRUD and query examples for MongoDB homework
const appDb = db.getSiblingDB("project_management");

// ---------------- CREATE ----------------
appDb.users.insertOne({
  id: 11,
  login: "new_user",
  password: "securepass123",
  firstName: "New",
  lastName: "User",
  email: "new.user@example.com",
  role: "WORKER",
  createdAt: new Date()
});

appDb.projects.insertOne({
  id: 11,
  name: "New Project",
  description: "Created from queries.js",
  code: "PROJ-11",
  ownerId: 2,
  memberIds: [2, 4],
  active: true,
  createdAt: new Date(),
  updatedAt: new Date()
});

appDb.tasks.insertOne({
  id: 31,
  code: "PROJ-11-TASK-31",
  title: "Initial task",
  description: "Created by query example",
  projectId: 11,
  assigneeId: 4,
  reporterId: 2,
  status: "NEW",
  priority: "HIGH",
  dueDate: new Date(),
  tags: ["api", "urgent"],
  comments: [],
  history: [],
  createdAt: new Date(),
  updatedAt: new Date()
});

// ---------------- READ ----------------
appDb.users.find({ role: { $eq: "WORKER" } });
appDb.users.find({ role: { $ne: "GUEST" } });
appDb.projects.find({ ownerId: { $in: [1, 2] } });
appDb.tasks.find({ id: { $gt: 10, $lt: 20 } });
appDb.tasks.find({
  $and: [{ status: { $in: ["NEW", "IN_PROGRESS"] } }, { priority: { $eq: "HIGH" } }]
});
appDb.tasks.find({
  $or: [{ assigneeId: 4 }, { reporterId: 4 }]
});
appDb.users.find({ firstName: { $regex: "wi", $options: "i" } });

// ---------------- UPDATE ----------------
appDb.tasks.updateOne(
  { code: "PROJ-11-TASK-31" },
  { $set: { status: "IN_PROGRESS", updatedAt: new Date() } }
);

appDb.tasks.updateOne(
  { code: "PROJ-11-TASK-31" },
  { $addToSet: { tags: "backend" } }
);

appDb.tasks.updateOne(
  { code: "PROJ-11-TASK-31" },
  {
    $push: {
      comments: {
        userId: 2,
        text: "Started implementation",
        createdAt: new Date()
      }
    }
  }
);

appDb.tasks.updateOne(
  { code: "PROJ-11-TASK-31" },
  { $pull: { tags: "urgent" } }
);

// ---------------- DELETE ----------------
appDb.tasks.deleteOne({ code: "PROJ-11-TASK-31" });
appDb.projects.deleteMany({ active: false, id: { $gt: 1000 } }); // safe sample
appDb.users.deleteMany({ login: { $regex: "^tmp_" } });

// ---------------- AGGREGATION / LOOKUP ----------------
appDb.tasks.aggregate([
  { $group: { _id: "$status", count: { $sum: 1 } } },
  { $sort: { count: -1 } }
]);

appDb.tasks.aggregate([
  { $match: { status: { $in: ["NEW", "IN_PROGRESS"] } } },
  {
    $lookup: {
      from: "projects",
      localField: "projectId",
      foreignField: "id",
      as: "project"
    }
  },
  {
    $lookup: {
      from: "users",
      localField: "assigneeId",
      foreignField: "id",
      as: "assignee"
    }
  },
  {
    $project: {
      code: 1,
      title: 1,
      status: 1,
      projectName: { $arrayElemAt: ["$project.name", 0] },
      assignee: { $arrayElemAt: ["$assignee.login", 0] }
    }
  }
]);
