#ifndef HTML_H
#define HTML_H

#include "page.h"

#define _SITE_TITLE "Max's Homepage"

#define _SITE_SOURCE_DIR       "content"
#define _SITE_BLOCK_DIR_PATH   _SITE_SOURCE_DIR "/blocks"
#define _SITE_STYLE_SHEET_PATH "style.css"

#define _SITE_HTML_FONT ""

#define _SITE_SCRIPT "<script src=\"script.js\" defer></script>"

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

typedef struct {
        long len;
        char *content;
} page_block;

extern page_content_arr content_arr;

// global template content (loaded at startup)
extern char *site_header;
extern char *site_footer;
extern char *site_hgroup;
extern char *site_hgroup_updated;

// initialize templates
int html_init_templates(void);
void html_cleanup_templates(void);

// create html files
int html_create_page(page_header *, char *, char *);
int html_create_index(char *, char *, page_header_arr *, const char *[], int);
char *html_escape_content(char *);

#endif // HTML_H
