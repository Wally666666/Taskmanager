#include "task_manager.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

// ------------------------------
// Internal: Generate Unique ID
// ------------------------------
static int generate_unique_student_id(task_manager_data &manager)
{
    int candidate = 100;
    // Keep incrementing until we find an unused ID
    while (student_id_exists(manager, candidate))
    {
        candidate++;
    }
    return candidate;
}

// ------------------------------
// New Student (Auto ID)
// ------------------------------
student_data new_student(task_manager_data &manager, string name, string email)
{
    student_data s;
    s.id = generate_unique_student_id(manager);
    s.name = name;
    s.email = email;
    s.task_ids = {}; // Empty task list
    return s;
}

// Add a new student
void add_student(task_manager_data &manager, const student_data &student)
{
    add(manager.students, student);
}

// Remove student by ID
void remove_student(task_manager_data &manager, int student_id)
{
    for (int i = 0; i < manager.students.length(); i++)
    {
        if (manager.students[i].id == student_id)
        {
            remove_at(manager.students, i);
            break;
        }
    }
}

// Find student by ID
student_data *find_student(task_manager_data &manager, int student_id)
{
    for (int i = 0; i < manager.students.length(); i++)
    {
        if (manager.students[i].id == student_id)
        {
            return &manager.students[i];
        }
    }
    return nullptr;
}

// Find student by partial name (case-insensitive)
student_data *find_student_by_name(task_manager_data &manager, const string &name)
{
    for (int i = 0; i < manager.students.length(); i++)
    {
        if (contains(to_lowercase(manager.students[i].name), to_lowercase(name)))
        {
            return &manager.students[i];
        }
    }
    return nullptr;
}

// Display all students
void display_all_students(const task_manager_data &manager)
{
    write_line("=== All Students ===");
    for (int i = 0; i < manager.students.length(); i++)
    {
        const auto &s = manager.students[i];
        write_line("ID: " + to_string(s.id) + " | Name: " + s.name + " | Email: " + s.email);
    }
}

// Check if ID exists
bool student_id_exists(const task_manager_data &manager, int student_id)
{
    for (int i = 0; i < manager.students.length(); i++)
    {
        if (manager.students[i].id == student_id)
        {
            return true;
        }
    }
    return false;
}

// ------------------------------
// Load Students from CSV
// ------------------------------
void load_students_from_csv(task_manager_data &manager, const string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        write_line("Error: Could not open file " + filename);
        return;
    }

    // Clear existing students
    manager.students = {};

    std::string line;
    // Skip header line
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string id_str, name, email;

        // Split by comma
        std::getline(ss, id_str, ',');
        std::getline(ss, name, ',');
        std::getline(ss, email);

        student_data s;
        s.id = std::stoi(id_str);
        s.name = name;
        s.email = email;
        s.task_ids = {};

        add(manager.students, s);
    }

    file.close();
    write_line("Loaded " + to_string(manager.students.length()) + " students from CSV.");
}

// ------------------------------
// Save Students to CSV
// ------------------------------
void save_students_to_csv(const task_manager_data &manager, const string &filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        write_line("Error: Could not open file " + filename);
        return;
    }

    // Write header
    file << "id,name,email\n";

    for (int i = 0; i < manager.students.length(); i++)
    {
        const auto &s = manager.students[i];
        file << s.id << "," << s.name << "," << s.email << "\n";
    }

    file.close();
    write_line("Saved " + to_string(manager.students.length()) + " students to CSV.");
}