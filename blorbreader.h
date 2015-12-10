#pragma once

GList *blorbreader_get_pictures (const char *filename, GError **error);

GList *blorbreader_get_sounds (const char *filename, GError **error);

void blorbreader_get_first_chunk_of_type (const char *filename, 
    const char *type, unsigned char **data, int *data_len, GError **error);

void blorbreader_get_first_optional_chunk_of_type (const char *filename, 
    const char *type, unsigned char **data, int *data_len, GError **error);

GdkPixbuf *blorbreader_get_pixbuf (const char *filename, int znum, 
    int max_width, int max_height, GError **error);

int blorbreader_read_int32 (unsigned char *p);

