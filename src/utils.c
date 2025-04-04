#include "../include/utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

void log_message(const char *format, ...)
{
    va_list    args;
    time_t     now;
    struct tm *timeinfo;
    char       timestamp[32];

    // Get current time
    time(&now);
    timeinfo = localtime(&now);

    // Format timestamp
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(stderr, "[%s][%d] ", timestamp, (int)getpid());

    // Print message
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
}

const char *get_file_extension(const char *path)
{
    const char *dot;
    if(!path)
    {
        return "";
    }

    // locate last occurence of '.'
    dot = strrchr(path, '.');
    if(!dot || dot == path)
    {
        return "";
    }

    return dot + 1;
}

const char *get_mime_type(const char *ext)
{
    if(!ext)
    {
        return "application/octet-stream";
    }

    if(strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0)
    {
        return "text/html";
    }
    else if(strcasecmp(ext, "txt") == 0)
    {
        return "text/plain";
    }
    else if(strcasecmp(ext, "css") == 0)
    {
        return "text/css";
    }
    else if(strcasecmp(ext, "js") == 0)
    {
        return "application/javascript";
    }
    else if(strcasecmp(ext, "json") == 0)
    {
        return "application/json";
    }
    else if(strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0)
    {
        return "image/jpeg";
    }
    else if(strcasecmp(ext, "png") == 0)
    {
        return "image/png";
    }
    else if(strcasecmp(ext, "gif") == 0)
    {
        return "image/gif";
    }
    else if(strcasecmp(ext, "svg") == 0)
    {
        return "image/svg+xml";
    }
    else if(strcasecmp(ext, "ico") == 0)
    {
        return "image/x-icon";
    }
    else if(strcasecmp(ext, "pdf") == 0)
    {
        return "application/pdf";
    }
    else if(strcasecmp(ext, "xml") == 0)
    {
        return "application/xml";
    }

    return "application/octet-stream";
}

int url_decode(const char *src, char *dest, size_t size)
{
    size_t src_len;
    size_t j;

    if(!src || !dest || size == 0)
    {
        return -1;
    }

    src_len = strlen(src);

    // Mainly handle % and +
    for(size_t i = 0, j = 0; i < src_len && j < size - 1; i++)
    {
        if(src[i] == '%' && i + 2 < src_len)
        {
            /* Get the hex digits */
            char a = src[i + 1];
            char b = src[i + 2];

            /* Convert hex to decimal */
            if(a >= '0' && a <= '9')
            {
                a -= '0';
            }
            else if(a >= 'a' && a <= 'f')
            {
                a = a - 'a' + 10;
            }
            else if(a >= 'A' && a <= 'F')
            {
                a = a - 'A' + 10;
            }
            else
            {
                dest[j++] = src[i];
                continue;
            }

            if(b >= '0' && b <= '9')
            {
                b -= '0';
            }
            else if(b >= 'a' && b <= 'f')
            {
                b = b - 'a' + 10;
            }
            else if(b >= 'A' && b <= 'F')
            {
                b = b - 'A' + 10;
            }
            else
            {
                dest[j++] = src[i];
                continue;
            }

            dest[j++] = (char)((a << 4) | b);
            i += 2;
        }
        else if(src[i] == '+')
        {
            dest[j++] = ' ';
        }
        else
        {
            dest[j++] = src[i];
        }
    }

    dest[j] = '\0';
    return 0;
}

int generate_key(char *key, size_t size)
{
    struct timeval      tv;
    static unsigned int counter = 0;    // persistent
    char                hostname[256];

    if(!key || size < 32)
    {
        return -1;
    }

    gettimeofday(&tv, NULL);

    if(gethostname(hostname, sizeof(hostname)) < 0)
    {
        strncpy(hostname, "unknown", sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }

    snprintf(key, size, "%ld_%ld_%d_%u_%s", (long)tv.tv_sec, (long)tv.tv_usec, (int)getpid(), counter++, hostname);
    return 0;
}
