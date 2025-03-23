#include "tryons_text.h"
#include <obs-module.h>

void update_text_source(const char *text)
{
	obs_source_t *source = obs_get_source_by_name("DonationText");
	if (source) {
		obs_data_t *settings = obs_source_get_settings(source);
		obs_data_set_string(settings, "text", text);
		obs_source_update(source, settings);
		obs_data_release(settings);
		obs_source_release(source);
	}
}
