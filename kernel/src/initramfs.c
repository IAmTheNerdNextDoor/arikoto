#include <vfs.h>
#include <kernel.h>
#include <string.h>
#include <heap.h>
#include <stdbool.h>
#include <stdint.h>

#define INITRAMFS_MAX_FILES 256
#define INITRAMFS_MAX_FILE_NAME 128
#define INITRAMFS_MAX_FILE_SIZE (1024*1024)

struct initramfs_file {
    char name[INITRAMFS_MAX_FILE_NAME];
    void *data;
    size_t size;
    bool is_dir;
};

static struct initramfs_file initramfs_files[INITRAMFS_MAX_FILES];
static size_t initramfs_file_count = 0;

static void normalize_path(const char *in, char *out, size_t outsize) {
    while (*in == '.' && in[1] == '/') in += 2;
    while (*in == '/') in++;
    size_t len = strlen(in);
    while (len > 1 && in[len-1] == '/') len--;
    if (len >= outsize) len = outsize-1;
    memcpy(out, in, len);
    out[len] = 0;
    if (out[0] == 0) {
        out[0] = '\0';
    }
}

static int initramfs_parse(void *addr, size_t size) {
    uint8_t *p = (uint8_t *)addr;
    uint8_t *end = p + size;
    size_t count = 0;

    while (p + 110 < end) {
        if (memcmp(p, "070701", 6) != 0) break;

        char name[INITRAMFS_MAX_FILE_NAME] = {0};
        uint32_t namesize, filesize;
        #define HEXFIELD(off) ({ \
            uint32_t v = 0; \
            for (int i=0; i<8; i++) { \
                char c = p[(off)+i]; \
                v = (v << 4) | (c >= '0' && c <= '9' ? c-'0' : c >= 'a' && c <= 'f' ? c-'a'+10 : c >= 'A' && c <= 'F' ? c-'A'+10 : 0); \
            } v; \
        })
        namesize = HEXFIELD(94);
        filesize = HEXFIELD(54);
        #undef HEXFIELD

        char *fname = (char *)(p + 110);
        if (namesize > INITRAMFS_MAX_FILE_NAME-1) namesize = INITRAMFS_MAX_FILE_NAME-1;
        memcpy(name, fname, namesize);
        name[namesize] = 0;

        uint8_t *filedata = (uint8_t *)fname + ((namesize + 3) & ~3);

        uint32_t mode = 0;
        for (int i=0; i<8; i++) {
            char c = p[14+i];
            mode = (mode << 4) | (c >= '0' && c <= '9' ? c-'0' : c >= 'a' && c <= 'f' ? c-'a'+10 : c >= 'A' && c <= 'F' ? c-'A'+10 : 0);
        }
        bool is_dir = (mode & 0170000) == 0040000;
        bool is_file = (mode & 0170000) == 0100000;
        if ((is_file || is_dir) && count < INITRAMFS_MAX_FILES) {
            strncpy(initramfs_files[count].name, name, INITRAMFS_MAX_FILE_NAME-1);
            initramfs_files[count].data = is_file ? filedata : NULL;
            initramfs_files[count].size = is_file ? filesize : 0;
            initramfs_files[count].is_dir = is_dir;
            count++;
        }
        if (strcmp(name, "TRAILER!!!") == 0) break;

        size_t skip = 110 + ((namesize + 3) & ~3) + ((filesize + 3) & ~3);
        p += skip;
    }
    initramfs_file_count = count;
    return 0;
}

static struct initramfs_file *find_file(const char *name) {
    char normname[INITRAMFS_MAX_FILE_NAME];
    normalize_path(name, normname, sizeof(normname));
    for (size_t i = 0; i < initramfs_file_count; i++) {
        if (strcmp(initramfs_files[i].name, normname) == 0) return &initramfs_files[i];
    }
    return NULL;
}

static int initramfs_read(const char *relative_path, char *buffer, size_t size, size_t offset) {
    char normpath[INITRAMFS_MAX_FILE_NAME];
    normalize_path(relative_path, normpath, sizeof(normpath));
    struct initramfs_file *f = find_file(normpath);
    if (!f) return VFS_ERROR_NOT_FOUND;
    if (f->is_dir) return VFS_ERROR_NOT_SUPPORTED;
    if (offset >= f->size) return 0;
    size_t tocopy = f->size - offset;
    if (tocopy > size) tocopy = size;
    memcpy(buffer, (char*)f->data + offset, tocopy);
    return (int)tocopy;
}

static int initramfs_delete(const char *relative_path __attribute__((unused))) {
    return VFS_ERROR_NOT_SUPPORTED;
}

static int initramfs_create(const char *relative_path __attribute__((unused)), const char *data __attribute__((unused)), size_t data_size __attribute__((unused))) {
    return VFS_ERROR_NOT_SUPPORTED;
}

static int initramfs_list(const char *relative_path, char *buffer, size_t size) {
    char dirpath[INITRAMFS_MAX_FILE_NAME];
    normalize_path(relative_path ? relative_path : "", dirpath, sizeof(dirpath));
    size_t pos = 0;
    for (size_t i = 0; i < initramfs_file_count; i++) {
        const char *fname = initramfs_files[i].name;
        if (dirpath[0] == '\0') {
            if (strchr(fname, '/') == NULL) {
                size_t nlen = strlen(fname);
                if (pos + nlen + 1 >= size) return VFS_ERROR_BUFFER_TOO_SMALL;
                strcpy(buffer + pos, fname);
                pos += nlen + 1;
            }
        } else {
            size_t dlen = strlen(dirpath);
            if (strncmp(fname, dirpath, dlen) == 0 && fname[dlen] == '/') {
                const char *sub = fname + dlen + 1;
                if (strchr(sub, '/') == NULL) {
                    size_t nlen = strlen(sub);
                    if (pos + nlen + 1 >= size) return VFS_ERROR_BUFFER_TOO_SMALL;
                    strcpy(buffer + pos, sub);
                    pos += nlen + 1;
                }
            }
        }
    }
    if (pos < size) buffer[pos] = '\0';
    return VFS_SUCCESS;
}

struct vfs_operations initramfs_ops = {
    .read = initramfs_read,
    .delete = initramfs_delete,
    .create = initramfs_create,
    .list = initramfs_list,
};

int vfs_mount_initramfs(const char *mount_path, void *initramfs_addr, size_t initramfs_size) {
    if (!mount_path || !initramfs_addr || initramfs_size == 0) return VFS_ERROR_INVALID_ARG;
    initramfs_parse(initramfs_addr, initramfs_size);
    return vfs_mount(mount_path, &initramfs_ops);
}
