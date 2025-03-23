#include <obs-module.h>
#include "tryons_common.h"
#include "tryons_network.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("tryons", "en-US")

bool obs_module_load(void)
{
	blog(LOG_INFO, "Tryons plugin loaded.");
	if (!tryons_network_init())
		return false;
	return true;
}

void obs_module_unload(void)
{
	tryons_network_shutdown();
	blog(LOG_INFO, "Tryons plugin unloaded.");
}
