#include "VNDS_utils.h"

#include <string.h>
#include <ctype.h>
#include <SDL3/SDL.h> // for SDL_IOStream may remove later

bool VNDS_ReadLine(const char** source, VNDS_String* out)
{
    const char* start = *source;
    const char* end = strpbrk(start, "\r\n");
    
    if (!end)
        end = start + strlen(start);
        
    if ((end == start) && !(*end))
        return false;
        
    *source = *end ? end + 1 : end;
    if (**source == '\n')
        (*source)++;
    
    if (out)
    {
        out->data = start;
        out->length = end - start;
    }
    
    return true;
}

void VNDS_TrimString(VNDS_String* source)
{
    size_t start = 0;
    size_t end = source->length;

    while ((start < end) && (isspace((unsigned char)source->data[start])))
        start++;

    while ((end > start) && (isspace((unsigned char)source->data[end - 1])))
        end--;

    source->data += start;
    source->length = end - start;
}

bool VNDS_SplitString(VNDS_String source, VNDS_StringSplit* split, const char* separator)
{
    split->num_splits = 0;
    size_t start = 0;
    size_t end = 0;
    
    while (true)
    {
        while ((start < source.length) && (strchr(separator, source.data[start])))
            start++;
            
        if (start == source.length)
            return (split->num_splits > 0) ? true : false;
        else if (split->num_splits == VNDS_MAX_STRING_SPLITS)
            return true;
            
        end = start;
        while ((end < source.length) && (!strchr(separator, source.data[end])))
            end++;
            
        split->splits[split->num_splits].data = &source.data[start];
        split->splits[split->num_splits].length = end - start;
        split->num_splits++;
        
        start = end;
    }
}

bool VNDS_StringStartsWithWord(VNDS_String source, VNDS_String target)
{
    if (source.length < target.length)
        return false;
        
    if (strncmp(source.data, target.data, target.length) != 0)
        return false;
        
    if ((source.length != target.length) && (!isspace((unsigned char)source.data[target.length])))
        return false;
        
    return true;
}

int VNDS_StringToInt(VNDS_String source)
{
    char buf[255];
    if (source.length >= sizeof(buf))
        return 0;
        
    memcpy(buf, source.data, source.length);
    buf[source.length] = '\0';
    
    return atoi(buf);
}

char* VNDS_StringToCharArray(VNDS_String source)
{
    char* res;
    asprintf(&res, "%.*s", (int)source.length, source.data);
    return res;
}

VNDS_String VNDS_StringFromCharArray(const char* source)
{
    return (VNDS_String){source, strlen(source)};
}

bool VNDS_StringEqual(VNDS_String s1, VNDS_String s2)
{
    if (s1.length != s2.length)
        return false;
        
    return strncmp(s1.data, s2.data, s1.length) == 0;
}

uint8_t* VNDS_LoadFile(const char* path, size_t* ret_size)
{
    SDL_IOStream* stream = SDL_IOFromFile(path, "rb");
    if (!stream)
        return NULL;
        
    size_t size = SDL_GetIOSize(stream);
    uint8_t* buf = malloc(size + 1);
    buf[size] = '\0';
    SDL_ReadIO(stream, buf, size);
    SDL_CloseIO(stream);
    
    if (ret_size)
        *ret_size = size;

    return buf;
}

bool VNDS_FileExists(const char* path)
{
    return SDL_GetPathInfo(path, NULL);
}
