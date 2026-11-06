/* The few libxml2 calls OpenLieroX makes, on OXP (DalsinAI/gfopenxml): its
 * HTML reader (oxp_html.h) parses, and the tree is the libxml2 shape the
 * game walks (name, children, next, content, doc). Only what the game uses;
 * no libxml2 code. amiga/libxml_oxp.c.
 * MIT licence. Copyright (c) 2026 AmiCradle Team. */
#ifndef AMIGA_OXP_LIBXML_H
#define AMIGA_OXP_LIBXML_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char xmlChar;

typedef enum {
    XML_ELEMENT_NODE = 1,
    XML_ATTRIBUTE_NODE = 2,
    XML_TEXT_NODE = 3,
    XML_DOCUMENT_NODE = 9,
    XML_HTML_DOCUMENT_NODE = 13
} xmlElementType;

typedef struct _xmlAttr {
    struct _xmlAttr *next;
    xmlChar *name, *value;
} xmlAttr, *xmlAttrPtr;

struct _xmlDoc;
typedef struct _xmlNode {
    void *_private;
    xmlElementType type;
    const xmlChar *name;               /* "text" for text */
    struct _xmlNode *children, *last, *parent, *next, *prev;
    struct _xmlDoc *doc;
    xmlChar *content;                  /* a text node's text */
    xmlAttr *properties;
} xmlNode, *xmlNodePtr, *htmlNodePtr;

typedef struct _xmlDoc {
    void *_private;
    xmlElementType type;
    char *name;
    struct _xmlNode *children, *last, *parent, *next, *prev;
    struct _xmlDoc *doc;
    const xmlChar *URL;                /* always NULL here */
} xmlDoc, *xmlDocPtr, *htmlDocPtr;

typedef struct _xmlError { int code; } xmlError, *xmlErrorPtr;
typedef void (*xmlStructuredErrorFunc)(void *ctx, xmlErrorPtr error);

typedef struct _htmlParserCtxt { const char *data; int size; } htmlParserCtxt, *htmlParserCtxtPtr;

typedef struct _htmlSAXHandler {
    void (*characters)(void *ctx, const xmlChar *ch, int len);
    void *unused[31];                  /* the game only memsets them */
} htmlSAXHandler, *htmlSAXHandlerPtr;

enum { HTML_PARSE_RECOVER = 1, HTML_PARSE_NOERROR = 32, HTML_PARSE_NOWARNING = 64 };

#define LIBXML_TEST_VERSION ((void)0)

int xmlStrcmp(const xmlChar *a, const xmlChar *b);
int xmlStrcasecmp(const xmlChar *a, const xmlChar *b);
int xmlStrlen(const xmlChar *s);
void xmlFree(void *p);

xmlNodePtr xmlDocGetRootElement(const xmlDoc *doc);
xmlChar *xmlGetProp(const xmlNode *node, const xmlChar *name);
xmlChar *xmlNodeListGetString(xmlDocPtr doc, const xmlNode *list, int inLine);
xmlNodePtr xmlNewNode(void *ns, const xmlChar *name);
void xmlNodeAddContent(xmlNodePtr node, const xmlChar *content);
xmlNodePtr xmlAddChild(xmlNodePtr parent, xmlNodePtr child);
xmlNodePtr xmlCopyNode(xmlNodePtr node, int recursive);
void xmlUnlinkNode(xmlNodePtr node);
void xmlFreeNode(xmlNodePtr node);
void xmlFreeDoc(xmlDocPtr doc);
void xmlDocDumpFormatMemory(xmlDocPtr doc, xmlChar **mem, int *size, int format);

htmlParserCtxtPtr htmlCreateMemoryParserCtxt(const char *buffer, int size);
htmlDocPtr htmlCtxtReadDoc(htmlParserCtxtPtr ctxt, const xmlChar *cur, const char *URL, const char *encoding, int options);
void htmlFreeParserCtxt(htmlParserCtxtPtr ctxt);
htmlDocPtr htmlSAXParseDoc(const xmlChar *cur, const char *encoding, htmlSAXHandlerPtr sax, void *userData);

void xmlSetStructuredErrorFunc(void *ctx, xmlStructuredErrorFunc handler);
void xmlResetLastError(void);
void xmlCleanupParser(void);

#ifdef __cplusplus
}
#endif
#endif
