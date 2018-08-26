#pragma once

int charutils_gunichar2_strlen (const gunichar2 *s);
void charutils_utf16_char_to_utf8 (gunichar2 c, char *out, int len);
GString *charutils_utf16_string_to_utf8 (const gunichar2 *s, int len);
void charutils_append_utf16_char_to_utf8_string (gunichar2 c, GString *str);

