#ifndef TRYONS_COMMON_H
#define TRYONS_COMMON_H

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

// Declare the global variables
extern obs_source_t *g_audio_source;
extern obs_source_t *g_text_source;

// Declare the network update functions with C linkage.
extern void tryons_text_source_network_update(const char *new_text);
extern void tryons_audio_source_network_update(const char *new_path);

#ifdef __cplusplus
}
#endif

#endif // TRYONS_COMMON_H
