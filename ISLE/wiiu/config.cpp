#include "config.h"

#include <SDL2/SDL_log.h>
#include <iniparser.h>

void WIIU_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
	SDL_Log("Overriding default config for Wii U");

	iniparser_set(p_dictionary, "isle:diskpath", "sdmc:/wiiu/isle-U/content/isle/LEGO/disk");
	iniparser_set(p_dictionary, "isle:cdpath", "sdmc:/wiiu/isle-U/content/isle");
}
