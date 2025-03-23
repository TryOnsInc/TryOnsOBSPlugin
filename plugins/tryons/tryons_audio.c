#include "tryons_audio.h"
#include <obs-module.h>
#include <obs-frontend-api.h>

void play_audio_file(const char *filepath)
{
	// Create settings for the media source and set the file path.
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "local_file", filepath);

	// Create a new media source of type "ffmpeg_source" with a unique name.
	obs_source_t *audio_source = obs_source_create("ffmpeg_source", "DonationAudio", settings, NULL);
	obs_data_release(settings);

	if (audio_source) {
		// Get the current active scene.
		obs_source_t *current_scene = obs_frontend_get_current_scene();
		if (current_scene) {
			// Convert the scene source to an obs_scene_t.
			obs_scene_t *scene = obs_scene_from_source(current_scene);
			if (scene) {
				// Add the newly created audio source to the current scene.
				obs_scene_add(scene, audio_source);
			}
			obs_source_release(current_scene);
		}
		// Release our reference to the audio source.
		obs_source_release(audio_source);
	}
}
