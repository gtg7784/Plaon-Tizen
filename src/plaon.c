#include <glib.h>
#include <Ecore.h>
#include <tizen.h>
#include <service_app.h>

#include "plaon.h"

#define EVENT_INTERVAL_SECOND 1.0f
#define CAMERA_CAPTURE_INTERVAL_MS 10000
#define IMAGE_FILE_PREFIX "CAM_"

typedef struct app_data_s {
	Ecore_Timer *event_timer;
} app_data;

static long long int __get_monotonic_ms(void) {
	long long int ret_time = 0;
	struct timespec time_s;

	if (0 == clock_gettime(CLOCK_MONOTONIC, &time_s))
		ret_time = time_s.tv_sec* 1000 + time_s.tv_nsec / 1000000;
	else
		_E("Failed to get ms");

	return ret_time;
}

static int __image_data_to_file(const char *filename, 	const void *image_data, unsigned int size) {
	FILE *fp = NULL;
	char *data_path = NULL;
	char file[PATH_MAX] = {'\0', };

	//	실습 8. 파일로 저장하기
	data_path = app_get_data_path();

	snprintf(file, PATH_MAX, "%s%s.jpg", data_path, filename);
	free(data_path);
	data_path = NULL;

	_D("File : %s", file);

	fp = fopen(file, "w");
	if (!fp) {
		_E("Failed to open file: %s", file);
		return -1;
	}

	if (fwrite(image_data, size, 1, fp) != 1) {
		_E("Failed to write image to file : %s", file);
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return 0;
}

static void __resource_camera_capture_completed_cb(const void *image, unsigned int size, void *user_data) {
	char filename[PATH_MAX] = {'\0', };

	snprintf(filename, PATH_MAX, "%s%lld", IMAGE_FILE_PREFIX, __get_monotonic_ms());

	__image_data_to_file(filename, image, size);
}

static Eina_Bool _control_interval_event_cb(void *data) {
	int ret = 0;

	static long long int last_captured_time = 0;
	long long int now = __get_monotonic_ms();

	if (now > last_captured_time + CAMERA_CAPTURE_INTERVAL_MS) {
//		실습 2. 촬영 시작
		ret = resource_camera_capture(__resource_camera_capture_completed_cb, NULL);
		if (ret < 0)
			_E("Failed to capture camera");

		last_captured_time = now;
	}

	return ECORE_CALLBACK_RENEW;
}

static bool service_app_create(void *data) {
	app_data *ad = (app_data *)data;

	//	실습 1. 타이머 추가
	ad->event_timer = ecore_timer_add(EVENT_INTERVAL_SECOND, _control_interval_event_cb, ad);
	if (!ad->event_timer) {
		_E("Failed to add action timer");
		return false;
	}

	return true;
}

static void service_app_terminate(void *data) {
	app_data *ad = (app_data *)data;

	if (ad->event_timer)
		ecore_timer_del(ad->event_timer);

	resource_camera_close();

	free(ad);
}

void service_app_control(app_control_h app_control, void *data) {
    // Todo: add your code here.
    return;
}

static void service_app_lang_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LANGUAGE_CHANGED*/
	return;
}

static void service_app_region_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void service_app_low_battery(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LOW_BATTERY*/
}

static void service_app_low_memory(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[]) {
	app_data *ad = NULL;
	int ret = 0;
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	ret = service_app_main(argc, argv, &event_callback, ad);

	return ret;
}
