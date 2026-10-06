#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "VN.h"

typedef enum VNDS_Modifier
{
    VNDS_EQUAL = 0,
    VNDS_PLUS,
    VNDS_MINUS,
    VNDS_GREATER_OR_EQUAL,
    VNDS_LESS_OR_EQUAL,
    VNDS_GREATER,
    VNDS_LESS,
    VNDS_EQUAL_EQUAL,
    VNDS_MODIFIER_COUNT
}VNDS_Modifier;

const char* VNDS_MODIFIER_STRING[] =
{
    "=",
    "+",
    "-",
    ">=",
    "<=",
    ">",
    "<",
    "=="
};

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

const char* VNDS_INSTRUCTION_TYPE_STRING[] =
{
    "bgload",
    "setimg",
    "sound",
    "music",
    "text",
    "choice",
    "setvar",
    "gsetvar",
    "if",
    "fi",
    "jump",
    "delay",
    "random",
    "label",
    "goto",
    "cleartext"
};

typedef struct VNDS_BGLoadInstruction
{
    VNDS_InstructionType type;
    char* file;
    int fade;
}VNDS_BGLoadInstruction;

typedef struct VNDS_SetIMGInstruction
{
    VNDS_InstructionType type;
    char* file;
    VN_Point position;
}VNDS_SetIMGInstruction;

typedef struct VNDS_SoundInstruction
{
    VNDS_InstructionType type;
    char* file;
    int num_loops;
}VNDS_SoundInstruction;

typedef struct VNDS_MusicInstruction
{
    VNDS_InstructionType type;
    char* file;
}VNDS_MusicInstruction;

typedef struct VNDS_TextInstruction
{
    VNDS_InstructionType type;
    char* file;
}VNDS_TextInstruction;

typedef struct VNDS_ChoiceInstruction
{
    VNDS_InstructionType type;
    char** choices;
    int num_choices;
}VNDS_ChoiceInstruction;

typedef struct VNDS_SetVarInstruction
{
    VNDS_InstructionType type;
    char* variable;
    VNDS_Modifier modifier;
    char* value;
}VNDS_SetVarInstruction;

typedef struct VNDS_GSetVarInstruction
{
    VNDS_InstructionType type;
    char* variable;
    VNDS_Modifier modifier;
    char* value;
}VNDS_GSetVarInstruction;

typedef struct VNDS_IfInstruction
{
    VNDS_InstructionType type;
    char* left;
    VNDS_Modifier modifier;
    char* right;
}VNDS_IfInstruction;

typedef struct VNDS_FiInstruction
{
    VNDS_InstructionType type;
}VNDS_FiInstruction;

typedef struct VNDS_JumpInstruction
{
    VNDS_InstructionType type;
    char* file;
    char* label;
}VNDS_JumpInstruction;

typedef struct VNDS_DelayInstruction
{
    VNDS_InstructionType type;
    int delay;
}VNDS_DelayInstruction;

typedef struct VNDS_RandomInstruction
{
    VNDS_InstructionType type;
    char* variable;
    int low;
    int high;
}VNDS_RandomInstruction;

typedef struct VNDS_LabelInstruction
{
    VNDS_InstructionType type;
    char* name;
}VNDS_LabelInstruction;

typedef struct VNDS_GotoInstruction
{
    VNDS_InstructionType type;
    char* name;
}VNDS_GotoInstruction;

typedef struct VNDS_ClearTextInstruction
{
    VNDS_InstructionType type;
    char* value;
}VNDS_ClearTextInstruction;

typedef union VNDS_Instruction
{
    VNDS_InstructionType type;
    
    VNDS_BGLoadInstruction bg_load;
    VNDS_SetIMGInstruction set_img;
    VNDS_SoundInstruction sound;
    VNDS_MusicInstruction music;
    VNDS_TextInstruction text;
    VNDS_ChoiceInstruction choice;
    VNDS_SetVarInstruction set_var;
    VNDS_GSetVarInstruction gset_var;
    VNDS_IfInstruction if_instruction;
    VNDS_FiInstruction fi;
    VNDS_JumpInstruction jump;
    VNDS_DelayInstruction delay;
    VNDS_RandomInstruction random;
    VNDS_LabelInstruction label;
    VNDS_GotoInstruction goto_instruction;
    VNDS_ClearTextInstruction clear_text;
}VNDS_Instruction;

char* VNDS_ReadLine(const char** str)
{
    const char* start = *str;
    const char* end = strpbrk(start, "\r\n");
    
    if (!end)
    {
        *str = start + strlen(start);
        if (*str == start)
            return NULL;
        else
            return strdup(start); 
    }
    
    *str = end + 1;
    
    if (*end == '\r' && *(*str) == '\n')
        (*str)++;
        
    char* line = malloc(end - start + 1);
    line[end - start] = '\0';
    memcpy(line, start, end - start);
    
    return line;
}

void VNDS_TrimString(char *str)
{
    char *start = str;
    char *end;

    while (*start && isspace((unsigned char)*start))
        start++;

    if (*start == '\0')
    {
        *str = '\0';
        return;
    }

    end = start + strlen(start) - 1;

    while (end > start && isspace((unsigned char)*end))
        end--;

    memmove(str, start, (size_t)(end - start + 1));
    str[end - start + 1] = '\0';
}

VNDS_Instruction* VNDS_LoadScript(const char* script, int* num_instructions)
{
    int instruction_capacity = 100;
    VNDS_Instruction* instructions = calloc(instruction_capacity, sizeof(VNDS_Instruction));
    *num_instructions = 0;
    
    char* line = VNDS_ReadLine(&script);
    while (line)
    {
        VNDS_TrimString(line);
        for (int i = 0; i < VNDS_INSTRUCTION_COUNT; i++)
        {
            if (strncmp(line, VNDS_INSTRUCTION_TYPE_STRING[i], strlen(VNDS_INSTRUCTION_TYPE_STRING[i])) == 0)
            {
                
            }
        }
    
        free(line);
        line = VNDS_ReadLine(&script);
    }
    
    return instructions;
}


uint8_t* VNDS_LoadFile(const char* path, size_t* ret_size)
{
    FILE* fp = fopen(path, "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    uint8_t* buff = malloc(size + 1);
    buff[size] = 0;
    fseek(fp, 0, SEEK_SET);
    fread(buff, size, 1, fp);
    fclose(fp);
    if (ret_size)
        *ret_size = size;
        
    return buff;
}

int main(int argc, char** argv)
{
    char* script = VNDS_LoadFile("main.scr", NULL);

    int num_instructions = 0;
    VNDS_Instruction* instructions = VNDS_LoadScript(script, &num_instructions);
    
    return EXIT_SUCCESS;    
}
