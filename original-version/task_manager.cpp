#include "task_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;

bool safe_stoi(const string &str, int &out)
{
    if (str.empty())
        return false;
    for (char c : str)
        if (c < '0' || c > '9')
            return false;
    out = stoi(str);
    return true;
}

task_data new_task(int id, string title, string priority, string due_date, string status)
{
    task_data t;
    t.id = id;
    t.title = title;
    t.priority = priority;
    t.due_date = due_date;
    t.status = status;
    t.students = {};
    return t;
}

void add_task(task_manager_data &manager, const task_data &task)
{
    manager.tasks.add_node(task);
}

task_data *find_task_by_id(task_manager_data &manager, int task_id)
{
    node<task_data> *curr = manager.tasks.first;
    while (curr != nullptr)
    {
        if (curr->data.id == task_id)
            return &curr->data;
        curr = curr->next;
    }
    return nullptr;
}

vector<string> split(const string &s, char delim)
{
    vector<string> parts;
    string part;
    istringstream ss(s);
    while (getline(ss, part, delim))
        parts.push_back(part);
    return parts;
}

bool parse_date(const string &date_str, int &day, int &month, int &year)
{
    auto parts = split(date_str, '/');
    if (parts.size() != 3)
        return false;

    try
    {
        day = stoi(parts[0]);
        month = stoi(parts[1]);
        year = stoi(parts[2]);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool is_date_earlier(const std::string &a, const std::string &b)
{
    int d1, m1, y1, d2, m2, y2;
    if (!parse_date(a, d1, m1, y1))
        return false;
    if (!parse_date(b, d2, m2, y2))
        return false;

    if (y1 != y2)
        return y1 < y2;
    if (m1 != m2)
        return m1 < m2;
    return d1 < d2;
}
void sort_tasks_by_due_date(linked_list<task_data> &list)
{
    dynamic_array<task_data> temp;
    node<task_data> *curr = list.first;
    while (curr != nullptr)
    {
        add(temp, curr->data);
        curr = curr->next;
    }

    for (int i = 0; i < temp.length(); i++)
        for (int j = i + 1; j < temp.length(); j++)
        {
            if (is_date_earlier(temp[j].due_date, temp[i].due_date))
            {
                task_data t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    list.clear();
    for (int i = 0; i < temp.length(); i++)
        list.add_node(temp[i]);
}

int generate_unique_task_id(task_manager_data &manager)
{
    int id = 1000;
    node<task_data> *curr = manager.tasks.first;
    while (curr != nullptr)
    {
        if (curr->data.id >= id)
            id = curr->data.id + 1;
        curr = curr->next;
    }
    return id;
}

// Check if a task is assigned to a given student ID
bool is_task_assigned_to(task_manager_data &manager, int task_id, int student_id)
{
    task_data *task = find_task_by_id(manager, task_id);
    if (!task)
        return false;

    // Check task's assigned students
    for (int i = 0; i < task->students.length(); i++)
    {
        if (task->students[i].id == student_id)
            return true;
    }

    // Check student's assigned task IDs
    student_data *student = find_student(manager, student_id);
    if (!student)
        return false;

    for (int i = 0; i < student->task_ids.length(); i++)
    {
        if (student->task_ids[i] == task_id)
            return true;
    }

    return false;
}

// Create a deep copy of a task linked list
linked_list<task_data> copy_list(const linked_list<task_data> &source)
{
    linked_list<task_data> dest;
    node<task_data> *curr = source.first;
    while (curr != nullptr)
    {
        dest.add_node(curr->data);
        curr = curr->next;
    }
    return dest;
}

// Free all nodes in a task linked list
void free_list(linked_list<task_data> &list)
{
    list.clear();
}

void save_tasks_to_csv(const task_manager_data &manager, const string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
        return;

    file << "id,title,priority,due_date,status,assigned_student_ids\n";
    node<task_data> *curr = manager.tasks.first;
    while (curr != nullptr)
    {
        task_data t = curr->data;
        string assigned_ids = "";
        for (int i = 0; i < t.students.length(); i++)
        {
            assigned_ids += std::to_string(t.students[i].id);
            if (i != t.students.length() - 1)
                assigned_ids += ",";
        }

        file << t.id << "," << t.title << "," << t.priority << "," << t.due_date << "," << t.status << "," << assigned_ids << "\n";
        curr = curr->next;
    }
    file.close();
}

void load_tasks_from_csv(task_manager_data &manager, const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
        return;
    manager.tasks.clear();
    string line;
    std::getline(file, line);
    while (std::getline(file, line))
    {
        stringstream ss(line);
        string id, t, p, d, s, assigned_ids_str;

        std::getline(ss, id, ',');
        std::getline(ss, t, ',');
        std::getline(ss, p, ',');
        std::getline(ss, d, ',');
        std::getline(ss, s, ',');
        std::getline(ss, assigned_ids_str);

        int task_id = stoi(id);
        task_data task = new_task(task_id, t, p, d, s);

        if (!assigned_ids_str.empty())
        {
            stringstream id_ss(assigned_ids_str);
            string sid_str;
            while (std::getline(id_ss, sid_str, ','))
            {
                int student_id = stoi(sid_str);

                student_data *stu = find_student(manager, student_id);
                if (stu != nullptr)
                {
                    add(task.students, *stu);
                    add(stu->task_ids, task_id);
                }
            }
        }

        add_task(manager, task);
    }
    file.close();
}

void save_all_data(task_manager_data &manager)
{
    save_students_to_csv(manager, "students.csv");
    save_tasks_to_csv(manager, "tasks.csv");
    save_help_requests(manager, "help_requests.csv");
}

void load_all_data(task_manager_data &manager)
{
    load_students_from_csv(manager, "students.csv");
    load_tasks_from_csv(manager, "tasks.csv");
    load_help_requests(manager, "help_requests.csv");
}

// Assign a student to a task (bidirectional link)
void assign_student_to_task(task_manager_data &manager, int student_id, int task_id)
{
    student_data *student = find_student(manager, student_id);
    task_data *task = find_task_by_id(manager, task_id);

    if (!student || !task)
        return;

    // Prevent duplicate project
    if (is_student_assigned_to_task(manager, student_id, task_id))
        return;

    // Add task ID to student
    add(student->task_ids, task_id);

    // Add student to task
    add(task->students, *student);
}

// Check if student is already assigned to the task
bool is_student_assigned_to_task(task_manager_data &manager, int student_id, int task_id)
{
    task_data *task = find_task_by_id(manager, task_id);
    if (!task)
        return false;

    for (int i = 0; i < task->students.length(); i++)
    {
        if (task->students[i].id == student_id)
            return true;
    }
    return false;
}

// Trim whitespace from string
string trim(const string &s)
{
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

// Parse multiple IDs separated by comma
void parse_and_assign_students(task_manager_data &manager, int task_id, const string &id_str)
{
    stringstream ss(id_str);
    string part;

    while (getline(ss, part, ','))
    {
        part = trim(part);
        int sid;
        if (safe_stoi(part, sid))
        {
            assign_student_to_task(manager, sid, task_id);
        }
    }
}

// Remove a student from a task (bidirectional)
void remove_student_from_task(task_manager_data &manager, int task_id, int student_id)
{
    task_data *task = find_task_by_id(manager, task_id);
    student_data *student = find_student(manager, student_id);
    if (!task || !student)
        return;

    // Remove from task's students
    for (int i = 0; i < task->students.length(); i++)
    {
        if (task->students[i].id == student_id)
        {
            remove_at(task->students, i);
            break;
        }
    }

    // Remove from student's task_ids
    for (int i = 0; i < student->task_ids.length(); i++)
    {
        if (student->task_ids[i] == task_id)
        {
            remove_at(student->task_ids, i);
            break;
        }
    }
}

// Set task status to To-Do / Done
void set_task_status(task_manager_data &manager, int task_id, const string &status)
{
    task_data *task = find_task_by_id(manager, task_id);
    if (!task)
        return;
    task->status = status;
}

// Check if help request already exists
bool has_help_request(task_manager_data &manager, int task_id, int student_id)
{
    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        help_data p = manager.help_requests[i];
        if (p.task_id == task_id && p.student_id == student_id)
            return true;
    }
    return false;
}

// Add help request to list
void add_help_request(task_manager_data &manager, int task_id, int student_id)
{
    if (!has_help_request(manager, task_id, student_id))
    {
        manager.help_requests.add({task_id, student_id, "inquired"});
    }
}

// Remove help request from list
void remove_help_request(task_manager_data &manager, int task_id, int student_id)
{
    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        help_data p = manager.help_requests[i];
        if (p.task_id == task_id && p.student_id == student_id)
        {
            manager.help_requests.remove(i);
            break;
        }
    }
}

// Save help requests to CSV file, handle commas in request/reply by quoting fields
void save_help_requests(task_manager_data &manager, const string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
        return;

    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        help_data h = manager.help_requests[i];

        // Write numeric fields
        file << h.task_id << "," << h.student_id << ",";

        // Wrap status, request, reply in quotes to support commas inside text
        file << "\"" << h.status << "\",";
        file << "\"" << h.request << "\",";
        file << "\"" << h.reply << "\"" << endl;
    }

    file.close();
}

// Load help requests from CSV, correctly parse quoted fields with commas
void load_help_requests(task_manager_data &manager, const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
        return;

    manager.help_requests.clear();
    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        vector<string> fields;
        string field;
        bool in_quotes = false;

        // Parse CSV line safely, handling commas inside quoted fields
        for (char c : line)
        {
            if (c == '"')
            {
                in_quotes = !in_quotes;
            }
            else if (c == ',' && !in_quotes)
            {
                fields.push_back(field);
                field.clear();
            }
            else
            {
                field += c;
            }
        }
        fields.push_back(field); // add the last field

        // Need exactly 5 fields: task_id, student_id, status, request, reply
        if (fields.size() != 5)
            continue;

        int task_id, student_id;
        if (safe_stoi(fields[0], task_id) && safe_stoi(fields[1], student_id))
        {
            help_data req;
            req.task_id = task_id;
            req.student_id = student_id;

            strcpy_s(req.status, fields[2].c_str());
            strcpy_s(req.request, fields[3].c_str());
            strcpy_s(req.reply, fields[4].c_str());

            manager.help_requests.add(req);
        }
    }

    file.close();
}