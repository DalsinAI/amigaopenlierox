/* OpenLieroX for AmigaOS: no libcurl. HTTP.cpp keeps its code; every
 * transfer fails at once with CURLE_COULDNT_CONNECT, as with no network.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_CURL_STUB_H
#define AMIGA_CURL_STUB_H
#include <stddef.h>
typedef void CURL;
typedef enum { CURLE_OK = 0, CURLE_COULDNT_CONNECT = 7 } CURLcode;
typedef enum {
	CURLOPT_URL = 1, CURLOPT_PROXY, CURLOPT_USERAGENT, CURLOPT_NOSIGNAL,
	CURLOPT_CONNECTTIMEOUT, CURLOPT_FOLLOWLOCATION, CURLOPT_MAXREDIRS,
	CURLOPT_IPRESOLVE, CURLOPT_TIMEOUT, CURLOPT_WRITEDATA,
	CURLOPT_WRITEFUNCTION, CURLOPT_NOPROGRESS, CURLOPT_PROGRESSDATA,
	CURLOPT_PROGRESSFUNCTION, CURLOPT_HTTPPOST
} CURLoption;
typedef enum {
	CURLINFO_CONTENT_LENGTH_UPLOAD = 1, CURLINFO_CONTENT_TYPE,
	CURLINFO_SIZE_UPLOAD, CURLINFO_SPEED_DOWNLOAD, CURLINFO_SPEED_UPLOAD
} CURLINFO;
typedef enum {
	CURLFORM_END = 0, CURLFORM_COPYNAME, CURLFORM_COPYCONTENTS,
	CURLFORM_CONTENTSLENGTH, CURLFORM_CONTENTTYPE, CURLFORM_FILENAME
} CURLformoption;
#define CURL_IPRESOLVE_V4 1L
#define CURL_GLOBAL_ALL 3L
struct curl_httppost { int unused; };
struct curl_slist { char *data; struct curl_slist *next; };

static inline CURLcode curl_global_init(long) { return CURLE_OK; }
static inline void curl_global_cleanup(void) {}
static inline CURL *curl_easy_init(void) { static int handle; return &handle; }
static inline void curl_easy_cleanup(CURL *) {}
static inline CURLcode curl_easy_setopt(CURL *, CURLoption, ...) { return CURLE_OK; }
static inline CURLcode curl_easy_perform(CURL *) { return CURLE_COULDNT_CONNECT; }
static inline const char *curl_easy_strerror(CURLcode) { return "no network in the Amiga port"; }
static inline CURLcode curl_easy_getinfo(CURL *, CURLINFO info, const void *out)
{
	if (info == CURLINFO_CONTENT_TYPE) *(const char **)(void *)out = NULL;
	else *(double *)(void *)out = 0.0;
	return CURLE_OK;
}
static inline int curl_formadd(struct curl_httppost **, struct curl_httppost **, ...) { return 1; }
static inline void curl_formfree(struct curl_httppost *) {}
static inline struct curl_slist *curl_slist_append(struct curl_slist *l, const char *) { return l; }
static inline void curl_slist_free_all(struct curl_slist *) {}
#endif
