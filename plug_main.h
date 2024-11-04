#ifndef PLUG_MAIN_H_
#define PLUG_MAIN_H_

#include <stdlib.h>

void (*plug_init)(char* filename) = NULL;
void* (*plug_pre_reload)(void) = NULL;
void (*plug_post_reload)(void*) = NULL;
void (*plug_update)(void) = NULL;

#endif // PLUG_MAIN_H_