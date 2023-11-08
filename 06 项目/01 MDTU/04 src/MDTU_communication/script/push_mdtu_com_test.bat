adb push E:\vmwareWork\snmp_work\opensdk_release\app_demo\MDTU_communication\build\bin\mqtt_test /bin
adb push E:\vmwareWork\snmp_work\opensdk_release\app_demo\MDTU_communication\mdtucom_application\mdtu_com.ini /etc
adb shell chmod 777 /bin/mqtt_test
adb shell chmod 777 /etc/mdtu_com.ini
adb shell