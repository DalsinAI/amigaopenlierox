/* OpenLieroX for AmigaOS: no libzip. Only the client's mod download
 * (a network feature) unzips, so every zip_open() fails.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_ZIP_STUB_H
#define AMIGA_ZIP_STUB_H
struct zip;
struct zip_file;
#define ZIP_FL_NOCASE 1
static inline struct zip *zip_open(const char *, int, int *) { return 0; }
static inline int zip_close(struct zip *) { return -1; }
static inline int zip_name_locate(struct zip *, const char *, int) { return -1; }
static inline int zip_get_num_files(struct zip *) { return 0; }
static inline const char *zip_get_name(struct zip *, int, int) { return 0; }
static inline struct zip_file *zip_fopen_index(struct zip *, int, int) { return 0; }
static inline long zip_fread(struct zip_file *, void *, unsigned long) { return -1; }
static inline int zip_fclose(struct zip_file *) { return -1; }
#endif
