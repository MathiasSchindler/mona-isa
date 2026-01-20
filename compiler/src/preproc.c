#include "preproc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_string(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, s, len);
    return out;
}
#include <ctype.h>

typedef struct {
    char *name;
    char *value;
} Macro;

typedef struct {
    Macro *items;
    size_t count;
    size_t cap;
} MacroTable;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

static void buf_init(Buffer *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int buf_reserve(Buffer *b, size_t add) {
    size_t need = b->len + add + 1;
    if (need <= b->cap) return 1;
    size_t next = (b->cap == 0) ? 256 : b->cap * 2;
    while (next < need) next *= 2;
    char *mem = (char *)realloc(b->data, next);
    if (!mem) return 0;
    b->data = mem;
    b->cap = next;
    return 1;
}

static int buf_append_n(Buffer *b, const char *s, size_t n) {
    if (!buf_reserve(b, n)) return 0;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 1;
}

static int buf_append(Buffer *b, const char *s) {
    return buf_append_n(b, s, strlen(s));
}

static char *dup_error(const char *msg) {
    size_t len = strlen(msg) + 1;
    char *out = (char *)malloc(len);
    if (out) memcpy(out, msg, len);
    return out;
}

static char *dup_error_path(const char *msg, const char *path) {
    char buf[512];
    snprintf(buf, sizeof(buf), "%s: %s", msg, path ? path : "<unknown>");
    return dup_error(buf);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (n != (size_t)len) { free(buf); return NULL; }
    buf[len] = '\0';
    return buf;
}

static void macros_free(MacroTable *t) {
    for (size_t i = 0; i < t->count; i++) {
        free(t->items[i].name);
        free(t->items[i].value);
    }
    free(t->items);
    t->items = NULL;
    t->count = 0;
    t->cap = 0;
}

static const char *macro_find(const MacroTable *t, const char *name, size_t len) {
    for (size_t i = 0; i < t->count; i++) {
        if (strlen(t->items[i].name) == len && strncmp(t->items[i].name, name, len) == 0) {
            return t->items[i].value;
        }
    }
    return NULL;
}

static int macro_set(MacroTable *t, const char *name, size_t name_len, const char *value) {
    for (size_t i = 0; i < t->count; i++) {
        if (strlen(t->items[i].name) == name_len && strncmp(t->items[i].name, name, name_len) == 0) {
            free(t->items[i].value);
            t->items[i].value = dup_string(value ? value : "");
            return t->items[i].value != NULL;
        }
    }
    if (t->count + 1 > t->cap) {
        size_t next = (t->cap == 0) ? 8 : t->cap * 2;
        Macro *mem = (Macro *)realloc(t->items, next * sizeof(Macro));
        if (!mem) return 0;
        t->items = mem;
        t->cap = next;
    }
    char *n = (char *)malloc(name_len + 1);
    if (!n) return 0;
    memcpy(n, name, name_len);
    n[name_len] = '\0';
    t->items[t->count].name = n;
    t->items[t->count].value = dup_string(value ? value : "");
    if (!t->items[t->count].value) return 0;
    t->count++;
    return 1;
}

static void macro_unset(MacroTable *t, const char *name, size_t name_len) {
    for (size_t i = 0; i < t->count; i++) {
        if (strlen(t->items[i].name) == name_len && strncmp(t->items[i].name, name, name_len) == 0) {
            free(t->items[i].name);
            free(t->items[i].value);
            if (i + 1 < t->count) memmove(&t->items[i], &t->items[i + 1], (t->count - i - 1) * sizeof(Macro));
            t->count--;
            return;
        }
    }
}

static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static char *join_path(const char *base, const char *rel) {
    if (!rel || rel[0] == '\0') return NULL;
    if (rel[0] == '/') return dup_string(rel);
    const char *slash = strrchr(base ? base : "", '/');
    size_t base_len = slash ? (size_t)(slash - base) : 0;
    size_t rel_len = strlen(rel);
    size_t total = base_len + 1 + rel_len + 1;
    char *out = (char *)malloc(total);
    if (!out) return NULL;
    if (base_len > 0) {
        memcpy(out, base, base_len);
        out[base_len] = '/';
        memcpy(out + base_len + 1, rel, rel_len);
        out[base_len + 1 + rel_len] = '\0';
    } else {
        memcpy(out, rel, rel_len);
        out[rel_len] = '\0';
    }
    return out;
}

static int can_open_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static const char *map_system_include(const char *inc) {
    if (!inc) return inc;
    if (strcmp(inc, "stdio.h") == 0) return "clib.h";
    if (strcmp(inc, "string.h") == 0) return "clib.h";
    if (strcmp(inc, "ctype.h") == 0) return "clib.h";
    return inc;
}

static char *resolve_include_path(const char *base, const char *inc) {
    char *full = join_path(base, inc);
    if (full && can_open_file(full)) return full;
    free(full);

    char rel[512];
    snprintf(rel, sizeof(rel), "../../clib/include/%s", inc);
    char *alt = join_path(base, rel);
    if (alt && can_open_file(alt)) return alt;
    free(alt);

    char abs[512];
    snprintf(abs, sizeof(abs), "/clib/include/%s", inc);
    if (can_open_file(abs)) return dup_string(abs);
    return NULL;
}

static int append_expanded_line(const char *line, size_t len, MacroTable *macros, Buffer *out) {
    int in_str = 0;
    int in_char = 0;
    int escape = 0;
    size_t i = 0;
    while (i < len) {
        char c = line[i];
        if (in_str) {
            if (!buf_append_n(out, &c, 1)) return 0;
            if (escape) { escape = 0; }
            else if (c == '\\') { escape = 1; }
            else if (c == '"') { in_str = 0; }
            i++;
            continue;
        }
        if (in_char) {
            if (!buf_append_n(out, &c, 1)) return 0;
            if (escape) { escape = 0; }
            else if (c == '\\') { escape = 1; }
            else if (c == '\'') { in_char = 0; }
            i++;
            continue;
        }
        if (c == '"') { in_str = 1; if (!buf_append_n(out, &c, 1)) return 0; i++; continue; }
        if (c == '\'') { in_char = 1; if (!buf_append_n(out, &c, 1)) return 0; i++; continue; }

        if (isalpha((unsigned char)c) || c == '_') {
            size_t start = i;
            i++;
            while (i < len) {
                char n = line[i];
                if (!isalnum((unsigned char)n) && n != '_') break;
                i++;
            }
            size_t nlen = i - start;
            const char *rep = macro_find(macros, line + start, nlen);
            if (rep) {
                if (!buf_append(out, rep)) return 0;
            } else {
                if (!buf_append_n(out, line + start, nlen)) return 0;
            }
            continue;
        }
        if (!buf_append_n(out, &c, 1)) return 0;
        i++;
    }
    return 1;
}

static int preprocess_file(const char *path, MacroTable *macros, Buffer *out, char **out_error, int depth) {
    if (depth > 16) {
        if (out_error && !*out_error) *out_error = dup_error("error: include nesting too deep");
        return 0;
    }
    char *src = read_file(path);
    if (!src) {
        if (out_error && !*out_error) *out_error = dup_error_path("error: include not found", path);
        return 0;
    }
    int cond_stack[64];
    int cond_top = 0;
    cond_stack[cond_top++] = 1;
    const char *p = src;
    while (*p) {
        const char *line_start = p;
        const char *line_end = strchr(p, '\n');
        size_t line_len = line_end ? (size_t)(line_end - line_start) : strlen(line_start);

        const char *s = skip_ws(line_start);
        if (*s == '#') {
            s++;
            s = skip_ws(s);
            if (strncmp(s, "ifdef", 5) == 0 && (s[5] == ' ' || s[5] == '\t')) {
                s += 5;
                s = skip_ws(s);
                const char *name = s;
                if (!isalpha((unsigned char)*name) && *name != '_') {
                    if (out_error && !*out_error) *out_error = dup_error("error: invalid macro name");
                    free(src);
                    return 0;
                }
                s++;
                while (isalnum((unsigned char)*s) || *s == '_') s++;
                size_t name_len = (size_t)(s - name);
                int defined = macro_find(macros, name, name_len) != NULL;
                int active = cond_stack[cond_top - 1] && defined;
                if (cond_top >= 64) { if (out_error && !*out_error) *out_error = dup_error("error: too many nested conditionals"); free(src); return 0; }
                cond_stack[cond_top++] = active;
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else if (strncmp(s, "ifndef", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
                s += 6;
                s = skip_ws(s);
                const char *name = s;
                if (!isalpha((unsigned char)*name) && *name != '_') {
                    if (out_error && !*out_error) *out_error = dup_error("error: invalid macro name");
                    free(src);
                    return 0;
                }
                s++;
                while (isalnum((unsigned char)*s) || *s == '_') s++;
                size_t name_len = (size_t)(s - name);
                int defined = macro_find(macros, name, name_len) != NULL;
                int active = cond_stack[cond_top - 1] && !defined;
                if (cond_top >= 64) { if (out_error && !*out_error) *out_error = dup_error("error: too many nested conditionals"); free(src); return 0; }
                cond_stack[cond_top++] = active;
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else if (strncmp(s, "endif", 5) == 0 && (s[5] == '\0' || isspace((unsigned char)s[5]))) {
                if (cond_top <= 1) {
                    if (out_error && !*out_error) *out_error = dup_error("error: #endif without #if");
                    free(src);
                    return 0;
                }
                cond_top--;
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else if (strncmp(s, "include", 7) == 0 && (s[7] == ' ' || s[7] == '\t' || s[7] == '"' || s[7] == '<')) {
                if (!cond_stack[cond_top - 1]) {
                    if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                    goto next_line;
                }
                s += 7;
                s = skip_ws(s);
                if (*s == '"' || *s == '<') {
                    char endch = (*s == '<') ? '>' : '"';
                    s++;
                    const char *q = strchr(s, endch);
                    if (!q) { if (out_error && !*out_error) *out_error = dup_error("error: unterminated include"); free(src); return 0; }
                    size_t inc_len = (size_t)(q - s);
                    char *inc = (char *)malloc(inc_len + 1);
                    if (!inc) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                    memcpy(inc, s, inc_len);
                    inc[inc_len] = '\0';
                    const char *mapped = map_system_include(inc);
                    if (mapped != inc) {
                        free(inc);
                        inc = dup_string(mapped);
                        if (!inc) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                    }
                    char *full = resolve_include_path(path, inc);
                    free(inc);
                    if (!full) { if (out_error && !*out_error) *out_error = dup_error("error: include not found"); free(src); return 0; }
                    int ok = preprocess_file(full, macros, out, out_error, depth + 1);
                    free(full);
                    if (!ok) { free(src); return 0; }
                } else {
                    if (out_error && !*out_error) *out_error = dup_error("error: expected \"...\" or <...> after #include");
                    free(src);
                    return 0;
                }
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else if (strncmp(s, "define", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
                if (!cond_stack[cond_top - 1]) {
                    if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                    goto next_line;
                }
                s += 6;
                s = skip_ws(s);
                const char *name = s;
                if (!isalpha((unsigned char)*name) && *name != '_') {
                    if (out_error && !*out_error) *out_error = dup_error("error: invalid macro name");
                    free(src);
                    return 0;
                }
                s++;
                while (isalnum((unsigned char)*s) || *s == '_') s++;
                size_t name_len = (size_t)(s - name);
                s = skip_ws(s);
                size_t value_len = 0;
                if (*s) {
                    const char *end = line_end ? line_end : s + strlen(s);
                    value_len = (size_t)(end - s);
                    while (value_len > 0 && (s[value_len - 1] == ' ' || s[value_len - 1] == '\t' || s[value_len - 1] == '\r')) {
                        value_len--;
                    }
                }
                char *value = (char *)malloc(value_len + 1);
                if (!value) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                if (value_len > 0) memcpy(value, s, value_len);
                value[value_len] = '\0';
                if (!macro_set(macros, name, name_len, value)) {
                    if (out_error && !*out_error) *out_error = dup_error("error: out of memory");
                    free(value);
                    free(src);
                    return 0;
                }
                free(value);
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else if (strncmp(s, "undef", 5) == 0 && (s[5] == ' ' || s[5] == '\t')) {
                if (!cond_stack[cond_top - 1]) {
                    if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                    goto next_line;
                }
                s += 5;
                s = skip_ws(s);
                const char *name = s;
                if (!isalpha((unsigned char)*name) && *name != '_') {
                    if (out_error && !*out_error) *out_error = dup_error("error: invalid macro name");
                    free(src);
                    return 0;
                }
                s++;
                while (isalnum((unsigned char)*s) || *s == '_') s++;
                size_t name_len = (size_t)(s - name);
                macro_unset(macros, name, name_len);
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
            } else {
                if (out_error && !*out_error) *out_error = dup_error("error: unsupported preprocessor directive");
                free(src);
                return 0;
            }
        } else {
            if (!cond_stack[cond_top - 1]) {
                if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
                goto next_line;
            }
            if (!append_expanded_line(line_start, line_len, macros, out)) {
                if (out_error && !*out_error) *out_error = dup_error("error: out of memory");
                free(src);
                return 0;
            }
            if (!buf_append_n(out, "\n", 1)) { if (out_error && !*out_error) *out_error = dup_error("error: out of memory"); free(src); return 0; }
        }
next_line:
        if (!line_end) break;
        p = line_end + 1;
    }
    if (cond_top != 1) {
        if (out_error && !*out_error) *out_error = dup_error("error: unterminated #if");
        free(src);
        return 0;
    }
    free(src);
    return 1;
}

int preprocess_source(const char *path, char **out_source, char **out_error) {
    if (out_error) *out_error = NULL;
    if (!out_source || !path) return 0;
    *out_source = NULL;
    MacroTable macros = {0};
    Buffer out;
    buf_init(&out);
    if (!preprocess_file(path, &macros, &out, out_error, 0)) {
        macros_free(&macros);
        free(out.data);
        return 0;
    }
    macros_free(&macros);
    *out_source = out.data ? out.data : dup_string("");
    if (!*out_source) {
        if (out_error && !*out_error) *out_error = dup_error("error: out of memory");
        return 0;
    }
    return 1;
}
