# Task Manager Portfolio

This repository records the first iteration of a personal C++ task-management project. It is being prepared as a practical portfolio piece for future internship applications, with emphasis on readable code, data handling, testing, and incremental improvement.

## Iterations

- `original-version/` — a cleaned public copy of the original implementation. It preserves the original program structure and behavior while removing local build outputs and course-specific references.
- `optimized-version/` contains the improved implementation for side-by-side review. The working copy remains locally in `optimized/` for continued development.

The complete untouched snapshot is retained locally in `original/` and is not modified.

## Features

- Manager and student views with role-based access.
- Task creation, status updates, filtering, searching, and sorting.
- Student-task assignment with bidirectional links.
- CSV persistence for students, tasks, and help requests.
- Automated tests for core data-management behavior.

## Build

The project uses C++20 and SplashKit. See `original-version/README.md` for the original build commands and required local environment.

## Iteration plan

Future iterations will focus on stronger input validation, clearer ownership rules, and portable build configuration.
