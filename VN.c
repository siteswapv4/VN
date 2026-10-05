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

typedef struct VN_Fade
{
    uint64_t start;
    uint64_t end;
}VN_Fade;

typedef struct VN_Image
{
    SDL_Texture* texture;
}VN_Image;

typedef struct VN_Background
{
    VN_Image* image;
    VN_Image* old_image;
    
    VN_Fade fade;
}VN_Background;

typedef struct VN_Foreground
{
    VN_Image* image;
    VN_Point position;
    
    VN_Fade fade;
    bool disappearing;
}VN_Foreground;

typedef struct VN_Text
{
    TTF_Text* text;
    bool confirm;
}VN_Text;

typedef struct VN_Context
{
    const char* error;

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
        
    VN_context->text.text = TTF_CreateText(VN_context->text_engine, VN_context->font, "", 0);
    TTF_SetTextWrapWidth(VN_context->text.text, width);
    
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


VN_Fade VN_NewFade(uint64_t fade_time)
{
    return (VN_Fade){SDL_GetTicks(), SDL_GetTicks() + fade_time};
}


bool VN_SetBackground(VN_Image* image, uint64_t fade_time)
{
    if (!VN_context)
        return false;

    VN_context->background.old_image = VN_context->background.image;
    VN_context->background.image = image;
    VN_context->background.fade = VN_NewFade(fade_time);

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
    foreground->fade = VN_NewFade(fade_time);
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
            VN_context->foregrounds[i].fade = VN_NewFade(fade_time);
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
        VN_context->foregrounds[i].fade = VN_NewFade(fade_time);
        VN_context->foregrounds[i].disappearing = true;
    }
    
    return true;
}


bool VN_SetText(const char* text, bool confirm)
{
    if (!VN_context)
        return false;
        
    TTF_SetTextString(VN_context->text.text, text, 0);
    VN_context->text.confirm = confirm;
    
    return true;
}

bool VN_ClearText(void)
{
    if (!VN_context)
        return false;
        
    TTF_SetTextString(VN_context->text.text, "", 0);
    VN_context->text.confirm = false;
    
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

float VN_GetFadeCoeff(VN_Fade fade)
{
    if (SDL_GetTicks() >= fade.end)
        return 1.0f;
    else
        return 1.0f - (float)(fade.end - SDL_GetTicks()) / (float)(fade.end - fade.start);
}

bool VN_ProcessSDLEvent(SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        VN_Event vn_event = {0};
        vn_event.type = VN_EVENT_QUIT;
        VN_PushEvent(&vn_event);
    }
    else if ((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) && (event->button.button == SDL_BUTTON_LEFT))
    {
        VN_Event vn_event = {0};
        vn_event.type = VN_EVENT_TEXT_CONFIRMED;
        VN_PushEvent(&vn_event);
    }
}

bool VN_CheckBackground(void)
{
    if (!VN_context->background.old_image)
        return true;

    if (VN_GetFadeCoeff(VN_context->background.fade) >= 1.0f)
    {
        VN_context->background.old_image = NULL;
    }
    
    return true;
}

bool VN_CheckForegrounds(void)
{
    for (int i = VN_context->num_foregrounds - 1; i >= 0; i--)
    {
        if ((VN_context->foregrounds[i].disappearing) && (VN_GetFadeCoeff(VN_context->foregrounds[i].fade) >= 1.0f))
        {
            SDL_memmove(VN_context->foregrounds + i, VN_context->foregrounds + i + 1, (VN_context->num_foregrounds - (i + 1)) * sizeof(VN_Foreground));
            VN_context->num_foregrounds--;
        }
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
    
    float coeff = VN_GetFadeCoeff(VN_context->background.fade);
    
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
        
        float coeff = VN_GetFadeCoeff(foreground->fade);
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
    TTF_DrawRendererText(VN_context->text.text, 0.0f, 400.0f);
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
