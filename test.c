#include "VN.h"
#include <stdlib.h>

#define NUM_TEXTS 3
const char* texts[] =
{
    "This is the first text",
    "This is the second text",
    "This is the final text"
};

int main(int argc, char** argv)
{
    VN_Init("test", 500, 500);
    
    VN_Image* background = VN_LoadImage("background.jpg");
    VN_Image* sprite = VN_LoadImage("sprite.png");
    
    int current_text = 0;
    
    VN_SetBackground(background, 2000);
    VN_SetText(texts[current_text], true);
    
    VN_SetForeground(sprite, (VN_Point){0, 0}, 2000);
    
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
                if (current_text < NUM_TEXTS)
                {
                    current_text++;
                    VN_SetText(texts[current_text], true);
                }
                else
                {
                    VN_ClearText();
                }
            }
        }
        
        VN_Step();
    }
    
    return EXIT_SUCCESS;
}
