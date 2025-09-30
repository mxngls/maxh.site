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

#define _SITE_HEADER \
	"    <header>\n"\
	"        <div class=\"bubble\">\n"\
	"            <div id=\"blog\">\n"\
	"                <a href=\"/#index\" role=\"button\">blog</a>\n"\
	"            </div>\n"\
	"            <div id=\"contact\">\n"\
	"                <ul>\n"\
	"                    <li>\n"\
	"                    	<a href=\"https://www.are.na/max-h-ezqbxuoriw4/channels\">arena</a>\n"\
	"                    </li>\n"\
	"                    <li>\n"\
	"                    	<a href=\"https://bsky.app/profile/maxh.site\">bluesky</a>\n"\
	"                    </li>\n"\
	"                    <li>\n"\
	"                    	<a href=\"mailto:maximilian._REMOVE_.e.@gmail.com\">e-mail</a>\n"\
	"                    </li>\n"\
	"                    <li>\n"\
	"                    	<a href=\"https://github.com/mxngls\">github</a>\n"\
	"                    </li>\n"\
	"                </ul>\n"\
	"            </div>\n"\
	"        </div>\n"\
	"    </header>\n"

#define _SITE_FOOTER \
	"    <footer>\n"\
	"        <ul>\n"\
        "           <li id=\"license\">\n"\
	"              <span>Licensed under\n"\
	"              <a rel=\"license\" href=\"https://creativecommons.org/licenses/by-nc-sa/4.0\">CC BY-NC-SA 4.0</a>\n"\
	"              unless marked otherwise.</span>\n"\
        "           </li>\n"\
        "           <li class=\"logo\">\n"\
	"              <a href=\"feed.atom\" alt=\"Atom (feed) logo\">\n"\
	"                  <svg width=\"20\" height=\"20\" viewBox=\"0 -960 960 960\" fill=\"currentColor\"><use href=\"rss-logo.svg#rss-logo\"></use></svg>\n"\
        "              </a>\n"\
	"           </li>\n"\
	"        </ul>\n"\
	"    </footer>\n"
// clang-format on

#define _SITE_FOOTNOTE_WEBCOMPONENT "<script src=\"footnotes.js\"></script>"

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
