#include <vfs.h>
#include <kernel.h>

#define MAX_MOUNT_POINTS 8192
#define MAX_FILES 8192
#define MAX_FILE_SIZE 8192
#define MAX_FILE_NAME_LENGTH 64
#define MAX_MOUNT_PATH_LENGTH 64

char buffer[1024];

/* Ramdisk operations */
struct vfs_operations ramdisk_ops = {
    .read = ramdisk_read,
    .delete = ramdisk_delete,
    .create = ramdisk_create,
};

/* File structure */
struct vfs_file {
    char name[MAX_FILE_NAME_LENGTH];
    char data[MAX_FILE_SIZE];
    size_t size;
};

/* File table */
struct vfs_file file_table[MAX_FILES];
size_t file_count = 0;

/* Mount point structure */
struct mount_point {
    char path[MAX_MOUNT_PATH_LENGTH];
    struct vfs_operations *ops;
    bool mounted;
} mount_table[MAX_MOUNT_POINTS];

size_t mount_count = 0;

int ramdisk_create(const char *name, const char *data) {
    if (file_count >= MAX_FILES) {
        return -1;
    }
    if (strlen(name) >= MAX_FILE_NAME_LENGTH) {
        return -1;
    }

    struct vfs_file *file = &file_table[file_count];

    strncpy(file->name, name, MAX_FILE_NAME_LENGTH - 1);
    file->name[MAX_FILE_NAME_LENGTH - 1] = '\0';

    size_t data_len = strlen(data);
    if(data_len >= MAX_FILE_SIZE) {
       return -1;
    }
    strncpy(file->data, data, MAX_FILE_SIZE -1);
    file->data[MAX_FILE_SIZE - 1] = '\0';
    file->size = data_len;

    file_count++;
    return 0;
}

int ramdisk_read(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }

    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(file_table[i].name, path) == 0) {
            size_t to_copy = (file_table[i].size < size) ? file_table[i].size : size;
            memcpy(buffer, file_table[i].data, to_copy);
            buffer[to_copy] = '\0';
            return 0;
        }
    }
    return -1;
}

int ramdisk_delete(const char *path) {
    if (!path) {
      return -1;
    }

    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(file_table[i].name, path) == 0) {
            for (size_t j = i; j < file_count - 1; j++) {
                file_table[j] = file_table[j + 1];
            }
            file_count--;
            return 0;
        }
    }
    return -1;
}

int vfs_mount(const char *path, struct vfs_operations *ops) {
    if (mount_count >= MAX_MOUNT_POINTS) {
        return -1;
    }
    if (strlen(path) >= MAX_MOUNT_PATH_LENGTH) {
        return -1;
    }

    size_t i;
    for (i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (!mount_table[i].mounted) {
            break;
        }
    }
    if (i == MAX_MOUNT_POINTS) {
        return -1;
    }

    strncpy(mount_table[i].path, path, MAX_MOUNT_PATH_LENGTH - 1);
    mount_table[i].path[MAX_MOUNT_PATH_LENGTH - 1] = '\0';
    mount_table[i].ops = ops;
    mount_table[i].mounted = true;
    mount_count++;
    return 0;
}

struct mount_point* find_mount_point(const char *path) {
    if (!path) {
      return NULL;
    }

    for (size_t i = 0; i < mount_count; i++) {
        if (strncmp(path, mount_table[i].path, strlen(mount_table[i].path)) == 0) {
            return &mount_table[i];
        }
    }
    return NULL;
}

int vfs_read(const char *path, char *buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }

    struct mount_point *mp = find_mount_point(path);
    if (!mp || !mp->ops || !mp->ops->read) {
        return -1;
    }

    return mp->ops->read(path + strlen(mp->path), buffer, size);
}

int vfs_delete(const char *path) {
    if(!path) {
      return -1;
    }

    struct mount_point *mp = find_mount_point(path);
    if (!mp || !mp->ops || !mp->ops->delete) {
      return -1;
    }

    return mp->ops->delete(path + strlen(mp->path));
}

int vfs_create(const char *path, const char *data) {
     if(!path || !data) {
      return -1;
    }

    struct mount_point *mp = find_mount_point(path);
    if (!mp || !mp->ops) {
        return -1;
    }

    if (!mp->ops->create) {
        return -1;
    }

    return mp->ops->create(path + strlen(mp->path), data);
}

int vfs_list_files(char *buffer, size_t size) {
    if (!buffer || size == 0) {
        return -1;
    }

    size_t pos = 0;
    buffer[0] = '\0';

    for (size_t i = 0; i < mount_count; i++) {
        pos += build_string(buffer + pos, size - pos, "Mount Point: ", "%s", mount_table[i].path, "\n", NULL);
        if (pos >= size) return -1;

        for (size_t j = 0; j < file_count; j++) {
            char full_path[MAX_MOUNT_PATH_LENGTH + MAX_FILE_NAME_LENGTH + 2];

           build_string(full_path, sizeof(full_path), "%s", mount_table[i].path, "/", "%s", file_table[j].name, NULL);


            if (strncmp(mount_table[i].path, full_path, strlen(mount_table[i].path)) == 0) {
                 pos += build_string(buffer + pos, size - pos, "  ", "%s", file_table[j].name, "\n", NULL);

                if (pos >= size) return -1;

            }
        }
    }
    return 0;
}

int init_ramdisk(struct vfs_file_init *files, size_t num_files, const char* mount_path) {
    if (!files || num_files == 0) {
        return -1;
    }

    if (vfs_mount(mount_path, &ramdisk_ops) != 0) {
        return -1;
    }

    for (size_t i = 0; i < num_files; i++) {
        if (vfs_create(files[i].name, files[i].data) != 0) {
           return -1;
        }
    }

    return 0;
}
