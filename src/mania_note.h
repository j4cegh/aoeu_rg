#pragma once

#include "object.h"
#include "textures.h"

typedef struct mania_note
{
    object *host_obj;
    int lane;
} mania_note;

mania_note *mania_note_init(object *host_obj);
void mania_note_free(mania_note *ptr);
void mania_note_update(mania_note *ptr);
void mania_note_render(mania_note *ptr);
int *mania_note_get_key(mania_note *ptr);
loaded_texture *mania_note_get_tex(mania_note *ptr);
char *mania_note_serialize(mania_note *ptr);
void mania_note_overwrite(mania_note *ptr, char *serialized_data);
char mania_note_key_good(mania_note *ptr);
