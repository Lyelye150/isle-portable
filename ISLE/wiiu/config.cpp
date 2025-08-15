#include "config.h"

#include <SDL2/SDL_log>
#include <iniparser.h>

void WIIU_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    iniparser_set(p_dictionary, "isle:diskpath", "sdmc:/wiiu/apps/isle-U/content/ISLE/LEGO/disk");
    iniparser_set(p_dictionary, "isle:cdpath", "sdmc:/wiiu/apps/isle-U/content/ISLE/");
    iniparser_set(p_dictionary, "isle:savepath", "sdmc:/wiiu/apps/isle-U/content/ISLE/SAVE");
}
