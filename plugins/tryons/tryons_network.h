#ifndef TRYONS_NETWORK_H
#define TRYONS_NETWORK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool tryons_network_init(void);
void tryons_network_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // TRYONS_NETWORK_H
