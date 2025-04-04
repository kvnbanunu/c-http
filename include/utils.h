#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/**
 * Log a message to stderr
 * @param format Format string
 * @param ... Variable arguments
 */
void log_message(const char *format, ...);

/**
 * Get the file extension from a path
 * @param path File path
 * @return Pointer to extension or empty string if not found
 */
const char *get_file_extension(const char *path);

/**
 * Get the MIME type for a file extension
 * @param ext File extension
 * @return MIME type string
 */
const char *get_mime_type(const char *ext);

/**
 * URL decode a string
 * @param src Source string
 * @param dest Destination buffer
 * @param size Size of destination buffer
 * @return 0 on success, -1 on failure
 */
int url_decode(const char *src, char *dest, size_t size);

/**
 * Generate a unique key for database storage
 * @param key Buffer to store the key
 * @param size Size of the buffer
 * @return 0 on success, -1 on failure
 */
int generate_key(char *key, size_t size);

#endif /* UTILS_H */
