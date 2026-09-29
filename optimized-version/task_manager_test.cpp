#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "task_manager.h"
using Catch::Approx;

TEST_CASE("Student Management Functions Work Correctly", "[student]")
{
    // Initialize empty task manager
    task_manager_data manager;
    manager.students = {};
    manager.tasks.clear();
    manager.help_requests = {};

    SECTION("Create and add a new student with auto-generated ID")
    {
        student_data stu = new_student(manager, "Test Student", "test@example.com");
        add_student(manager, stu);

        // Verify student was added
        REQUIRE(manager.students.length() == 1);
        REQUIRE(manager.students[0].name == "Test Student");
        REQUIRE(manager.students[0].email == "test@example.com");
        // Auto ID starts at 100
        REQUIRE(manager.students[0].id == 100);
    }

    SECTION("Find student by ID returns valid pointer")
    {
        student_data stu = new_student(manager, "Find Me", "find@me.com");
        add_student(manager, stu);

        student_data *found = find_student(manager, 100);
        REQUIRE(found != nullptr);
        REQUIRE(found->name == "Find Me");

        // Non-existent ID returns nullptr
        student_data *not_found = find_student(manager, 9999);
        REQUIRE(not_found == nullptr);
    }

    SECTION("Find student by name (case-insensitive)")
    {
        student_data stu = new_student(manager, "Alice Smith", "alice@test.com");
        add_student(manager, stu);

        student_data *found1 = find_student_by_name(manager, "alice");
        student_data *found2 = find_student_by_name(manager, "SMITH");
        REQUIRE(found1 != nullptr);
        REQUIRE(found2 != nullptr);

        student_data *no_match = find_student_by_name(manager, "Bob");
        REQUIRE(no_match == nullptr);
    }

    SECTION("Remove student by ID removes from list")
    {
        student_data stu = new_student(manager, "Delete Me", "delete@me.com");
        add_student(manager, stu);
        REQUIRE(manager.students.length() == 1);

        remove_student(manager, 100);
        REQUIRE(manager.students.length() == 0);
    }

    SECTION("student_id_exists detects existing and missing IDs")
    {
        student_data stu = new_student(manager, "Exist Test", "exist@test.com");
        add_student(manager, stu);

        REQUIRE(student_id_exists(manager, 100) == true);
        REQUIRE(student_id_exists(manager, 999) == false);
    }
}

TEST_CASE("Task Management Functions Work Correctly", "[task]")
{
    task_manager_data manager;
    manager.students = {};
    manager.tasks.clear();
    manager.help_requests = {};

    SECTION("Create and add a new task")
    {
        task_data task = new_task(1000, "Finish Project", "High", "2025-12-31", "To-Do");
        add_task(manager, task);

        // Verify task exists in linked list
        REQUIRE(manager.tasks.first != nullptr);
        REQUIRE(manager.tasks.first->data.title == "Finish Project");
    }

    SECTION("find_task_by_id locates valid task")
    {
        task_data task = new_task(1001, "Unit Testing", "Medium", "2025-11-30", "Done");
        add_task(manager, task);

        task_data *found = find_task_by_id(manager, 1001);
        REQUIRE(found != nullptr);
        REQUIRE(found->status == "Done");

        task_data *not_found = find_task_by_id(manager, 9999);
        REQUIRE(not_found == nullptr);
    }

    SECTION("generate_unique_task_id creates incremental IDs")
    {
        task_data t1 = new_task(1000, "Task 1", "Low", "2025-10-01", "To-Do");
        task_data t2 = new_task(1001, "Task 2", "Low", "2025-10-02", "To-Do");
        add_task(manager, t1);
        add_task(manager, t2);

        int new_id = generate_unique_task_id(manager);
        REQUIRE(new_id == 1002);
    }
}

TEST_CASE("Student-Task Project Functions", "[project]")
{
    task_manager_data manager;
    manager.students = {};
    manager.tasks.clear();
    manager.help_requests = {};

    // Setup test data
    student_data stu = new_student(manager, "Assigned Student", "assign@test.com");
    add_student(manager, stu);
    task_data task = new_task(1000, "Project Test", "High", "2025-12-01", "To-Do");
    add_task(manager, task);

    SECTION("Assign student to task creates bidirectional link")
    {
        assign_student_to_task(manager, 100, 1000);

        // Verify task has student
        task_data *t = find_task_by_id(manager, 1000);
        REQUIRE(t->students.length() == 1);
        REQUIRE(t->students[0].id == 100);

        // Verify student has task ID
        student_data *s = find_student(manager, 100);
        REQUIRE(s->task_ids.length() == 1);
        REQUIRE(s->task_ids[0] == 1000);
    }

    SECTION("is_student_assigned_to_task detects project")
    {
        assign_student_to_task(manager, 100, 1000);
        REQUIRE(is_student_assigned_to_task(manager, 100, 1000) == true);
        REQUIRE(is_student_assigned_to_task(manager, 100, 999) == false);
    }

    SECTION("Remove student from task breaks bidirectional link")
    {
        assign_student_to_task(manager, 100, 1000);
        remove_student_from_task(manager, 1000, 100);

        task_data *t = find_task_by_id(manager, 1000);
        student_data *s = find_student(manager, 100);
        REQUIRE(t->students.length() == 0);
        REQUIRE(s->task_ids.length() == 0);
    }

    SECTION("set_task_status updates task status correctly")
    {
        set_task_status(manager, 1000, "Done");
        task_data *t = find_task_by_id(manager, 1000);
        REQUIRE(t->status == "Done");
    }
}

TEST_CASE("Help Request Management", "[help]")
{
    task_manager_data manager;
    manager.students = {};
    manager.tasks.clear();
    manager.help_requests = {};

    SECTION("Add and check help request existence")
    {
        add_help_request(manager, 1000, 100);
        REQUIRE(has_help_request(manager, 1000, 100) == true);
        REQUIRE(has_help_request(manager, 1000, 101) == false);
    }

    SECTION("Remove help request deletes from list")
    {
        add_help_request(manager, 1000, 100);
        REQUIRE(manager.help_requests.length() == 1);

        remove_help_request(manager, 1000, 100);
        REQUIRE(manager.help_requests.length() == 0);
    }
}

TEST_CASE("Date Comparison and Sorting", "[sort][date]")
{
    task_manager_data manager;
    manager.tasks.clear();

    task_data t1 = new_task(1000, "Early", "Low", "01/10/2025", "To-Do");
    task_data t2 = new_task(1001, "Late", "Low", "15/11/2025", "To-Do");
    add_task(manager, t1);
    add_task(manager, t2);

    SECTION("Sort tasks by due date ascending")
    {
        sort_tasks_by_due_date(manager.tasks);
        // First node should be earlier date
        REQUIRE(manager.tasks.first->data.id == 1000);
    }
}