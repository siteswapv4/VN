#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* For SDL PropertiesID and SDL_IOStream
 * Could replace the 2 with a less heavy dependency later
 * but since VN depends on it it's not that bad for now
 */
#include <SDL3/SDL.h>

#include "VNDS_utils.h"
#include "VN.h"

#define VNDS_FRAME_MS 16

#define VNDS_DS_WIDTH 256
#define VNDS_DS_HEIGHT 192

bool VNDS_SetBackground(void* data);

bool VNDS_SetForeground(void* data);

bool VNDS_SetSound(void* data);

bool VNDS_SetMusic(void* data);

bool VNDS_SetText(void* data);

bool VNDS_SetDelay(void* data);

bool VNDS_SetChoice(void* data);

bool VNDS_SetVar(void* data);

bool VNDS_If(void* data);

bool VNDS_Fi(void* data);

bool VNDS_Jump(void* data);

bool VNDS_GSetVar(void* data);

bool VNDS_Goto(void* data);

bool VNDS_Random(void* data);

bool VNDS_ClearText(void* data);

bool VNDS_SetLabel(void* data);

typedef enum VNDS_WaitType
{
    VNDS_WAIT_NONE = 0,
    VNDS_WAIT_DELAY,
    VNDS_WAIT_CHOICE,
    VNDS_WAIT_TEXT_SCROLL,
    VNDS_WAIT_TEXT_CONFIRM
}VNDS_WaitType;

typedef enum VNDS_InstructionType
{
    VNDS_INSTRUCTION_BGLOAD = 0,
    VNDS_INSTRUCTION_SETIMG,
    VNDS_INSTRUCTION_SOUND,
    VNDS_INSTRUCTION_MUSIC,
    VNDS_INSTRUCTION_TEXT,
    VNDS_INSTRUCTION_CHOICE,
    VNDS_INSTRUCTION_SETVAR,
    VNDS_INSTRUCTION_GSETVAR,
    VNDS_INSTRUCTION_IF,
    VNDS_INSTRUCTION_FI,
    VNDS_INSTRUCTION_JUMP,
    VNDS_INSTRUCTION_DELAY,
    VNDS_INSTRUCTION_RANDOM,
    VNDS_INSTRUCTION_LABEL,
    VNDS_INSTRUCTION_GOTO,
    VNDS_INSTRUCTION_CLEARTEXT,
    // TODO : colored text ?
    VNDS_INSTRUCTION_COUNT
}VNDS_InstructionType;

typedef struct VNDS_Instruction
{
    const char* type;
    bool (*func)(void*);
    bool split;
}VNDS_Instruction;

static const VNDS_Instruction VNDS_INSTRUCTIONS[VNDS_INSTRUCTION_COUNT] = 
{
    {"bgload",    VNDS_SetBackground, true},
    {"setimg",    VNDS_SetForeground, true},
    {"sound",     VNDS_SetSound,      true},
    {"music",     VNDS_SetMusic,      true},
    {"text",      VNDS_SetText,       false},
    {"choice",    VNDS_SetChoice,     false},
    {"setvar",    VNDS_SetVar,        true},
    {"gsetvar",   VNDS_GSetVar,       true},
    {"if",        VNDS_If,            true},
    {"fi",        VNDS_Fi,            false},
    {"jump",      VNDS_Jump,          true},
    {"delay",     VNDS_SetDelay,      true},
    {"random",    VNDS_Random,        true},
    {"label",     VNDS_SetLabel,      true},
    {"goto",      VNDS_Goto,          true},
    {"cleartext", VNDS_ClearText,     true}
};

typedef struct VNDS_Script
{
    char* data;
    const char* position;
}VNDS_Script;

typedef struct VNDS_Context
{
    char* novel_path;
    
    int image_width;
    int image_height;

    VNDS_Script script;
    
    SDL_PropertiesID backgrounds;
    SDL_PropertiesID foregrounds;
    SDL_PropertiesID sounds;
    
    SDL_PropertiesID variables;
    SDL_PropertiesID global_variables;
    
    VNDS_WaitType wait;
}VNDS_Context;

static VNDS_Context* VNDS_context = NULL;

bool VNDS_LoadScript(VNDS_String name)
{
    char* path;
    asprintf(&path, "%s/script/%.*s", VNDS_context->novel_path, (int)name.length, name.data);
    
    /* DO NOT USER 'name' AGAIN AFTER HERE */
    if (VNDS_context->script.data)
        free(VNDS_context->script.data);
    
    VNDS_context->script.data = VNDS_LoadFile(path, NULL);
    VNDS_context->script.position = VNDS_context->script.data;
    
    free(path);
    return VNDS_context->script.data != NULL;
}

void VNDS_FreeProperty(void* userdata, void* data)
{
    free(data);
}

void VNDS_DestroyImage(void* userdata, void* image)
{
    VN_DestroyImage(image);
}

void VNDS_DestroyAudio(void* userdata, void* audio)
{
    VN_DestroyAudio(audio);
}

bool VNDS_ClearEverything()
{
    VN_SetBackground(NULL, 0);
    VN_ClearForegrounds(0);
    VN_SetMusic(NULL);
    VN_SetSound(NULL, 0);
}

VN_Image* VNDS_LoadBackground(VNDS_String string_name)
{
    char* name = VNDS_StringToCharArray(string_name);
    VN_Image* background = SDL_GetPointerProperty(VNDS_context->backgrounds, name, NULL);
    if (background)
    {
        free(name);
        return background;
    }
        
    char* path;
    asprintf(&path, "%s/background/%s", VNDS_context->novel_path, name);
    background = VN_LoadImage(path);
    if (!background)
    {
        free(name);
        free(path);
        return NULL;
    }
    
    SDL_SetPointerPropertyWithCleanup(VNDS_context->backgrounds, name, background, VNDS_DestroyImage, NULL);
    
    free(path);
    free(name);
    
    return background;
}

VN_Image* VNDS_LoadForeground(VNDS_String string_name)
{
    char* name = VNDS_StringToCharArray(string_name);
    VN_Image* foreground = SDL_GetPointerProperty(VNDS_context->foregrounds, name, NULL);
    if (foreground)
    {
        free(name);
        return foreground;
    }
        
    char* path;
    asprintf(&path, "%s/foreground/%s", VNDS_context->novel_path, name);
    foreground = VN_LoadImage(path);
    if (!foreground)
    {
        free(name);
        free(path);
        return NULL;
    }
    
    SDL_SetPointerPropertyWithCleanup(VNDS_context->foregrounds, name, foreground, VNDS_DestroyImage, NULL);
    
    free(path);
    free(name);
    return foreground;
}

VN_Audio* VNDS_LoadSound(VNDS_String string_name)
{ 
    char* name = VNDS_StringToCharArray(string_name);
    VN_Audio* sound = SDL_GetPointerProperty(VNDS_context->sounds, name, NULL);
    if (sound)
    {
        free(name);
        return sound;
    }
        
    char* path;
    asprintf(&path, "%s/sound/%s", VNDS_context->novel_path, name);
    sound = VN_LoadAudio(path);
    if (!sound)
    {
        free(name);
        free(path);
        return NULL;
    }
    
    SDL_SetPointerPropertyWithCleanup(VNDS_context->sounds, name, sound, VNDS_DestroyAudio, NULL);
    
    free(path);
    free(name);
    return sound;
}

bool VNDS_SetBackground(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    if (split->splits[1].data[0] == '~')
    {
        VN_SetBackground(NULL, 0);
        return true;
    }

    VN_Image* background = VNDS_LoadBackground(split->splits[1]);
    int fade = 0;
    if (split->num_splits >= 3)
        fade = VNDS_StringToInt(split->splits[2]) * VNDS_FRAME_MS;
    
    VN_ClearForegrounds(0);
    VN_SetBackground(background, fade);
    
    return true;
}

bool VNDS_SetForeground(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    if (split->splits[1].data[0] == '~')
    {
        VN_ClearForegrounds(0);
        return true;
    }
    
    if (split->num_splits < 4)
        return false;
        
    VN_Image* foreground = VNDS_LoadForeground(split->splits[1]);
    if (!foreground)
        return false;
        
    VN_Point position = {VNDS_StringToInt(split->splits[2]), VNDS_StringToInt(split->splits[3])};
    position.x *=  VNDS_context->image_width / (float)VNDS_DS_WIDTH;
    position.y *=  VNDS_context->image_height / (float)VNDS_DS_HEIGHT;
    int fade = 0; // cannot set fade ?
    
    //VN_ClearForegrounds(0);
    VN_SetForeground(foreground, position, fade);

    return true;
}

bool VNDS_SetSound(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    if (split->splits[1].data[0] == '~')
    {
        VN_SetSound(NULL, 0);
        return true;
    }
        
    VN_Audio* sound = VNDS_LoadSound(split->splits[1]);
    if (!sound)
        return false;
        
        
    if (split->num_splits >= 3)
    {
        int num_loops = VNDS_StringToInt(split->splits[2]);
        if (num_loops != 0)
            VN_SetSound(sound, num_loops - 1);
    }
    else
        VN_SetSound(sound, 0);

    return true;
}

bool VNDS_SetMusic(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    if (split->splits[1].data[0] == '~')
    {
        VN_SetMusic(NULL);
    }
    
    VN_Audio* music = VNDS_LoadSound(split->splits[1]);
    if (!music)
        return false;
        
    VN_SetMusic(music);
    
    return true;
}

bool VNDS_SetText(void* data_line)
{
    VNDS_context->wait = VNDS_WAIT_NONE;

    VNDS_String* line = data_line;
    VNDS_String text = {line->data + 4, line->length - 4}; // this is safe because the line starts with "text"
    VNDS_TrimString(&text);
    
    if (text.length == 0)
        return false;
        
    if ((text.data[0] == '~') || (text.data[0] == '!'))
    {
        VN_ClearText();
        return true;
    }
    
    char* text_array = VNDS_StringToCharArray(text);
    if (text_array[0] == '@')
    {
        VNDS_context->wait = VNDS_WAIT_TEXT_SCROLL;
        VN_SetText(text_array + 1);
    }
    else
    {
        VNDS_context->wait = VNDS_WAIT_TEXT_CONFIRM;
        VN_SetText(text_array);
    }
    free(text_array);

    return true;
}

bool VNDS_SetDelay(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_DELAY;

    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    int delay = VNDS_StringToInt(split->splits[1]) * VNDS_FRAME_MS;
    VN_SetDelay(delay);

    return true;
}

bool VNDS_SetChoice(void* data_string)
{
    VNDS_context->wait = VNDS_WAIT_CHOICE;
    
    VNDS_String* line = data_string;
    VNDS_String choice_string = {line->data + 6, line->length - 6}; // safe because string starts with "choice"
    VNDS_TrimString(&choice_string);
    
    VNDS_StringSplit choices_split;
    if ((!VNDS_SplitString(choice_string, &choices_split, "|")) || (choices_split.num_splits < 2))
        return false;
    
    /* building choices costs resources but I don't think it matters that
     * much since execution is stopped after a choice anyway
     */
    char** choices = malloc(choices_split.num_splits * sizeof(char*));
    for (int i = 0; i < choices_split.num_splits; i++)
    {
        choices[i] = VNDS_StringToCharArray(choices_split.splits[i]);
    }
    
    VN_SetChoice((const char* const*)choices, choices_split.num_splits);
    
    for (int i = 0; i < choices_split.num_splits; i++)
    {
        free(choices[i]);
    }
    free(choices);
        
    return true;
}

bool VNDS_SetVarInternal(const char* left, const char* modifier, const char* right)
{
    if (modifier[0] == '=')
    {
        char* right_dup = strdup(right);
        return SDL_SetPointerPropertyWithCleanup(VNDS_context->variables, left, right_dup, VNDS_FreeProperty, NULL);
    }

    char* variable = SDL_GetPointerProperty(VNDS_context->variables, left, NULL);
    if (!variable)
        return false;
        
    int lefti = atoi(variable);
    int righti = atoi(right);
    int res = 0.0f;
    
    /* Not sure if + can concatenate */
    if (modifier[0] == '+')
    {
        res = lefti + righti;
    }
    else if (modifier[0] == '-')
    {
        res = lefti - righti;
    }
    else
    {
        return false;
    }
    
    char* res_str;
    asprintf(&res_str, "%d", &res);
    return SDL_SetPointerPropertyWithCleanup(VNDS_context->variables, left, res_str, VNDS_FreeProperty, NULL);
}

bool VNDS_SetVar(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 4)
        return false;
        
    char* left = VNDS_StringToCharArray(split->splits[1]);
    char* modifier = VNDS_StringToCharArray(split->splits[2]);
    char* right = VNDS_StringToCharArray(split->splits[3]);
    
    bool retval = VNDS_SetVarInternal(left, modifier, right);
    
    free(left);
    free(modifier);
    free(right);
    
    return retval;
}

bool VNDS_If(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;

    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 4)
        return false;
        
    bool res = false;
    char* name = VNDS_StringToCharArray(split->splits[1]);
    char* var = SDL_GetPointerProperty(VNDS_context->variables, name, NULL);
    free(name);
        
    int lefti = var ? atoi(var) : 0;
    int righti = VNDS_StringToInt(split->splits[3]);
    
    if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray("==")))
    {
        res = lefti == righti;
    }
    else if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray(">=")))
    {
        res = lefti >= righti;
    }
    else if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray("<=")))
    {
        res = lefti <= righti;
    }
    else if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray(">")))
    {
        res = lefti > righti;
    }
    else if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray("<")))
    {
        res = lefti < righti;
    }
    else if (VNDS_StringEqual(split->splits[2], VNDS_StringFromCharArray("!=")))
    {
        res = lefti != righti;
    }
    
    if (res)
        return true;
        
    VNDS_String line;
    int num_if = 1;
    while (VNDS_ReadLine(&VNDS_context->script.position, &line))
    {
        VNDS_TrimString(&line);
        if (VNDS_StringStartsWithWord(line, VNDS_StringFromCharArray("if")))
        {
            num_if++;
        }
        else if (VNDS_StringStartsWithWord(line, VNDS_StringFromCharArray("fi")))
        {
            num_if--;
        }
        
        if (num_if == 0)
            return true;
    }
    
    return false;
}

bool VNDS_Fi(void* data_string)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    return true;
}

bool VNDS_Jump(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    char* label = NULL;
    if (split->num_splits >= 3)
        label = VNDS_StringToCharArray(split->splits[2]);
        
    VNDS_ClearEverything();
    VNDS_LoadScript(split->splits[1]);
    /* Strings have to be rebuilt after this */
    
    if (label)
    {
        VNDS_StringSplit label_split;
        label_split.num_splits = 2;
        label_split.splits[0] = VNDS_StringFromCharArray("goto");
        label_split.splits[1] = VNDS_StringFromCharArray(label); 
        
        VNDS_Goto(&label_split);
        
        free(label);
    }
    
    return true;
}

bool VNDS_GSetVar(void* data_split)
{
    return VNDS_SetVar(data_split);
}

bool VNDS_Random(void* data_split)
{
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 4)
        return false;
        
    char* name = VNDS_StringToCharArray(split->splits[1]);

    int low = VNDS_StringToInt(split->splits[2]);
    int high = VNDS_StringToInt(split->splits[2]);

    int num = low + rand() % (high - low + 1);

    char* val;
    asprintf(&val, "%d", num);
    
    SDL_SetPointerPropertyWithCleanup(VNDS_context->variables, name, val, VNDS_FreeProperty, NULL);
    free(name);

    return true;
}

bool VNDS_Goto(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
    
    VNDS_StringSplit* split = data_split;
    if (split->num_splits < 2)
        return false;
        
    VNDS_ClearEverything();
        
    const char* position = VNDS_context->script.data;
    VNDS_String line;
    while (VNDS_ReadLine(&position, &line))
    {
        if (VNDS_StringStartsWithWord(line, VNDS_StringFromCharArray("label")))
        {
            VNDS_String label = {line.data + 5, line.length - 5};
            VNDS_TrimString(&label);
            if (VNDS_StringEqual(label, split->splits[1]))
            {
                VNDS_context->script.position = position;
                return true;
            }
        }
    }
    
    return false;
}

bool VNDS_ClearText(void* data_split)
{
    VNDS_String str = VNDS_StringFromCharArray("text ~");
    return VNDS_SetText(&str);
}

bool VNDS_SetLabel(void* data_split)
{
    VNDS_context->wait = VNDS_WAIT_NONE;
}

bool VNDS_GetImageSize()
{
    char* path;
    asprintf(&path, "%s/img.ini", VNDS_context->novel_path);
    uint8_t* data = VNDS_LoadFile(path, NULL);
    free(path);
    
    const char* position = data;
    VNDS_String line;
    while (VNDS_ReadLine(&position, &line))
    {
        VNDS_StringSplit split;
        if (VNDS_SplitString(line, &split, "x") && (split.num_splits == 2))
        {
            VNDS_context->image_width = VNDS_StringToInt(split.splits[0]);
            VNDS_context->image_height = VNDS_StringToInt(split.splits[1]);
        }
    
        if (VNDS_SplitString(line, &split, "=") && (split.num_splits == 2))
        {
            if (strncmp(split.splits[0].data, "width", split.splits[0].length) == 0)
                VNDS_context->image_width = VNDS_StringToInt(split.splits[1]);
            else if (strncmp(split.splits[0].data, "height", split.splits[0].length) == 0)
                VNDS_context->image_height = VNDS_StringToInt(split.splits[1]);
        }
    }
    
    free(data);
    return true;
}

bool VNDS_Parse()
{
    VNDS_String line;
    while (VNDS_ReadLine(&VNDS_context->script.position, &line))
    {
        VNDS_TrimString(&line);
        if ((line.length == 0) || (line.data[0] == '#'))
            continue;
            
        SDL_Log("%.*s", (int)line.length, line.data);
        
        for (int i = 0; i < VNDS_INSTRUCTION_COUNT; i++)
        {
            if (VNDS_StringStartsWithWord(line, VNDS_StringFromCharArray(VNDS_INSTRUCTIONS[i].type)))
            {
                if (VNDS_INSTRUCTIONS[i].func)
                {
                    if (VNDS_INSTRUCTIONS[i].split)
                    {
                        VNDS_StringSplit split;
                        if (VNDS_SplitString(line, &split, " \t"))
                            VNDS_INSTRUCTIONS[i].func(&split);
                    }
                    else
                    {
                        VNDS_INSTRUCTIONS[i].func(&line);
                    }
                    /* Strings may be corrupted after this if a jump instruction was encountered */
                }
                break;
            }
        }
        
        if (VNDS_context->wait != VNDS_WAIT_NONE)
            return true;
    }
    
    return true;
}

bool VNDS_Init(const char* novel_path)
{
    srand(time(NULL));
    SDL_Init(0);

    VNDS_context = calloc(1, sizeof(VNDS_Context));
    
    VNDS_context->novel_path = strdup(novel_path);
    VNDS_context->image_width = VNDS_DS_WIDTH;
    VNDS_context->image_height = VNDS_DS_HEIGHT;
    
    VNDS_context->backgrounds = SDL_CreateProperties();
    VNDS_context->foregrounds = SDL_CreateProperties();
    VNDS_context->sounds = SDL_CreateProperties();
    
    VNDS_context->variables = SDL_CreateProperties();
    VNDS_context->global_variables = SDL_CreateProperties();
    
    VNDS_GetImageSize();
    VNDS_LoadScript((VNDS_String){"main.scr", 8});
    
    char* font_path;
    asprintf(&font_path, "%s/default.ttf", VNDS_context->novel_path);
    VN_Init("VNDS", VNDS_context->image_width, VNDS_context->image_height, VNDS_FileExists(font_path) ? font_path : "font.otf");
    free(font_path);
    
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "no novel path\n");
        return EXIT_FAILURE;
    }

    VNDS_Init(argv[1]);
    
    VNDS_Parse();
    
    while (true)
    {
        bool advance = false;
        
        VN_Event event;
        while (VN_PollEvent(&event))
        {
            if (event.type == VN_EVENT_QUIT)
                return 0;
            if (event.type == VN_EVENT_DELAY_ELAPSED)
            {
                if (VNDS_context->wait == VNDS_WAIT_DELAY)
                    advance = true;
            }
            else if (event.type == VN_EVENT_TEXT_FINISHED_SCROLLING)
            {
                if (VNDS_context->wait == VNDS_WAIT_TEXT_SCROLL)
                {
                    VN_ClearText();
                    advance = true;
                }
            }
            else if (event.type == VN_EVENT_TEXT_CONFIRMED)
            {
                if (VNDS_context->wait == VNDS_WAIT_TEXT_CONFIRM)
                {
                    VN_ClearText();
                    VN_SetSound(NULL, 0);
                    advance = true;
                }
            }
            else if (event.type == VN_EVENT_CHOICE_MADE)
            {
                if (VNDS_context->wait == VNDS_WAIT_CHOICE)
                {
                    char* choice_string;
                    asprintf(&choice_string, "%d", event.choice + 1);
                    VNDS_SetVarInternal("selected", "=", choice_string);
                    free(choice_string);
                    VN_ClearChoice();
                    advance = true;
                }
            }
        }
        
        if (advance)
            VNDS_Parse();
    
        VN_Step();
    }
    
    return EXIT_SUCCESS;    
}
