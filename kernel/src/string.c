#include <memory.h>

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if (d == s) {
        return d;
    }

    if (s < d && d < s + n) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)(p1[i] - p2[i]);
        }
    }
    return 0;
}
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    for (size_t i = 0; i < n; ++i) {
        if (p[i] == (unsigned char)c) {
            return (void *)(p + i);
        }
    }
    return NULL;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    size_t i = 0;
    while (i < n && (*d = *src) != '\0') {
        d++;
        src++;
        i++;
    }
    while (i < n) {
        *d++ = '\0';
        i++;
    }
    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++ != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
     while (*s != '\0') {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    if (c == '\0') {
       return (char*) s;
    }
    return NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    if (c == '\0') {
       return strchr(s, '\0');
    }
    while (*s != '\0') {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }
    return (char *)last;
}

size_t strspn(const char *s, const char *accept) {
    size_t count = 0;
    while (*s && strchr(accept, *s)) {
        count++;
        s++;
    }
    return count;
}

size_t strcspn(const char *s, const char *reject) {
     size_t count = 0;
    while (*s) {
        if (strchr(reject, *s)) {
            return count;
        }
        count++;
        s++;
    }
    return count;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s != '\0') {
        if (strchr(accept, *s) != NULL) {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack;
    }

    for (; *haystack != '\0'; haystack++) {
        if (strncmp(haystack, needle, strlen(needle)) == 0) {
            return (char *)haystack;
        }
    }
    return NULL;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token_start;

    if (str == NULL) {
        str = *saveptr;
    }

    while (*str && strchr(delim, *str)) {
        str++;
    }

    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }

    token_start = str;

    while (*str && !strchr(delim, *str)) {
        str++;
    }

    if (*str) {
        *str = '\0';
        *saveptr = str + 1;
    } else {
        *saveptr = str;
    }

    return token_start;
}

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    void *new_str = allocate_page();

    if (new_str == NULL) {
        return NULL;
    }

    size_t num_extra_pages = (len > PAGE_SIZE) ? ((len - PAGE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE) : 0;

    for(size_t i = 0; i < num_extra_pages; i++){
        void* extra_page = allocate_page();
        if(!extra_page) {
             free_page(new_str);

              for(size_t j = 0; j < i; j++){
                  free_page((char*)new_str + (j+1) * PAGE_SIZE);
              }
              return NULL;
        }
    }
    return (char *)memcpy(new_str, s, len);
}
