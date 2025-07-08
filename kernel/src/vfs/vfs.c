#include <vfs/vfs.h>
#include <misc/kernel.h>
#include <misc/string.h>

struct ramdisk_file {
    char name[MAX_FILE_NAME_LENGTH];
    char data[RAMDISK_MAX_FILE_SIZE];
    size_t size;
    bool in_use;
};

static struct ramdisk_file ramdisk_file_table[RAMDISK_MAX_FILES];
static size_t ramdisk_file_count = 0;

struct mount_point {
    char path[MAX_MOUNT_PATH_LENGTH];
    struct vfs_operations *ops;
    bool mounted;
    size_t path_len;
};

static struct mount_point vfs_mount_table[VFS_MAX_MOUNT_POINTS];
static size_t vfs_mount_count = 0;

static int ramdisk_create(const char *relative_path, const char *data, size_t data_size);
static int ramdisk_read(const char *relative_path, char *buffer, size_t size, size_t offset);
static int ramdisk_delete(const char *relative_path);
static int ramdisk_list(const char *relative_path, char *buffer, size_t size);

static struct ramdisk_file* ramdisk_find_file(const char *name, bool find_empty_slot) {
    for (size_t i = 0; i < RAMDISK_MAX_FILES; ++i) {
        if (ramdisk_file_table[i].in_use) {
            if (!find_empty_slot && name && strcmp(ramdisk_file_table[i].name, name) == 0) {
                return &ramdisk_file_table[i];
            }
        } else if (find_empty_slot) {
            return &ramdisk_file_table[i];
        }
    }
    return NULL;
}

static int ramdisk_create(const char *relative_path, const char *data, size_t data_size) {
    if (!relative_path || !data) {
        return VFS_ERROR_INVALID_ARG;
    }
    if (ramdisk_file_count >= RAMDISK_MAX_FILES) {
        return VFS_ERROR_FULL;
    }
    if (strlen(relative_path) >= MAX_FILE_NAME_LENGTH) {
        return VFS_ERROR_TOO_LARGE;
    }
     if (data_size >= RAMDISK_MAX_FILE_SIZE) {
        return VFS_ERROR_TOO_LARGE;
    }
    if (ramdisk_find_file(relative_path, false) != NULL) {
        return VFS_ERROR_ALREADY_EXISTS;
    }

    struct ramdisk_file *file = ramdisk_find_file(NULL, true);
    if (!file) {
        return VFS_ERROR_FULL;
    }

    memset(file, 0, sizeof(*file));
    strncpy(file->name, relative_path, MAX_FILE_NAME_LENGTH - 1);
    file->name[MAX_FILE_NAME_LENGTH - 1] = '\0';

    memcpy(file->data, data, data_size);
    file->size = data_size;
    file->in_use = true;

    ramdisk_file_count++;
    return VFS_SUCCESS;
}

static int ramdisk_read(const char *relative_path, char *buffer, size_t size, size_t offset) {
    if (!relative_path || !buffer || size == 0) {
        return VFS_ERROR_INVALID_ARG;
    }

    struct ramdisk_file *file = ramdisk_find_file(relative_path, false);
    if (!file) {
        return VFS_ERROR_NOT_FOUND;
    }

    if (offset >= file->size) {
        return 0;
    }

    size_t bytes_to_read = file->size - offset;
    if (bytes_to_read > size) {
        bytes_to_read = size;
    }

    memcpy(buffer, file->data + offset, bytes_to_read);

    return (int)bytes_to_read;
}

static int ramdisk_delete(const char *relative_path) {
    if (!relative_path) {
        return VFS_ERROR_INVALID_ARG;
    }

    struct ramdisk_file *file = ramdisk_find_file(relative_path, false);
    if (!file) {
        return VFS_ERROR_NOT_FOUND;
    }

    file->in_use = false;
    ramdisk_file_count--;

    return VFS_SUCCESS;
}

static int ramdisk_list(const char *relative_path, char *buffer, size_t size) {
    if (!buffer || size == 0) return VFS_ERROR_INVALID_ARG;
    if (relative_path && relative_path[0] != '\0' && strcmp(relative_path, "/") != 0) {
        return VFS_ERROR_NOT_FOUND;
    }

    size_t buffer_pos = 0;
    buffer[0] = '\0';

    for (size_t i = 0; i < RAMDISK_MAX_FILES; ++i) {
        if (ramdisk_file_table[i].in_use) {
            size_t name_len = strlen(ramdisk_file_table[i].name);
            if (buffer_pos + name_len + 1 > size) {
                if (buffer_pos < size) buffer[buffer_pos] = '\0';
                return VFS_ERROR_BUFFER_TOO_SMALL;
            }
            memcpy(buffer + buffer_pos, ramdisk_file_table[i].name, name_len);
            buffer_pos += name_len;
            buffer[buffer_pos] = '\0';
            buffer_pos++;
        }
    }

    if (buffer_pos > 0 && buffer_pos < size) {
         buffer[buffer_pos] = '\0';
    } else if (buffer_pos == 0 && size > 0) {
         buffer[0] = '\0';
    }

    return VFS_SUCCESS;
}

static int ramdisk_create_wrapper(const char *relative_path, const char *data, size_t data_size) {
    (void)data_size;
    if (!data) return VFS_ERROR_INVALID_ARG;
    return ramdisk_create(relative_path, data, data_size);
}

struct vfs_operations ramdisk_ops = {
    .read = ramdisk_read,
    .delete = ramdisk_delete,
    .create = ramdisk_create_wrapper,
    .list = ramdisk_list,
};

static struct mount_point* vfs_find_mount_point(const char *path) {
    if (!path) return NULL;

    struct mount_point *best_match = NULL;
    size_t longest_match_len = 0;

    for (size_t i = 0; i < vfs_mount_count; ++i) {
        if (!vfs_mount_table[i].mounted) continue;

        const char *mount_path = vfs_mount_table[i].path;
        size_t current_mount_len = vfs_mount_table[i].path_len;

        if (strncmp(path, mount_path, current_mount_len) == 0) {
            bool match = false;
            if (current_mount_len == 1 && mount_path[0] == '/') {
                match = true;
            } else if (path[current_mount_len] == '\0') {
                 match = true;
            } else if (path[current_mount_len] == '/') {
                 match = true;
            }

            if (match && current_mount_len >= longest_match_len) {
                longest_match_len = current_mount_len;
                best_match = &vfs_mount_table[i];
            }
        }
    }
    return best_match;
}

static const char* vfs_get_relative_path(const struct mount_point *mount_info, const char *full_path) {
    if (!mount_info || !full_path) return NULL;

    size_t mount_len = mount_info->path_len;

    if (mount_len == 1 && mount_info->path[0] == '/') {
         if (full_path[1] == '\0') return "/";
         return full_path + 1;
    }

    if (strlen(full_path) == mount_len) {
        return "/";
    } else if (full_path[mount_len] == '/') {
         if (full_path[mount_len+1] == '\0') return "/";
        return full_path + mount_len + 1;
    } else {
         return NULL;
    }
}


int vfs_mount(const char *path, struct vfs_operations *ops) {
    if (!path || !ops || path[0] != '/') {
        return VFS_ERROR_INVALID_ARG;
    }
    if (vfs_mount_count >= VFS_MAX_MOUNT_POINTS) {
        return VFS_ERROR_FULL;
    }
    size_t path_len = strlen(path);
    if (path_len == 0 || path_len >= MAX_MOUNT_PATH_LENGTH) {
        return VFS_ERROR_TOO_LARGE;
    }
    if (path_len > 1 && path[path_len - 1] == '/') {
        return VFS_ERROR_INVALID_ARG;
    }

    for (size_t i = 0; i < vfs_mount_count; ++i) {
        if (vfs_mount_table[i].mounted && strcmp(vfs_mount_table[i].path, path) == 0) {
            return VFS_ERROR_ALREADY_EXISTS;
        }
    }

    struct mount_point *mp = NULL;
    for (size_t i = 0; i < VFS_MAX_MOUNT_POINTS; ++i) {
        if (!vfs_mount_table[i].mounted) {
            mp = &vfs_mount_table[i];
            break;
        }
    }

    if (!mp) {
         return VFS_ERROR_FULL;
    }


    memset(mp, 0, sizeof(*mp));
    strncpy(mp->path, path, MAX_MOUNT_PATH_LENGTH - 1);
    mp->path[MAX_MOUNT_PATH_LENGTH - 1] = '\0';
    mp->ops = ops;
    mp->mounted = true;
    mp->path_len = path_len;

    vfs_mount_count++;
    return VFS_SUCCESS;
}

int vfs_read(const char *path, char *buffer, size_t size, size_t offset) {
    if (!path || !buffer) {
        return VFS_ERROR_INVALID_ARG;
    }

    struct mount_point *mp = vfs_find_mount_point(path);
    if (!mp) return VFS_ERROR_NOT_FOUND;
    if (!mp->ops || !mp->ops->read) {
        return VFS_ERROR_NOT_SUPPORTED;
    }

    const char *relative_path = vfs_get_relative_path(mp, path);
    if (!relative_path) {
        return VFS_ERROR_INVALID_ARG;
    }

    return mp->ops->read(relative_path, buffer, size, offset);
}

int vfs_delete(const char *path) {
    if (!path) {
        return VFS_ERROR_INVALID_ARG;
    }

    struct mount_point *mp = vfs_find_mount_point(path);
     if (!mp) return VFS_ERROR_NOT_FOUND;
    if (!mp->ops || !mp->ops->delete) {
        return VFS_ERROR_NOT_SUPPORTED;
    }

    const char *relative_path = vfs_get_relative_path(mp, path);
    if (!relative_path) return VFS_ERROR_INVALID_ARG;

    if (strcmp(relative_path, "/") == 0) {
        return VFS_ERROR_INVALID_ARG;
    }

    return mp->ops->delete(relative_path);
}

int vfs_create(const char *path, const char *data) {
     if (!path || !data) {
         return VFS_ERROR_INVALID_ARG;
     }
     if (strlen(path) >= MAX_PATH_LENGTH) {
         return VFS_ERROR_TOO_LARGE;
     }

    struct mount_point *mp = vfs_find_mount_point(path);
     if (!mp) return VFS_ERROR_NOT_FOUND;
    if (!mp->ops || !mp->ops->create) {
        return VFS_ERROR_NOT_SUPPORTED;
    }

    const char *relative_path = vfs_get_relative_path(mp, path);
    if (!relative_path) return VFS_ERROR_INVALID_ARG;

    if (strcmp(relative_path, "/") == 0) {
        return VFS_ERROR_INVALID_ARG;
    }

    size_t data_len = strlen(data);
    return mp->ops->create(relative_path, data, data_len);
}

int vfs_list_files(const char *path, char *buffer, size_t size) {
    if (!path || !buffer) {
        return VFS_ERROR_INVALID_ARG;
    }

    struct mount_point *mp = vfs_find_mount_point(path);
     if (!mp) return VFS_ERROR_NOT_FOUND;
    if (!mp->ops || !mp->ops->list) {
        return VFS_ERROR_NOT_SUPPORTED;
    }

    const char *relative_path = vfs_get_relative_path(mp, path);
     if (!relative_path) return VFS_ERROR_INVALID_ARG;

    return mp->ops->list(relative_path, buffer, size);
}

int init_ramdisk(struct vfs_file_init *files, size_t num_files, const char* mount_path) {
    if (!mount_path) return VFS_ERROR_INVALID_ARG;
    if (num_files > 0 && !files) return VFS_ERROR_INVALID_ARG;


    int result = vfs_mount(mount_path, &ramdisk_ops);
    if (result != VFS_SUCCESS) {
        return result;
    }

    char full_path_buffer[MAX_PATH_LENGTH];

    for (size_t i = 0; i < num_files; ++i) {
        if (!files[i].name || !files[i].data) {
            return VFS_ERROR_INVALID_ARG;
        }
        if (files[i].name[0] == '/') {
            return VFS_ERROR_INVALID_ARG;
        }

        int written;
         if (strcmp(mount_path, "/") == 0) {
             written = build_string(full_path_buffer, MAX_PATH_LENGTH,
                                      "/", "%s", files[i].name, NULL);
         } else {
              written = build_string(full_path_buffer, MAX_PATH_LENGTH,
                                      "%s", mount_path, "/",
                                      "%s", files[i].name, NULL);
         }


        if (written < 0 || (size_t)written >= MAX_PATH_LENGTH) {
            return VFS_ERROR_TOO_LARGE;
        }

        result = vfs_create(full_path_buffer, files[i].data);
        if (result != VFS_SUCCESS) {
            return result;
        }
    }

    return VFS_SUCCESS;
}
