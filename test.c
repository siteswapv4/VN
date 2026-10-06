#include "VN.h"
#include <stdlib.h>
#include <stdio.h>

void SetBackground(uintptr_t* data)
{
    VN_SetBackground((VN_Image*)data[0], (uint64_t)data[1]);
    VN_SetDelay((uint64_t)data[1]);
}

void SetForeground(uintptr_t* data)
{
    VN_SetForeground((VN_Image*)data[0], (VN_Point){(uint64_t)data[1], (uint64_t)data[2]}, (uint64_t)data[3]);
    VN_SetDelay((uint64_t)data[3]);
}

void ClearForegrounds(uintptr_t* data)
{
    VN_ClearForegrounds((uint64_t)data[0]);
    VN_SetDelay((uint64_t)data[0]);
}

void SetText(uintptr_t* data)
{
    VN_SetText((const char*)data[0]);
}

void ClearText(uintptr_t* data)
{
    VN_ClearText();
}

typedef struct Instruction
{
    void (*func)(uintptr_t*);
    uintptr_t data[4];
}Instruction;

#define NOVAL 0

#define NUM_INSTRUCTIONS 5

int main(int argc, char** argv)
{
    VN_Init("test", 640, 480);
    
    VN_Image* background = VN_LoadImage("background.jpg");
    
    VN_Image* sprite1 = VN_LoadImage("sprite1.png");
    VN_Image* sprite2 = VN_LoadImage("sprite2.png");
    
    VN_Audio* music = VN_LoadAudio("music.mp3");
    VN_Audio* sound = VN_LoadAudio("sound.ogg");
    
    /*
    Instruction instructions[NUM_INSTRUCTIONS] = 
    {
        {SetBackground, {(uintptr_t)background, 1000,  NOVAL, NOVAL}},
        {SetForeground, {(uintptr_t)sprite1,    -100,  0,     1000 }},
        {SetForeground, {(uintptr_t)sprite2,    200,   0,     1000 }},
        {SetText,       {(uintptr_t)"hello !",  NOVAL, NOVAL, NOVAL}},
        {ClearText,     {NOVAL,                 NOVAL, NOVAL, NOVAL}}
    };
    
    instructions[0].func(instructions[0].data);
    int current_instruction = 1;
    */
    
    const char* choice1 = "yes";
    const char* choice2 = "no";
    
    const char* choices[2] = {choice1, choice2};
    
    const char* answers[2] = {"That's great!", "You should work on projects that you like instead!"};
    
    VN_SetBackground(background, 0);
    VN_SetForeground(sprite1, (VN_Point){-100, 0}, 0);
    VN_SetForeground(sprite2, (VN_Point){200, 0}, 0);
    
    VN_SetText("Do you like making this engine ?");
    
    VN_SetMusic(music);
    VN_SetSound(sound, 0);
    
    while (true)
    {
        VN_Event event;
        while (VN_PollEvent(&event))
        {
            if (event.type == VN_EVENT_QUIT)
            {
                goto exit;
            }
            else if (event.type == VN_EVENT_TEXT_CONFIRMED)
            {
                VN_SetChoice(choices, 2);
                VN_SetSound(NULL, 0);
            }
            else if (event.type == VN_EVENT_CHOICE_MADE)
            {
                VN_SetText(answers[event.choice]);
                VN_SetMusic(NULL);
            }
        }
        
        VN_Step();
    }

exit:
    VN_SetMusic(NULL);
    VN_SetSound(NULL, 0);
    
    VN_DestroyAudio(music);
    VN_DestroyAudio(sound);

    VN_DestroyImage(background);
    VN_DestroyImage(sprite1);
    VN_DestroyImage(sprite2);
    
    VN_Quit();
    
    return EXIT_SUCCESS;
}
