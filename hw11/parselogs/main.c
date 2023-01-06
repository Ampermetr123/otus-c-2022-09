#include <assert.h>
#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <gmodule.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STAT_SIZE 10
#define PTR_TO_ULONG(p) ((unsigned long)(p))
#define ULONG_TO_PTR(p) ((void *)(p))
#define _unused_ __attribute__((unused))

/**
 * @brief Создание массива путей к файлам,
 * который находтся в директории dirpath и имена которых начинаются с prefix
 */
GPtrArray *crate_files_array(const char *dirpath, const char *prefix) {
  GError *error = NULL;
  GDir *dir = g_dir_open(dirpath, 0, &error);
  if (error != NULL) {
    g_error("%s", error->message);
    g_clear_error(&error);
    return NULL;
  }
  GPtrArray *ar = g_ptr_array_new();
  const gchar *filename;
  while ((filename = g_dir_read_name(dir))) {
    gchar *filepath = g_strdup_printf("%s/%s", dirpath, filename);
    if (g_file_test(filepath, G_FILE_TEST_IS_REGULAR) && g_str_has_prefix(filename, prefix)) {
      g_ptr_array_add(ar, filepath);
    } else {
      g_free(filepath);
    }
  }
  g_dir_close(dir);
  return ar;
}


/**
 * @brief Выбриает из combined-строки: url , переданные данные (trafic) и refer.
 * @return true , если все подстроки найдены
 * @return false, если ошибка
 */
bool parse_log_string(char *str, char **url, char **refer, unsigned long *trafic) {
  // 127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
  // "http://www.example.com/start.html" "Mozilla/4.08 [en] (Win98; I ;Nav)"
  char *save_ptr, *stmp, *str_trafic;
  strtok_r(str, "\"", &save_ptr);                // пропустили до "
  strtok_r(NULL, " ", &save_ptr);                // пропустили GET
  *url = strtok_r(NULL, " ", &save_ptr);         // взяли /apache_pb.gif
  strtok_r(NULL, "\" ", &save_ptr);              // пропустили до "
  strtok_r(NULL, " ", &save_ptr);                // пропустили 200
  str_trafic = strtok_r(NULL, " \"", &save_ptr); // взяли 2326
  if (!str_trafic) {
    return false;
  }
  *trafic = strtol(str_trafic, &stmp, 10);
  if (stmp == str_trafic || errno == ERANGE) {
    return false;
  }
  *refer = strtok_r(NULL, "\"", &save_ptr); // http://www.example.com/start.html
  if (*refer == NULL)
    return false;
  return true;
}

/*******************************************************************************
 *                         KEY (char*) - VAL (long) STUFF                       *
 *******************************************************************************/
typedef struct {
  gchar *key;
  unsigned long val;
} KeyVal;

gint compare_keyval_by_val(gconstpointer a, gconstpointer b, _unused_ gpointer userdata) {
  if (((const KeyVal *)(a))->val < ((const KeyVal *)(b))->val)
    return 1;
  if (((const KeyVal *)(a))->val > ((const KeyVal *)(b))->val)
    return -1;
  return 0;
}

gint compare_keyval_by_key(gconstpointer a, gconstpointer b, _unused_ gpointer userdata) {
  return g_strcmp0(((const KeyVal *)(a))->key, ((const KeyVal *)(b))->key);
}

void print_keyval(gpointer a, gpointer counter) {
  if (counter) {
    int *pnum = (int *)(counter);
    g_print("%d. ", *pnum);
    (*pnum)++;
  }
  g_print("%s -> %ld\n", ((const KeyVal *)(a))->key, ((const KeyVal *)(a))->val);
}

void on_destroy_keyval(gpointer a) { 
  if (a){
    g_free(((KeyVal *)(a))->key); 
    g_free(a);
  }
}



/** Выборка ТОП самых больших по значению элементов из хеш таблицы в последовательнось */
gboolean ht_iter_move_to_seq(gpointer key, gpointer value, gpointer user_data) {
  GSequence *seq = (GSequence *)(user_data);
  if (g_sequence_get_length(seq) < STAT_SIZE) {
    KeyVal *kv = g_new(KeyVal, 1);
    kv->key = key;
    kv->val = PTR_TO_ULONG(value);
    g_sequence_insert_sorted(seq, kv, compare_keyval_by_val, NULL);
    return true;
  }
  KeyVal *last_kv = (KeyVal *)g_sequence_get(g_sequence_iter_prev(g_sequence_get_end_iter(seq)));
  assert(last_kv);
  if (last_kv->val >= PTR_TO_ULONG(value)) {
    g_free(key);
    return true; // самый маленький элемент в последовательности больше
  }
  KeyVal *kv = g_new(KeyVal, 1);
  if (!kv){
    g_error("Memory allocation error");
    exit(EXIT_FAILURE);
  }
  kv->key = key;
  kv->val = PTR_TO_ULONG(value);
  g_sequence_insert_sorted(seq, kv, compare_keyval_by_val, kv);
  g_sequence_remove(g_sequence_iter_prev(g_sequence_get_end_iter(seq)));
  return true;
}

/*******************************************************************************
 *                             HASH TABLE HELPERS                              *
 *******************************************************************************/

void ht_insert_dupkey_with_increment(GHashTable *ht, gpointer key, gpointer value) {
  gpointer stored_val = g_hash_table_lookup(ht, key);
  if (stored_val == NULL) {
    g_hash_table_insert(ht, g_strdup(key), value);
  } else {
    unsigned long val = PTR_TO_ULONG(stored_val) + PTR_TO_ULONG(value);
    g_hash_table_insert(ht, key, ULONG_TO_PTR(val));
  }
}

gboolean ht_iter_move_to_ht(gpointer key, gpointer value, gpointer userdata) {
  GHashTable *ht = (GHashTable *)(userdata);
  gpointer stored_val = g_hash_table_lookup(ht, key);
  if (stored_val == NULL) {
    g_hash_table_insert(ht, key, value);
  } else {
    unsigned long val = PTR_TO_ULONG(stored_val) + PTR_TO_ULONG(value);
    g_hash_table_insert(ht, key, ULONG_TO_PTR(val));
    g_free(key);
  }
  return true;
}

/*******************************************************************************
 *                              PROCESSING THREAD                              *
 *******************************************************************************/

typedef struct {
  GPtrArray *files;
  volatile gint index;
} SharedData;

typedef struct {
  GHashTable *url_ht;
  GHashTable *refer_ht;
  unsigned long total_trafic;
} ThreadResult;

/**
 * @brief Поток обработки файлов
 * @param data  Входные данные - указатель на SharedData, со списком файлов и индексом не
 * обработанного файла
 * @return void* Выходные данне - указатель на ThreadResult с двумя хеш таблицами, построенными для
 * всех обработаных файлов
 */
void *thread_process(void *data) {
  SharedData *sd = (SharedData *)(data);
  GError *err = NULL;

  // Формируем хеш-таблицы: url -> summary trafic и refer -> counts
  GHashTable *url_ht = g_hash_table_new(g_str_hash, g_str_equal);
  GHashTable *refer_ht = g_hash_table_new(g_str_hash, g_str_equal);
  ThreadResult *res = g_new(ThreadResult, 1);
  if (!res || !url_ht || !refer_ht) {
    g_error("Memory allocation error");
    return NULL;
  }
  res->url_ht = url_ht;
  res->refer_ht = refer_ht;
  res->total_trafic = 0;

  // Атомарно забираем файл на обработку
  gint i = 0;
  while ((uint)(i = g_atomic_int_add(&sd->index, 1)) < sd->files->len) {
    const gchar *file_path = g_ptr_array_index(sd->files, i);
    g_message("Processing file %s", file_path);
    // Подготовка к чтению по строкам
    GFile *f = g_file_new_for_path(file_path);
    if (!f) {
      g_error("Error creating GFile for %s", file_path);
      continue;
    }
    GFileInputStream *fs = g_file_read(f, NULL, &err);
    if (!fs) {
      g_object_unref(f);
      g_error("Error on read file %s: %s", file_path, err->message);
      g_error_free(err);
      continue;
    }
    GDataInputStream *ds = g_data_input_stream_new(G_INPUT_STREAM(fs));
    if (!ds) {
      g_error("Error creating GDataInputSteam for %s", file_path);
      g_object_unref(f);
      g_object_unref(fs);
      continue;
    }

    gchar *line;
    err = NULL;
    long total_lines = 0;
    long bad_lines = 0;
    while ((line = g_data_input_stream_read_line(ds, NULL, NULL, &err)) != NULL && !err) {
      total_lines++;
      char *url = NULL, *refer = NULL;
      unsigned long trafic;
      if (parse_log_string(line, &url, &refer, &trafic) == true) {
        ht_insert_dupkey_with_increment(url_ht, url, ULONG_TO_PTR(trafic));
        ht_insert_dupkey_with_increment(refer_ht, refer, ULONG_TO_PTR(1));
        res->total_trafic += trafic;
      } else {
        bad_lines++;
      }
      g_free(line);
    }
    g_object_unref(ds);
    g_object_unref(f);
    if (err) {
      g_error("Error on reading file %s", file_path);
    } else if (bad_lines > 0) {
      g_warning("unable to parse %lu (%.1f%%) lines in file %s", bad_lines,
                100.0 * bad_lines / total_lines, file_path);
    }
  } // while
  return res;
}

/*******************************************************************************
 *                                    MAIN                                     *
 *******************************************************************************/

// Параметры командной строки (значения по умолчанию)
const char *dir_path = ".";
const char *filename_prefix = "access.log";
int nthreads = 4;

void print_usage(char *prog_name) {
  fprintf(stdout,
          "Apache HTTP server log files analizer for combinted files type\n"
          "Usage: %s [-d <dir>] [-p <prefix>] [-j <N>]\n"
          "-d <dir> - directory with log files, default %s\n"
          "-p <prefix> - log files name prefix, default %s\n"
          "-j <N> - number of processing threads, default %d\n",
          prog_name, dir_path, filename_prefix, nthreads);
}

int main(int argc, char *argv[]) {
  // разбор коммандной строки
  int opt;
  while ((opt = getopt(argc, argv, "d:p:j:h")) != -1) {
    switch (opt) {
    case 'd':
      dir_path = optarg;
      break;
    case 'p':
      filename_prefix = optarg;
      break;
    case 'h':
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    case 'j': {
      char *tmp;
      nthreads = strtol(optarg, &tmp, 10);
      if (tmp == optarg || errno == ERANGE || nthreads <= 0) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
    } break;
    default: /* '?' */
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  };

  // список файлов на обработку
  GPtrArray *files_ar = crate_files_array(dir_path, filename_prefix);
  if (!files_ar) {
    return EXIT_FAILURE;
  }
  if (files_ar->len == 0) {
    g_print("No log-files found\n");
    g_ptr_array_free(files_ar, TRUE);
    return EXIT_SUCCESS;
  }

  GSequence *top_urls = g_sequence_new(on_destroy_keyval);
  GSequence *top_refers = g_sequence_new(on_destroy_keyval);
  GHashTable *sum_url_ht = NULL;
  GHashTable *sum_refer_ht = NULL;
  unsigned long sum_trafic = 0;

  GPtrArray *thread = g_ptr_array_new();
  if (!top_urls || !top_refers || !thread) {
    g_error("Internal memory error");
    return EXIT_FAILURE;
  }

  SharedData sd = {files_ar, 0u};

  for (guint i = 0; i < MIN(files_ar->len, (guint)(nthreads)); i++) {
    GThread *p = g_thread_new("thread", thread_process, &sd);
    if (p == NULL) {
      g_error("Internal error on creating threads");
      exit(EXIT_FAILURE);
    }
    g_ptr_array_add(thread, p);
  }
  ThreadResult *res = NULL;
  for (guint i = 0; i < thread->len; i++) {
    GThread *thr = (GThread *)g_ptr_array_index(thread, i);
    res = (ThreadResult *)g_thread_join(thr);
    if (!res) {
      break; // поток завершился с ошибкой.
    }
    sum_trafic += res->total_trafic;
    if (sum_refer_ht == NULL || sum_url_ht == NULL) {
      sum_refer_ht = res->refer_ht;
      sum_url_ht = res->url_ht;
    } else {
      // объединяем таблицы
      g_hash_table_foreach_remove(res->url_ht, ht_iter_move_to_ht, sum_url_ht);
      g_hash_table_destroy(res->url_ht);
      g_hash_table_foreach_remove(res->refer_ht, ht_iter_move_to_ht, sum_refer_ht);
      g_hash_table_destroy(res->refer_ht);
    }
    g_free(res);
  }
  if (res != NULL){
      // Выбираем из хеш таблиц топ STAT_SIZE записей
      g_hash_table_foreach_remove(sum_url_ht, ht_iter_move_to_seq, top_urls);
      g_hash_table_foreach_remove(sum_refer_ht, ht_iter_move_to_seq, top_refers);
      g_print("\nTotal trafic: %lu\n", sum_trafic);

      puts("\nTOP weighted URLs: ");
      int counter = 1;
      g_sequence_foreach(top_urls, print_keyval, &counter);

      counter = 1;
      puts("\nTOP refers: ");
      g_sequence_foreach(top_refers, print_keyval, &counter);
  }

  if (sum_url_ht) g_hash_table_destroy(sum_url_ht);
  g_sequence_free(top_urls);
  if (sum_refer_ht) g_hash_table_destroy(sum_refer_ht);
  g_sequence_free(top_refers);
  g_ptr_array_set_free_func(files_ar, g_free);
  g_ptr_array_free(files_ar, TRUE);
  g_ptr_array_free(thread, TRUE);
  return (res != NULL) ? EXIT_SUCCESS: EXIT_FAILURE;
}
