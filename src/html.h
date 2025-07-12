#ifndef HTML_H
#define HTML_H

#include "page.h"

#define _SITE_STYLE_SHEET_PATH "style.css"
#define _SITE_TITLE            "Max's Homepage"

// clang-format off
#define _SITE_HTML_FONT \
	"    <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n" \
	"    <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n" \
	"    <link href=\"https://fonts.googleapis.com/css2?family=Source+Sans+3:ital,wght@0,200..900;1,200..900&family=Source+Serif+4:ital,opsz,wght@0,8..60,200..900;1,8..60,200..900&display=swap\" rel=\"stylesheet\">\n"

// NOTE: currently not used
// #define _SITE_HEADER \
// 	"    <header>\n"\
// 	"        <nav>\n" \
// 	"            <ul>\n" \
// 	"                <li><a href=\"/\">/</a></li>\n" \
// 	"                <li id=\"index-title\"><b>maxh.site</b></li>\n" \
// 	"            </ul>\n" \
// 	"        </nav>\n" \
// 	"    </header>\n"

#define _SITE_FOOTER \
	"    <footer>\n"\
	"    <ul>\n"\
	"	 <li>\n"\
	"	     <a href=\"mailto:maximilian._REMOVE_.e.@gmail.com\">\n"\
	"               e-mail\n"\
	"	     </a>\n"\
	"        </li>\n"\
	"        <li>\n"\
	"            <a href=\"about.html\">about</a>\n"\
	"        </li>\n"\
	"        <li>\n"\
	"            <a href=\"feed.atom\">rss</a>\n"\
	"        </li>\n"\
	"    </ul>\n"\
	"    </footer>\n"
// clang-format on

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
int html_create_index(char *, char *, page_header_arr *, const char *[], int);
char *html_escape_content(char *);

#endif // HTML_H
