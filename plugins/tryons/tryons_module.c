#include <obs-module.h>

// Forward declarations of your source info structures from the audio and text parts.
extern struct obs_source_info tryons_audio_source;
extern struct obs_source_info tryons_text_source;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("tryons", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Tryons Plugin: Audio and Text Sources with Networking";
}

bool obs_module_load(void)
{
	// ... (existing source registration and GDI+ startup code)
	obs_register_source(&si);
	obs_register_source(&si_v2);
	obs_register_source(&si_v3);

	const GdiplusStartupInput gdip_input;
	GdiplusStartup(&gdip_token, &gdip_input, nullptr);

	// Start the networking thread
	network_thread_running.store(true);
	network_thread = std::thread(network_thread_func);

	return true;
}

void obs_module_unload(void)
{
	// Stop the network thread
	network_thread_running.store(false);
	if (network_thread.joinable()) {
		network_thread.join();
	}

	GdiplusShutdown(gdip_token);
}
