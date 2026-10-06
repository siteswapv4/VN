#include "VN.h"
#include <stdlib.h>

#define NUM_TEXTS 3
const char* texts[] =
{
    "This is the first text and I'm gonna make it longer by writing more characters in it just this way",
    "This is the second text, I'm gonna make it longer this time again and I believe this is gonna work fine",
    "This is the final text, the testing is done for now and I hope this text will display properly"
};

int main(int argc, char** argv)
{
    VN_Init("test", 500, 500);

    int current_text = 0;    
    VN_SetText(texts[current_text]);
    
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
