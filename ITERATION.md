# Iteration record

## Purpose

This repository is a personal C++ task-management project prepared as an internship portfolio. It is intentionally presented as an iteration: the first public version establishes the baseline, and the second version records focused engineering improvements.

## Version layout

- `original-version/` keeps the cleaned public snapshot of the original implementation. Its source structure and behavior are preserved; generated executables, logs, and course-specific references were removed for public presentation.
- `optimized-version/` contains the improved implementation. It is separated from the baseline so that the changes can be reviewed directly.

The untouched source snapshot is retained locally in `original/`.

## Improvements in the optimized version

1. Fixed linked-list predecessor traversal. The original `find_previous_node` routine advanced through `current->data`, which is a data object rather than the next node. The optimized version correctly follows `current->next`.
2. Removed generated binaries and logs from the project source tree so the repository contains reviewable source, tests, and sample data.
3. Replaced course-specific sample wording with neutral project examples, making the repository suitable for a personal portfolio.
4. Added clearer documentation for build requirements, feature scope, and the planned engineering direction.

## Verification

The existing Catch2 test suite is retained in both versions. Compilation and test execution require a local C++20 compiler and SplashKit installation, as described in each version's README.

## Next iteration

Future work will consider stronger input validation, clearer ownership rules for dynamic data, and a portable build configuration. Those changes will be reviewed separately so each iteration remains easy to understand.
