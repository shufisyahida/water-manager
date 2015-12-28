#include <asf.h>

#include "touch_api.h"
#include "touch_button.h"
#include "uptime.h"

#ifdef QTOUCH_STUDIO_MASKS
	extern TOUCH_DATA_T SNS_array[2][2];
	extern TOUCH_DATA_T SNSK_array[2][2];
#endif

#define GET_SENSOR_STATE(SENSOR_NUMBER) \
	qt_measure_data.qt_touch_status.sensor_states[(SENSOR_NUMBER / 8)] & (1 << (SENSOR_NUMBER % 8))


uint16_t qt_measurement_period_msec = 25;

void tb_init(void) {
	qt_config_data.qt_di				= DEF_QT_DI;
	qt_config_data.qt_neg_drift_rate	= DEF_QT_NEG_DRIFT_RATE;
	qt_config_data.qt_pos_drift_rate	= DEF_QT_POS_DRIFT_RATE;
	qt_config_data.qt_max_on_duration	= DEF_QT_MAX_ON_DURATION;
	qt_config_data.qt_drift_hold_time	= DEF_QT_DRIFT_HOLD_TIME;
	qt_config_data.qt_recal_threshold	= DEF_QT_RECAL_THRESHOLD;
	qt_config_data.qt_pos_recal_delay	= DEF_QT_POS_RECAL_DELAY;
	qt_filter_callback					= 0;


	#ifdef QTOUCH_STUDIO_MASKS
		SNS_array[0][0] = 0x40;
		SNS_array[0][1] = 0x0;
		SNS_array[1][0] = 0x0;
		SNS_array[1][1] = 0x0;

		SNSK_array[0][0] = 0x80;
		SNSK_array[0][1] = 0x0;
		SNSK_array[1][0] = 0x0;
		SNSK_array[1][1] = 0x0;
	#endif
	
	/* The sensor is wired up with SNS=PF6 and SNSK=PF7
	 * When using "pin configurability" this will result in channel 0
	 * because it is the first and only channel that is used.
	 * For the standard qtouch library setup we would need to use
	 * channel 3 since we are using the last two pins on the port.
	 */
	qt_enable_key(CHANNEL_0, NO_AKS_GROUP, 10, HYST_6_25);

	qt_init_sensing();
}


bool tb_is_touched(void) {
	
	static int16_t last = 0;
	int16_t now = getUptimeMs();
	
	if(now - last >= 25) {
		while(qt_measure_sensors(getUptimeMs()) & QTLIB_BURST_AGAIN);
		last = now;
	}
	
	if (GET_SENSOR_STATE(0)) {
		return 0;
	} else {
		return 1;
	}
}