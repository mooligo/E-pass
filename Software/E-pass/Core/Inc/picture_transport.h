#ifndef __PICTURE_TRANSPORT_H__
#define __PICTURE_TRANSPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void PictureTransport_Init(void);
void PictureTransport_Task(void);
void PictureTransport_OnUsbRx(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __PICTURE_TRANSPORT_H__ */
