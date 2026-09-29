#include "splashkit.h"
#include "task_manager.h"
#include <iostream>
#include <string>
#include <windows.h> // new for WM_COPYDATA
#include <cstring>

#define MSG_NEW_HELP_REQUEST 1001 // Student → Manager：request
#define MSG_HELP_REPLY 1002       // Manager → Student：reply

#define MANAGER_WINDOW_TITLE "Task Manager"
#define STUDENT_WINDOW_TITLE "Student Panel"

#if defined(MANAGER_BUILD)
#define WINDOW_TITLE "Task Manager"
#elif defined(STUDENT_BUILD)
#define WINDOW_TITLE "Student Panel"
#else
#define WINDOW_TITLE "Team Task Manager"
#endif

// ===================== Global IPC Data =====================
// Global pointers for window message callback

task_manager_data *g_manager = nullptr;
app_data *g_app = nullptr;

// Original window procedure
WNDPROC g_original_wndproc = nullptr;

const int SCREEN_WIDTH = 850;
const int SCREEN_HEIGHT = 650;

// ===================== WM_COPYDATA Communication Functions =====================

// Send help data from Student to Manager
void send_help_to_manager(help_data &help)
{
    HWND manager_hwnd = FindWindowA(NULL, MANAGER_WINDOW_TITLE);
    if (manager_hwnd == NULL)
        return;

    COPYDATASTRUCT cd{};
    cd.dwData = MSG_NEW_HELP_REQUEST;
    cd.cbData = sizeof(help_data);
    cd.lpData = &help;

    SendMessageA(manager_hwnd, WM_COPYDATA, NULL, (LPARAM)&cd);
}

// Send reply from Manager to Student
void send_reply_to_student(help_data &help)
{
    HWND student_hwnd = FindWindowA(NULL, STUDENT_WINDOW_TITLE);
    if (student_hwnd == NULL)
        return;

    COPYDATASTRUCT cd{};
    cd.dwData = MSG_HELP_REPLY;
    cd.cbData = sizeof(help_data);
    cd.lpData = &help;

    SendMessageA(student_hwnd, WM_COPYDATA, NULL, (LPARAM)&cd);
}

// Window procedure to receive IPC messages
LRESULT CALLBACK ipc_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COPYDATA && g_manager != nullptr && g_app != nullptr)
    {
        COPYDATASTRUCT *cd = (COPYDATASTRUCT *)lParam;

        // Receive new help request (Manager only)
        if (cd->dwData == MSG_NEW_HELP_REQUEST)
        {
            help_data received = *(help_data *)cd->lpData;

            // Update if exists, add if not
            int idx = find_help_request_index(*g_manager, received.task_id, received.student_id);
            if (idx != -1)
                g_manager->help_requests[idx] = received;
            else
                g_manager->help_requests.add(received);

            g_app->message = "NEW HELP RECEIVED!";
            return 1;
        }

        // Receive reply (Student only)
        if (cd->dwData == MSG_HELP_REPLY)
        {
            help_data replied = *(help_data *)cd->lpData;
            int idx = find_help_request_index(*g_manager, replied.task_id, replied.student_id);
            if (idx != -1)
            {
                g_manager->help_requests[idx] = replied;
                g_app->message = "NEW REPLY FROM MANAGER!";
            }
            return 1;
        }
    }

    // Pass message to SplashKit default handler
    return CallWindowProc(g_original_wndproc, hwnd, msg, wParam, lParam);
}

// Hook window to enable IPC
void init_ipc()
{
    HWND sk_hwnd = GetActiveWindow();
    g_original_wndproc = (WNDPROC)SetWindowLongPtr(sk_hwnd, GWLP_WNDPROC, (LONG_PTR)ipc_wnd_proc);
}

void new_app(app_data &app, task_manager_data &manager)
{
    app.state = LOGIN_SCREEN;
    app.is_quit = false;
    app.message = "";

    app.input_name = "";
    app.input_email = "";
    app.input_id = "";
    app.input_title = "";
    app.input_priority = "";
    app.input_due = "";
    app.input_IDs = "";
    app.search_keyword = "";
    app.search_student = "";
    app.search_taskid = "";
    app.search_status = "";
    app.filter_priority = "";
    app.target_task_id = -1;

    app.confirm_delete = false;
    app.active_box = 0;

    // Initialize login data
    app.login_username = "";
    app.login_password = "";
    app.current_role = NO_ROLE;

    // Initialize student exclusive data
    app.student_id_input = "";
    app.stored_student_id = -1;
    app.has_valid_student_id = false;

    manager.students = {};
    manager.tasks = linked_list<task_data>();
    manager.help_requests = {};
}

bool is_valid_email(const string &s)
{
    if (s.empty())
        return false;
    bool has_at = s.find("@") != string::npos;
    bool has_dot = s.find(".") != string::npos;
    return has_at && has_dot;
}

bool is_valid_id(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
        if (!(c >= '0' && c <= '9'))
            return false;
    return true;
}

bool is_valid_IDs(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c == ',')))
            return false;
    return true;
}

bool is_valid_name(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' '))
        {
            return false;
        }
    }
    return true;
}

bool is_valid_text(const string &s)
{
    return !s.empty();
}

bool is_valid_date(const string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c == '/')))
            return false;
    return true;
}

string to_lower(string s)
{
    for (int i = 0; i < s.length(); i++)
        s[i] = tolower(s[i]);
    return s;
}

// Validate user login
bool validate_login(const string &username, const string &password, user_role &role)
{
    if (username == "manager" && password == "admin123")
    {
        role = MANAGER_ROLE;
        return true;
    }
    if (username == "student" && password == "student123")
    {
        role = STUDENT_ROLE;
        return true;
    }
    return false;
}

string get_assigned_students(task_data t)
{
    string assigned = "";

    for (int i = 0; i < t.students.length(); i++)
    {
        assigned += to_string(t.students[i].id);
        if (i != t.students.length() - 1)
            assigned += ", ";
    }
    if (assigned.empty())
        assigned = "None";

    return assigned;
}

string get_help_tasks(task_manager_data &manager, int student_id)
{
    string str = "";

    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        if (manager.help_requests[i].student_id == student_id)
        {
            if (!str.empty())
                str += ",";
            str += to_string(manager.help_requests[i].task_id);
            if (strcmp(manager.help_requests[i].status, "replied") == 0)
                str += " !!! ";
            else
                str += " ??? ";
        }
    }

    return str;
}

// Get all student IDs that requested help for a specific task, separated by commas
string get_help_students_for_task(task_manager_data &manager, int task_id)
{
    string result = "";

    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        if (manager.help_requests[i].task_id == task_id)
        {
            if (!result.empty())
                result += ",";
            result += to_string(manager.help_requests[i].student_id);
            if (strcmp(manager.help_requests[i].status, "replied") == 0)
                result += " !!! ";
            else
                result += " ??? ";
        }
    }
    return result.empty() ? "No help requests" : result;
}

// Find help request index by task_id and student_id
// Return index if found, return -1 if not found
int find_help_request_index(task_manager_data &manager, int task_id, int student_id)
{
    for (int i = 0; i < manager.help_requests.length(); i++)
    {
        help_data &h = manager.help_requests[i];
        if (h.task_id == task_id && h.student_id == student_id)
        {
            return i;
        }
    }
    return -1;
}

// ---------------- DRAW FUNCTIONS ----------------
void draw_help_icon(double x, double y)
{
    fill_rectangle(COLOR_RED, x, y, 26, 26);
    draw_text("?", COLOR_WHITE, font_named("Arial"), 20, x + 6, y + 2);
}

void draw_login_screen(app_data &app)
{
    rectangle rect_username = rectangle_from(250, 200, 300, 30);
    rectangle rect_password = rectangle_from(250, 260, 300, 30);

    clear_screen(COLOR_LIGHT_BLUE);
    draw_text("TASK MANAGER - LOGIN", COLOR_NAVY, font_named("Arial"), 32, 200, 100);

    draw_text("Username:", COLOR_BLACK, font_named("Arial"), 20, 150, 200);
    draw_text("Password:", COLOR_BLACK, font_named("Arial"), 20, 150, 260);

    draw_rectangle(COLOR_BLACK, rect_username);
    draw_rectangle(COLOR_BLACK, rect_password);

    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_username))
        {
            start_reading_text(rect_username);
            app.active_box = 1;
        }
        else if (point_in_rectangle(mouse_position(), rect_password))
        {
            start_reading_text(rect_password);
            app.active_box = 2;
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
        }
    }

    if (!reading_text() && app.active_box != 0)
    {
        if (!text_entry_cancelled())
        {
            string input = text_input();
            if (app.active_box == 1)
                app.login_username = input;
            else if (app.active_box == 2)
                app.login_password = input;
        }
        app.active_box = 0;
    }

    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box == 1)
            draw_text(string(app.login_password.length(), '*'), COLOR_BLACK,
                      font_named("Arial"), 18, rect_password.x + 5, rect_password.y);
        else
            draw_text(app.login_username, COLOR_BLACK, font_named("Arial"), 18, rect_username.x + 5, rect_username.y);
    }
    else
    {
        draw_text(app.login_username, COLOR_BLACK, font_named("Arial"), 18, rect_username.x + 5, rect_username.y);
        draw_text(string(app.login_password.length(), '*'), COLOR_BLACK, font_named("Arial"), 18, rect_password.x + 5, rect_password.y);
    }

    draw_text("Press ENTER to login", COLOR_DARK_GREEN, font_named("Arial"), 20, 300, 320);
    draw_text("Press ESC to exit", COLOR_DARK_RED, font_named("Arial"), 20, 320, 360);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 200, 420);

    draw_text("Demo Accounts:", COLOR_NAVY, font_named("Arial"), 18, 300, 480);
    draw_text("Manager: username=manager, password=admin123", COLOR_DARK_GRAY, font_named("Arial"), 16, 230, 510);
    draw_text("Student: username=student, password=student123", COLOR_DARK_GRAY, font_named("Arial"), 16, 230, 540);
}

void draw_main_menu(app_data &app)
{
    clear_screen(COLOR_WHITE);
    draw_text("TEAM TASK MANAGER", COLOR_NAVY, font_named("Arial"), 30, 220, 30);

    string role_text = (app.current_role == MANAGER_ROLE) ? "Manager Mode" : "Student Mode";
    color role_color = (app.current_role == MANAGER_ROLE) ? COLOR_DARK_GREEN : COLOR_DARK_BLUE;
    draw_text("User: " + role_text, role_color, font_named("Arial"), 15, 260, 65);

    draw_text("1. Manage Students", COLOR_BLACK, font_named("Arial"), 20, 100, 100);
    draw_text("2. Add Task (Assign Students)", COLOR_BLACK, font_named("Arial"), 20, 100, 130);
    draw_text("3. View All Tasks", COLOR_BLACK, font_named("Arial"), 20, 100, 160);
    draw_text("4. Search Task", COLOR_BLACK, font_named("Arial"), 20, 100, 190);
    draw_text("5. Update Task", COLOR_BLACK, font_named("Arial"), 20, 100, 220);
    draw_text("6. Sort Tasks by Due Date", COLOR_BLACK, font_named("Arial"), 20, 100, 250);
    draw_text("7. View All Help Requestions", COLOR_BLACK, font_named("Arial"), 20, 100, 280);
    draw_text("8. Exit", COLOR_BLACK, font_named("Arial"), 20, 100, 310);

    if (!app.message.empty())
        draw_text(app.message, COLOR_DARK_GREEN, font_named("Arial"), 20, 150, 500);
}

// ---------------- STUDENT EXCLUSIVE MAIN MENU ----------------
void draw_student_main_menu(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);
    draw_text("TEAM TASK MANAGER - STUDENT PORTAL", COLOR_NAVY, font_named("Arial"), 28, 180, 40);
    draw_text("Student Mode (Read Only)", COLOR_DARK_BLUE, font_named("Arial"), 18, 300, 80);

    // Student exclusive menu options
    draw_text("1. View Your To-Do Tasks", COLOR_BLACK, font_named("Arial"), 22, 150, 140);
    draw_text("2. View All Your Tasks", COLOR_BLACK, font_named("Arial"), 22, 150, 180);
    draw_text("3. Request Help", COLOR_BLACK, font_named("Arial"), 22, 150, 220);

    draw_text("4. Search Task", COLOR_BLACK, font_named("Arial"), 22, 150, 260);
    draw_text("5. Filter Tasks by Priority", COLOR_BLACK, font_named("Arial"), 22, 150, 300);
    draw_text("6. Sort Tasks by Due Date", COLOR_BLACK, font_named("Arial"), 22, 150, 340);
    draw_text("7. Exit", COLOR_BLACK, font_named("Arial"), 22, 150, 380);

    // Student ID input box at bottom
    rectangle id_rect = rectangle_from(350, 480, 200, 30);
    draw_text("Enter Your Student ID:", COLOR_BLACK, font_named("Arial"), 20, 150, 485);
    draw_rectangle(COLOR_BLACK, id_rect);
    fill_rectangle(COLOR_LIGHT_GRAY, id_rect);

    // Handle mouse click for ID input
    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), id_rect))
        {
            start_reading_text(id_rect);
            app.active_box = 1;
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
            app.message = "";
        }
    }

    // Process ID input
    if (!reading_text() && app.active_box == 1)
    {
        if (!text_entry_cancelled())
        {
            string input = text_input();
            int sid;
            if (safe_stoi(input, sid))
            {
                if (student_id_exists(manager, sid))
                {
                    app.student_id_input = input;
                    app.stored_student_id = sid;
                    app.has_valid_student_id = true;
                    app.message = "Student ID registered successfully!";
                }
                else
                {
                    app.message = "No matchig Student ID found! ";
                }
            }
            else
            {
                app.student_id_input = "";
                app.stored_student_id = -1;
                app.has_valid_student_id = false;
                app.message = "Invalid Student ID! Numbers only.";
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    // Draw input text
    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    }
    else
    {
        draw_text(app.student_id_input, COLOR_BLACK, font_named("Arial"), 18, id_rect.x + 6, id_rect.y + 6);
    }

    // Show status
    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 540);
    }
    else
    {
        draw_text("You can enter your student ID to filter tasks",
                  COLOR_BLUE, font_named("Arial"), 18, 150, 540);
    }

    if (!app.message.empty())
        draw_text(app.message, COLOR_DARK_GREEN, font_named("Arial"), 18, 300, 600);
}

void draw_student_menu(app_data &app)
{
    clear_screen(COLOR_WHITE);
    draw_text("====== STUDENT MANAGEMENT ======", COLOR_DARK_GREEN, font_named("Arial"), 24, 180, 100);

    draw_text("1. Add New Student", COLOR_BLACK, font_named("Arial"), 20, 150, 150);
    draw_text("2. View All Students", COLOR_BLACK, font_named("Arial"), 20, 150, 180);
    draw_text("3. Modify Student Info", COLOR_BLACK, font_named("Arial"), 20, 150, 210);
    draw_text("4. Delete Student", COLOR_BLACK, font_named("Arial"), 20, 150, 240);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_add_student(app_data &app)
{
    rectangle rect_name = rectangle_from(220, 160, 300, 30);
    rectangle rect_email = rectangle_from(220, 210, 300, 30);

    clear_screen(COLOR_WHITE);
    draw_text("Add New Student", COLOR_BLUE, font_named("Arial"), 26, 250, 100);
    draw_text("Name:", COLOR_BLACK, font_named("Arial"), 20, 150, 160);
    draw_text("Email:", COLOR_BLACK, font_named("Arial"), 20, 150, 210);

    draw_rectangle(COLOR_BLACK, rect_name);
    draw_rectangle(COLOR_BLACK, rect_email);

    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_name))
        {
            start_reading_text(rect_name);
            app.active_box = 1;
            app.message = "Please enter a valid name (letters and spaces only)";
        }
        else if (point_in_rectangle(mouse_position(), rect_email))
        {
            start_reading_text(rect_email);
            app.active_box = 2;
            app.message = "Please enter a valid email";
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
            app.message = "";
        }
    }

    if (!reading_text() && app.active_box != 0)
    {
        if (!text_entry_cancelled())
        {
            string input_val = text_input();

            if (app.active_box == 1)
            {
                if (is_valid_name(input_val))
                {
                    app.input_name = input_val;
                    app.message = "Name is valid!";
                }
                else
                {
                    app.message = "Invalid name! Only letters and spaces allowed. Try again.";
                    start_reading_text(rect_name);
                    app.active_box = 1;
                }
            }
            else if (app.active_box == 2)
            {
                if (is_valid_email(input_val))
                {
                    app.input_email = input_val;
                    app.message = "Email is valid!";
                }
                else
                {
                    app.message = "Invalid Email! Must include @ and . Try again.";
                    start_reading_text(rect_email);
                    app.active_box = 2;
                }
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box == 1)
            draw_text(app.input_email, COLOR_BLACK, font_named("Arial"), 18, rect_email.x + 2, rect_email.y);
        else
            draw_text(app.input_name, COLOR_BLACK, font_named("Arial"), 18, rect_name.x + 2, rect_name.y);
    }
    else
    {
        draw_text(app.input_name, COLOR_BLACK, font_named("Arial"), 18, rect_name.x + 2, rect_name.y);
        draw_text(app.input_email, COLOR_BLACK, font_named("Arial"), 18, rect_email.x + 2, rect_email.y);
    }

    draw_text("Press ENTER to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 350);
}

void draw_modify_student(app_data &app, task_manager_data &manager)
{
    rectangle rect_id = rectangle_from(290, 150, 250, 30);
    rectangle rect_name = rectangle_from(290, 200, 250, 30);
    rectangle rect_email = rectangle_from(290, 250, 250, 30);

    clear_screen(COLOR_WHITE);
    draw_text("Modify Student Information", COLOR_BLUE, font_named("Arial"), 26, 200, 80);

    draw_text("Student ID (Search):", COLOR_BLACK, font_named("Arial"), 20, 100, 150);
    draw_text("New Name:", COLOR_BLACK, font_named("Arial"), 20, 100, 200);
    draw_text("New Email:", COLOR_BLACK, font_named("Arial"), 20, 100, 250);

    draw_rectangle(COLOR_BLACK, rect_id);
    draw_rectangle(COLOR_BLACK, rect_name);
    draw_rectangle(COLOR_BLACK, rect_email);

    int sid;
    student_data *student = nullptr;
    if (safe_stoi(app.input_id, sid))
    {
        student = find_student(manager, sid);
    }

    if (student != nullptr)
    {
        draw_text("Found Student: " + student->name + " | " + student->email,
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 100, 300);
        app.message = "Student found! You can edit name/email.";
    }
    else if (!app.input_id.empty())
    {
        app.message = "Student ID not found!";
    }

    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_id))
        {
            start_reading_text(rect_id);
            app.active_box = 1;
        }
        else if (point_in_rectangle(mouse_position(), rect_name))
        {
            start_reading_text(rect_name);
            app.active_box = 2;
        }
        else if (point_in_rectangle(mouse_position(), rect_email))
        {
            start_reading_text(rect_email);
            app.active_box = 3;
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
        }
    }

    if (!reading_text() && app.active_box != 0)
    {
        if (!text_entry_cancelled())
        {
            string val = text_input();
            if (app.active_box == 1)
            {
                if (is_valid_id(val))
                    app.input_id = val;
                else
                    app.message = "Invalid ID! Numbers only.";
            }
            else if (app.active_box == 2)
            {
                if (is_valid_name(val))
                    app.input_name = val;
                else
                    app.message = "Invalid name! Letters only.";
            }
            else if (app.active_box == 3)
            {
                if (is_valid_email(val))
                    app.input_email = val;
                else
                    app.message = "Invalid email! Must contain @ and .";
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box != 1)
            draw_text(app.input_id, COLOR_BLACK, font_named("Arial"), 18, rect_id.x + 5, rect_id.y);
        if (app.active_box != 2)
            draw_text(app.input_name, COLOR_BLACK, font_named("Arial"), 18, rect_name.x + 5, rect_name.y);
        if (app.active_box != 3)
            draw_text(app.input_email, COLOR_BLACK, font_named("Arial"), 18, rect_email.x + 5, rect_email.y);
    }
    else
    {
        draw_text(app.input_id, COLOR_BLACK, font_named("Arial"), 18, rect_id.x + 5, rect_id.y);
        draw_text(app.input_name, COLOR_BLACK, font_named("Arial"), 18, rect_name.x + 5, rect_name.y);
        draw_text(app.input_email, COLOR_BLACK, font_named("Arial"), 18, rect_email.x + 5, rect_email.y);
    }

    draw_text("Press 'Enter' to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 350);
}

void draw_view_students(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);
    draw_text("=== ALL STUDENTS ===", COLOR_BLUE, font_named("Arial"), 24, 200, 80);
    draw_text("ID    | Name           | Email", COLOR_BLACK, font_named("Arial"), 18, 100, 110);
    draw_text("----------------------------------------------------", COLOR_BLACK, font_named("Arial"), 16, 100, 130);

    int y = 150;
    for (int i = 0; i < manager.students.length(); i++)
    {
        student_data s = manager.students[i];
        draw_text(to_string(s.id) + "   | " + s.name + " | " + s.email, COLOR_BLACK, font_named("Arial"), 16, 100, y);
        y += 25;
    }
    draw_text("Total: " + to_string(manager.students.length()), COLOR_RED, font_named("Arial"), 18, 100, y + 10);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_delete_student(app_data &app, task_manager_data &manager)
{
    rectangle rect_id = rectangle_from(260, 160, 200, 30);

    clear_screen(COLOR_WHITE);
    draw_text("Delete Student", COLOR_RED, font_named("Arial"), 26, 250, 100);
    draw_text("Student ID:", COLOR_BLACK, font_named("Arial"), 20, 150, 160);
    draw_rectangle(COLOR_BLACK, rect_id);

    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_id))
        {
            start_reading_text(rect_id);
            app.active_box = 1;
            app.message = "Please enter numeric student ID";
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
            app.message = "";
        }
    }

    if (!reading_text() && app.active_box == 1)
    {
        if (!text_entry_cancelled())
        {
            string val = text_input();
            if (is_valid_id(val))
            {
                app.input_id = val;
                app.message = "ID is valid!";
            }
            else
            {
                app.message = "Invalid ID! Only numbers allowed. Try again.";
                start_reading_text(rect_id);
                app.active_box = 1;
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    }
    else
    {
        draw_text(app.input_id, COLOR_BLACK, font_named("Arial"), 18, rect_id.x + 2, rect_id.y);
    }

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 260);

    if (app.confirm_delete)
    {
        int sid;
        if (safe_stoi(app.input_id, sid))
        {
            student_data *s = find_student(manager, sid);
            if (s != nullptr)
            {
                draw_text("ID: " + to_string(s->id), COLOR_BLACK, font_named("Arial"), 20, 150, 320);
                draw_text("Name: " + s->name, COLOR_BLACK, font_named("Arial"), 20, 150, 350);
                draw_text("Delete? Y/N", COLOR_DARK_RED, font_named("Arial"), 20, 150, 390);
            }
        }
    }

    draw_text("Press 'Enter' to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);
    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
}

void draw_add_task(app_data &app, task_manager_data &manager)
{
    rectangle rect_title = rectangle_from(300, 130, 300, 30);
    rectangle rect_prio = rectangle_from(300, 180, 300, 30);
    rectangle rect_due = rectangle_from(300, 220, 300, 30);
    rectangle rect_assign = rectangle_from(300, 260, 300, 30);

    clear_screen(COLOR_WHITE);
    draw_text("========= Add New Task =========", COLOR_BLUE, font_named("Arial"), 24, 250, 80);
    draw_text("Title:", COLOR_BLACK, font_named("Arial"), 20, 120, 130);
    draw_text("Priority:", COLOR_BLACK, font_named("Arial"), 20, 120, 180);
    draw_text("Due Date:", COLOR_BLACK, font_named("Arial"), 20, 120, 220);
    draw_text("Assign Student IDs:", COLOR_BLACK, font_named("Arial"), 20, 120, 260);
    draw_text("(use ,)", COLOR_BLACK, font_named("Arial"), 16, 250, 282);

    draw_rectangle(COLOR_BLACK, rect_title);
    draw_rectangle(COLOR_BLACK, rect_prio);
    draw_rectangle(COLOR_BLACK, rect_due);
    draw_rectangle(COLOR_BLACK, rect_assign);

    draw_text("======== Student List (ID order) =======", COLOR_DARK_GREEN, font_named("Arial"), 22, 150, 320);
    int x = 150;
    int y = 350;
    for (int i = 0; i < manager.students.length(); i++)
    {
        student_data s = manager.students[i];
        if (i > 4)
        {
            x = 390;
            y = 350;
        }
        if (i > 9)
        {
            x = 630;
            y = 350;
        }

        draw_text("ID: " + to_string(s.id) + " | " + s.name, COLOR_BLACK, font_named("Arial"), 16, x, y);
        y += 22;
    }

    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_title))
        {
            start_reading_text(rect_title);
            app.active_box = 1;
            app.message = "Please enter task title";
        }
        else if (point_in_rectangle(mouse_position(), rect_prio))
        {
            start_reading_text(rect_prio);
            app.active_box = 2;
            app.message = "Please enter priority (High/Medium/Low)";
        }
        else if (point_in_rectangle(mouse_position(), rect_due))
        {
            start_reading_text(rect_due);
            app.active_box = 3;
            app.message = "Please enter due date (e.g. DD/MM/YYYY)";
        }
        else if (point_in_rectangle(mouse_position(), rect_assign))
        {
            start_reading_text(rect_assign);
            app.active_box = 4;
            app.message = "Example: 100,101,102";
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
            app.message = "";
        }
    }

    if (!reading_text() && app.active_box != 0)
    {
        if (!text_entry_cancelled())
        {
            string val = text_input();
            if (app.active_box == 1)
            {
                if (is_valid_text(val))
                {
                    app.input_title = val;
                    app.message = "Title valid!";
                }
                else
                {
                    app.message = "Title cannot be empty!";
                    start_reading_text(rect_title);
                }
            }
            else if (app.active_box == 2)
            {
                if (is_valid_text(val))
                {
                    app.input_priority = val;
                    app.message = "Priority valid!";
                }
                else
                {
                    app.message = "Priority cannot be empty!";
                    start_reading_text(rect_prio);
                }
            }
            else if (app.active_box == 3)
            {
                if (is_valid_date(val))
                {
                    app.input_due = val;
                    app.message = "Date valid!";
                }
                else
                {
                    app.message = "Invalid date! Use numbers and / only.";
                    start_reading_text(rect_due);
                }
            }
            else if (app.active_box == 4)
            {
                if (is_valid_IDs(val))
                {
                    app.input_IDs = val;
                    app.message = "IDs valid!";
                }
                else
                {
                    app.message = "Invalid IDs! Use numbers and , only.";
                    start_reading_text(rect_assign);
                }
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box != 1)
            draw_text(app.input_title, COLOR_BLACK, font_named("Arial"), 18, rect_title.x + 2, rect_title.y);
        if (app.active_box != 2)
            draw_text(app.input_priority, COLOR_BLACK, font_named("Arial"), 18, rect_prio.x + 2, rect_prio.y);
        if (app.active_box != 3)
            draw_text(app.input_due, COLOR_BLACK, font_named("Arial"), 18, rect_due.x + 2, rect_due.y);
        if (app.active_box != 4)
            draw_text(app.input_IDs, COLOR_BLACK, font_named("Arial"), 18, rect_assign.x + 2, rect_assign.y);
    }
    else
    {
        draw_text(app.input_title, COLOR_BLACK, font_named("Arial"), 18, rect_title.x + 2, rect_title.y);
        draw_text(app.input_priority, COLOR_BLACK, font_named("Arial"), 18, rect_prio.x + 2, rect_prio.y);
        draw_text(app.input_due, COLOR_BLACK, font_named("Arial"), 18, rect_due.x + 2, rect_due.y);
        draw_text(app.input_IDs, COLOR_BLACK, font_named("Arial"), 18, rect_assign.x + 2, rect_assign.y);
    }

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    draw_text("Press 'Enter' to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);
    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
}

void draw_view_tasks(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);
    draw_text("=============== ALL TASKS ===============", COLOR_BLUE, font_named("Arial"), 22, 150, 60);
    draw_text("ID | Title               | Priority | Due Date   | Status  | Assigned Students",
              COLOR_BLACK, font_named("Arial"), 16, 50, 110);
    draw_text("-------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, 130);

    int y = 150;
    node<task_data> *curr = manager.tasks.first;

    while (curr != nullptr)
    {
        task_data t = curr->data;

        string assigned = get_assigned_students(t);

        draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + t.status + " | " + assigned,
                  COLOR_BLACK, font_named("Arial"), 16, 50, y);
        y += 25;
        curr = curr->next;
    }
    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

// ---------------- View Student's To-Do Tasks ----------------
void draw_view_student_todo_tasks(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);
    draw_text("=== YOUR TO-DO TASKS ===", COLOR_BLUE, font_named("Arial"), 24, 200, 80);
    draw_text("ID | Title               | Priority | Due Date   | Assigned Students",
              COLOR_BLACK, font_named("Arial"), 16, 50, 110);
    draw_text("-------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, 130);

    int y = 150;
    int count = 0;

    if (!app.has_valid_student_id)
    {
        draw_text("Please enter your student ID in main menu first!", COLOR_RED, font_named("Arial"), 20, 50, 260);
    }
    else
    {
        node<task_data> *curr = manager.tasks.first;
        while (curr != nullptr)
        {
            task_data t = curr->data;
            if (t.status == "To-Do" && is_task_assigned_to(manager, t.id, app.stored_student_id))
            {
                string assigned = get_assigned_students(t);

                draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + assigned,
                          COLOR_DARK_GREEN, font_named("Arial"), 16, 50, y);
                y += 25;
                count++;
            }
            curr = curr->next;
        }
    }

    draw_text("Total To-Do Tasks: " + to_string(count), COLOR_RED, font_named("Arial"), 18, 50, y + 50);

    if (app.has_valid_student_id)
    {
        draw_help_icon(124, 520);
        draw_text("Help Request for Tasks: " + get_help_tasks(manager, app.stored_student_id), COLOR_BLACK, font_named("Arial"), 18, 150, 520);
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

// ---------------- View All Student Tasks ----------------
void draw_view_student_all_tasks(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);
    draw_text("=== ALL YOUR TASKS ===", COLOR_BLUE, font_named("Arial"), 24, 200, 80);
    draw_text("ID | Title               | Priority | Due Date   | Status  | Assigned Students",
              COLOR_BLACK, font_named("Arial"), 16, 50, 110);
    draw_text("--------------------------------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, 130);

    int y = 150;
    int count = 0;

    if (!app.has_valid_student_id)
    {
        draw_text("Please enter your student ID in main menu first!", COLOR_RED, font_named("Arial"), 20, 50, 260);
    }
    else
    {
        node<task_data> *curr = manager.tasks.first;
        while (curr != nullptr)
        {
            task_data t = curr->data;
            if (is_task_assigned_to(manager, t.id, app.stored_student_id))
            {
                string assigned = get_assigned_students(t);

                draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + t.status + " | " + assigned,
                          COLOR_DARK_GREEN, font_named("Arial"), 16, 50, y);
                y += 25;
                count++;
            }
            curr = curr->next;
        }
    }

    draw_text("Total Your Tasks: " + to_string(count), COLOR_RED, font_named("Arial"), 18, 50, y + 30);

    if (app.has_valid_student_id)
    {
        draw_help_icon(124, 520);
        draw_text("Help Request for Tasks: " + get_help_tasks(manager, app.stored_student_id), COLOR_BLACK, font_named("Arial"), 18, 150, 520);
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_search_task_menu(app_data &app)
{
    clear_screen(COLOR_WHITE);
    draw_text("====== SEARCH TASK =======", COLOR_DARK_GREEN, font_named("Arial"), 24, 180, 100);

    draw_text("1. By Title", COLOR_BLACK, font_named("Arial"), 20, 150, 140);
    draw_text("2. By Student ", COLOR_BLACK, font_named("Arial"), 20, 150, 180);
    draw_text("3. By TaskID ", COLOR_BLACK, font_named("Arial"), 20, 150, 220);
    draw_text("4. By Status", COLOR_BLACK, font_named("Arial"), 20, 150, 260);

    if (app.current_role == MANAGER_ROLE)
    {
        draw_text("5. By Priority", COLOR_BLACK, font_named("Arial"), 20, 150, 300);
    }

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_search_by_title(app_data &app, task_manager_data &manager)
{
    rectangle rect_key = rectangle_from(200, 110, 300, 30);
    clear_screen(COLOR_WHITE);
    draw_text("=============== SEARCH BY TITLE ===============", COLOR_BLUE, font_named("Arial"), 22, 50, 80);
    draw_text("Keyword: ", COLOR_BLACK, font_named("Arial"), 18, 75, 112);
    draw_rectangle(COLOR_BLACK, rect_key);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_key))
    {
        start_reading_text(rect_key);
        app.active_box = 1;
        app.message = "Enter search keyword";
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.search_keyword = text_input();
        app.active_box = 0;
        app.message = "";
    }

    if (reading_text())
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.search_keyword, COLOR_BLACK, font_named("Arial"), 18, rect_key.x + 2, rect_key.y);

    draw_text("===============================================", COLOR_BLACK, font_named("Arial"), 22, 50, 150);
    draw_text("ID | Title               | Priority | Due Date  | Status | Assigned Students", COLOR_BLACK, font_named("Arial"), 16, 50, 180);
    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, 200);

    int count = 0;
    int y = 230;
    string key = to_lower(app.search_keyword);
    node<task_data> *curr = manager.tasks.first;

    if (app.search_keyword != "")
    {
        while (curr != nullptr)
        {
            task_data t = curr->data;
            if (to_lower(t.title).find(key) != string::npos)
            {
                string assigned = get_assigned_students(t);

                draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + t.status + " | " + assigned,
                          COLOR_BLACK, font_named("Arial"), 16, 50, y);
                y += 25;
                count++;
            }
            curr = curr->next;
        }
    }
    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, y);
    draw_text("Found " + to_string(count) + " matching task(s).", COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y + 30);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_search_by_student(app_data &app, task_manager_data &manager)
{
    rectangle rect_id = rectangle_from(250, 110, 200, 30);
    clear_screen(COLOR_WHITE);
    draw_text("============== SEARCH BY STUDENT ID===============", COLOR_BLUE, font_named("Arial"), 22, 50, 80);
    draw_text("Student ID: ", COLOR_BLACK, font_named("Arial"), 18, 140, 112);
    draw_rectangle(COLOR_BLACK, rect_id);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_id))
    {
        start_reading_text(rect_id);
        app.active_box = 1;
        app.message = "Enter student ID";
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.search_student = text_input();
        app.active_box = 0;
        app.message = "";
    }

    if (reading_text())
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.search_student, COLOR_BLACK, font_named("Arial"), 18, rect_id.x + 2, rect_id.y);

    draw_text("==================================================", COLOR_BLACK, font_named("Arial"), 22, 50, 140);
    draw_text("ID | Title               | Priority | Due Date  | Status | Assigned Students",
              COLOR_BLACK, font_named("Arial"), 16, 50, 170);
    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, 200);

    int count = 0;
    int y = 230;
    int sid;
    if (!safe_stoi(app.search_student, sid))
    {
        if (app.search_student != "")
            draw_text("Invalid student ID!", COLOR_RED, font_named("Arial"), 18, 50, 245);
    }
    else
    {
        node<task_data> *curr = manager.tasks.first;
        while (curr != nullptr)
        {
            task_data t = curr->data;
            if (is_task_assigned_to(manager, t.id, sid))
            {
                string assigned = get_assigned_students(t);

                draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + t.status + " | " + assigned,
                          COLOR_BLACK, font_named("Arial"), 16, 50, y);
                y += 25;
                count++;
            }
            curr = curr->next;
        }
    }

    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, y);
    draw_text("Found " + to_string(count) + " tasks assigned to Student " + app.search_student + ".",
              COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y + 30);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_search_by_taskid(app_data &app, task_manager_data &manager)
{
    rectangle rect_id = rectangle_from(250, 110, 200, 30);
    clear_screen(COLOR_WHITE);
    draw_text("=============== SEARCH BY TASK ID ===============", COLOR_BLUE, font_named("Arial"), 22, 50, 80);
    draw_text("Task ID: ", COLOR_BLACK, font_named("Arial"), 18, 150, 112);
    draw_rectangle(COLOR_BLACK, rect_id);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_id))
    {
        start_reading_text(rect_id);
        app.active_box = 1;
        app.message = "Enter task ID";
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.search_taskid = text_input();
        app.active_box = 0;
        app.message = "";
    }

    if (reading_text())
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.search_taskid, COLOR_BLACK, font_named("Arial"), 18, rect_id.x + 2, rect_id.y);

    draw_text("=================================================", COLOR_BLACK, font_named("Arial"), 22, 50, 140);
    draw_text("ID | Title               | Priority | Due Date  | Status | Assigned Students", COLOR_BLACK, font_named("Arial"), 16, 50, 170);
    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, 200);

    int count = 0;
    int y = 230;
    int tid;
    if (!safe_stoi(app.search_taskid, tid))
    {
        if (app.search_taskid != "")
            draw_text("Invalid task ID!", COLOR_RED, font_named("Arial"), 18, 50, 310);
    }
    else
    {
        task_data *t = find_task_by_id(manager, tid);
        if (t != nullptr)
        {
            string assigned = get_assigned_students(*t);

            draw_text(to_string(t->id) + "  | " + t->title + " | " + t->priority + " | " + t->due_date + " | " + t->status + " | " + assigned,
                      COLOR_BLACK, font_named("Arial"), 16, 50, y);
            count = 1;
        }
    }

    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, y + 25);
    draw_text("Found " + to_string(count) + " matching task.", COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y + 55);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_filter_status(app_data &app, task_manager_data &manager)
{
    rectangle rect_status = rectangle_from(260, 110, 200, 30);

    clear_screen(COLOR_WHITE);
    draw_text("================ FILTER BY STATUS ===============", COLOR_BLUE, font_named("Arial"), 22, 50, 80);
    draw_text("Status (To-Do / Done): ", COLOR_BLACK, font_named("Arial"), 18, 60, 112);
    draw_rectangle(COLOR_BLACK, rect_status);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_status))
    {
        start_reading_text(rect_status);
        app.active_box = 1;
        app.message = "Enter To-Do or Done";
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.search_status = text_input();
        app.active_box = 0;
        app.message = "";
    }

    if (reading_text())
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.search_status, COLOR_BLACK, font_named("Arial"), 18, rect_status.x + 2, rect_status.y);

    draw_text("================================================", COLOR_BLACK, font_named("Arial"), 22, 50, 160);
    // Split header to highlight "Status" column
    draw_text("ID | Title               | Priority | Due Date  | ", COLOR_BLACK, font_named("Arial"), 16, 50, 190);
    draw_text("Status", COLOR_DARK_GREEN, font_named("Arial"), 16, 480, 190); // Highlight Status header
    draw_text(" | Assigned Students", COLOR_BLACK, font_named("Arial"), 16, 530, 190);
    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, 210);

    int count = 0;
    int y = 240;
    string target_status = to_lower(app.search_status);
    node<task_data> *curr = manager.tasks.first;

    while (curr != nullptr)
    {
        task_data t = curr->data;

        if (to_lower(t.status) == target_status)
        {
            string assigned = get_assigned_students(t);

            // Draw task info before Status
            draw_text(to_string(t.id) + "  | " + t.title + " | " + t.priority + " | " + t.due_date + " | ",
                      COLOR_BLACK, font_named("Arial"), 16, 50, y);
            // Draw highlighted Status value
            draw_text(t.status, COLOR_DARK_GREEN, font_named("Arial"), 16, 480, y);
            // Draw remaining info
            draw_text(" | " + assigned,
                      COLOR_BLACK, font_named("Arial"), 16, 530, y);

            y += 25;
            count++;
        }
        curr = curr->next;
    }

    draw_text("----------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 18, 50, y);
    draw_text("Total " + app.search_status + " Tasks: " + to_string(count),
              COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y + 30);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_filter_prority(app_data &app, task_manager_data &manager)
{
    rectangle rect_p = rectangle_from(350, 110, 200, 30);
    clear_screen(COLOR_WHITE);
    draw_text("============   FILTER BY PRIORITY =============  ", COLOR_BLUE, font_named("Arial"), 22, 50, 80);
    draw_text("Priority to filter (High/Medium/Low): ", COLOR_BLACK, font_named("Arial"), 18, 50, 112);
    draw_rectangle(COLOR_BLACK, rect_p);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_p))
    {
        start_reading_text(rect_p);
        app.active_box = 1;
        app.message = "Enter priority";
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.filter_priority = text_input();
        app.active_box = 0;
        app.message = "";
    }

    if (reading_text())
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.filter_priority, COLOR_BLACK, font_named("Arial"), 18, rect_p.x + 2, rect_p.y);

    draw_text("===============================================", COLOR_BLACK, font_named("Arial"), 22, 50, 150);
    // Highlight "Priority" column in header and rows
    draw_text("ID | Title               | Due Date  | Status | ",
              COLOR_BLACK, font_named("Arial"), 16, 50, 180);
    draw_text("Priority", COLOR_BLUE, font_named("Arial"), 16, 490, 180); // Highlight header
    draw_text(" | Assigned Students",
              COLOR_BLACK, font_named("Arial"), 16, 540, 180);

    draw_text("--------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 22, 50, 200);

    int count = 0;
    int y = 250;
    string p = to_lower(app.filter_priority);
    node<task_data> *curr = manager.tasks.first;

    while (curr != nullptr)
    {
        task_data t = curr->data;
        bool show = true;

        // Filter by student ID if valid
        if (app.has_valid_student_id)
        {
            if (!is_task_assigned_to(manager, t.id, app.stored_student_id))
                show = false;
        }

        if (show && to_lower(t.priority) == p)
        {
            string assigned = get_assigned_students(t);

            // Draw text before Priority
            draw_text(to_string(t.id) + "  | " + t.title + " | " + t.due_date + " | " + t.status + " | ",
                      COLOR_BLACK, font_named("Arial"), 16, 50, y);
            // Draw highlighted Priority value
            draw_text(t.priority, COLOR_BLUE, font_named("Arial"), 16, 490, y);
            // Draw remaining text
            draw_text(" | " + assigned,
                      COLOR_BLACK, font_named("Arial"), 16, 540, y);

            y += 25;
            count++;
        }
        curr = curr->next;
    }

    draw_text("--------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 22, 50, y);
    draw_text("Total " + app.filter_priority + " Priority Tasks: " + to_string(count),
              COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y + 30);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 520);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_sort_by_student(app_data &app, task_manager_data &manager)
{
    clear_screen(COLOR_WHITE);

    draw_text("============ SORT TASKS BY DUE DATE ==============",
              COLOR_BLUE, font_named("Arial"), 22, 80, 60);

    draw_text("------------------------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, 100);
    draw_text("ID    | Title                                                   | Due Date   | Status | Assigned",
              COLOR_BLACK, font_named("Arial"), 16, 50, 120);
    draw_text("------------------------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, 140);

    int y = 170;
    int count = 0;

    // Get sorted task list
    linked_list<task_data> sorted = copy_list(manager.tasks);
    sort_tasks_by_due_date(sorted);
    node<task_data> *curr = sorted.first;

    while (curr != nullptr)
    {
        task_data t = curr->data;
        bool show = true;

        // Filter by student ID if valid
        if (app.has_valid_student_id)
        {
            if (!is_task_assigned_to(manager, t.id, app.stored_student_id))
                show = false;
        }

        if (show)
        {
            string assigned = get_assigned_students(t);

            draw_text(to_string(t.id) + " | " + t.title + " | ",
                      COLOR_DARK_GREEN, font_named("Arial"), 16, 50, y);

            float dateX = 320;
            fill_rectangle(COLOR_LIGHT_YELLOW, dateX, y, 110, 22);
            draw_text(t.due_date, COLOR_DARK_RED, font_named("Arial"), 16, dateX + 2, y + 2);

            draw_text(" | " + t.status + " | " + assigned,
                      COLOR_DARK_GREEN, font_named("Arial"), 16, dateX + 115, y);

            y += 25;
            count++;
        }
        curr = curr->next;
    }

    draw_text("------------------------------------------------------------------------------------------------------------------",
              COLOR_BLACK, font_named("Arial"), 16, 50, y);
    draw_text("Total tasks shown: " + to_string(count), COLOR_DARK_GREEN,
              font_named("Arial"), 18, 50, y + 30);

    if (app.has_valid_student_id)
    {
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);

        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);

    free_list(sorted);
}

// ---------------- Request Help Page for student----------------
void draw_request_help_from_student(app_data &app, task_manager_data &manager)
{
    rectangle rect_taskid = rectangle_from(350, 90, 200, 36);
    rectangle rect_request = rectangle_from(350, 140, 400, 60); // Input box for request
    static bool confirm_mode = false;

    clear_screen(COLOR_WHITE);
    draw_text("====== REQUEST HELP FOR TASK ======", COLOR_BLUE, font_named("Arial"), 22, 220, 50);

    if (!app.has_valid_student_id)
    {
        draw_text("Please enter your student ID first!", COLOR_RED, font_named("Arial"), 22, 120, 200);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
        return;
    }

    // Task ID input
    draw_text("Enter Task ID :", COLOR_BLACK, font_named("Arial"), 20, 150, 95);
    draw_rectangle(COLOR_BLACK, rect_taskid);

    // Request input label
    draw_text("Enter Help Request:", COLOR_BLACK, font_named("Arial"), 20, 150, 145);
    draw_rectangle(COLOR_BLACK, rect_request);

    // Input handling for Task ID
    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_taskid))
        {
            start_reading_text(rect_taskid);
            app.active_box = 88;
            app.message = "";
            confirm_mode = false;
            app.target_task_id = -1;
        }
        else if (point_in_rectangle(mouse_position(), rect_request))
        {
            start_reading_text(rect_request);
            app.active_box = 89;
            app.message = "";
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
        }
    }

    if (!reading_text() && !text_entry_cancelled())
    {
        if (app.active_box == 88)
        {
            string input = text_input();
            app.search_taskid = input;

            if (safe_stoi(input, app.target_task_id))
            {
                confirm_mode = true;
            }
            else
            {
                app.message = "Invalid Task ID!";
                confirm_mode = false;
                app.target_task_id = -1;
            }
            app.active_box = 0;
        }
        else if (app.active_box == 89)
        {
            // Store the entered request temporarily
            app.search_keyword = text_input();
            app.active_box = 0;
        }
    }
    // Draw input text
    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box == 88)
            draw_text(app.search_keyword, COLOR_BLACK, font_named("Arial"), 18, rect_request.x + 6, rect_request.y + 6);
        else
            draw_text(app.search_taskid, COLOR_BLACK, font_named("Arial"), 18, rect_taskid.x + 6, rect_taskid.y + 6);
    }
    else
    {
        draw_text(app.search_taskid, COLOR_BLACK, font_named("Arial"), 18, rect_taskid.x + 6, rect_taskid.y + 6);
        draw_text(app.search_keyword, COLOR_BLACK, font_named("Arial"), 18, rect_request.x + 6, rect_request.y + 6);
    }

    // Show task info if found
    if (confirm_mode && app.target_task_id != -1)
    {
        task_data *task = find_task_by_id(manager, app.target_task_id);
        if (task == nullptr || !is_task_assigned_to(manager, app.target_task_id, app.stored_student_id))
        {
            draw_text("Task not found or not assigned to you!", COLOR_RED, font_named("Arial"), 20, 150, 250);
        }
        else
        {
            // Show task details
            draw_text("Task Found:", COLOR_DARK_GREEN, font_named("Arial"), 20, 150, 200);
            draw_text("ID: " + to_string(task->id), COLOR_BLACK, font_named("Arial"), 18, 150, 230);
            draw_text("Title: " + task->title, COLOR_BLACK, font_named("Arial"), 18, 150, 260);
            draw_text("Status: " + task->status, COLOR_BLACK, font_named("Arial"), 18, 150, 290);

            string assigned = get_assigned_students(*task);
            draw_text("Assigned Students: " + assigned, COLOR_BLACK, font_named("Arial"), 18, 150, 320);

            int idx = find_help_request_index(manager, app.target_task_id, app.stored_student_id);
            if (idx != -1)
            {
                draw_text(string("Request: ") + manager.help_requests[idx].request, COLOR_BLACK, font_named("Arial"), 18, 150, 360);
                draw_text(string("Reply: ") + manager.help_requests[idx].reply, COLOR_BLUE, font_named("Arial"), 18, 150, 390);

                draw_text("Help request exists. You can update your request.", COLOR_ORANGE, font_named("Arial"), 18, 150, 450);
                draw_text("Press ENTER to update", COLOR_RED, font_named("Arial"), 18, 150, 480);
            }
            else
            {
                draw_text("Create new help request with your message.", COLOR_BLUE, font_named("Arial"), 18, 150, 380);
                draw_text("Press ENTER to confirm", COLOR_RED, font_named("Arial"), 18, 150, 410);
            }
        }
    }

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 580);

    if (app.has_valid_student_id)
    {
        draw_text("Help Request for Tasks:" + get_help_tasks(manager, app.stored_student_id), COLOR_BLACK, font_named("Arial"), 18, 150, 520);
        draw_text("Active Student ID: " + to_string(app.stored_student_id),
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
    }
    else
        draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

// ---------------- View All Help Requests  for manager----------------
void draw_view_help_requests(app_data &app, task_manager_data &manager)
{
    rectangle rect_taskid = rectangle_from(280, 120, 200, 30);
    rectangle rect_stuid = rectangle_from(280, 160, 200, 30);

    clear_screen(COLOR_WHITE);
    draw_text("====== VIEW HELP REQUESTS ======", COLOR_BLUE, font_named("Arial"), 22, 230, 80);

    // Draw input labels
    draw_text("Task ID:", COLOR_BLACK, font_named("Arial"), 20, 150, 125);
    draw_text("Student ID:", COLOR_BLACK, font_named("Arial"), 20, 150, 165);

    // Draw input boxes
    draw_rectangle(COLOR_BLACK, rect_taskid);
    draw_rectangle(COLOR_BLACK, rect_stuid);

    // Handle mouse input focus
    if (mouse_clicked(LEFT_BUTTON))
    {
        if (point_in_rectangle(mouse_position(), rect_taskid))
        {
            start_reading_text(rect_taskid);
            app.active_box = 10;
            app.message = "Please enter a valid task id";
        }
        else if (point_in_rectangle(mouse_position(), rect_stuid))
        {
            start_reading_text(rect_stuid);
            app.active_box = 11;
            app.message = "Please enter a valid sutent id";
        }
        else
        {
            end_reading_text();
            app.active_box = 0;
        }
    }

    // Process text input
    if (!reading_text() && app.active_box != 0)
    {
        if (!text_entry_cancelled())
        {
            string input = text_input();
            if (app.active_box == 10)
            {
                if (is_valid_id(input))
                {
                    app.search_taskid = input;
                    app.message = "Task Id is valid!";
                }
                else
                {
                    app.message = "Invalid id! Only number allowed. Try again.";
                    start_reading_text(rect_taskid);
                    app.active_box = 10;
                }
            }
            else if (app.active_box == 11)
            {
                if (is_valid_id(input))
                {
                    app.search_student = input;
                    app.message = "Student Id is valid!";
                }
                else
                {
                    app.message = "Invalid id! Only number allowed. Try again.";
                    start_reading_text(rect_stuid);
                    app.active_box = 11;
                }
            }
        }
        else
        {
            app.active_box = 0;
            app.message = "";
        }
    }

    // Draw text in input boxes
    if (reading_text())
    {
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        if (app.active_box == 10)
            draw_text(app.search_student, COLOR_BLACK, font_named("Arial"), 18, rect_stuid.x + 5, rect_stuid.y + 3);
        else
            draw_text(app.search_taskid, COLOR_BLACK, font_named("Arial"), 18, rect_taskid.x + 5, rect_taskid.y + 3);
    }
    else
    {
        draw_text(app.search_taskid, COLOR_BLACK, font_named("Arial"), 18, rect_taskid.x + 5, rect_taskid.y + 3);
        draw_text(app.search_student, COLOR_BLACK, font_named("Arial"), 18, rect_stuid.x + 5, rect_stuid.y + 3);
    }

    // Display table header with highlighted "Help Request Students" column
    draw_text("========================================================================",
              COLOR_BLACK, font_named("Arial"), 16, 50, 210);
    // Highlight Help Request Students with ORANGE color
    draw_text("ID   | Title                | Priority | Due Date   | Status             | ",
              COLOR_BLACK, font_named("Arial"), 16, 50, 235);
    draw_text("Help Request Students",
              COLOR_DARK_ORANGE, font_named("Arial"), 16, 588, 235); // Highlighted header
    draw_text("========================================================================",
              COLOR_BLACK, font_named("Arial"), 16, 50, 260);

    int y = 290;
    node<task_data> *curr = manager.tasks.first;

    // Iterate all tasks and display details
    while (curr != nullptr)
    {
        task_data t = curr->data;

        // Get help request students string for this task
        string help_students_str = get_help_students_for_task(manager, t.id);

        // ONLY SHOW tasks that HAVE help requests
        if (help_students_str != "No help requests")
        {
            // Draw normal task info
            draw_text(to_string(t.id) + " | " + t.title + " | " + t.priority + " | " + t.due_date + " | " + t.status + " | ",
                      COLOR_BLACK, font_named("Arial"), 16, 50, y);
            // Draw highlighted student list
            draw_text(help_students_str,
                      COLOR_DARK_ORANGE, font_named("Arial"), 16, 588, y);

            y += 25;
        }

        curr = curr->next;
    }
    draw_text("Press ENTER to details", COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 550);
    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);

    if (!app.message.empty())
        draw_text(app.message, COLOR_RED, font_named("Arial"), 18, 150, 580);
}

// ---------------- View Help Request Detail & Reply ----------------
void draw_help_detail_for_reply(app_data &app, task_manager_data &manager)
{
    rectangle rect_reply = rectangle_from(220, 300, 400, 80);

    clear_screen(COLOR_WHITE);
    draw_text("====== HELP REQUEST DETAIL ======", COLOR_BLUE, font_named("Arial"), 22, 220, 80);

    int tid, sid;
    bool valid_task = safe_stoi(app.search_taskid, tid);
    bool valid_stu = safe_stoi(app.search_student, sid);

    // Get help request index
    int help_index = -1;
    if (valid_task && valid_stu)
    {
        help_index = find_help_request_index(manager, tid, sid);
    }

    if (help_index == -1)
    {
        draw_text("Help request not found!", COLOR_RED, font_named("Arial"), 22, 150, 150);
    }
    else
    {
        // Access help data by index
        help_data &help = manager.help_requests[help_index];

        // Show full help request info
        draw_text("Task ID: " + to_string(help.task_id), COLOR_BLACK, font_named("Arial"), 20, 150, 130);
        draw_text("Student ID: " + to_string(help.student_id), COLOR_BLACK, font_named("Arial"), 20, 150, 160);
        draw_text(string("Status: ") + help.status, COLOR_DARK_RED, font_named("Arial"), 20, 150, 190);
        draw_text(string("Request: ") + help.request, COLOR_BLUE, font_named("Arial"), 20, 150, 240);
        draw_text("Reply: ", COLOR_BLACK, font_named("Arial"), 20, 150, 302);

        // Draw reply input box
        draw_rectangle(COLOR_BLACK, rect_reply);

        // Mouse click to edit reply
        if (mouse_clicked(LEFT_BUTTON))
        {
            if (point_in_rectangle(mouse_position(), rect_reply))
            {
                start_reading_text(rect_reply);
                app.active_box = 20;
            }
            else
            {
                end_reading_text();
                app.active_box = 0;
            }
        }

        // Process reply input
        if (!reading_text() && app.active_box == 20 && !text_entry_cancelled())
        {
            strcpy_s(help.reply, text_input().c_str());
            strcpy_s(help.status, "replied"); // auto update status

            // ===================== Send Reply to Student =====================
            send_reply_to_student(help);

            app.message = "Reply saved!";
            app.active_box = 0;
        }

        // Draw text
        if (reading_text())
        {
            draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
        }
        else
        {
            draw_text(help.reply, COLOR_BLACK, font_named("Arial"), 18, rect_reply.x + 5, rect_reply.y + 5);
        }

        draw_text("Press ENTER to save reply", COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 470);
    }

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);

    if (!app.message.empty())
        draw_text(app.message, COLOR_DARK_GREEN, font_named("Arial"), 18, 150, 520);
}

void draw_update_task_menu(app_data &app)
{
    clear_screen(COLOR_WHITE);
    draw_text("====== UPDATE TASK ======", COLOR_DARK_GREEN, font_named("Arial"), 24, 180, 100);

    draw_text("1. Update Status ", COLOR_BLACK, font_named("Arial"), 20, 150, 140);
    draw_text("2. Update Assigned Student ", COLOR_BLACK, font_named("Arial"), 20, 150, 180);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 150, 550);
}

void draw_update_task_status(app_data &app, task_manager_data &manager)
{
    rectangle rect_task = rectangle_from(150, 140, 250, 30);
    rectangle rect_status = rectangle_from(300, 350, 250, 30);

    clear_screen(COLOR_WHITE);
    draw_text("============= UPDATE TASK STATUS =============", COLOR_BLUE, font_named("Arial"), 22, 100, 80);

    draw_text("Task ID: ", COLOR_BLACK, font_named("Arial"), 20, 50, 140);
    draw_rectangle(COLOR_BLACK, rect_task);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_task))
    {
        start_reading_text(rect_task);
        app.active_box = 1;
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.input_id = text_input();
        app.active_box = 0;
    }

    if (reading_text() && app.active_box == 1)
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.input_id, COLOR_BLACK, font_named("Arial"), 18, rect_task.x + 2, rect_task.y);

    draw_text("-------------------------------------------------------------", COLOR_BLACK, font_named("Arial"), 16, 50, 190);
    draw_text("ID | Title               | Current Status", COLOR_BLACK, font_named("Arial"), 16, 50, 210);
    draw_text("-------------------------------------------------------------", COLOR_BLACK, font_named("Arial"), 16, 50, 230);

    int y = 260;
    int tid;
    task_data *target = nullptr;
    if (safe_stoi(app.input_id, tid))
        target = find_task_by_id(manager, tid);

    if (target != nullptr)
    {
        draw_text(to_string(target->id) + " | " + target->title + " | " + target->status,
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y);
    }
    else if (!app.input_id.empty())
    {
        draw_text("Task not found!", COLOR_RED, font_named("Arial"), 18, 50, y);
    }

    if (target != nullptr)
    {
        draw_text("New Status (To-Do / Done): ", COLOR_BLACK, font_named("Arial"), 20, 50, 350);
        draw_rectangle(COLOR_BLACK, rect_status);

        if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_status))
        {
            start_reading_text(rect_status);
            app.active_box = 2;
        }

        if (!reading_text() && app.active_box == 2 && !text_entry_cancelled())
        {
            string new_status = text_input();

            if (new_status == "todo" || new_status == "Todo" || new_status == "To-Do")
                new_status = "To-Do";
            else if (new_status == "done" || new_status == "Done")
                new_status = "Done";

            if (new_status == "To-Do" || new_status == "Done")
            {
                target->status = new_status;
                app.message = "Status updated successfully!";
            }
            app.active_box = 0;
        }

        if (reading_text() && app.active_box == 2)
            draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    }

    draw_text("Press 'Enter' to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);

    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
}

void draw_update_task_students(app_data &app, task_manager_data &manager)
{
    rectangle rect_task = rectangle_from(150, 140, 250, 30);
    rectangle rect_stu = rectangle_from(300, 350, 250, 30);

    clear_screen(COLOR_WHITE);
    draw_text("============= UPDATE ASSIGNED STUDENTS =============", COLOR_BLUE, font_named("Arial"), 22, 100, 80);

    draw_text("Task ID: ", COLOR_BLACK, font_named("Arial"), 20, 50, 140);
    draw_rectangle(COLOR_BLACK, rect_task);

    if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_task))
    {
        start_reading_text(rect_task);
        app.active_box = 1;
    }

    if (!reading_text() && app.active_box == 1 && !text_entry_cancelled())
    {
        app.input_id = text_input();
        app.active_box = 0;
    }

    if (reading_text() && app.active_box == 1)
        draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    else
        draw_text(app.input_id, COLOR_BLACK, font_named("Arial"), 18, rect_task.x + 2, rect_task.y);

    draw_text("---------------------------------------------------------", COLOR_BLACK, font_named("Arial"), 16, 50, 190);
    draw_text("ID | Title               | Assigned Students", COLOR_BLACK, font_named("Arial"), 16, 50, 210);
    draw_text("---------------------------------------------------------", COLOR_BLACK, font_named("Arial"), 16, 50, 230);

    int y = 260;
    int tid;
    task_data *target = nullptr;
    if (safe_stoi(app.input_id, tid))
        target = find_task_by_id(manager, tid);

    if (target != nullptr)
    {
        string assigned = get_assigned_students(*target);
        draw_text(to_string(target->id) + " | " + target->title + " | " + assigned,
                  COLOR_DARK_GREEN, font_named("Arial"), 18, 50, y);
    }
    else if (!app.input_id.empty())
    {
        draw_text("Task not found!", COLOR_RED, font_named("Arial"), 18, 50, y);
    }

    if (target != nullptr)
    {
        draw_text("Student ID to Add/Remove:", COLOR_BLACK, font_named("Arial"), 20, 50, 350);
        draw_rectangle(COLOR_BLACK, rect_stu);

        if (mouse_clicked(LEFT_BUTTON) && point_in_rectangle(mouse_position(), rect_stu))
        {
            start_reading_text(rect_stu);
            app.active_box = 2;
        }

        if (!reading_text() && app.active_box == 2 && !text_entry_cancelled())
        {
            string sid_str = text_input();
            int sid;
            if (safe_stoi(sid_str, sid))
            {
                if (is_student_assigned_to_task(manager, sid, target->id))
                    remove_student_from_task(manager, target->id, sid);
                else
                    assign_student_to_task(manager, sid, target->id);
            }
            app.active_box = 0;
        }

        if (reading_text() && app.active_box == 2)
            draw_collected_text(COLOR_BLACK, font_named("Arial"), 18, option_defaults());
    }

    draw_text("Press 'Enter' to confirm", COLOR_RED, font_named("Arial"), 18, 150, 550);
    draw_text("Press SPACE to return", COLOR_GRAY, font_named("Arial"), 18, 380, 550);
}

// --------------------------------------------------------
void draw_app(app_data &app, task_manager_data &manager)
{
    switch (app.state)
    {
    case LOGIN_SCREEN:
        draw_login_screen(app);
        break;
    case MAIN_MENU:
        draw_main_menu(app);
        break;
    case STUDENT_MAIN_MENU:
        draw_student_main_menu(app, manager);
        break;
    case STUDENT_MENU:
        draw_student_menu(app);
        break;
    case ADD_STUDENT:
        draw_add_student(app);
        break;
    case MODIFY_STUDENT:
        draw_modify_student(app, manager);
        break;
    case VIEW_STUDENTS:
        draw_view_students(app, manager);
        break;
    case DELETE_STUDENT:
        draw_delete_student(app, manager);
        break;
    case ADD_TASK:
        draw_add_task(app, manager);
        break;
    case VIEW_TASKS:
        draw_view_tasks(app, manager);
        break;
    case VIEW_STUDENT_TODO_TASKS:
        draw_view_student_todo_tasks(app, manager);
        break;
    case VIEW_STUDENT_ALL_TASKS:
        draw_view_student_all_tasks(app, manager);
        break;
    case REQUEST_HELP:
        draw_request_help_from_student(app, manager);
        break;
    case SEARCH_TASK:
        draw_search_task_menu(app);
        break;
    case SEARCH_BY_TITLE:
        draw_search_by_title(app, manager);
        break;
    case SEARCH_BY_STUDENT:
        draw_search_by_student(app, manager);
        break;
    case SEARCH_BY_TASKID:
        draw_search_by_taskid(app, manager);
        break;
    case FILTER_STATUS:
        draw_filter_status(app, manager);
        break;
    case FILTER_PROROTY:
        draw_filter_prority(app, manager);
        break;
    case SORT_BY_STUDENT:
        draw_sort_by_student(app, manager);
        break;
    case UPDATE_TASK_MENU:
        draw_update_task_menu(app);
        break;
    case UPDATE_TASK_STATUS:
        draw_update_task_status(app, manager);
        break;
    case UPDATE_TASK_STUDENTS:
        draw_update_task_students(app, manager);
        break;
    case VIEW_HELP_REPLY:
        draw_view_help_requests(app, manager);
        break;
    case VIEW_HELP_DETAIL:
        draw_help_detail_for_reply(app, manager);
        break;
    default:
        break;
    }
}

// ---------------- UPDATE FUNCTIONS ----------------
void update_login_screen(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == RETURN_KEY)
    {
        user_role role;
        if (validate_login(app.login_username, app.login_password, role))
        {
            app.current_role = role;
            if (app.current_role == STUDENT_ROLE)
            {
                app.state = STUDENT_MAIN_MENU;
                app.message = "Student login - Data loaded!";
            }
            else
            {
                app.state = MAIN_MENU;
                app.message = "Manager login successful!";
            }
            end_reading_text();
        }
        else
        {
            if ((app.login_username != "") && (app.login_password != ""))
                app.message = "Invalid username or password!";
        }
    }
    if (key == ESCAPE_KEY)
    {
        app.is_quit = true;
    }
}

void update_main_menu(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == NUM_1_KEY)
        app.state = STUDENT_MENU;
    if (key == NUM_2_KEY)
    {
        app.input_title = "";
        app.input_priority = "";
        app.input_due = "";
        app.message = "";
        app.active_box = 0;
        app.state = ADD_TASK;
    }
    if (key == NUM_3_KEY)
        app.state = VIEW_TASKS;
    if (key == NUM_4_KEY)
        app.state = SEARCH_TASK;
    if (key == NUM_5_KEY)
        app.state = UPDATE_TASK_MENU;
    if (key == NUM_6_KEY)
        app.state = SORT_BY_STUDENT;
    if (key == NUM_7_KEY)
    {
        app.search_taskid = "";
        app.search_student = "";
        app.message = "";
        app.active_box = 0;
        app.state = VIEW_HELP_REPLY;
    }
    if (key == NUM_8_KEY)
        app.is_quit = true;
}

// ---------------- Update Student Main Menu ----------------
void update_student_main_menu(app_data &app, key_code key)
{
    if (app.active_box == 0)
    {
        if (key == NUM_1_KEY)
            app.state = VIEW_STUDENT_TODO_TASKS;
        if (key == NUM_2_KEY)
            app.state = VIEW_STUDENT_ALL_TASKS;
        if (key == NUM_3_KEY)
        {
            app.search_taskid = "";
            app.message = "";
            app.state = REQUEST_HELP;
        }
        if (key == NUM_4_KEY)
            app.state = SEARCH_TASK;
        if (key == NUM_5_KEY)
            app.state = FILTER_PROROTY;
        if (key == NUM_6_KEY)
            app.state = SORT_BY_STUDENT;
        if (key == NUM_7_KEY)
            app.is_quit = true;
    }
}

void update_student_menu(app_data &app, key_code key)
{
    if (app.current_role == STUDENT_ROLE)
    {
        if (key == NUM_1_KEY || key == NUM_3_KEY)
        {
            app.message = "Access denied! Student mode is read-only.";
            return;
        }
    }

    if (key == NUM_1_KEY)
    {
        app.input_name = "";
        app.input_email = "";
        app.message = "";
        app.active_box = 0;
        app.state = ADD_STUDENT;
    }
    if (key == NUM_2_KEY)
        app.state = VIEW_STUDENTS;
    if (key == NUM_3_KEY)
    {
        app.input_id = "";
        app.input_name = "";
        app.input_email = "";
        app.message = "";
        app.active_box = 0;
        app.state = MODIFY_STUDENT;
    }

    if (key == NUM_4_KEY)
    {
        app.input_id = "";
        app.message = "";
        app.confirm_delete = false;
        app.active_box = 0;
        app.state = DELETE_STUDENT;
    }
    if (key == SPACE_KEY)
    {
        app.state = MAIN_MENU;
        app.active_box = 0;
        end_reading_text();
    }
}

void update_modify_student(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == RETURN_KEY)
    {
        int sid;
        if (!safe_stoi(app.input_id, sid))
        {
            app.message = "Please enter a valid student ID!";
            return;
        }

        student_data *s = find_student(manager, sid);
        if (s == nullptr)
        {
            app.input_id = "";
            app.message = "Student not found!";
            return;
        }

        if (app.active_box == 2 && app.input_name.empty())
            return;

        if (app.active_box == 3 && app.input_email.empty())
            return;

        if (!app.input_name.empty() && is_valid_name(app.input_name))
        {
            s->name = app.input_name;
            app.message = "Student info updated successfully!";
            app.state = STUDENT_MENU;
            end_reading_text();
        }

        if (!app.input_email.empty() && is_valid_email(app.input_email) && app.active_box != 2)
        {
            s->email = app.input_email;
            app.message = "Student info updated successfully!";
            app.state = STUDENT_MENU;
            end_reading_text();
        }
    }

    if ((key == SPACE_KEY) && (app.active_box != 2))
    {
        app.state = STUDENT_MENU;
        app.active_box = 0;
        app.message = "";
        end_reading_text();
    }
}

void update_add_student(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == RETURN_KEY)
    {
        if (is_valid_name(app.input_name) && is_valid_email(app.input_email))
        {
            student_data s = new_student(manager, app.input_name, app.input_email);
            add_student(manager, s);
            app.message = "Student added successfully!";
            app.state = STUDENT_MENU;
            app.active_box = 0;
            end_reading_text();
        }
        else
            app.message = "Please fill all fields correctly!";
    }
    if ((key == SPACE_KEY) && (app.active_box != 1))
    {
        app.state = STUDENT_MENU;
        app.active_box = 0;
        end_reading_text();
    }
}

void update_delete_student(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == RETURN_KEY)
    {
        if (!app.confirm_delete)
        {
            int sid;
            if (!safe_stoi(app.input_id, sid))
                app.message = "Error: Invalid ID format!";
            else if (find_student(manager, sid) == nullptr)
                app.message = "Error: Student not found!";
            else
            {
                app.confirm_delete = true;
                app.message = "";
            }
        }
    }
    if (app.confirm_delete)
    {
        if (key == Y_KEY)
        {
            remove_student(manager, stoi(app.input_id));
            app.message = "Student deleted!";
            app.state = STUDENT_MENU;
            app.confirm_delete = false;
            app.active_box = 0;
            end_reading_text();
        }
        if (key == N_KEY)
        {
            app.state = STUDENT_MENU;
            app.confirm_delete = false;
            app.active_box = 0;
            end_reading_text();
        }
    }
    if (key == SPACE_KEY)
    {
        app.state = STUDENT_MENU;
        app.active_box = 0;
        app.message = "";
        end_reading_text();
    }
}

void update_add_task(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == RETURN_KEY)
    {
        if (is_valid_text(app.input_title) && is_valid_text(app.input_priority) && is_valid_date(app.input_due))
        {
            int task_id = generate_unique_task_id(manager);
            task_data t = new_task(task_id, app.input_title, app.input_priority, app.input_due, "To-Do");
            add_task(manager, t);
            parse_and_assign_students(manager, task_id, app.input_IDs);

            app.message = "Task added with assigned students!";
            app.state = MAIN_MENU;
            app.active_box = 0;
            end_reading_text();
            app.input_title = "";
            app.input_priority = "";
            app.input_due = "";
            app.input_id = "";
            app.input_IDs = "";
        }
        else
            app.message = "Fill all fields correctly!";
    }
    if ((key == SPACE_KEY) && (app.active_box != 1))
    {
        app.state = MAIN_MENU;
        app.active_box = 0;
        app.message = "";
        end_reading_text();
    }
}

void update_search_task(app_data &app, key_code key)
{
    if (key == NUM_1_KEY)
        app.state = SEARCH_BY_TITLE;
    if (key == NUM_2_KEY)
        app.state = SEARCH_BY_STUDENT;
    if (key == NUM_3_KEY)
        app.state = SEARCH_BY_TASKID;
    if (key == NUM_4_KEY)
        app.state = FILTER_STATUS;

    if (app.current_role == MANAGER_ROLE)
    {
        if (key == NUM_5_KEY)
            app.state = FILTER_PROROTY;
    }

    if (key == SPACE_KEY)
    {
        if (app.current_role == STUDENT_ROLE)
            app.state = STUDENT_MAIN_MENU;
        else
            app.state = MAIN_MENU;
    }
}

void update_search_submenus(app_data &app, key_code key)
{
    if (key == SPACE_KEY)
    {
        if (app.current_role == STUDENT_ROLE)
            app.state = STUDENT_MAIN_MENU;
        else
            app.state = SEARCH_TASK;
        app.search_keyword = "";
        app.search_student = "";
        app.search_taskid = "";
        app.search_status = "";
        app.filter_priority = "";
        app.active_box = 0;
        app.message = "";
        end_reading_text();
    }
}

void update_sort_by_student(app_data &app, key_code key)
{
    if (key == SPACE_KEY)
    {
        if (app.current_role == STUDENT_ROLE)
            app.state = STUDENT_MAIN_MENU;
        else
            app.state = MAIN_MENU;

        app.search_student = "";
        app.active_box = 0;
        app.message = "";

        end_reading_text();
    }
}

void update_update_task_menu(app_data &app, key_code key)
{
    if (key == NUM_1_KEY)
    {
        app.input_id = "";
        app.state = UPDATE_TASK_STATUS;
    }
    if (key == NUM_2_KEY)
    {
        app.input_id = "";
        app.state = UPDATE_TASK_STUDENTS;
    }
    if (key == SPACE_KEY)
    {
        app.state = MAIN_MENU;
        app.active_box = 0;
        app.message = "";
        end_reading_text();
    }
}

void update_update_task_status(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == SPACE_KEY)
    {
        app.state = UPDATE_TASK_MENU;
        app.input_id = "";
        app.active_box = 0;
        app.message = "";
        end_reading_text();
        return;
    }

    if (key == RETURN_KEY)
    {
        int tid;
        if (safe_stoi(app.input_id, tid))
        {
            task_data *t = find_task_by_id(manager, tid);
            if (t != nullptr)
            {
                if (t->status == "To-Do")
                    t->status = "Done";
                else
                    t->status = "To-Do";
            }
        }
    }
}

void update_update_task_students(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == SPACE_KEY)
    {
        app.state = UPDATE_TASK_MENU;
        app.input_id = "";
        app.active_box = 0;
        app.message = "";
        end_reading_text();
        return;
    }
}

// ---------------- Update Request Help ----------------
void update_request_help(app_data &app, task_manager_data &manager, key_code key)
{
    if (key == SPACE_KEY && app.active_box != 89)
    {
        app.state = STUDENT_MAIN_MENU;
        app.active_box = 0;
        app.search_taskid = "";
        app.search_keyword = ""; // Clear request input
        app.message = "";
        app.target_task_id = -1;
        end_reading_text();
        return;
    }

    if (!app.has_valid_student_id)
        return;

    if (key == RETURN_KEY && app.target_task_id != -1)
    {
        bool exists = has_help_request(manager, app.target_task_id, app.stored_student_id);
        int idx = find_help_request_index(manager, app.target_task_id, app.stored_student_id);

        if (exists)
        {
            // Update existing request and set status to "inquired"
            if (idx != -1)
            {
                help_data &h = manager.help_requests[idx];
                if (!app.search_keyword.empty())
                {
                    strcpy_s(h.request, app.search_keyword.c_str());
                    strcpy_s(h.reply, "");
                }
                strcpy_s(h.status, "inquired");
                app.message = "Help request updated successfully!";

                // ===================== Send to Manager via WM_COPYDATA =====================
                send_help_to_manager(manager.help_requests[idx]);
            }
        }
        else
        {
            // Add new help request with student's message
            add_help_request(manager, app.target_task_id, app.stored_student_id);

            // Set the request text for the newly added help request
            idx = find_help_request_index(manager, app.target_task_id, app.stored_student_id);
            if (idx != -1)
            {
                help_data &h = manager.help_requests[idx];

                strcpy_s(h.request, app.search_keyword.c_str());
                strcpy_s(h.status, "inquired");
            }
            app.message = "Help request sent successfully!";

            // ===================== Send to Manager =====================
            send_help_to_manager(manager.help_requests[idx]);
        }
    }
}

void update_help_reply(app_data &app, task_manager_data &manager, key_code key)
{
    // Press ENTER to view detail
    if (key == RETURN_KEY)
    {
        if (is_valid_id(app.search_taskid) && is_valid_id(app.search_student))
        {
            app.state = VIEW_HELP_DETAIL;
            app.message = "";
            app.active_box = 0;
            end_reading_text();
        }
    }
    if (key == SPACE_KEY)
    {
        app.state = MAIN_MENU;
        app.search_taskid = "";
        app.search_student = "";
        app.active_box = 0;
        end_reading_text();
    }
}
void update_app(app_data &app, task_manager_data &manager, key_code key)
{
    switch (app.state)
    {
    case LOGIN_SCREEN:
        update_login_screen(app, manager, key);
        break;
    case MAIN_MENU:
        update_main_menu(app, manager, key);
        break;
    case STUDENT_MAIN_MENU:
        update_student_main_menu(app, key);
        break;
    case STUDENT_MENU:
        update_student_menu(app, key);
        break;
    case ADD_STUDENT:
        update_add_student(app, manager, key);
        break;
    case MODIFY_STUDENT:
        update_modify_student(app, manager, key);
        break;
    case DELETE_STUDENT:
        update_delete_student(app, manager, key);
        break;
    case ADD_TASK:
        update_add_task(app, manager, key);
        break;
    case SEARCH_TASK:
        update_search_task(app, key);
        break;
    case REQUEST_HELP:
        update_request_help(app, manager, key);
        break;
    case SEARCH_BY_TITLE:
    case SEARCH_BY_STUDENT:
    case SEARCH_BY_TASKID:
    case FILTER_STATUS:
    case FILTER_PROROTY:
        update_search_submenus(app, key);
        break;
    case SORT_BY_STUDENT:
        update_sort_by_student(app, key);
        break;
    case UPDATE_TASK_MENU:
        update_update_task_menu(app, key);
        break;
    case UPDATE_TASK_STATUS:
        update_update_task_status(app, manager, key);
        break;
    case UPDATE_TASK_STUDENTS:
        update_update_task_students(app, manager, key);
        break;
    case VIEW_STUDENTS:
    case VIEW_STUDENT_TODO_TASKS:
    case VIEW_STUDENT_ALL_TASKS:
        if (key == SPACE_KEY)
        {
            if (app.current_role == STUDENT_ROLE)
                app.state = STUDENT_MAIN_MENU;
            else
                app.state = STUDENT_MENU;
            app.message = "";
        }
        break;
    case VIEW_TASKS:
        if (key == SPACE_KEY)
        {
            if (app.current_role == STUDENT_ROLE)
                app.state = STUDENT_MAIN_MENU;
            else
                app.state = MAIN_MENU;
            app.message = "";
        }
        break;
    case VIEW_HELP_REPLY:
        update_help_reply(app, manager, key);
        break;
    case VIEW_HELP_DETAIL:
        if (key == SPACE_KEY && app.active_box != 20)
        {
            app.state = VIEW_HELP_REPLY;
            app.search_taskid = "";
            app.search_student = "";
            app.message = "";
            app.active_box = 0;
            end_reading_text();
        }
        break;
    default:
        break;
    }
}

key_code get_input_key()
{
    if (key_typed(SPACE_KEY))
        return SPACE_KEY;
    if (key_typed(RETURN_KEY))
        return RETURN_KEY;
    if (key_typed(KEYPAD_ENTER))
        return RETURN_KEY;
    if (key_typed(NUM_1_KEY))
        return NUM_1_KEY;
    if (key_typed(NUM_2_KEY))
        return NUM_2_KEY;
    if (key_typed(NUM_3_KEY))
        return NUM_3_KEY;
    if (key_typed(NUM_4_KEY))
        return NUM_4_KEY;
    if (key_typed(NUM_5_KEY))
        return NUM_5_KEY;
    if (key_typed(NUM_6_KEY))
        return NUM_6_KEY;
    if (key_typed(NUM_7_KEY))
        return NUM_7_KEY;
    if (key_typed(NUM_8_KEY))
        return NUM_8_KEY;
    if (key_typed(NUM_9_KEY))
        return NUM_9_KEY;
    if (key_typed(Y_KEY))
        return Y_KEY;
    if (key_typed(N_KEY))
        return N_KEY;
    if (key_typed(ESCAPE_KEY))
        return ESCAPE_KEY;
    return UNKNOWN_KEY;
}

int main()
{
    app_data app;
    task_manager_data manager;
    key_code input_key;

    open_window(WINDOW_TITLE, SCREEN_WIDTH, SCREEN_HEIGHT);

    load_font("Arial", "arial.ttf");
    new_app(app, manager);
    load_all_data(manager);

    // ===================== Init IPC =====================
    g_manager = &manager;
    g_app = &app;
    init_ipc();

    while (!quit_requested() && !app.is_quit)
    {
        process_events();
        input_key = get_input_key();

        draw_app(app, manager);

        if (input_key != UNKNOWN_KEY)
            update_app(app, manager, input_key);

        refresh_screen(60);
    }

    save_all_data(manager);
    close_all_windows();

    return 0;
}