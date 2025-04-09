#include <vfs.h>
#include <print.h>
#include <string.h>
#include <stdbool.h>

static bool check_result(int result, int expected, const char* test_name) {
    if (result == expected) {
        printk(COLOR_GREEN, "[ OK ] %s (Result: %d)\n", test_name, result);
        return true;
    } else {
        printk(COLOR_RED, "[FAIL] %s - Expected %d, Got %d\n", test_name, expected, result);
        return false;
    }
}

static bool check_content(const char* buffer, int bytes_read, const char* expected, const char* test_name) {
    if (bytes_read < 0) {
         printk(COLOR_RED, "[FAIL] %s - Read error occurred (Result: %d)\n", test_name, bytes_read);
         return false;
    }
    size_t expected_len = strlen(expected);
    if ((size_t)bytes_read != expected_len) {
        printk(COLOR_RED, "[FAIL] %s - Expected %d bytes read, Got %d bytes\n", test_name, expected_len, bytes_read);
        return false;
    }
    if (strncmp(buffer, expected, expected_len) == 0) {
        printk(COLOR_GREEN, "[ OK ] %s (Content Match)\n", test_name);
        return true;
    } else {
        printk(COLOR_RED, "[FAIL] %s - Content Mismatch\n", test_name);
        return false;
    }
}

static bool find_in_list(const char* list_buffer, size_t list_size, const char* filename) {
    const char* current = list_buffer;
    while (current < list_buffer + list_size && *current != '\0') {
        if (strcmp(current, filename) == 0) {
            return true;
        }
        current += strlen(current) + 1;
    }
    return false;
}

void vfs_test(void) {
    int result;
    char read_buffer[512];
    char list_buffer[1024];
    bool test_passed = true;

    printk(COLOR_YELLOW, "\n--- Testing VFS ---\n");

    printk(COLOR_CYAN, "\n[SETUP] Mounting Ramdisk at / ...\n");
    result = vfs_mount("/", &ramdisk_ops);
    test_passed &= check_result(result, VFS_SUCCESS, "Mount Ramdisk at /");
    if (!test_passed) {
        printk(COLOR_RED, " Critical Setup Failed: Cannot proceed with tests.\n");
        return;
    }

    printk(COLOR_CYAN, "\n[TEST] File Creation ...\n");
    const char* where_content = "Where do we go from here?\n";
    const char* file1_content = "This is file one.";
    const char* file2_content = "";

    result = vfs_create("/where.txt", where_content);
    test_passed &= check_result(result, VFS_SUCCESS, "Create /where.txt");

    result = vfs_create("/file1.txt", file1_content);
    test_passed &= check_result(result, VFS_SUCCESS, "Create /file1.txt");

    result = vfs_create("/empty.txt", file2_content);
    test_passed &= check_result(result, VFS_SUCCESS, "Create /empty.txt");

    printk(COLOR_CYAN, "\n[TEST] File Reading ...\n");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/where.txt", read_buffer, sizeof(read_buffer) - 1, 0);
    test_passed &= check_content(read_buffer, result, where_content, "Read /where.txt (Full)");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/file1.txt", read_buffer, sizeof(read_buffer) - 1, 0);
    test_passed &= check_content(read_buffer, result, file1_content, "Read /file1.txt (Full)");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/empty.txt", read_buffer, sizeof(read_buffer) - 1, 0);
    test_passed &= check_content(read_buffer, result, file2_content, "Read /empty.txt (Full)");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/where.txt", read_buffer, 5, 0);
    test_passed &= check_content(read_buffer, result, "Where", "Read /where.txt (Partial, Start)");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/where.txt", read_buffer, sizeof(read_buffer) - 1, 6);
    test_passed &= check_content(read_buffer, result, "do we go from here?\n", "Read /where.txt (Offset)");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/file1.txt", read_buffer, sizeof(read_buffer) - 1, 100);
    test_passed &= check_result(result, 0, "Read /file1.txt (Past EOF)");


    printk(COLOR_CYAN, "\n[TEST] File Listing ...\n");
    memset(list_buffer, 0, sizeof(list_buffer));
    result = vfs_list_files("/", list_buffer, sizeof(list_buffer));
    if (check_result(result, VFS_SUCCESS, "List /")) {
        test_passed &= find_in_list(list_buffer, sizeof(list_buffer), "where.txt");
        printk(COLOR_WHITE, "Checking for where.txt: %s\n", find_in_list(list_buffer, sizeof(list_buffer), "where.txt") ? "Found" : "Not Found");
        test_passed &= find_in_list(list_buffer, sizeof(list_buffer), "file1.txt");
         printk(COLOR_WHITE, "Checking for file1.txt: %s\n", find_in_list(list_buffer, sizeof(list_buffer), "file1.txt") ? "Found" : "Not Found");
        test_passed &= find_in_list(list_buffer, sizeof(list_buffer), "empty.txt");
         printk(COLOR_WHITE, "Checking for empty.txt: %s\n", find_in_list(list_buffer, sizeof(list_buffer), "empty.txt") ? "Found" : "Not Found");

        if (!test_passed) {
            printk(COLOR_RED, "[FAIL] File listing content check failed.\n");
        } else {
             printk(COLOR_GREEN, "[ OK ] File listing content check passed.\n");
        }
    } else {
        test_passed = false;
    }

    printk(COLOR_CYAN, "\n[TEST] File Deletion ...\n");
    result = vfs_delete("/file1.txt");
    test_passed &= check_result(result, VFS_SUCCESS, "Delete /file1.txt");

    memset(read_buffer, 0, sizeof(read_buffer));
    result = vfs_read("/file1.txt", read_buffer, sizeof(read_buffer) - 1, 0);
    test_passed &= check_result(result, VFS_ERROR_NOT_FOUND, "Read /file1.txt (After Delete)");

    memset(list_buffer, 0, sizeof(list_buffer));
    result = vfs_list_files("/", list_buffer, sizeof(list_buffer));
    if (check_result(result, VFS_SUCCESS, "List / (After Delete)")) {
        bool still_present = find_in_list(list_buffer, sizeof(list_buffer), "file1.txt");
        test_passed &= !still_present;
        if(still_present) {
             printk(COLOR_RED, "[FAIL] /file1.txt still found in listing after delete.\n");
        } else {
             printk(COLOR_GREEN, "[ OK ] /file1.txt correctly removed from listing.\n");
        }
        test_passed &= find_in_list(list_buffer, sizeof(list_buffer), "where.txt");
         printk(COLOR_WHITE, "Checking for where.txt: %s\n", find_in_list(list_buffer, sizeof(list_buffer), "where.txt") ? "Found" : "Not Found");
    } else {
         test_passed = false;
    }

    printk(COLOR_CYAN, "\n[TEST] Error Conditions ...\n");

    result = vfs_create("/where.txt", "overwrite attempt");
    test_passed &= check_result(result, VFS_ERROR_ALREADY_EXISTS, "Create Existing File (/where.txt)");

    result = vfs_read("/nonexistent.txt", read_buffer, sizeof(read_buffer) - 1, 0);
    test_passed &= check_result(result, VFS_ERROR_NOT_FOUND, "Read Non-Existent File");

    result = vfs_delete("/nonexistent.txt");
    test_passed &= check_result(result, VFS_ERROR_NOT_FOUND, "Delete Non-Existent File");

    result = vfs_list_files("/nosuchdir/", list_buffer, sizeof(list_buffer));
    test_passed &= check_result(result, VFS_ERROR_NOT_FOUND, "List Non-Existent Directory");

    printk(COLOR_CYAN, "\n[CLEANUP] Deleting remaining test files ...\n");
    vfs_delete("/where.txt");
    vfs_delete("/empty.txt");

    printk(COLOR_YELLOW, "\n--- VFS Tests Complete ---\n");
    if (test_passed) {
        printk(COLOR_GREEN, "All VFS tests passed!\n\n");
    } else {
        printk(COLOR_RED, "One or more VFS tests failed!\n\n");
    }
}
