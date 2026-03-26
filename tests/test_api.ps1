# ============================================
# Project Management API - Test Suite
# Variant 8: Project and Task Management
# ============================================

$BASE_URL = "http://localhost:8080"
$ErrorActionPreference = "Continue"

Write-Host "========================================"
Write-Host "  Project Management API - Test Suite  "
Write-Host "========================================"
Write-Host ""

# --------------------------------------------
# Test 1: Health Check
# --------------------------------------------
Write-Host "[Test 1] Health Check..."
try {
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/health" -Method Get
    if ($response.status -eq "ok") {
        Write-Host "  PASSED: Server is running" -ForegroundColor Green
    } else {
        Write-Host "  FAILED: Unexpected response" -ForegroundColor Red
    }
} catch {
    Write-Host "  FAILED: Server not responding" -ForegroundColor Red
    exit 1
}
Write-Host ""

# --------------------------------------------
# Test 2: Register WORKER User
# --------------------------------------------
Write-Host "[Test 2] Register WORKER user..."
try {
    $body = @{
        login = "worker1"
        password = "pass123"
        firstName = "Worker"
        lastName = "One"
        email = "worker@example.com"
    } | ConvertTo-Json
    
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/users" -Method Post -ContentType "application/json" -Body $body
    if ($response.id -and $response.role -eq "WORKER") {
        Write-Host "  PASSED: WORKER created (id=$($response.id))" -ForegroundColor Green
    }
} catch {
    if ($_.Exception.Response.StatusCode -eq 409) {
        Write-Host "  INFO: WORKER already exists" -ForegroundColor Yellow
    }
}
Write-Host ""

# --------------------------------------------
# Test 3: Register TRACKER User (for project tests)
# --------------------------------------------
# --------------------------------------------
# Test 3: Register TRACKER User (for project tests)
# --------------------------------------------
Write-Host "[Test 3] Register TRACKER user (for project/task tests)..."
try {
    $body = @{
        login = "tracker1"
        password = "pass123"
        firstName = "Tracker"
        lastName = "One"
        email = "tracker@example.com"
        role = "TRACKER"  # ← ДОБАВЬ ЭТУ СТРОКУ!
    } | ConvertTo-Json
    
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/users" -Method Post -ContentType "application/json" -Body $body
    if ($response.id -and $response.role -eq "TRACKER") {
        Write-Host "  PASSED: TRACKER created (id=$($response.id))" -ForegroundColor Green
    }
} catch {
    if ($_.Exception.Response.StatusCode -eq 409) {
        Write-Host "  INFO: TRACKER already exists" -ForegroundColor Yellow
    }
}
Write-Host ""

# --------------------------------------------
# Test 4: Login as WORKER
# --------------------------------------------
Write-Host "[Test 4] Login as WORKER..."
try {
    $body = @{ login = "worker1"; password = "pass123" } | ConvertTo-Json
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/auth" -Method Post -ContentType "application/json" -Body $body
    if ($response.token) {
        $GLOBAL:WORKER_TOKEN = $response.token
        Write-Host "  PASSED: WORKER token received" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 5: Login as TRACKER (for protected tests)
# --------------------------------------------
Write-Host "[Test 5] Login as TRACKER..."
try {
    $body = @{ login = "tracker1"; password = "pass123" } | ConvertTo-Json
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/auth" -Method Post -ContentType "application/json" -Body $body
    if ($response.token) {
        $GLOBAL:TRACKER_TOKEN = $response.token
        Write-Host "  PASSED: TRACKER token received" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 6: Create Project (as TRACKER)
# --------------------------------------------
Write-Host "[Test 6] Create project (as TRACKER)..."
try {
    $headers = @{ "Authorization" = "Bearer $TRACKER_TOKEN" }
    $body = @{ name = "Test Project"; description = "Test desc" } | ConvertTo-Json
    
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/projects" -Method Post -ContentType "application/json" -Body $body -Headers $headers
    if ($response.id -and $response.code) {
        $GLOBAL:PROJECT_ID = $response.id
        Write-Host "  PASSED: Project created (id=$($response.id), code=$($response.code))" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 7: Create Project as WORKER (should fail)
# --------------------------------------------
Write-Host "[Test 7] Create project as WORKER (should fail)..."
try {
    $headers = @{ "Authorization" = "Bearer $WORKER_TOKEN" }
    $body = @{ name = "Worker Project" } | ConvertTo-Json
    
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/projects" -Method Post -ContentType "application/json" -Body $body -Headers $headers
    Write-Host "  FAILED: Should have returned 403" -ForegroundColor Red
} catch {
    if ($_.Exception.Response.StatusCode -eq 403) {
        Write-Host "  PASSED: WORKER correctly denied (403)" -ForegroundColor Green
    } else {
        Write-Host "  FAILED: Wrong error code" -ForegroundColor Red
    }
}
Write-Host ""

# --------------------------------------------
# Test 8: Get All Projects
# --------------------------------------------
Write-Host "[Test 8] Get all projects..."
try {
    $headers = @{ "Authorization" = "Bearer $TRACKER_TOKEN" }
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/projects" -Method Get -Headers $headers
    if ($response.projects -and $response.projects.Count -ge 1) {
        Write-Host "  PASSED: Found $($response.projects.Count) project(s)" -ForegroundColor Green
    } else {
        Write-Host "  FAILED: No projects returned" -ForegroundColor Red
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 9: Create Task (as TRACKER)
# --------------------------------------------
Write-Host "[Test 9] Create task in project..."
try {
    $headers = @{ "Authorization" = "Bearer $TRACKER_TOKEN" }
    $body = @{ title = "Test Task"; description = "Test"; assigneeId = 1 } | ConvertTo-Json
    
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/projects/$PROJECT_ID/tasks" -Method Post -ContentType "application/json" -Body $body -Headers $headers
    if ($response.id -and $response.code) {
        $GLOBAL:TASK_CODE = $response.code
        Write-Host "  PASSED: Task created (code=$($response.code))" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 10: Get Project Tasks
# --------------------------------------------
Write-Host "[Test 10] Get all tasks in project..."
try {
    $headers = @{ "Authorization" = "Bearer $TRACKER_TOKEN" }
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/projects/$PROJECT_ID/tasks" -Method Get -Headers $headers
    if ($response.tasks -and $response.tasks.Count -ge 1) {
        Write-Host "  PASSED: Found $($response.tasks.Count) task(s)" -ForegroundColor Green
    } else {
        Write-Host "  FAILED: No tasks returned" -ForegroundColor Red
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 11: Get Task by Code
# --------------------------------------------
Write-Host "[Test 11] Get task by code..."
try {
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/tasks/$TASK_CODE" -Method Get
    if ($response.code -eq $TASK_CODE) {
        Write-Host "  PASSED: Task found" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 12: Get User by Login
# --------------------------------------------
Write-Host "[Test 12] Get user by login..."
try {
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/users/login/worker1" -Method Get
    if ($response.login -eq "worker1") {
        Write-Host "  PASSED: User found" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 13: Search Users (with auth)
# --------------------------------------------
Write-Host "[Test 13] Search users by name..."
try {
    $headers = @{ "Authorization" = "Bearer $TRACKER_TOKEN" }
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/users/search?firstName=Worker" -Method Get -Headers $headers
    if ($response.users) {
        Write-Host "  PASSED: Found $($response.users.Count) user(s)" -ForegroundColor Green
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)" -ForegroundColor Red
}
Write-Host ""

# --------------------------------------------
# Test 14: Search Users without Token
# --------------------------------------------
Write-Host "[Test 14] Search users without token (should fail)..."
try {
    $response = Invoke-RestMethod -Uri "$BASE_URL/api/users/search" -Method Get
    Write-Host "  FAILED: Should have returned 401" -ForegroundColor Red
} catch {
    if ($_.Exception.Response.StatusCode -eq 401) {
        Write-Host "  PASSED: Request without token rejected (401)" -ForegroundColor Green
    }
}
Write-Host ""

# ============================================
# Summary
# ============================================
Write-Host ""
Write-Host "========================================"
Write-Host "  Testing completed                    "
Write-Host "========================================"