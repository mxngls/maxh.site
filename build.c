#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fts.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <git2.h>

#ifndef _SITE_EXT_TARGET_DIR
#define _SITE_EXT_TARGET_DIR "docs"
#endif

#ifndef _SITE_EXT_GIT_DIR
#define _SITE_EXT_GIT_DIR ".git"
#endif

#define _SITE_TITLE            "Max's Homepage"
#define _SITE_SOURCE_DIR       "src/"
#define _SITE_INDEX_PATH       "index.html"
#define _SITE_STYLE_SHEET_PATH "style.css"
#define _SITE_PATH_MAX         100
#define _SITE_PAGES_MAX        50

// clang-format off
#define _SITE_HTML_FONT \
	"    <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n" \
	"    <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n" \
	"    <link href=\"https://fonts.googleapis.com/css2?family=Source+Sans+3:ital,wght@0,200..900;1,200..900&family=Source+Serif+4:ital,opsz,wght@0,8..60,200..900;1,8..60,200..900&display=swap\" rel=\"stylesheet\">\n"

#define _SITE_HEADER \
	"    <header>\n"\
	"        <nav>\n" \
	"            <ul>\n" \
	"                <li><a href=\"/\">Home</a></li>\n" \
	"                <li id=\"index-title\"><b>maxh.site</b></li>\n" \
	"            </ul>\n" \
	"        </nav>\n" \
	"    </header>\n"
// clang-format on

typedef struct {
        const char *title;
        const char *subtitle;
        struct meta {
                char path[_SITE_PATH_MAX];
                int64_t created;
                int64_t modified;
        } meta;
} page_header;

typedef struct {
        page_header *elems[_SITE_PAGES_MAX];
        int len;
} page_header_arr;

typedef struct {
        char *file_path;
        git_time_t creat_time;
        git_time_t mod_time;
} tracked_file;

typedef struct {
        tracked_file *files;
        int len;
        int capacity;
} tracked_file_arr;

typedef struct {
        char *old_path;
        char *new_path;
        git_time_t creat_time;
} rename_record;

typedef struct {
        rename_record *records;
        int len;
        int capacity;
} renamed_file_arr;

static tracked_file_arr tracked_arr = {.files = NULL, .len = 0, .capacity = 0};
static renamed_file_arr rename_arr = {0};

// utils
int __copy_file(const char *, const char *);
FTS *__init_fts(const char *);
int __create_output_dirs(void);

// work with page headers
int compare_page_header(const void *, const void *);
int parse_page_header(FILE *, page_header *);

// libgit2 related (obtain modification and creation times)
void add_rename(const char *, const char *, git_time_t);
void trace_rename(const char *, git_time_t *, git_time_t *);
tracked_file *find_file_by_path(const char *);
int get_times_cb(const git_diff_delta *, __attribute__((unused)) float progress, void *payload);
int get_times(void);

// main routines
page_header *process_page_file(FTSENT *);
int process_index_file(char *, page_header_arr *);
int create_html_page(page_header *, char *, const char *);
int create_html_index(char *, const char *, page_header_arr *);

int __copy_file(const char *from, const char *to) {
        FILE *from_file = NULL;
        FILE *to_file = NULL;

        if ((from_file = fopen(from, "r")) == NULL) {
                fprintf(stderr, "from_file: %s\n\t%s (errno: %d, line: %d)\n", from,
                        strerror(errno), errno, __LINE__);
                return -1;
        }

        if ((to_file = fopen(to, "w")) == NULL) {
                fprintf(stderr, "to_file: %s\n\t%s (errno: %d, line: %d)\n", to, strerror(errno),
                        errno, __LINE__);
                fclose(from_file);
                return -1;
        }

        char *line = NULL;
        size_t bufsize = 0;
        ssize_t len = 0;
        int res = 0;

        while ((len = getline(&line, &bufsize, from_file)) > 0) {
                if (fwrite(line, 1, (size_t)len, to_file) != (size_t)len) {
                        fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno,
                                __LINE__);
                        res = -1;
                        break;
                }
        }

        if (len < 0 && !feof(from_file) && ferror(from_file)) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                res = -1;
        }

        free(line);
        fclose(from_file);
        fclose(to_file);

        return res;
}

int __create_output_dirs(void) {
        mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

        if (mkdir(_SITE_EXT_TARGET_DIR, mode) != 0 && errno != EEXIST) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                return -1;
        }

        if (mkdir(_SITE_EXT_TARGET_DIR, mode) != 0 && errno != EEXIST) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                return -1;
        }
        return 0;
}

FTS *__init_fts(const char *source) {
        FTS *ftsp = NULL;
        char *paths[] = {(char *)source, NULL};
        int _fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;

        if ((ftsp = fts_open(paths, _fts_options, NULL)) == NULL) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                return NULL;
        }

        if (fts_children(ftsp, 0) == NULL) {
                printf("No pages to convert. Aborting\n");
                return NULL;
        }

        return ftsp;
}

int compare_page_header(const void *a, const void *b) {
        const page_header *header_a = *(const page_header **)a;
        const page_header *header_b = *(const page_header **)b;

        // descending order (newest first)
        if (header_a->meta.created > header_b->meta.created) return -1;
        if (header_a->meta.created < header_b->meta.created) return 1;
        return 0;
}

int parse_page_header(FILE *file, page_header *header) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read = 0;
        ssize_t readt = 0;

        header->title = NULL;
        header->subtitle = NULL;

        bool in_header = true;
        while (in_header && (readt += read = getline(&line, &len, file))) {
                // newline
                if (read <= 1 || line[0] == '\n') {
                        in_header = false;
                        break;
                }

                // remove newline
                if (line[read - 1] == '\n') {
                        line[read - 1] = '\0';
                        read--;
                }

                // split fields
                char *colon = strchr(line, ':');
                if (!colon) continue;

                // key-value pair
                *colon = '\0';
                char *key = line;
                char *value = colon + 1;

                while (isspace(*value)) {
                        value++;
                }

                if (!value) {
                        value = NULL;
                }

                if (strncmp(key, "title", read) == 0) header->title = strdup(value);
                else if (strncmp(key, "subtitle", read) == 0) header->subtitle = strdup(value);
        }

        free(line);

        if (!header->title || !header->subtitle) return -1;

        return (int)readt;
}

int create_html_index(char *page_content, const char *output_path, page_header_arr *header_arr) {
        // html destination
        FILE *dest_file = fopen(output_path, "w");
        if (dest_file == NULL) {
                fprintf(stderr, "Failed to create %s: %s\n", output_path, strerror(errno));
                return -1;
        }

        int fprintf_ret = 0;

        fprintf_ret = fprintf(
            dest_file,
            // clang-format off
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "    <head>\n"
            "    <meta charset=\"utf-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "    <link href=\"/atom.xml\" type=\"application/atom+xml\" rel=\"alternate\">\n"
            "    <link rel=\"stylesheet\" href=\"%s\" type=\"text/css\">\n" _SITE_HTML_FONT "\n"
            "    <title>%s</title>\n"
            "</head>\n"
            "<body>\n"
	         _SITE_HEADER
            "    <main>\n",
            // clang-format on
            _SITE_STYLE_SHEET_PATH, _SITE_TITLE);

        // content
        char *dest_line = strtok((char *)page_content, "\n");
        while (dest_line) {
                if (!*dest_line) continue;
                fprintf_ret = fprintf(dest_file, "%s\n", dest_line);
                dest_line = strtok(NULL, "\n");
        }

        // sort by creation time
        qsort(header_arr->elems, header_arr->len, sizeof(page_header *), compare_page_header);

        // add a list of posts to the index
        fprintf_ret = fprintf(dest_file, "<section>\n"
                                         "    <dl id=\"post-list\">\n");

        for (int i = 0; i < header_arr->len; i++) {
                fprintf_ret = fprintf(dest_file,
                                      "    <dt>\n"
                                      "         <b><a href=\"%s\">%s</a></b>\n"
                                      "    </dt>\n"
                                      "    <dd>%s</dd>\n",
                                      header_arr->elems[i]->meta.path, header_arr->elems[i]->title,
                                      header_arr->elems[i]->subtitle);
        }

        fprintf_ret = fprintf(dest_file, "    </dl>\n"
                                         "</section>\n");

        // close <main>
        fprintf_ret = fprintf(dest_file, "    </main>\n"
                                         "</body>\n"
                                         "</html>\n");

        if (fprintf_ret < 0) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                fclose(dest_file);
                return -1;
        }

        fclose(dest_file);

        return 0;
}

void format_ts(char *format_str, char *formatted, time_t timestamp) {
        time_t time = (time_t)timestamp;
        struct tm tm;
        if (gmtime_r(&time, &tm)) {
                strftime(formatted, 256, format_str, &tm);
        } else {
                strcpy(formatted, "Invalid date");
        }
}

int create_html_page(page_header *header, char *page_content, const char *output_path) {
        // html destination
        FILE *dest_file = fopen(output_path, "w");
        if (dest_file == NULL) {
                fprintf(stderr, "Failed to create %s: %s\n", output_path, strerror(errno));
                free(header);
                return -1;
        }

        int fprintf_ret = 0;

        char created_formatted[256];
        format_ts("%d %b, %Y", created_formatted, header->meta.created);

        fprintf_ret = fprintf(
            dest_file,
            // clang-format off
            "<!DOCTYPE html>"
            "<html lang=\"en\">\n"
            "<head>\n"
            "    <meta charset=\"utf-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "	 <link href=\"/atom.xml\" type=\"application/atom+xml\" rel=\"alternate\">\n"
            "    <link rel=\"stylesheet\" href=\"%s\" type=\"text/css\">\n" _SITE_HTML_FONT "\n"
            "    <title>%s</title>\n"
            "</head>\n"
            "<body>\n"
	         _SITE_HEADER
            "    <main>\n"
	    "        <article>\n",
            // clang-format on
            _SITE_STYLE_SHEET_PATH, header->title);

        // add header group
        fprintf_ret = fprintf(dest_file,
                              "            <hgroup>\n"
                              "                <p><small id=\"date-created\">%s</small></p>\n"
                              "                <h1>%s</h1>\n"
                              "                <p>%s</p>\n"
                              "            </hgroup>\n",
                              created_formatted, header->title, header->subtitle);

        // content
        char *line = strtok((char *)page_content, "\n");
        while (line) {
                if (!*line) continue;
                fprintf_ret = fprintf(dest_file, "%s\n", line);
                line = strtok(NULL, "\n");
        }
        fprintf_ret = fprintf(dest_file, "        </article>\n"
                                         "    </main>\n");

        if (header->meta.modified) {
                char modified_formatted[256];
                format_ts("%Y-%m-%d", modified_formatted, header->meta.modified);
                fprintf_ret =
                    fprintf(dest_file,
                            "        <footer>\n"
                            "            <small id=\"date-updated\">Last Updated on %s</small>\n"
                            "        </footer>\n",
                            modified_formatted);
        }

        // close html
        fprintf_ret = fprintf(dest_file, "</body>\n"
                                         "</html>\n");

        if (fprintf_ret < 0) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                fclose(dest_file);
                return -1;
        }

        fclose(dest_file);

        return 0;
}

page_header *process_page_file(FTSENT *ftsentp) {
        page_header *res = NULL;
        FILE *source_file = NULL;
        tracked_file *tracked = NULL;
        page_header *header = NULL;
        char *page_content = NULL;

        if ((source_file = fopen(ftsentp->fts_path, "r")) == NULL) {
                fprintf(stderr, "Failed to open source %s: %s (errno: %d, line: %d)\n",
                        ftsentp->fts_path, strerror(errno), errno, __LINE__);
                goto error;
        }

        // output path
        char page_path[_SITE_PATH_MAX];
        snprintf(page_path, sizeof(page_path), "%s/%s", _SITE_EXT_TARGET_DIR, ftsentp->fts_name);

        if ((header = calloc(1, sizeof(page_header))) == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                goto error;
        }
        char page_href[100] = "/";
        strcat(page_href, ftsentp->fts_name);
        strncpy(header->meta.path, page_href, _SITE_PATH_MAX - 1);

        if ((tracked = find_file_by_path(ftsentp->fts_path))) {
                header->meta.created = tracked->creat_time;
                header->meta.modified = tracked->mod_time;
        }

        // read content
        int header_len = -1;
        if ((header_len = parse_page_header(source_file, header)) == -1) {
                fprintf(stderr, "Title and subtitle headers missing: %s\n", ftsentp->fts_name);
                goto error;
        };
        size_t content_size = ftsentp->fts_statp->st_size - header_len;
        page_content = malloc(content_size + 1);
        if (page_content == NULL) {
                fprintf(stderr, "Memory allocation failed for content\n");
                goto error;
        }
        size_t bytes_read = fread(page_content, 1, content_size, source_file);
        if (bytes_read != content_size) {
                if (feof(source_file)) {
                        fprintf(stderr, "Page has no content. Aborting.\n");
                } else if (ferror(source_file)) {
                        fprintf(stderr, "Failed to open source %s: %s (errno: %d, line: %d)\n",
                                ftsentp->fts_path, strerror(errno), errno, __LINE__);
                } else {
                        fprintf(
                            stderr,
                            "Reported Source file size length and number of bytes read differs.\n");
                }
                goto error;
        }
        page_content[bytes_read] = '\0';

        // create valid html file
        if (create_html_page(header, page_content, page_path) != 0) {
                goto error;
        };

        res = header;
        header = NULL;
        goto cleanup;

error:
        res = NULL;

cleanup:
        if (source_file) fclose(source_file);
        if (page_content) free(page_content);
        if (header) free(header);

        return res;
}

int process_index_file(char *index_file_path, page_header_arr *header_arr) {
        int res = 0;
        FILE *source_file = NULL;
        char *page_content = NULL;

        if ((source_file = fopen(index_file_path, "r")) == NULL) {
                fprintf(stderr, "Failed to open source %s: %s (errno: %d, line: %d)\n",
                        index_file_path, strerror(errno), errno, __LINE__);
                goto error;
        }

        struct stat source_file_stat;
        if (stat(index_file_path, &source_file_stat) != 0) {
                fprintf(stderr, "%s (errno: %d, line: %d)\n", strerror(errno), errno, __LINE__);
                goto error;
        }

        // output path
        char page_path[_SITE_PATH_MAX];
        const char *filename = strrchr(index_file_path, '/');
        filename ? filename++ : (filename = index_file_path);
        snprintf(page_path, sizeof(page_path), "%s/%s", _SITE_EXT_TARGET_DIR, filename);

        size_t content_size = source_file_stat.st_size;
        if ((page_content = malloc(content_size + 1)) == NULL) {
                fprintf(stderr, "Memory allocation failed for content\n");
                goto error;
        }

        ssize_t bytes_read = fread(page_content, 1, source_file_stat.st_size, source_file);
        if (bytes_read != source_file_stat.st_size) {
                if (feof(source_file)) {
                        fprintf(stderr, "Unexpected EOF. Read %zu bytes, expected %jd\n",
                                bytes_read, (intmax_t)source_file_stat.st_size);
                } else if (ferror(source_file)) {
                        fprintf(stderr, "Failed to read from source %s: %s (errno: %d, line: %d)\n",
                                index_file_path, strerror(errno), errno, __LINE__);
                } else {
                        fprintf(
                            stderr,
                            "Reported Source file size length and number of bytes read differs.\n");
                }
                goto error;
        }
        page_content[bytes_read] = '\0';
        res = create_html_index(page_content, page_path, header_arr);
        goto cleanup;

error:
        res = -1;

cleanup:
        if (page_content) free(page_content);
        if (source_file) fclose(source_file);

        return res;
}

tracked_file *find_file_by_path(const char *file_path) {
        for (int i = 0; i < tracked_arr.len; i++) {
                if (strcmp(tracked_arr.files[i].file_path, file_path) == 0) {
                        return &tracked_arr.files[i];
                }
        }
        return NULL;
}

void add_rename(const char *old_path, const char *new_path, git_time_t timestamp) {
        if (rename_arr.records == NULL) {
                rename_arr.records = malloc(sizeof(rename_record) * 100);
                rename_arr.capacity = 100;
        } else if (rename_arr.capacity == rename_arr.len) {
                rename_arr.capacity *= 2;
                rename_arr.records =
                    realloc(rename_arr.records, rename_arr.capacity * sizeof(rename_record));
        }

        rename_arr.records[rename_arr.len] = (rename_record){
            .old_path = strdup(old_path),
            .new_path = strdup(new_path),
            .creat_time = timestamp,
        };
        rename_arr.len++;
}

void trace_rename(const char *final_path, git_time_t *creation_time,
                  git_time_t *modification_time) {

        char *current_path = strdup(final_path);

        for (int i = rename_arr.len - 1; i >= 0; i--) {
                if (strcmp(rename_arr.records[i].new_path, current_path) != 0) continue;

                *modification_time =
                    *modification_time == 0 ? rename_arr.records[i].creat_time : *modification_time;
                *creation_time = rename_arr.records[i].creat_time;

                current_path = strdup(rename_arr.records[i].old_path);

                i = rename_arr.len;
        }

        free(current_path);
}

int get_times_cb(const git_diff_delta *delta, __attribute__((unused)) float progress,
                 void *payload) {
        if (!delta || !delta->new_file.path) return 0;

        // Ensure array capacity
        if (tracked_arr.files == NULL) {
                tracked_arr.files = malloc(sizeof(tracked_file) * 100);
                tracked_arr.capacity = 100;
        } else if (tracked_arr.capacity == tracked_arr.len) {
                tracked_arr.capacity *= 2;
                tracked_arr.files =
                    realloc(tracked_arr.files, tracked_arr.capacity * sizeof(tracked_file));
                if (!tracked_arr.files) return -1;
        }

        const char *file_path = delta->new_file.path;
        const char *old_file_path = delta->old_file.path;

        git_signature *signature = (git_signature *)payload;
        git_time_t author_time = signature->when.time;

        // rename detected
        if (delta->similarity > 50 && strcmp(old_file_path, file_path) != 0) {
                add_rename(old_file_path, file_path, author_time);

                if (access(file_path, F_OK) == 0 && !find_file_by_path(file_path)) {
                        tracked_file new_file = {
                            .file_path = strdup(file_path),
                            .creat_time = author_time,
                            .mod_time = author_time,
                        };
                        tracked_arr.files[tracked_arr.len] = new_file;
                        tracked_arr.len++;
                }
                return 0;
        }

        // regular file change
        if (access(file_path, F_OK) != 0) return 0;

        tracked_file *tracked = find_file_by_path(file_path);
        if (tracked) {
                tracked->mod_time = author_time;
                return 0;
        }

        // new file
        tracked_file new_file = {
            .file_path = strdup(file_path),
            .creat_time = author_time,
            .mod_time = 0,
        };

        tracked_arr.files[tracked_arr.len] = new_file;
        tracked_arr.len++;

        return 0;
}

int get_times(void) {
        int res = 0;

        git_libgit2_init();

        git_oid oid;
        git_repository *repo = NULL;
        git_revwalk *walker = NULL;
        git_commit *commit = NULL;
        git_commit *parent = NULL;
        git_tree *tree = NULL;
        git_tree *parent_tree = NULL;
        git_diff *diff = NULL;

        if (git_repository_open(&repo, _SITE_EXT_GIT_DIR) != 0) goto error;
        if (git_revwalk_new(&walker, repo)) goto error;
        if (git_revwalk_sorting(walker, GIT_SORT_TIME)) goto error;
        if (git_revwalk_push_head(walker)) goto error;

        while (git_revwalk_next(&oid, walker) == 0) {
                // free previously allocted resources
                // clang-format off
                if (commit) { git_commit_free(commit); commit = NULL; }
                if (parent) { git_commit_free(parent); parent = NULL; }
                if (tree) { git_tree_free(tree); tree = NULL; }
                if (parent_tree) { git_tree_free(parent_tree); parent_tree = NULL; }
                if (diff) { git_diff_free(diff); diff = NULL; }
                // clang-format on

                if (git_commit_lookup(&commit, repo, &oid)) goto error;

                int parent_count = git_commit_parentcount(commit);
                if (parent_count != 1) continue;

                if (git_commit_parent(&parent, commit, 0)) goto error;
                if (git_commit_tree(&tree, commit)) goto error;
                if (git_commit_tree(&parent_tree, parent)) goto error;
                if (git_diff_tree_to_tree(&diff, repo, parent_tree, tree, NULL)) goto error;

                // enable dection of renamed files
                git_diff_find_options *find_opts = malloc(sizeof(git_diff_find_options));
                if (git_diff_find_options_init(find_opts, GIT_DIFF_FIND_OPTIONS_VERSION))
                        goto error;
                find_opts->flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_IGNORE_WHITESPACE;
                if (git_diff_find_similar(diff, find_opts)) goto error;

                const git_signature *signature = git_commit_author(commit);
                if (git_diff_foreach(diff, &get_times_cb, NULL, NULL, NULL, (void *)signature))
                        goto error;
        }

        // resolve renames
        for (int i = 0; i < tracked_arr.len; i++) {
                git_time_t creation_time = 0;
                git_time_t last_rename_time = 0;
                trace_rename(tracked_arr.files[i].file_path, &creation_time, &last_rename_time);
                if (creation_time > 0) {
                        tracked_arr.files[i].creat_time = creation_time;
                }
                if (last_rename_time > 0) {
                        tracked_arr.files[i].mod_time = last_rename_time;
                }
        }

        goto cleanup;

error:
        res = -1;
        const git_error *err = git_error_last();
        fprintf(stderr, "Git error: %s\n", err ? err->message : "unknown error");

cleanup:
        git_repository_free(repo);
        git_revwalk_free(walker);
        git_commit_free(commit);
        git_commit_free(parent);
        git_tree_free(tree);
        git_tree_free(parent_tree);

        return res;
}

int main(void) {
        int res = 0;
        FTS *ftsp = NULL;
        FTSENT *ftsentp = NULL;

        page_header_arr header_arr = {
            .elems = {0},
            .len = 0,
        };

        if (__create_output_dirs() != 0) {
                res = -1;
        }

        if (__copy_file(_SITE_SOURCE_DIR _SITE_STYLE_SHEET_PATH,
                        _SITE_EXT_TARGET_DIR _SITE_STYLE_SHEET_PATH) != 0) {
                res = -1;
        }

        res = get_times();

        if ((ftsp = __init_fts(_SITE_SOURCE_DIR)) == NULL) {
                res = -1;
        }

        while ((ftsentp = fts_read(ftsp)) != NULL) {
                // we only care for plain non-hidden __files__
                if (ftsentp->fts_info != FTS_F) continue;
                if (ftsentp->fts_name[0] == '.') continue;

                char *dot = strrchr(ftsentp->fts_name, '.');
                if (dot == NULL) continue;
                char *ext = dot + 1;

                // non-html files
                if (strcmp(ext, "html") != 0) {
                        char to_path[_SITE_PATH_MAX];
                        to_path[0] = '\0';

                        strlcat(to_path, _SITE_EXT_TARGET_DIR, sizeof(to_path));
                        size_t path_len = strlen(to_path);

                        // possibly add path separator
                        if (path_len > 0 && to_path[path_len - 1] != '/' &&
                            path_len + 1 < sizeof(to_path)) {
                                to_path[path_len] = '/';
                                to_path[path_len + 1] = '\0';
                        }

                        strlcat(to_path, ftsentp->fts_name, sizeof(to_path));

                        __copy_file(ftsentp->fts_path, to_path);
                        continue;
                }

                // ignore index for now
                if (strcmp(ftsentp->fts_name, _SITE_INDEX_PATH) == 0) {
                        continue;
                }

                page_header *header = NULL;
                if ((header = process_page_file(ftsentp)) == NULL) {
                        res = -1;
                } else {
                        header_arr.elems[header_arr.len] = header;
                        header_arr.len++;
                }
        }

        if (process_index_file(_SITE_SOURCE_DIR _SITE_INDEX_PATH, &header_arr) != 0) {
                res = -1;
        }

        // cleanup
        fts_close(ftsp);
        for (int i = 0; i < header_arr.len; i++) {
                free((char *)header_arr.elems[i]->title);
                free((char *)header_arr.elems[i]->subtitle);
                free(header_arr.elems[i]);
        }
        for (int i = 0; i < tracked_arr.len; i++) {
                free(tracked_arr.files[i].file_path);
        }

        return res;
}
