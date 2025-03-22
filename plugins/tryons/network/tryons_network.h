#ifndef TRYONS_NETWORK_H
#define TRYONS_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize networking (create a thread that listens for incoming messages)
bool tryons_network_init(void);

// Poll or process any pending network events (if you choose a polling model)
void tryons_network_poll(void);

// Shutdown the networking (stop thread, cleanup)
void tryons_network_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // TRYONS_NETWORK_H
