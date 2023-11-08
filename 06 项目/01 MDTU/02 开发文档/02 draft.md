```shell
adb pull /bin/mqtt_test E:\vmwareWork\cron_work\mdtu_commu_bak\update_tools\pull

adb pull /etc/rc E:\vmwareWork\cron_work\mdtu_commu_bak\update_tools\pull

adb push E:\vmwareWork\cron_work\mdtu_commu_bak\update_tools\pull\rc /etc
adb shell chmod 777 /etc/rc

adb pull /bin/crc32 E:\vmwareWork\cron_work\mdtu_commu_bak\update_tools\pull
```



fibocom develop 安装路径

C:\Program Files\Fibocom_Develop_Driver
