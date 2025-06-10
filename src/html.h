#ifndef HTML_H
#define HTML_H

#include "page.h"

#define _SITE_STYLE_SHEET_PATH "style.css"
#define _SITE_TITLE            "Max's Homepage"
#define _SITE_LICENSE          "CC BY-NC-ND 4.0"

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
	"                <li><a href=\"/feed.atom\">feed</a></li>\n" \
	"                <li id=\"index-title\"><b>maxh.site</b></li>\n" \
	"            </ul>\n" \
	"        </nav>\n" \
	"    </header>\n"

#define _SITE_FOOTER \
	"    <footer>\n" \
	"        <ul>\n" \
	"            <li>\n" \
	"                <a\n" \
	"                   href=\"https://creativecommons.org/licenses/by-nc-nd/4.0/\"\n" \
	"                   title=\"Creative Commons License Attribution-NonCommercial-NoDerivatives 4.0 International\"\n" \
	"                   target=\"_blank\"\n" \
	"                   rel=\"noopener\">\n" \
			    _SITE_LICENSE "\n" \
	"                </a>\n" \
	"            </li>\n" \
	"        </ul>\n" \
	"    </footer>\n"
//clang-format on

typedef struct {
        char *content;
        struct {
                char path[_SITE_PATH_MAX];
        } meta;
} page_content;

typedef struct {
        page_content *elems[_SITE_PAGES_MAX];
        int len;
} page_content_arr;

extern page_content_arr content_arr;

// create html files
int html_create_page(page_header *, char *, char *);
int html_create_index(char *, char *, page_header_arr *);
char *html_escape_content(char *);

#endif // HTML_H
