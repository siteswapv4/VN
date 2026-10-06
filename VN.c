#include "VN.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>


#define VN_ERROR(ERR) \
    do { \
        VN_context->error = ERR; \
        goto error; \
    }while (0)

#define VN_MAX_EVENTS 255
#define VN_MAX_FOREGROUNDS 255
#define VN_MAX_LINES 50

typedef struct VN_Duration
{
    uint64_t start;
    uint64_t end;
}VN_Duration;

typedef struct VN_Image
{
    SDL_Texture* texture;
}VN_Image;

typedef struct VN_Background
{
    VN_Image* image;
    VN_Image* old_image;
    
    VN_Duration fade;
}VN_Background;

typedef struct VN_Foreground
{
    VN_Image* image;
    VN_Point position;
    
    VN_Duration fade;
    bool disappearing;
}VN_Foreground;

typedef struct VN_Text
{
    TTF_Text* lines[VN_MAX_LINES];
    int num_lines;
    
    int total_width;
    VN_Duration scroll_time;
    bool finished_scrolling;
}VN_Text;

typedef struct VN_Context
{
    const char* error;

    int width;
    int height;

    SDL_Window* window;
    SDL_Renderer* renderer;
    
    TTF_Font* font;
    TTF_TextEngine* text_engine;
    
    VN_Background background;
    VN_Foreground foregrounds[VN_MAX_FOREGROUNDS];
    int num_foregrounds;
    
    VN_Text text;
    
    VN_Event events[VN_MAX_EVENTS];
    int num_events;
}VN_Context;


// TODO : thread safety ?
VN_Context* VN_context = NULL;


VN_Context* VN_Init(const char* name, int width, int height)
{
    VN_context = SDL_calloc(1, sizeof(VN_Context));
    if (!VN_context)
        goto error;
        
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        goto error;
        
    if (!SDL_CreateWindowAndRenderer(name, width, height, SDL_WINDOW_RESIZABLE, &VN_context->window, &VN_context->renderer))
        goto error;
        
    SDL_SetRenderLogicalPresentation(VN_context->renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(VN_context->renderer, SDL_BLENDMODE_BLEND);
    
    if (!TTF_Init())
        goto error;
    
    VN_context->font = TTF_OpenFont("font.otf", height / 15); // TODO : right font size
    if (!VN_context->font)
        goto error;
    
    VN_context->text_engine = TTF_CreateRendererTextEngine(VN_context->renderer);
    if (!VN_context->text_engine)
        goto error;
        
    for (int i = 0; i < VN_MAX_LINES; i++)
    {
        VN_context->text.lines[i] = TTF_CreateText(VN_context->text_engine, VN_context->font, "", 0);
    }
    
    VN_context->width = width;
    VN_context->height = height;
    
    return VN_context;
    
error:
    SDL_Log("Failed to init : %s", SDL_GetError());
    VN_Quit();

    return NULL;
}

bool VN_Quit(void)
{
    if (!VN_context)
        return false;
        
    if (VN_context->text_engine)
        TTF_DestroyRendererTextEngine(VN_context->text_engine);
        
    if (VN_context->font)
        TTF_CloseFont(VN_context->font);
        
    TTF_Quit();
        
    if (VN_context->renderer)
        SDL_DestroyRenderer(VN_context->renderer);
        
    if (VN_context->window)
        SDL_DestroyWindow(VN_context->window);
        
    SDL_Quit();
    
    VN_context = NULL;
    
    return true;
}


VN_Context* VN_GetContext(void)
{
    return VN_context;
}

bool VN_SetContext(VN_Context* context)
{
    VN_context = context;
}


const char* VN_GetError(void)
{
    if (!VN_context)
        return NULL;

    return VN_context->error;
}


bool VN_PollEvent(VN_Event* event)
{
    if (!VN_context)
        return false;

    if (VN_context->num_events <= 0)
        return false;
        
    if (!event)
        return true;
        
    *event = VN_context->events[VN_context->num_events - 1];
    VN_context->num_events--;

    return true;
}


VN_Image* VN_LoadImage(const char* path)
{
    if (!VN_context)
        return NULL;

    VN_Image* image = SDL_calloc(1, sizeof(VN_Image));
    
    image->texture = IMG_LoadTexture(VN_context->renderer, path);
    if (!image->texture)
        VN_ERROR(SDL_GetError());
        
    SDL_SetTextureBlendMode(image->texture, SDL_BLENDMODE_BLEND);
        
    return image;
        
error:
    VN_DestroyImage(image);
    return NULL;
}

bool VN_DestroyImage(VN_Image* image)
{
    if (!VN_context)
        return false;

    if (!image)
        VN_ERROR("NULL image");
        
    if (image->texture)
        SDL_DestroyTexture(image->texture);
        
    return true;
    
error:
    return false;
}


VN_Duration VN_NewDuration(uint64_t time)
{
    return (VN_Duration){SDL_GetTicks(), SDL_GetTicks() + time};
}


bool VN_SetBackground(VN_Image* image, uint64_t fade_time)
{
    if (!VN_context)
        return false;

    VN_context->background.old_image = VN_context->background.image;
    VN_context->background.image = image;
    VN_context->background.fade = VN_NewDuration(fade_time);

    return true;
}


bool VN_SetForeground(VN_Image* image, VN_Point position, uint64_t fade_time)
{
    if (!VN_context)
        return false;
        
    if (VN_context->num_foregrounds >= VN_MAX_FOREGROUNDS)
        VN_ERROR("Foreground number limit");
        
    VN_Foreground* foreground = &VN_context->foregrounds[VN_context->num_foregrounds];
    
    foreground->image = image;
    foreground->position = position;
    foreground->fade = VN_NewDuration(fade_time);
    foreground->disappearing = false;
    
    VN_context->num_foregrounds++;
        
    return true;

error:
    return false;
}

bool VN_RemoveForeground(VN_Image* image, uint64_t fade_time)
{
    if (!VN_context)
        return false;
        
    for (int i = 0; i < VN_context->num_foregrounds; i++)
    {
        if (VN_context->foregrounds[i].image == image)
        {
            VN_context->foregrounds[i].fade = VN_NewDuration(fade_time);
            VN_context->foregrounds[i].disappearing = true;
            
            return true;
        }
    }
        
    VN_context->error = "Foreground not found";
    return false;
}

bool VN_ClearForegrounds(uint64_t fade_time)
{
    if (!VN_context)
        return false;
        
    for (int i = 0; i < VN_context->num_foregrounds; i++)
    {
        VN_context->foregrounds[i].fade = VN_NewDuration(fade_time);
        VN_context->foregrounds[i].disappearing = true;
    }
    
    return true;
}

uint64_t VN_StringLengthToScrollTime(size_t length)
{
    return length * 30;
}

bool VN_SetText(const char* text)
{
    if (!VN_context)
        return false;
        
    size_t utf8_length = SDL_utf8strlen(text);
    size_t length = SDL_strlen(text);
    
    VN_context->text.scroll_time = VN_NewDuration(VN_StringLengthToScrollTime(utf8_length));

    size_t position = 0;    
    VN_context->text.num_lines = 0;
    VN_context->text.total_width = 0;
    
    while ((position != length) && (VN_context->text.num_lines < VN_MAX_LINES))
    {
        size_t size;
        TTF_MeasureString(VN_context->font, text + position, 0, VN_context->width, NULL, &size);
        
        if ((position + size != length) && (text[position + size] != ' '))
        {
            size_t size_cpy = size;
            while (size >= 0)
            {
                size--;
                if (text[position + size] == ' ')
                    break;
            }
            
            if (size == 0)
                size = size_cpy;
        }
        
        if ((position + size != length) && (text[position + size] == ' '))
            size++;
        
        TTF_SetTextString(VN_context->text.lines[VN_context->text.num_lines], text + position, size);
        int width;
        TTF_GetTextSize(VN_context->text.lines[VN_context->text.num_lines], &width, NULL);
        VN_context->text.total_width += width;
        position += size;
        VN_context->text.num_lines++;
    }
    
    VN_context->text.finished_scrolling = false;
    
    return true;
}

bool VN_ClearText(void)
{
    if (!VN_context)
        return false;
        
    VN_context->text.num_lines = 0;
    
    return true;
}


bool VN_PushEvent(VN_Event* event)
{
    if (VN_context->num_events == VN_MAX_EVENTS)
        VN_ERROR("Max event count");
        
    VN_context->events[VN_context->num_events] = *event;
    VN_context->num_events++;
        
    return true;
        
error:
    return false;
}

float VN_GetDurationCoeff(VN_Duration duration)
{
    if (SDL_GetTicks() >= duration.end)
        return 1.0f;
    else
        return 1.0f - (float)(duration.end - SDL_GetTicks()) / (float)(duration.end - duration.start);
}

bool VN_ProcessSDLEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        VN_Event vn_event = {0};
        vn_event.type = VN_EVENT_QUIT;
        VN_PushEvent(&vn_event);
    }
    else if (((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) && (event->button.button == SDL_BUTTON_LEFT)) ||
             ((event->type == SDL_EVENT_KEY_DOWN) && (!event->key.repeat) && (event->key.scancode == SDL_SCANCODE_RETURN)))
    {
        if (VN_context->text.finished_scrolling)
        {
            VN_Event vn_event = {0};
            vn_event.type = VN_EVENT_TEXT_CONFIRMED;
            VN_PushEvent(&vn_event);
        }
        else
        {
            VN_context->text.scroll_time = VN_NewDuration(0);
            VN_context->text.finished_scrolling = true;
            
            VN_Event vn_event = {0};
            vn_event.type = VN_EVENT_TEXT_FINISHED_SCROLLING;
            VN_PushEvent(&vn_event);
        }
    }
    else if ((event->type == SDL_EVENT_KEY_DOWN) && (!event->key.repeat))
    {
        if (event->key.scancode == SDL_SCANCODE_F11)
            SDL_SetWindowFullscreen(VN_context->window, !(SDL_GetWindowFlags(VN_context->window) & SDL_WINDOW_FULLSCREEN));
    }
    
    return true;
}

bool VN_CheckBackground(void)
{
    if (!VN_context->background.old_image)
        return true;

    if (VN_GetDurationCoeff(VN_context->background.fade) >= 1.0f)
    {
        VN_context->background.old_image = NULL;
    }
    
    return true;
}

bool VN_CheckForegrounds(void)
{
    for (int i = VN_context->num_foregrounds - 1; i >= 0; i--)
    {
        if ((VN_context->foregrounds[i].disappearing) && (VN_GetDurationCoeff(VN_context->foregrounds[i].fade) >= 1.0f))
        {
            SDL_memmove(VN_context->foregrounds + i, VN_context->foregrounds + i + 1, (VN_context->num_foregrounds - (i + 1)) * sizeof(VN_Foreground));
            VN_context->num_foregrounds--;
        }
    }
    
    return true;
}

bool VN_CheckText(void)
{
    if ((VN_context->text.num_lines == 0) || (VN_context->text.finished_scrolling))
        return true;
        
    if (VN_GetDurationCoeff(VN_context->text.scroll_time) >= 1.0f)
    {
        VN_context->text.finished_scrolling = true;
        VN_Event event = {0};
        event.type = VN_EVENT_TEXT_FINISHED_SCROLLING;
        VN_PushEvent(&event);
    }
    
    return true;
}

bool VN_RenderClear(void)
{
    SDL_SetRenderDrawColor(VN_context->renderer, 0, 0, 0, 255);
    return SDL_RenderClear(VN_context->renderer);
}

bool VN_RenderPresent(void)
{
    return SDL_RenderPresent(VN_context->renderer);
}

bool VN_RenderBackground(void)
{
    if (VN_context->background.old_image)
    {
        SDL_SetTextureAlphaMod(VN_context->background.old_image->texture, 255);
        SDL_RenderTexture(VN_context->renderer, VN_context->background.old_image->texture, NULL, NULL);
    }
    
    float coeff = VN_GetDurationCoeff(VN_context->background.fade);
    
    if (VN_context->background.image)
    {
        SDL_SetTextureAlphaMod(VN_context->background.image->texture, 255 * coeff);
        SDL_RenderTexture(VN_context->renderer, VN_context->background.image->texture, NULL, NULL);
    }
    else
    {
        SDL_SetRenderDrawColor(VN_context->renderer, 0, 0, 0, 255 * coeff);
        SDL_RenderFillRect(VN_context->renderer, NULL);
    }
    
    return true;
}

bool VN_RenderForegrounds(void)
{
    for (int i = 0; i < VN_context->num_foregrounds; i++)
    {
        VN_Foreground* foreground = &VN_context->foregrounds[i];
        
        float coeff = VN_GetDurationCoeff(foreground->fade);
        if (foreground->disappearing)
            coeff = 1.0f - coeff;
        
        SDL_SetTextureAlphaMod(foreground->image->texture, 255 * coeff);
        SDL_FRect rect = {foreground->position.x, foreground->position.y, foreground->image->texture->w, foreground->image->texture->h};
        SDL_RenderTexture(VN_context->renderer, foreground->image->texture, NULL, &rect);
    }
    
    return true;
}

bool VN_RenderText(void)
{
    if (VN_context->text.num_lines == 0)
        return true;

    int font_size = TTF_GetFontSize(VN_context->font);
    int displayed_width = 0;
    float coeff = VN_GetDurationCoeff(VN_context->text.scroll_time);
    int max_width = VN_context->text.total_width * coeff;
    
    for (int i = 0; i < VN_context->text.num_lines; i++)
    {
        int line_width;
        TTF_GetTextSize(VN_context->text.lines[i], &line_width, NULL);
        
        if (displayed_width + line_width < max_width)
        {
            TTF_DrawRendererText(VN_context->text.lines[i], 0.0f, 100 + font_size * i);
        }
        else
        {
            SDL_Rect clip_rect = {0, 100 + font_size * i, max_width - displayed_width, font_size};
            SDL_SetRenderClipRect(VN_context->renderer, &clip_rect);
            TTF_DrawRendererText(VN_context->text.lines[i], 0.0f, 100 + font_size * i);
            SDL_SetRenderClipRect(VN_context->renderer, NULL);
            break;
        }
        
        displayed_width += line_width;
    }
    
    return true;
}

bool VN_Step(void)
{
    if (!VN_context)
        return false;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        VN_ProcessSDLEvent(&event);
    }

    VN_CheckBackground();
    VN_CheckForegrounds();
    VN_CheckText();

    VN_RenderClear();
    
    VN_RenderBackground();
    VN_RenderForegrounds();
    
    VN_RenderText();

    VN_RenderPresent();
    
    SDL_Delay(16); // TODO : framerate and vsync

    return true;
    
error:
    return false;
}
