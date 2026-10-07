#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Aligned on the max number of choices allowed
 */
#define VNDS_MAX_STRING_SPLITS 10

/* These strings are specific to VNDS scripts
 * They never contain allocated data and become
 * invalid when the script data is freed
 */
typedef struct VNDS_String
{
    const char* data;
    size_t length;
}VNDS_String;

typedef struct VNDS_StringSplit
{
    VNDS_String splits[VNDS_MAX_STRING_SPLITS];
    int num_splits;
}VNDS_StringSplit;

/* 'source' is moved to the next line (or string end) 
 * 'out' can be NULL, source will be advanced
 */
bool VNDS_ReadLine(const char** source, VNDS_String* out);

void VNDS_TrimString(VNDS_String* source);

/* Splits at most VNDS_MAX_STRING_SPLITS times */
bool VNDS_SplitString(VNDS_String source, VNDS_StringSplit* split, const char* separator);

bool VNDS_StringStartsWithWord(VNDS_String source, VNDS_String target);

int VNDS_StringToInt(VNDS_String source);

char* VNDS_StringToCharArray(VNDS_String source);

VNDS_String VNDS_StringFromCharArray(const char* source);

bool VNDS_StringEqual(VNDS_String s1, VNDS_String s2);

uint8_t* VNDS_LoadFile(const char* path, size_t* ret_size);

bool VNDS_FileExists(const char* path);
