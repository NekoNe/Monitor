

int url_is_absolute(char *url);

int url_get_folder(char *url_in, char *url_out);

int url_is_file(char *url);

char *url_strnchr(char *str, char symb, size_t n);

int url_get_full_path(char *folder, char *rel_path, char *result);

