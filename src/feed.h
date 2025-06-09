#ifndef FEED_H

#include "html.h"
#include "page.h"

#define _SITE_AUTHOR          "Maximilian Hoenig"
#define _SITE_FEED_ID         "F2B3E23B-ECB6-4EE7-8197-D3C6C2594701"
#define _SITE_HOST            "maxh.site"
#define _SITE_URL             "https://"_SITE_HOST
#define _SITE_TAG_SCHEME_DATE "2024-02-12"

extern page_header_arr header_arr;
extern page_content_arr content_arr;

int create_feed(char *, page_header_arr *);

#endif // FEED_H
