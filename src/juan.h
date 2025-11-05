// ISC License
//
// Copyright 2025 Juan Milkah <juanmilkah@proton.me>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGE ARISING
// OUT OF OR IN CONNECTION WITH THE SOFTWARE.

#ifndef JUAN_H
#define JUAN_H

#include <errno.h>
#include <stddef.h>

// Initial capacity for Vec dynamic array
#define INIT_VEC_CAP 64

// File IO operations
// Return 0 on success, -1 on failure
int create_file_if_not_exists(const char *path);
int file_exists(const char *path);   // 0 if not exists, 1 if exists, -1 error
long size_of_file(const char *path); // -1 on error
int read_file_to_buffer(const char *path, void *buf,
                        size_t file_size); // 0 success, -1 error
int write_buffer_to_file(const char *path, void *buf,
                         size_t buf_size); // 0 success, -1 error
int append_buffer_to_file(const char *path, void *buf,
                          size_t buf_size); // 0 success, -1 error

// Dynamic Data structures
struct Vec {
        size_t cap;
        size_t len;
        void **items;
};

struct Vec *init_vec(void);
struct Vec *init_vec_with_cap(unsigned int cap);
int vec_insert_at(struct Vec *vec, void *item,
                  size_t index);           // 0 success, -1 error
int vec_push(struct Vec *vec, void *item); // 0 success, -1 error
int realloc_vec(struct Vec *vec);          // 0 success, -1 error
void drop_vec(struct Vec *vec);

// Logging levels
enum Level {
        J_ERROR,
        J_WARN,
        J_INFO,
        J_DEBUG,
};

const char *log_level_to_string(enum Level l);
void exit_with_error(const char *message);
void J_log(enum Level l, const char *message);

// String utilities
void split_newline_to_vec(char *s, struct Vec *v);
void split_at_delimiter_to_vec(char *s, struct Vec *v, char delimiter);

#ifdef JUAN_IMPLEMENTATION

#include "juan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long size_of_file(const char *path) {
        FILE *f = fopen(path, "r");
        if (!f) {
                return -1;
        }

        if (fseek(f, 0, SEEK_END) < 0) {
                fclose(f);
                return -1;
        }

        long size = ftell(f);
        fclose(f);
        if (size < 0) {
                return -1;
        }
        return size;
}

int read_file_to_buffer(const char *path, void *buf, size_t file_size) {
        FILE *f = fopen(path, "r");
        if (!f) {
                return -1;
        }

        size_t read = fread(buf, 1, file_size, f);
        fclose(f);
        return (read == file_size) ? 0 : -1;
}

int write_buffer_to_file(const char *path, void *buf, size_t buf_size) {
        FILE *f = fopen(path, "w");
        if (!f) {
                return -1;
        }

        size_t written = fwrite(buf, 1, buf_size, f);
        fclose(f);
        return (written == buf_size) ? 0 : -1;
}

int append_buffer_to_file(const char *path, void *buf, size_t buf_size) {
        FILE *f = fopen(path, "a");
        if (!f) {
                return -1;
        }

        size_t written = fwrite(buf, 1, buf_size, f);
        fclose(f);
        return (written == buf_size) ? 0 : -1;
}

int create_file_if_not_exists(const char *path) {
        FILE *f = fopen(path, "r");
        if (f) {
                fclose(f);
                return 0; // File exists, treat as success/no error.
        }

        f = fopen(path, "w");
        if (!f) {
                return -1;
        }

        fclose(f);
        return 0; // File created successfully
}

int file_exists(const char *path) {
        FILE *f = fopen(path, "r");
        if (f) {
                fclose(f);
                return 1; // Exists
        }

        if (errno == ENOENT) {
                return 0; // Doesn't exist
        } else {
                return -1; // Error in checking
        }
}

struct Vec *init_vec(void) { return init_vec_with_cap(INIT_VEC_CAP); }

struct Vec *init_vec_with_cap(unsigned int cap) {
        if (cap == 0) {
                cap = INIT_VEC_CAP;
        }
        void **items = malloc(cap * sizeof(void *));
        if (!items)
                return NULL;

        struct Vec *v = malloc(sizeof(struct Vec));
        if (!v) {
                free(items);
                return NULL;
        }

        v->cap = cap;
        v->items = items;
        v->len = 0;
        return v;
}

int realloc_vec(struct Vec *vec) {
        size_t new_cap = vec->cap * 2;
        void **new_items = reallocarray(vec->items, new_cap, sizeof(void *));
        if (!new_items) {
                return -1;
        }
        vec->items = new_items;
        vec->cap = new_cap;
        return 0;
}

int vec_insert_at(struct Vec *vec, void *item, size_t index) {
        if (!vec)
                return -1;

        if (index < vec->cap) {
                char *item_copy = strdup((const char *)item);
                if (!item_copy)
                        return -1;
                vec->items[index] = item_copy;
                return 0;
        }

        if (vec->len == vec->cap) {
                if (realloc_vec(vec) < 0) {
                        return -1;
                }
        }

        char *item_copy = strdup((const char *)item);
        if (!item_copy)
                return -1;
        vec->items[vec->len++] = item_copy;
        return 0;
}

int vec_push(struct Vec *vec, void *item) {
        int result = vec_insert_at(vec, item, vec->len);
        if (!result) {
                vec->len++;
        }

        return result;
}

void drop_vec(struct Vec *vec) {
        if (!vec)
                return;
        for (size_t i = 0; i < vec->len; i++) {
                free(vec->items[i]);
        }
        free(vec->items);
        free(vec);
}

const char *log_level_to_string(enum Level l) {
        switch (l) {
        case J_ERROR:
                return "ERROR";
        case J_WARN:
                return "WARN";
        case J_INFO:
                return "INFO";
        case J_DEBUG:
                return "DEBUG";
        default:
                return "UNKNOWN";
        }
}

void J_log(enum Level l, const char *message) {
        const char *level_str = log_level_to_string(l);
        fprintf(stderr, "%s: %s\n", level_str, message);
}

void exit_with_error(const char *message) {
        perror(message);
        exit(EXIT_FAILURE);
}

void split_at_delimiter_to_vec(char *s, struct Vec *v, char delimiter) {
        if (!s || !v)
                return;
        char *start = s;
        while (*start) {
                char *delim_pos = strchr(start, delimiter);
                size_t len =
                    delim_pos ? (size_t)(delim_pos - start) : strlen(start);
                char *part = malloc(len + 1);
                if (!part)
                        return;
                memcpy(part, start, len);
                part[len] = '\0';
                vec_push(v, part);
                if (!delim_pos)
                        break;
                start = delim_pos + 1;
        }
}

void split_newline_to_vec(char *s, struct Vec *v) {
        split_at_delimiter_to_vec(s, v, '\n');
}

#endif
#endif
