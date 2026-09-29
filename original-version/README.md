Separate compilation
 
clang++ student_manager.cpp task_manager.cpp main.cpp -DMANAGER_BUILD -std=c++20 -Wall -lmingw32 -mwindows -lSplashKit -o Manager.exe

clang++ student_manager.cpp task_manager.cpp main.cpp -DSTUDENT_BUILD -std=c++20 -Wall -lmingw32 -mwindows -lSplashKit -o Student.exe

clang++ student_manager.cpp task_manager.cpp task_manager_test.cpp catch_amalgamated.cpp  -l splashkit -std=c++20 -o test -Wall
./test.exe