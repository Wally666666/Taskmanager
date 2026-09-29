#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "splashkit.h"
#include "splashkit-arrays.h"
#include "linked_list.hpp"

/**
 * @struct student_data
 * @brief Represents a student participating in team tasks.
 * @field id Unique identifier for the student.
 * @field name Full name of the student.
 * @field email Contact email of the student.
 * @field task_ids List of task IDs assigned to this student (dynamic array).
 */
struct student_data
{
    int id;                      /* Unique student ID */
    string name;                 /* Student full name */
    string email;                /* Student email address */
    dynamic_array<int> task_ids; /* List of assigned task IDs */
};

/**
 * @struct task_data
 * @brief Represents a team task with details and assigned students.
 * @field id Unique identifier for the task.
 * @field title Title or description of the task.
 * @field priority Task priority level (High/Medium/Low).
 * @field due_date Task deadline in YYYY-MM-DD format.
 * @field students List of students assigned to complete this task.
 */
struct task_data
{
    int id;          /* Unique task ID */
    string title;    /* Task title/description */
    string priority; /* Task priority: High/Medium/Low */
    string due_date; /* Task due date (YYYY-MM-DD) */
    string status;
    dynamic_array<student_data> students; /* Students assigned to this task */
};

#pragma pack(push, 1)
struct help_data
{
    int task_id;       // 0~3
    int student_id;    // 4~7
    char request[256]; // 8~263
    char reply[256];   // 264~519
    char status[16];   // 520~535
};
#pragma pack(pop)

/*
struct help_data
{
    int task_id;
    int student_id;
    string status;
    string request;
    string reply;
};
*/
/**
 * @struct task_manager_data
 * @brief Central data store for managing all students and tasks in the system.
 * @field students Dynamic array storing all registered students.
 * @field tasks Custom linked list storing all created tasks (HD core feature).
 */
struct task_manager_data
{
    dynamic_array<student_data> students;   /* All student records */
    linked_list<task_data> tasks;           /* All tasks stored in a linked list */
    dynamic_array<help_data> help_requests; /* Help request system */
};

// User role definition
enum user_role
{
    NO_ROLE,
    STUDENT_ROLE, // Student: only view permission
    MANAGER_ROLE  // Manager: full permission
};

// Login user data
struct login_data
{
    string username;
    user_role role;
    bool is_logged_in = false;
};

enum app_state
{
    LOGIN_SCREEN,
    MAIN_MENU,
    STUDENT_MENU,
    ADD_STUDENT,
    VIEW_STUDENTS,
    DELETE_STUDENT,
    MODIFY_STUDENT,
    ADD_TASK,
    VIEW_TASKS,
    SEARCH_TASK,
    SEARCH_BY_TITLE,
    SEARCH_BY_STUDENT,
    SEARCH_BY_TASKID,
    FILTER_STATUS,
    FILTER_PROROTY,
    SORT_BY_STUDENT,
    UPDATE_TASK_MENU,
    UPDATE_TASK_STATUS,
    UPDATE_TASK_STUDENTS,
    // Student exclusive states
    STUDENT_MAIN_MENU,
    VIEW_STUDENT_TODO_TASKS,
    VIEW_STUDENT_ALL_TASKS,
    REQUEST_HELP,
    VIEW_HELP_REPLY,
    VIEW_HELP_DETAIL
};

struct app_data
{
    app_state state;
    bool is_quit;
    string message;

    string input_name;
    string input_email;
    string input_id;
    string input_title;
    string input_priority;
    string input_due;
    string input_IDs;
    string search_keyword;
    string search_student;
    string search_taskid;
    string search_status;
    string filter_priority;
    int target_task_id;

    bool confirm_delete;
    int active_box;

    // Login fields
    string login_username;
    string login_password;
    user_role current_role;

    // Student exclusive data
    string student_id_input;   // Store student's own ID input
    int stored_student_id;     // Validated student ID for filtering tasks
    bool has_valid_student_id; // Flag for valid student ID
};

// ------------------------------
// Student Management Functions
// ------------------------------

/**
 * Creates a new student with a unique auto-generated ID, name, and email.
 *
 * @param manager The task manager containing all students (for ID uniqueness check).
 * @param name The full name of the new student.
 * @param email The contact email of the new student.
 * @return A fully initialised student_data with unique ID.
 */
student_data new_student(task_manager_data &manager, string name, string email);

/**
 * Add a new student to the task manager's student list.
 *
 * @param manager The task manager that holds the list of all students.
 * @param student The student data to be added to the manager.
 */
void add_student(task_manager_data &manager, const student_data &student);

/**
 * Remove a student from the task manager using the student's unique ID.
 *
 * @param manager The task manager storing the student records.
 * @param student_id The unique ID of the student to remove.
 */
void remove_student(task_manager_data &manager, int student_id);

/**
 * Search for and return a pointer to a student by their ID.
 * Returns nullptr if the student is not found.
 *
 * @param manager The task manager storing the student records.
 * @param student_id The unique ID of the student to find.
 * @return A pointer to the matching student, or nullptr if not found.
 */
student_data *find_student(task_manager_data &manager, int student_id);

/**
 * Search for a student by a partial or full name (case-insensitive).
 * Returns a pointer to the first matching student, or nullptr if none found.
 *
 * @param manager The task manager storing the student records.
 * @param name The name (or partial name) to search for.
 * @return A pointer to the matching student, or nullptr if not found.
 */
student_data *find_student_by_name(task_manager_data &manager, const string &name);

bool student_id_exists(const task_manager_data &manager, int student_id);

/**
 * Display all students stored in the task manager in a formatted list.
 *
 * @param manager The task manager storing the student records.
 */
void display_all_students(const task_manager_data &manager);

/**
 * Check whether a student ID already exists in the task manager.
 *
 * @param manager The task manager storing the student records.
 * @param student_id The ID to check for existence.
 * @return True if the ID exists, false otherwise.
 */
bool student_id_exists(const task_manager_data &manager, int student_id);

// ------------------------------
// Task Management Functions
// ------------------------------

/**
 * Creates a new task with the given details.
 *
 * @param id Unique identifier for the new task.
 * @param title Descriptive title of the task.
 * @param priority Priority level (High/Medium/Low).
 * @param due_date Deadline for the task (YYYY-MM-DD).
 * @param status Current status of the task (To-Do / Done).
 * @return A fully initialised task_data structure.
 */
task_data new_task(int id, string title, string priority, string due_date, string status);

/**
 * Adds a new task to the task manager's linked list of tasks.
 *
 * @param manager The task manager storing all tasks.
 * @param task The task to add to the manager.
 */
void add_task(task_manager_data &manager, const task_data &task);

/**
 * Finds a task in the linked list by its unique ID.
 *
 * @param manager The task manager storing all tasks.
 * @param task_id The ID of the task to locate.
 * @return Pointer to the found task, or nullptr if not found.
 */
task_data *find_task_by_id(task_manager_data &manager, int task_id);

/**
 * Sorts all tasks in the linked list by their due date in ascending order.
 *
 * @param manager The task manager containing the tasks to sort.
 */
void sort_tasks_by_due_date(task_manager_data &manager);

/**
 * Generates a unique ID for a new task to avoid duplicates.
 *
 * @param manager The task manager storing existing tasks.
 * @return A new unique integer ID for a task.
 */
int generate_unique_task_id(task_manager_data &manager);

// ------------------------------
// CSV Data Persistence Functions
// ------------------------------

/**
 * Load student data from a CSV file into manager.students.
 * Existing students in the manager will be cleared.
 *
 * @param manager The task manager to load students into.
 * @param filename Path to the CSV file.
 */
void load_students_from_csv(task_manager_data &manager, const string &filename);

/**
 * Save all students in manager.students to a CSV file.
 *
 * @param manager The task manager containing students to save.
 * @param filename Path to the output CSV file.
 */
void save_students_to_csv(const task_manager_data &manager, const string &filename);

/**
 * Loads task data from a CSV file into the task manager.
 * Clears existing task data before loading.
 *
 * @param manager The task manager to load tasks into.
 * @param filename Path to the input CSV file.
 */
void load_tasks_from_csv(task_manager_data &manager, const string &filename);

/**
 * Saves all task data from the manager to a CSV file.
 *
 * @param manager The task manager containing tasks to save.
 * @param filename Path to the output CSV file.
 */
void save_tasks_to_csv(const task_manager_data &manager, const string &filename);

/**
 * Saves all students and tasks to their respective CSV files.
 *
 * @param manager The task manager containing all data to save.
 */
void save_all_data(task_manager_data &manager);

/**
 * Loads all students and tasks from their respective CSV files.
 *
 * @param manager The task manager to load data into.
 */
void load_all_data(task_manager_data &manager);

// Check if a task is assigned to a specific student
bool is_task_assigned_to(task_manager_data &manager, int task_id, int student_id);

// Create a deep copy of a task linked list
linked_list<task_data> copy_list(const linked_list<task_data> &source);

// Free memory used by a task linked list
void free_list(linked_list<task_data> &list);

// Sort a standalone task list by due date (ascending)
void sort_tasks_by_due_date(linked_list<task_data> &list);

// Assign a student to a task (bidirectional binding)
void assign_student_to_task(task_manager_data &manager, int student_id, int task_id);

// Check if student is already assigned to this task
bool is_student_assigned_to_task(task_manager_data &manager, int student_id, int task_id);

// Parse comma-separated student IDs and assign to task
void parse_and_assign_students(task_manager_data &manager, int task_id, const string &id_str);

// Remove a student from a task
void remove_student_from_task(task_manager_data &manager, int task_id, int student_id);

// Set task status
void set_task_status(task_manager_data &manager, int task_id, const string &status);

// ------------------------------
// Helper Functions
// ------------------------------
string to_lowercase(const string &str);
bool contains(const string &str, const string &substr);

// Trim whitespace helper
string trim(const string &s);
bool safe_stoi(const string &str, int &out);




// Check if help request already exists
bool has_help_request(task_manager_data &manager, int task_id, int student_id);
// Add help request to list
void add_help_request(task_manager_data &manager, int task_id, int student_id);
// Remove help request from list
void remove_help_request(task_manager_data &manager, int task_id, int student_id);

int find_help_request_index(task_manager_data &manager, int task_id, int student_id);

void save_help_requests(task_manager_data &manager, const string &filename);

void load_help_requests(task_manager_data &manager, const string &filename);

#endif