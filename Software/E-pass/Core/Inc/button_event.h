#ifndef __BUTTON_EVENT_H__
#define __BUTTON_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PREV,
    BUTTON_EVENT_NEXT
} ButtonEvent_t;

/* Optional init to clear pending state at startup. */
void ButtonEvent_Init(void);

/* Returns one pending event and clears it. */
ButtonEvent_t ButtonEvent_Get(void);

/* Returns non-zero if any button event is pending. */
uint8_t ButtonEvent_HasPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUTTON_EVENT_H__ */
