#include "mini-glib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

void* g_malloc(size_t n_bytes) {
    if (n_bytes == 0) return NULL;
    void *p = malloc(n_bytes);
    if (!p) {
        fprintf(stderr, "mini-glib: Out of memory attempting %zu bytes\n", n_bytes);
        abort();
    }
    return p;
}

void* g_malloc0(size_t n_bytes) {
    void *p = g_malloc(n_bytes);
    if (p) memset(p, 0, n_bytes);
    return p;
}

void* g_realloc(void *mem, size_t n_bytes) {
    if (n_bytes == 0) {
        g_free(mem);
        return NULL;
    }
    void *p = realloc(mem, n_bytes);
    if (!p) {
        fprintf(stderr, "mini-glib: Out of memory in realloc\n");
        abort();
    }
    return p;
}

void g_free(void *mem) {
    if (mem) free(mem);
}

char* g_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = (char*)g_malloc(len);
    memcpy(dup, str, len);
    return dup;
}

char* g_strdup_printf(const char *format, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    return g_strdup(buf);
}

GSList* g_slist_append(GSList *list, gpointer data) {
    GSList *new_node = (GSList*)g_malloc(sizeof(GSList));
    new_node->data = data;
    new_node->next = NULL;
    if (!list) return new_node;
    GSList *curr = list;
    while (curr->next) curr = curr->next;
    curr->next = new_node;
    return list;
}

GSList* g_slist_prepend(GSList *list, gpointer data) {
    GSList *new_node = (GSList*)g_malloc(sizeof(GSList));
    new_node->data = data;
    new_node->next = list;
    return new_node;
}

void g_slist_free(GSList *list) {
    while (list) {
        GSList *next = list->next;
        g_free(list);
        list = next;
    }
}

GList* g_list_append(GList *list, gpointer data) {
    GList *new_node = (GList*)g_malloc(sizeof(GList));
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = NULL;
    if (!list) return new_node;
    GList *curr = list;
    while (curr->next) curr = curr->next;
    curr->next = new_node;
    new_node->prev = curr;
    return list;
}

GList* g_list_prepend(GList *list, gpointer data) {
    GList *new_node = (GList*)g_malloc(sizeof(GList));
    new_node->data = data;
    new_node->next = list;
    new_node->prev = NULL;
    if (list) list->prev = new_node;
    return new_node;
}

void g_list_free(GList *list) {
    while (list) {
        GList *next = list->next;
        g_free(list);
        list = next;
    }
}

GString* g_string_new(const gchar *init) {
    GString *s = (GString*)g_malloc(sizeof(GString));
    size_t init_len = init ? strlen(init) : 0;
    s->allocated_len = init_len + 32;
    s->str = (gchar*)g_malloc(s->allocated_len);
    if (init) {
        memcpy(s->str, init, init_len + 1);
        s->len = init_len;
    } else {
        s->str[0] = '\0';
        s->len = 0;
    }
    return s;
}

GString* g_string_append(GString *string, const gchar *val) {
    if (!string || !val) return string;
    size_t add_len = strlen(val);
    if (string->len + add_len + 1 > string->allocated_len) {
        string->allocated_len = (string->len + add_len) * 2 + 16;
        string->str = (gchar*)g_realloc(string->str, string->allocated_len);
    }
    memcpy(string->str + string->len, val, add_len + 1);
    string->len += add_len;
    return string;
}

GString* g_string_append_printf(GString *string, const gchar *format, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    return g_string_append(string, buf);
}

gchar* g_string_free(GString *string, gboolean free_segment) {
    if (!string) return NULL;
    gchar *res = NULL;
    if (free_segment) {
        g_free(string->str);
    } else {
        res = string->str;
    }
    g_free(string);
    return res;
}

gint64 g_get_monotonic_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((gint64)ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

gint64 g_get_real_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((gint64)ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

GMainLoop* g_main_loop_new(void *context, gboolean is_running) {
    (void)context;
    GMainLoop *loop = (GMainLoop*)g_malloc0(sizeof(GMainLoop));
    loop->is_running = is_running;
    loop->ref_count = 1;
    return loop;
}

void g_main_loop_run(GMainLoop *loop) {
    if (!loop) return;
    loop->is_running = TRUE;
    while (loop->is_running) {
        usleep(10000); // 10ms poll tick
    }
}

void g_main_loop_quit(GMainLoop *loop) {
    if (loop) loop->is_running = FALSE;
}

void g_main_loop_unref(GMainLoop *loop) {
    if (!loop) return;
    loop->ref_count--;
    if (loop->ref_count <= 0) {
        g_free(loop);
    }
}
