#pragma once
#include <stdbool.h>
#include <stdint.h>


typedef enum VN_EventType
{
    VN_EVENT_QUIT,
    VN_EVENT_TEXT_CONFIRMED,
    VN_EVENT_CHOICE_MADE
}VN_EventType;


typedef struct VN_Point
{
    int x;
    int y;
}VN_Point;

typedef struct VN_Event
{
    VN_EventType type;
    uint64_t timestamp;

    int choice;
}VN_Event;


typedef struct VN_Image VN_Image;

typedef struct VN_Context VN_Context;


VN_Context* VN_Init(const char* name, int width, int height);

bool VN_Quit(void);


VN_Context* VN_GetContext(void);

bool VN_SetContext(VN_Context* context);


const char* VN_GetError(void);


bool VN_PollEvent(VN_Event* event);


VN_Image* VN_LoadImage(const char* path);

bool VN_DestroyImage(VN_Image* image);


bool VN_SetBackground(VN_Image* background, uint64_t fade_time);


bool VN_SetForeground(VN_Image* foreground, VN_Point position, uint64_t fade_time);

bool VN_RemoveForeground(VN_Image* foreground, uint64_t fade_time);

bool VN_ClearForegrounds(uint64_t fade_time);


bool VN_SetText(const char* text, bool confirm);

bool VN_ClearText(void);


bool VN_Step(void);
