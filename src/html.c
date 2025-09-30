#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "ghist.h"
#include "html.h"
#include "page.h"

// compare by creation time
static int __qsort_cb(const void *a, const void *b) {
        page_header *header_a = *(page_header **)a;
        page_header *header_b = *(page_header **)b;

        // descending order (newest first)
        if (header_a->meta.created > header_b->meta.created) return -1;
        if (header_a->meta.created < header_b->meta.created) return 1;
        return 0;
}

// package content
static char *html_create_content(page_header *header, char *page_content) {
        char hgroup_fmt[] = "            <hgroup id=\"post-header\">\n"
                            "                <span id=\"date-created\">%s</span>\n"
                            "                <h1>%s</h1>\n"
                            "                <p>%s</p>\n"
                            "            </hgroup>\n";

        char hgroup_with_updated_fmt[] = "            <hgroup id=\"post-header\">\n"
                                         "                <span id=\"date-created\">%s</span>\n"
                                         "                <h1>%s</h1>\n"
                                         "                <p>%s</p>\n"
                                         "                <span id=\"date-updated\">\n"
                                         "                    <small>Last Updated on %s</small>\n"
                                         "                </span>\n"
                                         "            </hgroup>\n";

        size_t buf_size = 24 * 1024;
        char *buf = NULL;
        if ((buf = malloc(buf_size)) == NULL) {
                ERROR(SITE_ERROR_MEMORY_ALLOCATION)
                return NULL;
        }

        char *pos = buf;
        int offset = 0;

        size_t created_formatted_size = 256;
        char created_formatted[created_formatted_size];
        if (header->meta.created) {
                ghist_format_ts("%d %b, %Y", created_formatted, header->meta.created);
        } else {
                snprintf(created_formatted, sizeof(created_formatted), "%s", "DRAFT");
        }

        // add header group (with or without modification date)
        if (header->meta.modified) {
                size_t modified_formatted_size = 256;
                char modified_formatted[modified_formatted_size];
                ghist_format_ts("%Y-%m-%d", modified_formatted, header->meta.modified);
                offset =
                    snprintf(pos, buf_size - offset, hgroup_with_updated_fmt, created_formatted,
                             header->title, header->subtitle, modified_formatted);
        } else {
                offset = snprintf(pos, buf_size - offset, hgroup_fmt, created_formatted,
                                  header->title, header->subtitle);
        }
        pos += offset;

        // separate main content from header group and post footer
        offset = snprintf(pos, buf_size - offset, "%s\n", "<div id=\"post-body\">");
        pos += offset;

        // add content
        char *line = strtok(page_content, "\n");
        while (line) {
                if (*line) {
                        offset = snprintf(pos, buf_size, "%s\n", line);
                        pos += offset;
                }
                line = strtok(NULL, "\n");
        }

        // close main content
        offset = snprintf(pos, buf_size - offset, "%s\n", "</div>");
        pos += offset;

        return buf;
}

// create plain html file
int html_create_page(page_header *header, char *plain_content, char *output_path) {
        // html destination
        FILE *dest_file = fopen(output_path, "w");
        if (dest_file == NULL) {
                ERRORF(SITE_ERROR_FILE_CREATE, output_path);
                free(header);
                return -1;
        }

        int fprintf_ret = 0;

        char created_formatted[256];
        ghist_format_ts("%d %b, %Y", created_formatted, header->meta.created);

        fprintf_ret = fprintf(
            dest_file,
            // clang-format off
            "<!DOCTYPE html>"
            "<html lang=\"en\">\n"
            "<head>\n"
            "    <meta charset=\"utf-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
            "	 <link href=\"/feed.atom\" type=\"application/atom+xml\" rel=\"alternate\">\n"
            "    <link rel=\"stylesheet\" href=\"%s\" type=\"text/css\">\n" _SITE_HTML_FONT "\n"
            "    <title>%s</title>\n"
	    "    %s\n"
            "</head>\n"
            "<body>\n"
	    "<div id=\"post\" class=\"content\">\n"
	    _SITE_HEADER
            "<main>\n"
	    "<div id=\"post-main\">\n",
            // clang-format on
            _SITE_STYLE_SHEET_PATH, header->title, _SITE_FOOTNOTE_WEBCOMPONENT);

        // write content
        char *html_content = NULL;
        if ((html_content = html_create_content(header, plain_content)) == NULL) {
                fclose(dest_file);
                return -1;
        }

        page_content *page_content = NULL;
        if ((page_content = malloc(sizeof(*page_content))) == NULL) {
                free(html_content);
                fclose(dest_file);
                return -1;
        }
        page_content->content = html_content;
        strcpy(page_content->meta.path, header->meta.path);

        content_arr.elems[content_arr.len] = page_content;
        content_arr.len++;
        fprintf_ret = fprintf(dest_file, "%s", html_content);

        // close html
        // clang-format off
        fprintf_ret = fprintf(dest_file, "<div id=\"back-to-top\" class=\"bubble\">"
			                 "    <a href=\"#post\" title=\"Back to top\">↑ Top</a></div>\n"
					 "</div>\n"
					 "</main>\n"
					 _SITE_FOOTER
                                         "</div>\n"
                                         "</body>\n"
                                         "</html>\n");
        // clang-format on

        if (fprintf_ret < 0) {
                ERRORF(SITE_ERROR_FILE_WRITE, dest_file);
                fclose(dest_file);
                return -1;
        }

        fclose(dest_file);

        return 0;
}

// create html index file
int html_create_index(char *page_content, char *output_path, page_header_arr *header_arr,
                      const char *index_excempt_arr[], int index_excempt_arr_n) {
        // html destination
        FILE *dest_file = fopen(output_path, "w");
        if (dest_file == NULL) {
                ERRORF(SITE_ERROR_FILE_CREATE, output_path);
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
            "    <link href=\"/feed.atom\" type=\"application/atom+xml\" rel=\"alternate\">\n"
            "    <link rel=\"stylesheet\" href=\"%s\" type=\"text/css\">\n" _SITE_HTML_FONT "\n"
            "    <title>%s</title>\n"
	    "	 %s\n"
            "</head>\n"
            "<body>\n"
	    "<div id=\"index\" class=\"content\">\n"
	    _SITE_HEADER
            "<main>\n",
            // clang-format on
            _SITE_STYLE_SHEET_PATH, _SITE_TITLE, _SITE_FOOTNOTE_WEBCOMPONENT);

        // content
        char *dest_line = strtok((char *)page_content, "\n");
        while (dest_line) {
                if (!*dest_line) continue;
                fprintf_ret = fprintf(dest_file, "%s\n", dest_line);
                dest_line = strtok(NULL, "\n");
        }

        // sort by creation time
        qsort(header_arr->elems, header_arr->len, sizeof(page_header *), __qsort_cb);

        // add a list of posts to the index
        fprintf_ret = fprintf(dest_file, "<section>\n"
                                         "<h1>Weblog</h1>\n"
                                         "<ul id=\"post-list\">\n");

        for (int i = 0; i < header_arr->len; i++) {
                bool skip = false;
                for (int j = 0; j < index_excempt_arr_n; j++) {
                        int path_len = (int)strlen(header_arr->elems[i]->meta.path);
                        if (path_len > 1 && strncmp(header_arr->elems[i]->meta.path + 1,
                                                    index_excempt_arr[j], path_len - 2) == 0)
                                skip = true;
                }
                if (skip) continue;
                size_t created_formatted_size = 256;
                char created_formatted[created_formatted_size];
                if (header_arr->elems[i]->meta.created) {
                        ghist_format_ts("%Y&#8209;%m&#8209;%d", created_formatted,
                                        header_arr->elems[i]->meta.created);
                } else {
                        snprintf(created_formatted, sizeof(created_formatted), "%s", "DRAFT");
                }
                fprintf_ret = fprintf(dest_file,
                                      "<li>\n"
                                      "<a href=\"%s\">\n"
                                      "<div class=\"title\">%s</div>\n"
                                      "<div class=\"subtitle\">%s</div>\n"
                                      "<div class=\"date\">%s</div>\n"
                                      "</a>\n"
                                      "</li>\n",
                                      header_arr->elems[i]->meta.path, header_arr->elems[i]->title,
                                      header_arr->elems[i]->subtitle, created_formatted);
        }

        fprintf_ret = fprintf(dest_file, "</ul>\n"
                                         "</section>\n");

        // close <main>
        // clang-format off
        fprintf_ret = fprintf(dest_file, "</main>\n"
					 _SITE_FOOTER
					 "</div>\n"
                                         "</body>\n"
                                         "</html>\n");
        // clang-format on

        if (fprintf_ret < 0) {
                ERRORF(SITE_ERROR_FILE_WRITE, dest_file);
                fclose(dest_file);
                return -1;
        }

        fclose(dest_file);

        return 0;
}

// escape html entities
char *html_escape_content(char *html_content) {
        int content_size = 0;
        char *html_content_copy = html_content;
        while (*html_content_copy++) {
                content_size++;
        }

        // quiete conservative estimate
        unsigned long escaped_size = (unsigned long)(content_size * 2);
        char *escaped = malloc(escaped_size);
        escaped[0] = '\0';

        while (*html_content) {
                switch (*html_content) {
                case '"':
                        strcat(escaped, "&quot;");
                        break;
                case '\'':
                        strcat(escaped, "&#39;");
                        break;
                case '&':
                        strcat(escaped, "&amp;");
                        break;
                case '<':
                        strcat(escaped, "&lt;");
                        break;
                case '>':
                        strcat(escaped, "&gt;");
                        break;
                default:
                        strncat(escaped, html_content, 1);
                }
                html_content++;
        }
        return escaped;
}
