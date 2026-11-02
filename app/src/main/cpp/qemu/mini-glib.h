#ifndef MINI_GLIB_H
#define MINI_GLIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* gpointer;
typedef const void* gconstpointer;
typedef int gboolean;
typedef char gchar;
typedef int gint;
typedef unsigned int guint;
typedef size_t gsize;
typedef int64_t gint64;
typedef uint64_t guint64;

#define TRUE  1
#define FALSE 0

// Memory
void* g_malloc(size_t n_bytes);
void* g_malloc0(size_t n_bytes);
void* g_realloc(void *mem, size_t n_bytes);
void  g_free(void *mem);
char* g_strdup(const char *str);
char* g_strdup_printf(const char *format, ...);

// Singly-linked list
typedef struct _GSList GSList;
struct _GSList {
    gpointer data;
    GSList *next;
};

GSList* g_slist_append(GSList *list, gpointer data);
GSList* g_slist_prepend(GSList *list, gpointer data);
void    g_slist_free(GSList *list);

// Doubly-linked list
typedef struct _GList GList;
struct _GList {
    gpointer data;
    GList *next;
    GList *prev;
};

GList* g_list_append(GList *list, gpointer data);
GList* g_list_prepend(GList *list, gpointer data);
void   g_list_free(GList *list);

// Dynamic string
typedef struct _GString GString;
struct _GString {
    gchar *str;
    gsize len;
    gsize allocated_len;
};

GString* g_string_new(const gchar *init);
GString* g_string_append(GString *string, const gchar *val);
GString* g_string_append_printf(GString *string, const gchar *format, ...);
gchar*   g_string_free(GString *string, gboolean free_segment);

// Monotonic Time
gint64 g_get_monotonic_time(void);
gint64 g_get_real_time(void);

// Main loop stub
typedef struct _GMainLoop GMainLoop;
struct _GMainLoop {
    gboolean is_running;
    int ref_count;
};

GMainLoop* g_main_loop_new(void *context, gboolean is_running);
void       g_main_loop_run(GMainLoop *loop);
void       g_main_loop_quit(GMainLoop *loop);
void       g_main_loop_unref(GMainLoop *loop);

#ifdef __cplusplus
}
#endif

#endif // MINI_GLIB_H
