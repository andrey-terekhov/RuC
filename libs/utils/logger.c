#include "logger.h"


log current_error_log = &default_error_log;
log current_warning_log = &default_warning_log;
log current_note_log = &default_note_log;


void default_error_log(const char *const tag, const char *const msg);

void default_warning_log(const char *const tag, const char *const msg);

void default_note_log(const char *const tag, const char *const msg);
