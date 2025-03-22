/*
 * tryons_audio_source.c – Audio‐only OBS source based on obs-ffmpeg-source.c
 *
 * This version removes video callbacks and implements only audio playback via
 * media_playback. It also provides minimal defaults and properties.
 */

#include <obs-module.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>

#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-formats.h"

#include <media-playback/media-playback.h>

#include "../tryons_common.h"

#define FF_LOG_S(source, level, format, ...) \
	blog(level, "[Media Audio Source '%s']: " format, obs_source_get_name(source), ##__VA_ARGS__)
#define FF_BLOG(level, format, ...) FF_LOG_S(s->source, level, format, ##__VA_ARGS__)

struct ffmpeg_source {
	media_playback_t *media;
	bool destroy_media;
	obs_source_t *source;
	char *input;
	char *input_format;
	char *ffmpeg_options;
	int buffering_mb;
	int speed_percent;
	bool is_looping;
	bool is_local_file;
	bool is_hw_decoding;
	bool full_decode;
	bool is_clear_on_media_end;
	bool restart_on_activate;
	bool close_when_inactive;
	bool seekable;
	bool log_changes;
	enum obs_media_state state;
	// Hotkeys for audio control (if desired)
	obs_hotkey_pair_id play_pause_hotkey;
	obs_hotkey_id stop_hotkey;
};

/* Minimal defaults for our audio source */
static void ffmpeg_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "is_local_file", true);
	obs_data_set_default_bool(settings, "looping", false);
	obs_data_set_default_bool(settings, "clear_on_media_end", true);
	obs_data_set_default_bool(settings, "restart_on_activate", true);
	obs_data_set_default_int(settings, "reconnect_delay_sec", 10);
	obs_data_set_default_int(settings, "buffering_mb", 2);
	obs_data_set_default_int(settings, "speed_percent", 100);
	obs_data_set_default_bool(settings, "hw_decode", false);
	obs_data_set_default_bool(settings, "seekable", false);
	obs_data_set_default_bool(settings, "close_when_inactive", false);
	obs_data_set_default_bool(settings, "log_changes", true);
}

/* Minimal properties for our audio source */
static obs_properties_t *ffmpeg_source_getproperties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, "is_local_file", obs_module_text("LocalFile"));
	obs_properties_add_path(props, "local_file", obs_module_text("LocalFile"), OBS_PATH_FILE, "(*.*)", NULL);
	obs_properties_add_bool(props, "looping", obs_module_text("Looping"));
	obs_properties_add_int_slider(props, "buffering_mb", obs_module_text("BufferingMB"), 0, 16, 1);
	obs_properties_add_text(props, "input", obs_module_text("Input"), OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "input_format", obs_module_text("InputFormat"), OBS_TEXT_DEFAULT);
	obs_properties_add_int_slider(props, "speed_percent", obs_module_text("SpeedPercent"), 1, 200, 1);
	obs_properties_add_bool(props, "hw_decode", obs_module_text("HardwareDecode"));
	obs_properties_add_bool(props, "close_when_inactive", obs_module_text("CloseWhenInactive"));
	obs_properties_add_bool(props, "seekable", obs_module_text("Seekable"));
	obs_properties_add_text(props, "ffmpeg_options", obs_module_text("FFmpegOptions"), OBS_TEXT_DEFAULT);

	return props;
}

/* Set the media state */
static void set_media_state(void *data, enum obs_media_state state)
{
	struct ffmpeg_source *s = data;
	s->state = state;
}

/* Audio callback: output audio frames */
static void get_audio(void *opaque, struct obs_source_audio *a)
{
	struct ffmpeg_source *s = opaque;
	obs_source_output_audio(s->source, a);
	if (!s->is_local_file)
		FF_BLOG(LOG_INFO, "Reconnected.");
}

/* Called when media playback stops */
static void media_stopped(void *opaque)
{
	struct ffmpeg_source *s = opaque;
	set_media_state(s, OBS_MEDIA_STATE_ENDED);
	obs_source_media_ended(s->source);
}

/* Open the media file and create the media playback object */
static void ffmpeg_source_open(struct ffmpeg_source *s)
{
	if (s->input && *s->input) {
		struct mp_media_info info = {
			.opaque = s,
			.a_cb = get_audio,
			.stop_cb = media_stopped,
			.path = s->input,
			.format = s->input_format,
			.buffering = s->buffering_mb * 1024 * 1024,
			.speed = s->speed_percent,
			.hardware_decoding = s->is_hw_decoding,
			.ffmpeg_options = s->ffmpeg_options,
			.is_local_file = s->is_local_file || s->seekable,
		};

		s->media = media_playback_create(&info);
	}
}

/* Start audio playback */
static void ffmpeg_source_start(struct ffmpeg_source *s)
{
	if (!s->media)
		ffmpeg_source_open(s);
	if (!s->media)
		return;
	media_playback_play(s->media, s->is_looping, false);
	set_media_state(s, OBS_MEDIA_STATE_PLAYING);
	obs_source_media_started(s->source);
}

/* Tick function for periodic updates */
static void ffmpeg_source_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct ffmpeg_source *s = data;
	if (s->destroy_media) {
		if (s->media) {
			media_playback_destroy(s->media);
			s->media = NULL;
		}
		s->destroy_media = false;
	}
}

/* Update the source settings */
static void ffmpeg_source_update(void *data, obs_data_t *settings)
{
	struct ffmpeg_source *s = data;
	bool active = obs_source_active(s->source);
	bool is_local_file = obs_data_get_bool(settings, "is_local_file");
	const char *input;
	const char *input_format;
	const char *ffmpeg_options;
	bool is_hw_decoding;
	int speed_percent;
	bool is_looping;

	bfree(s->input_format);
	if (is_local_file) {
		input = obs_data_get_string(settings, "local_file");
		input_format = NULL;
		is_looping = obs_data_get_bool(settings, "looping");
	} else {
		input = obs_data_get_string(settings, "input");
		input_format = obs_data_get_string(settings, "input_format");
		is_looping = false;
	}

	bfree(s->input);
	bfree(s->ffmpeg_options);
	s->input = input ? bstrdup(input) : NULL;
	s->input_format = input_format ? bstrdup(input_format) : NULL;
	ffmpeg_options = obs_data_get_string(settings, "ffmpeg_options");
	s->ffmpeg_options = ffmpeg_options ? bstrdup(ffmpeg_options) : NULL;
	is_hw_decoding = obs_data_get_bool(settings, "hw_decode");
	speed_percent = (int)obs_data_get_int(settings, "speed_percent");
	if (speed_percent < 1 || speed_percent > 200)
		speed_percent = 100;
	s->is_hw_decoding = is_hw_decoding;
	s->speed_percent = speed_percent;
	s->is_looping = is_local_file ? obs_data_get_bool(settings, "looping") : false;
	s->close_when_inactive = obs_data_get_bool(settings, "close_when_inactive");
	s->is_local_file = is_local_file;
	s->seekable = obs_data_get_bool(settings, "seekable");
	s->buffering_mb = (int)obs_data_get_int(settings, "buffering_mb");
	s->log_changes = obs_data_get_bool(settings, "log_changes");

	if (s->media)
		media_playback_set_looping(s->media, s->is_looping);

	ffmpeg_source_open(s);
	if ((!s->restart_on_activate || active))
		ffmpeg_source_start(s);
}

/* Return the display name for the source */
static const char *ffmpeg_source_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("TryonsAudio");
}

/* Audio play/pause callback */
static void ffmpeg_source_play_pause(void *data, bool pause)
{
	struct ffmpeg_source *s = data;
	if (!s->media)
		ffmpeg_source_open(s);
	if (!s->media)
		return;
	media_playback_play_pause(s->media, pause);
	if (pause)
		set_media_state(s, OBS_MEDIA_STATE_PAUSED);
	else {
		set_media_state(s, OBS_MEDIA_STATE_PLAYING);
		obs_source_media_started(s->source);
	}
}

/* Stop the audio playback */
static void ffmpeg_source_stop(void *data)
{
	struct ffmpeg_source *s = data;
	if (s->media) {
		media_playback_stop(s->media);
		set_media_state(s, OBS_MEDIA_STATE_STOPPED);
	}
}

/* Restart playback */
static void ffmpeg_source_restart(void *data)
{
	struct ffmpeg_source *s = data;
	if (obs_source_showing(s->source))
		ffmpeg_source_start(s);
	set_media_state(s, OBS_MEDIA_STATE_PLAYING);
}

/* Get duration, current time, and state */
static int64_t ffmpeg_source_get_duration(void *data)
{
	struct ffmpeg_source *s = data;
	int64_t dur = 0;
	if (s->media)
		dur = media_playback_get_duration(s->media) / INT64_C(1000);
	return dur;
}

static int64_t ffmpeg_source_get_time(void *data)
{
	struct ffmpeg_source *s = data;
	return media_playback_get_current_time(s->media);
}

static void ffmpeg_source_set_time(void *data, int64_t ms)
{
	struct ffmpeg_source *s = data;
	if (!s->media)
		return;
	media_playback_seek(s->media, ms);
}

static enum obs_media_state ffmpeg_source_get_state(void *data)
{
	struct ffmpeg_source *s = data;
	return s->state;
}

/* Missing file handling for local files */
static obs_missing_files_t *ffmpeg_source_missingfiles(void *data)
{
	struct ffmpeg_source *s = data;
	obs_missing_files_t *files = obs_missing_files_create();
	if (s->is_local_file && s->input && strcmp(s->input, "") != 0) {
		if (!os_file_exists(s->input)) {
			obs_missing_file_t *file =
				obs_missing_file_create(s->input, NULL, OBS_MISSING_FILE_SOURCE, s->source, NULL);
			obs_missing_files_add_file(files, file);
		}
	}
	return files;
}

/* Create the audio source instance */
static void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct ffmpeg_source *s = bzalloc(sizeof(struct ffmpeg_source));
	s->source = source;
	s->media = NULL;
	s->destroy_media = false;
	s->input = NULL;
	s->input_format = NULL;
	s->ffmpeg_options = NULL;
	s->buffering_mb = 2;
	s->speed_percent = 100;
	s->is_looping = false;
	s->is_local_file = true;
	s->is_hw_decoding = false;
	s->full_decode = false;
	s->is_clear_on_media_end = true;
	s->restart_on_activate = true;
	s->close_when_inactive = false;
	s->seekable = false;
	s->log_changes = true;
	s->state = OBS_MEDIA_STATE_STOPPED;

	ffmpeg_source_defaults(settings);
	ffmpeg_source_update(s, settings);

	return s;
}

/* Destroy the audio source instance */
static void ffmpeg_source_destroy(void *data)
{
	struct ffmpeg_source *s = data;
	if (s->media) {
		media_playback_destroy(s->media);
		s->media = NULL;
	}
	bfree(s->input);
	bfree(s->input_format);
	bfree(s->ffmpeg_options);
	bfree(s);
}

/* Activate the audio source */
static void ffmpeg_source_activate(void *data)
{
	struct ffmpeg_source *s = data;
	ffmpeg_source_restart(s);
}

/* Deactivate the audio source */
static void ffmpeg_source_deactivate(void *data)
{
	struct ffmpeg_source *s = data;
	if (s->media) {
		media_playback_stop(s->media);
		set_media_state(s, OBS_MEDIA_STATE_STOPPED);
	}
}

/* Source registration structure (note: no audio_tick field) */
struct obs_source_info tryons_audio_source = {
	.id = "tryons_audio_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_CONTROLLABLE_MEDIA,
	.get_name = ffmpeg_source_getname,
	.create = ffmpeg_source_create,
	.destroy = ffmpeg_source_destroy,
	.get_defaults = ffmpeg_source_defaults,
	.get_properties = ffmpeg_source_getproperties,
	.activate = ffmpeg_source_activate,
	.deactivate = ffmpeg_source_deactivate,
	.update = ffmpeg_source_update,
	.media_play_pause = ffmpeg_source_play_pause,
	.media_restart = ffmpeg_source_restart,
	.media_stop = ffmpeg_source_stop,
	.media_get_duration = ffmpeg_source_get_duration,
	.media_get_time = ffmpeg_source_get_time,
	.media_set_time = ffmpeg_source_set_time,
	.media_get_state = ffmpeg_source_get_state,
	.missing_files = ffmpeg_source_missingfiles,
};

/* Module load registration */
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("tryons_audio", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Tryons Audio Source";
}

bool obs_module_load(void)
{
	obs_register_source(&tryons_audio_source);
	return true;
}

// Definition for updating the audio source (called from the network thread)
void tryons_audio_source_network_update(const char *new_path)
{
	if (!g_audio_source)
		return;
	obs_data_t *settings = obs_source_get_settings(g_audio_source);
	if (!settings)
		return;
	obs_data_set_string(settings, "local_file", new_path);
	obs_source_update(g_audio_source, settings);
	obs_data_release(settings);
}
