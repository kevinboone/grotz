#pragma once

GString *fileutils_get_dirname (const char *file);
GString *fileutils_strip_extension (const char *filename);
GString *fileutils_get_extension (const char *filename);
GString *fileutils_get_filename (const char *path);
GString *fileutils_concat_path (const char *dir, const char *filename);
GString *fileutils_get_full_argv0 (void);

