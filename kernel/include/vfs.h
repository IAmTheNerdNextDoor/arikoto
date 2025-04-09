#pragma once

#include <stddef.h>
#include <stdbool.h>

#define MAX_FILE_NAME_LENGTH 64
#define MAX_PATH_LENGTH 256
#define MAX_MOUNT_PATH_LENGTH 64
#define RAMDISK_MAX_FILES 64
#define RAMDISK_MAX_FILE_SIZE 8192

#define VFS_MAX_MOUNT_POINTS 8

#define VFS_SUCCESS           0
#define VFS_ERROR_GENERAL    -1 /* Generic error */
#define VFS_ERROR_INVALID_ARG -2 /* Invalid argument(s) passed to function */
#define VFS_ERROR_NOT_FOUND   -3 /* Path or file not found */
#define VFS_ERROR_NO_MEM      -4 /* Not enough memory */
#define VFS_ERROR_FULL        -5 /* Filesystem or internal table is full */
#define VFS_ERROR_TOO_LARGE   -6 /* File or data exceeds size limits */
#define VFS_ERROR_MOUNT_FAILED -7 /* Filesystem mount operation failed */
#define VFS_ERROR_ALREADY_EXISTS -8 /* File or mount point already exists */
#define VFS_ERROR_NOT_SUPPORTED -9 /* Operation not supported by this filesystem */
#define VFS_ERROR_BUFFER_TOO_SMALL -10 /* Provided buffer is too small */

struct vfs_operations {
    int (*read)(const char *relative_path, char *buffer, size_t size, size_t offset);
    int (*delete)(const char *relative_path);
    int (*create)(const char *relative_path, const char *data, size_t data_size);
    int (*list)(const char *relative_path, char *buffer, size_t size);
};

struct vfs_file_init {
    const char *name;
    const char *data;
};

int vfs_mount(const char *path, struct vfs_operations *ops);
int vfs_list_files(const char *path, char *buffer, size_t size);
int vfs_create(const char *path, const char *data);
int vfs_read(const char *path, char *buffer, size_t size, size_t offset);
int vfs_delete(const char *path);
int init_ramdisk(struct vfs_file_init *files, size_t num_files, const char* mount_path);

extern struct vfs_operations ramdisk_ops;
