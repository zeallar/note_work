#ifndef MQTT_H
#define MQTT_H

void mqtt_thread_stop();
int mqtt_subscribe(char* topic);
int mqtt_unsubscribe(char* topic);
int mqtt_publish(char* topic,char* mes);
void* mqtt_run();
#endif