#ifndef LOG_H
#define LOG_H

//printf, dar scire si ora in fara 
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));


#endif 