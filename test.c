#include "VN.h"
#include <stdlib.h>
#include <stdio.h>

#define NUM_TEXTS 4
const char* texts[] =
{
    "Hey !",
    "I'm in a terrible mood today.",
    "Do you think this Visual Novel project is ever gonna be completed ?",
    "I hope so."
};

int main(int argc, char** argv)
{
    VN_Init("test", 640, 480);

    int current_text = 0;    
    VN_SetText(texts[current_text]);
    
    VN_Image* background = VN_LoadImage("background.jpg");
    
    VN_Image* sprite1 = VN_LoadImage("sprite1.png");
    VN_Image* sprite2 = VN_LoadImage("sprite2.png");
    
    VN_SetBackground(background, 500);
    VN_SetForeground(sprite1, (VN_Point){-100, 0}, 500);
    VN_SetForeground(sprite2, (VN_Point){200, 0}, 500);
    
    while (true)
    {
        VN_Event event;
        while (VN_PollEvent(&event))
        {
            if (event.type == VN_EVENT_QUIT)
            {
                return EXIT_SUCCESS;
            }
            else if (event.type == VN_EVENT_TEXT_CONFIRMED)
            {
                if (current_text < NUM_TEXTS - 1)
                {
                    current_text++;
                    VN_SetText(texts[current_text]);
                    
                    if (current_text % 2 != 0)
                    {
                        VN_RemoveForeground(sprite1, 500);
                        VN_SetForeground(sprite2, (VN_Point){200, 0}, 500);
                    }
                    else
                    {
                        VN_RemoveForeground(sprite2, 500);
                        VN_SetForeground(sprite1, (VN_Point){-100, 0}, 500);
                    }
                }
                else
                {
                    VN_ClearText();
                    VN_ClearForegrounds(500);
                    VN_SetBackground(NULL, 500);
                }
            }
        }
        
        VN_Step();
    }
    
    return EXIT_SUCCESS;
}
