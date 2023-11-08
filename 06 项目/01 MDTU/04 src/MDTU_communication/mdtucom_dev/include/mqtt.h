#ifndef MQTT_H
#define MQTT_H

void mqtt_thread_stop();
int32_t mqtt_subscribe(int8_t* topic);
int32_t mqtt_unsubscribe(int8_t* topic);
int32_t mqtt_publish(int8_t* topic,void* _s);
void* mqtt_run();
#endif