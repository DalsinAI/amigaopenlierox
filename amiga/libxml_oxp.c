/* The libxml2 calls OpenLieroX makes (amiga/include/libxml/oxp_libxml.h),
 * on OXP: oxp_html_parse reads the HTML, and its tree is copied into
 * malloc'd nodes of libxml2's shape, so the chat box can add and drop lines
 * for as long as a game runs and give their memory back.
 * MIT licence. Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>
#include <string.h>

#include "libxml/oxp_libxml.h"
#include "oxp_html.h"

static const xmlChar text_name[] = "text";

static xmlChar *dup_n(const char *s, size_t n)
{
    xmlChar *d = malloc(n + 1);
    if (d) { memcpy(d, s, n); d[n] = 0; }
    return d;
}
static xmlChar *dup_s(const xmlChar *s) { return s ? dup_n((const char *)s, strlen((const char *)s)) : NULL; }

int xmlStrcmp(const xmlChar *a, const xmlChar *b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp((const char *)a, (const char *)b);
}

int xmlStrcasecmp(const xmlChar *a, const xmlChar *b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    for (;; a++, b++) {
        int x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y || !x) return x - y;
    }
}

int xmlStrlen(const xmlChar *s) { return s ? (int)strlen((const char *)s) : 0; }
void xmlFree(void *p) { free(p); }

static void set_doc(xmlNodePtr n, xmlDocPtr d)
{
    for (; n; n = n->next) {
        n->doc = d;
        set_doc(n->children, d);
    }
}

static xmlNodePtr new_node(xmlElementType type, const xmlChar *name)
{
    xmlNodePtr n = calloc(1, sizeof *n);
    if (!n) return NULL;
    n->type = type;
    if (type == XML_TEXT_NODE) n->name = text_name;
    else if (!(n->name = dup_s(name))) { free(n); return NULL; }
    return n;
}

static void append(xmlNodePtr parent, xmlNodePtr child)
{
    child->parent = parent;
    child->prev = parent->last;
    child->next = NULL;
    if (parent->last) parent->last->next = child; else parent->children = child;
    parent->last = child;
}

xmlNodePtr xmlDocGetRootElement(const xmlDoc *doc)
{
    xmlNodePtr n;
    if (!doc) return NULL;
    for (n = doc->children; n; n = n->next) if (n->type == XML_ELEMENT_NODE) return n;
    return NULL;
}

xmlChar *xmlGetProp(const xmlNode *node, const xmlChar *name)
{
    const xmlAttr *a;
    if (!node || node->type != XML_ELEMENT_NODE || !name) return NULL;
    for (a = node->properties; a; a = a->next)
        if (!xmlStrcasecmp(a->name, name)) return dup_s(a->value);
    return NULL;
}

xmlChar *xmlNodeListGetString(xmlDocPtr doc, const xmlNode *list, int inLine)
{
    size_t n = 0;
    const xmlNode *t;
    xmlChar *s, *p;
    (void)doc; (void)inLine;
    for (t = list; t; t = t->next) if (t->type == XML_TEXT_NODE && t->content) n += strlen((const char *)t->content);
    if (!(s = p = malloc(n + 1))) return NULL;
    for (t = list; t; t = t->next)
        if (t->type == XML_TEXT_NODE && t->content) {
            size_t k = strlen((const char *)t->content);
            memcpy(p, t->content, k);
            p += k;
        }
    *p = 0;
    return s;
}

xmlNodePtr xmlNewNode(void *ns, const xmlChar *name)
{
    (void)ns;
    return new_node(XML_ELEMENT_NODE, name);
}

void xmlNodeAddContent(xmlNodePtr node, const xmlChar *content)
{
    xmlNodePtr t;
    if (!node || !content) return;
    if (node->type == XML_TEXT_NODE) {
        size_t a = node->content ? strlen((const char *)node->content) : 0, b = strlen((const char *)content);
        xmlChar *s = realloc(node->content, a + b + 1);
        if (s) { memcpy(s + a, content, b + 1); node->content = s; }
        return;
    }
    if (!(t = new_node(XML_TEXT_NODE, NULL))) return;
    if (!(t->content = dup_s(content))) { free(t); return; }
    append(node, t);
    t->doc = node->doc;
}

void xmlUnlinkNode(xmlNodePtr n)
{
    xmlNodePtr parent;
    if (!n) return;
    parent = n->parent;
    if (n->prev) n->prev->next = n->next;
    else if (parent) parent->children = n->next;
    else if (n->doc && n->doc->children == n) n->doc->children = n->next;
    if (n->next) n->next->prev = n->prev;
    else if (parent) parent->last = n->prev;
    else if (n->doc && n->doc->last == n) n->doc->last = n->prev;
    n->parent = n->prev = n->next = NULL;
}

xmlNodePtr xmlAddChild(xmlNodePtr parent, xmlNodePtr child)
{
    if (!parent || !child || parent == child) return NULL;
    xmlUnlinkNode(child);
    append(parent, child);
    child->doc = NULL;
    set_doc(child, parent->doc);
    child->next = NULL;
    return child;
}

static void free_attrs(xmlAttr *a)
{
    while (a) {
        xmlAttr *next = a->next;
        free(a->name); free(a->value); free(a);
        a = next;
    }
}

static void free_tree(xmlNodePtr n)
{
    while (n) {
        xmlNodePtr next = n->next;
        free_tree(n->children);
        free_attrs(n->properties);
        if (n->name != text_name) free((void *)n->name);
        free(n->content);
        free(n);
        n = next;
    }
}

void xmlFreeNode(xmlNodePtr n)
{
    if (!n) return;
    n->next = NULL;                    /* the node and what it holds, not its siblings */
    free_tree(n);
}

static xmlNodePtr copy_one(const xmlNode *src, int deep)
{
    xmlNodePtr n = new_node(src->type, src->name);
    const xmlAttr *a;
    xmlAttr **tail;
    xmlNodePtr c;
    if (!n) return NULL;
    if (src->content && !(n->content = dup_s(src->content))) { xmlFreeNode(n); return NULL; }
    tail = &n->properties;
    for (a = src->properties; a; a = a->next) {
        xmlAttr *b = calloc(1, sizeof *b);
        if (!b || !(b->name = dup_s(a->name)) || !(b->value = dup_s(a->value))) {
            if (b) { free(b->name); free(b); }
            xmlFreeNode(n);
            return NULL;
        }
        *tail = b;
        tail = &b->next;
    }
    if (deep)
        for (c = src->children; c; c = c->next) {
            xmlNodePtr k = copy_one(c, 1);
            if (!k) { xmlFreeNode(n); return NULL; }
            append(n, k);
        }
    return n;
}

xmlNodePtr xmlCopyNode(xmlNodePtr node, int recursive)
{
    return node ? copy_one(node, recursive) : NULL;
}

void xmlFreeDoc(xmlDocPtr doc)
{
    if (!doc) return;
    free_tree(doc->children);
    free(doc);
}

/* OXP's tree into libxml2's shape. */
static xmlNodePtr from_oxp(const oxp_node *o)
{
    xmlNodePtr n;
    const oxp_node *c;
    const oxp_dattr *a;
    xmlAttr **tail;
    if (o->type == OXP_NODE_TEXT || o->type == OXP_NODE_CDATA) {
        if (!(n = new_node(XML_TEXT_NODE, NULL))) return NULL;
        if (!(n->content = dup_n(o->text, o->len))) { xmlFreeNode(n); return NULL; }
        return n;
    }
    if (o->type != OXP_NODE_ELEMENT) return NULL;
    if (!(n = new_node(XML_ELEMENT_NODE, (const xmlChar *)o->name))) return NULL;
    tail = &n->properties;
    for (a = o->attrs; a; a = a->next) {
        xmlAttr *b = calloc(1, sizeof *b);
        if (!b || !(b->name = dup_s((const xmlChar *)a->local)) || !(b->value = dup_n(a->value, a->value_len))) {
            if (b) { free(b->name); free(b); }
            xmlFreeNode(n);
            return NULL;
        }
        *tail = b;                     /* OXP keeps them newest first; the order doesn't matter here */
        tail = &b->next;
    }
    for (c = o->first; c; c = c->next) {
        xmlNodePtr k;
        if (c->type != OXP_NODE_ELEMENT && c->type != OXP_NODE_TEXT && c->type != OXP_NODE_CDATA) continue;
        if (!(k = from_oxp(c))) { xmlFreeNode(n); return NULL; }
        append(n, k);
    }
    return n;
}

static htmlDocPtr parse(const char *data, size_t len)
{
    oxp_doc *o = oxp_html_parse(data, len);
    xmlDocPtr d;
    xmlNodePtr root;
    if (!o) return NULL;
    root = from_oxp(oxp_dom_root(o));
    oxp_dom_free(o);
    if (!root || !(d = calloc(1, sizeof *d))) { xmlFreeNode(root); return NULL; }
    d->type = XML_HTML_DOCUMENT_NODE;
    d->children = d->last = root;
    set_doc(root, d);
    return d;
}

htmlParserCtxtPtr htmlCreateMemoryParserCtxt(const char *buffer, int size)
{
    htmlParserCtxtPtr c;
    if (!buffer || size < 0 || !(c = malloc(sizeof *c))) return NULL;
    c->data = buffer;
    c->size = size;
    return c;
}

htmlDocPtr htmlCtxtReadDoc(htmlParserCtxtPtr ctxt, const xmlChar *cur, const char *URL, const char *encoding, int options)
{
    (void)URL; (void)encoding; (void)options;
    if (ctxt) return parse(ctxt->data, (size_t)ctxt->size);
    return cur ? parse((const char *)cur, strlen((const char *)cur)) : NULL;
}

void htmlFreeParserCtxt(htmlParserCtxtPtr ctxt) { free(ctxt); }

/* The SAX form: only characters, for the text outside script and style. */
static void characters(const xmlNode *n, htmlSAXHandlerPtr sax, void *user)
{
    for (; n; n = n->next) {
        if (n->type == XML_TEXT_NODE && n->content)
            sax->characters(user, n->content, (int)strlen((const char *)n->content));
        else if (n->type == XML_ELEMENT_NODE && xmlStrcasecmp(n->name, (const xmlChar *)"script") &&
                 xmlStrcasecmp(n->name, (const xmlChar *)"style"))
            characters(n->children, sax, user);
    }
}

htmlDocPtr htmlSAXParseDoc(const xmlChar *cur, const char *encoding, htmlSAXHandlerPtr sax, void *userData)
{
    htmlDocPtr d;
    (void)encoding;
    if (!cur) return NULL;
    d = parse((const char *)cur, strlen((const char *)cur));
    if (!sax) return d;                /* as libxml2: no handler, the tree */
    if (d && sax->characters) characters(d->children, sax, userData);
    xmlFreeDoc(d);
    return NULL;
}

/* The tree back as HTML text. */
typedef struct { char *s; size_t n, cap; int oom; } out;

static void emit(out *o, const char *s, size_t n)
{
    if (o->oom) return;
    if (o->n + n + 1 > o->cap) {
        size_t cap = o->cap ? o->cap * 2 : 1024;
        char *t;
        while (o->n + n + 1 > cap) cap *= 2;
        if (!(t = realloc(o->s, cap))) { o->oom = 1; return; }
        o->s = t; o->cap = cap;
    }
    memcpy(o->s + o->n, s, n);
    o->n += n;
    o->s[o->n] = 0;
}
static void emit_s(out *o, const char *s) { emit(o, s, strlen(s)); }

static void emit_escaped(out *o, const xmlChar *s, int attr)
{
    for (; s && *s; s++) {
        if (*s == '&') emit_s(o, "&amp;");
        else if (*s == '<') emit_s(o, "&lt;");
        else if (*s == '>') emit_s(o, "&gt;");
        else if (attr && *s == '"') emit_s(o, "&quot;");
        else emit(o, (const char *)s, 1);
    }
}

static const char *const void_names[] = { "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "source", "wbr", NULL };

static void dump(out *o, const xmlNode *n, int format, int depth)
{
    for (; n; n = n->next) {
        const xmlAttr *a;
        const char *const *v;
        int is_void = 0;
        if (n->type == XML_TEXT_NODE) { emit_escaped(o, n->content, 0); continue; }
        if (n->type != XML_ELEMENT_NODE) continue;
        emit_s(o, "<");
        emit_s(o, (const char *)n->name);
        for (a = n->properties; a; a = a->next) {
            emit_s(o, " ");
            emit_s(o, (const char *)a->name);
            emit_s(o, "=\"");
            emit_escaped(o, a->value, 1);
            emit_s(o, "\"");
        }
        emit_s(o, ">");
        for (v = void_names; *v; v++) if (!xmlStrcasecmp(n->name, (const xmlChar *)*v)) is_void = 1;
        if (is_void) continue;
        dump(o, n->children, format, depth + 1);
        emit_s(o, "</");
        emit_s(o, (const char *)n->name);
        emit_s(o, ">");
        if (format && depth < 2) emit_s(o, "\n");
    }
}

void xmlDocDumpFormatMemory(xmlDocPtr doc, xmlChar **mem, int *size, int format)
{
    out o = { NULL, 0, 0, 0 };
    if (mem) *mem = NULL;
    if (size) *size = 0;
    if (!doc || !mem) return;
    emit_s(&o, "<!DOCTYPE html>\n");
    dump(&o, doc->children, format, 0);
    if (o.oom) { free(o.s); return; }
    *mem = (xmlChar *)o.s;
    if (size) *size = (int)o.n;
}

void xmlSetStructuredErrorFunc(void *ctx, xmlStructuredErrorFunc handler) { (void)ctx; (void)handler; }
void xmlResetLastError(void) {}
void xmlCleanupParser(void) {}
