# Compiler and build configuration
cc := "gcc"
cflags := "-Wall -Wextra -Werror -std=c2x -D_POSIX_C_SOURCE=199309L -O3 -I" + srcdir + "/include"
ldflags := "-lncurses"
test_ldflags := "-lcheck"
gcov_flags := "-fprofile-arcs -ftest-coverage"

# Project structure
srcdir := "src"
includedir := srcdir + "/include"
bin := "tetris"

# Source files
tetris_src := srcdir + "/tetris.c"
main_src := srcdir + "/main.c"
srcs := tetris_src + " " + main_src

# Test configuration
test_src := "tests/test_tetris.c"
test_bin := "tetris_test"

# Coverage reporting
coverage_info := "coverage.info"
coverage_dir := "coverage_report"

os_name := os()
open_cmd := if os_name == "macos" {
    "open"
} else if os_name == "windows" {
    "start"
} else {
    "xdg-open"
}

# Default build target
default: install

# Build main executable
install:
    {{cc}} {{cflags}} {{srcs}} -o {{bin}} {{ldflags}}

# Clean build artifacts
clean:
    rm -rf {{bin}} {{test_bin}} *.gcda *.gcno {{coverage_dir}} build *.info highscore.txt

# Run tests
test: build-tests
    ./{{test_bin}}

# Build test executable
build-tests:
    {{cc}} {{cflags}} {{test_src}} {{tetris_src}} -o {{test_bin}} {{ldflags}} {{test_ldflags}}

# Generate coverage report
gcov-report:
    {{cc}} {{cflags}} {{gcov_flags}} {{test_src}} {{tetris_src}} -o {{test_bin}} {{ldflags}} {{test_ldflags}}
    ./{{test_bin}}
    lcov --capture --directory . --output-file {{coverage_info}}
    genhtml {{coverage_info}} --output-directory {{coverage_dir}}
    {{open_cmd}} {{coverage_dir}}/index.html

# Run memory checks
valgrind: build-tests
    valgrind --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all ./{{test_bin}}

# Lint code
lint:
    clang-format --dry-run --Werror {{srcs}} {{test_src}} {{includedir}}/*.h

# Format code
fmt:
    clang-format -i {{srcs}} {{test_src}} {{includedir}}/*.h
