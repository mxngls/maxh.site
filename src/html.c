#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ghist.h"
#include "html.h"
#include "page.h"

// compare by creation time
static int qsort_cb(const void *a, const void *b) {
        page_header *header_a = *(page_header **)a;
        page_header *header_b = *(page_header **)b;

        // descending order (newest first)
        if (header_a->meta.created > header_b->meta.created) return -1;
        if (header_a->meta.created < header_b->meta.created) return 1;
        return 0;
}

int html_create_page(page_header *header, char *page_content, char *output_path) {
        // html destination
        FILE *dest_file = fopen(output_path, "w");
        if (dest_file == NULL) {
                fprintf(stderr, "Failed to create %s: %s\n", output_path, strerror(errno));
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
                              "                <p>\n"
                              "                    <small id=\"date-created\">%s</small>\n"
                              "                </p>\n"
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
                ghist_format_ts("%Y-%m-%d", modified_formatted, header->meta.modified);
                fprintf_ret =
                    fprintf(dest_file,
                            "        <p>\n"
                            "            <small id=\"date-updated\">Last Updated on %s</small>\n"
                            "        </p>\n",
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

int html_create_index(char *page_content, char *output_path, page_header_arr *header_arr) {
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
        qsort(header_arr->elems, header_arr->len, sizeof(page_header *), qsort_cb);

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
