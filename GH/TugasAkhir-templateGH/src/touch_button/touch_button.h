#ifndef TOUCH_BUTTON_H_
#define TOUCH_BUTTON_H_

#if( BOARD != XMEGA_A3BU_XPLAINED )
#error Only for board XMEGA_A3BU_XPLAINED
#endif

#include <stdbool.h>

void tb_init(void);
bool tb_is_touched(void);

#endif /* TOUCH_BUTTON_H_ */