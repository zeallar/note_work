## MDTU 通讯协议

协议版本 `v1.1`

马来西亚 `DTU` `Json` 通讯协议

### 数据获取

```json
{"ac": "getinfo","val": 0}  // 基础信息
{
  "ac": "getinfo",
  "val": 0,
  "code": 0,
  "info": {
    "fver": 2,
    "hver": 1,
    "ccid": "898604C0192270201771",
    "imsi": "460084043701771",
    "imei": "861307060215453",
    "sn": "BG09PM001N",
    "model": "DT3400-SM"
  }
}

{"ac": "getinfo","val": 1}  // 基站附着信息
{
  "ac": "getinfo",
  "val": 1,
  "code": 0,
  "info": {
    "ip": "10.0.0.72",
    "apn": "CMIOTGDHZA.GD",
    "operator": "CHINA MOBILE(CMCC)",   // 运营商
    "lteBands": "B2/B3",   // 支持的 LTE 频段
    "lteStatus": "service available", // LTE 网络状态 //add to config
    "txPower": 0,   // 发射功率 (dBm) 	//at 获取
    "pCID": 0,      // network physical cell id
    "cid": 0,       // Cell ID for the registered 3GPP system
    "mcc": "460",   // Mobile country code
    "mnc": "00"     // Mobile network code
  }
}

{"ac": "getinfo","val": 2}  // 信号参数
{
  "ac": "getinfo",
  "val": 2,
  "code": 0,
  "info": {
    "ecio": 0,      // Ec/Io value representing negative 0.5 dB increments, e.g., 2 equals -1 dbm
    "rsrp": -87,    // Current RSRP in dBm, as measured by L1. Range: -44 to -140 (-44 equals -44 dBm, -140 equals -140 dBm)
    "rsrq": -6,     // RSRQ value in dB (signed integer value), as measured by L1. Range: -3 to -20 (-3 equals -3 dB, -20 equals -20 dB).
    "rssi": -60,    // RSSI in dBm. Indicates forward link pilot Power (AGC) + Ec/Io. A signed value; -125 or lower indicates no signal.
    "sinr": 0,      // Measured SINR in dB.
    "snr": 190,     // SNR level as a scaled integer in units of 0.1 dB; e.g., -16 dB has a value of -160 and 24.6 dB has a value of 246
  }
}

{"ac": "getinfo","val": 3}  // 串口信息
{"ac":"getinfo","val":3,"code":0,"info":[{"name":"RS232","param":{"bps":9600,"dbt":8,"pbt":0,"sbt":1}}]}

{"ac": "getinfo","val": 4}  // 扩展信息
{
  "ac": "getinfo",
  "val": 4,
  "code": 0,
  "info": {
    "bin": 0,       // RS232 数据转发到 usb 标志位
    "port": 4059,   // tcp 透传端口
    "timeout": 120, // tcp 透传客户端连接超时(s)
    "tcptmr": 0,    // tcp 透传无数据超时重启 (min) 0 为禁用
    "schedule": 0   // 周期重启(min) 0 为禁用
  }
}

{"ac": "getinfo","val": 5}  // APN 配置信息
{"ac":"getinfo","val":5,"code":0,"info":{"name":"CMIOTGDHZA.GD","user":"","passwd":""}}

{"ac": "getinfo","val": 6}  // ntp 配置信息 //自己移植库
{"ac":"getinfo","val":6,"code":0,"info":{"primary":"172.16.13.32","second":"172.16.13.33","interval":60,"timezone":4}}//interval min ,timezone 8 add to config

{"ac": "getinfo","val": 7}  // trap 配置  //不做
{
  "ac": "getinfo",
  "val": 7,
  "code": 0,
  "info": {
    "ip": "172.16.225.25", // trap 服务器ip
    "port": 666, // trap 服务器端口
    "ena": 1, // 启用 trap
    "trig": 1 // 触发标志位 (只有在启用 trap 并且触发标志位为 1 时才会自动上报 trap)
  }
}
```

### 参数配置

```json
{
  "ac": "set",
  "op": "serial",
  "name": "RS232", // 串口名称
  "val": {
    "dbt": 8, // 数据位 5/6/7/8 (5/6/7/8)
    "sbt": 1, // 停止位 1/2 (1/2)
    "pbt": 0, // 校验位 0/1/2 (无/奇/偶)
    "bps": 9600 // 波特率 (300 - 115200)  300/600/1200/2400/4800/9600/19200/38400/57600/115200
  }
}

{
  "ac": "set",
  "op": "general",
  "val": {
    "port": 4059,
    "bin": 0,
    "timeout": 120,
    "tcptmr": 30,
    "schedule": 30
  }
}
//to do at command
{
    "ac": "set",
    "op": "sn",
    "val": {
        "model": "DT3400-SM",\\--
        "sn": "xxxx",\\--
        "key": "b8e774f281c81e931b1f3a3a56ee5f42",\\--
        "hver": 1
    }
}
//to do  at command
{
  "ac": "set",
  "op": "apn",
  "val": {
    "name": "xxxx", // apn 名称\\--
    "user": "xxxx", // 用户名\\--
    "passwd": "xxxx" // 密码\\--
  }
}
//ok
{
  "ac": "set",
  "op": "ntp",
  "val": {
    "interval": 60, // 同步周期
    "timezone": 8, // 时区
    "primary": "172.16.225.24", // 主
    "second": "172.16.225.24" // 副
  }
}

{
  "ac": "set",
  "op": "snmp", ////不做
  "val": {
    "ip": "172.16.225.25", // trap 服务器ip
    "port": 666, // trap 服务器端口
    "ena": 1, // 启用 trap
    "trig": 1 // 触发标志位 (只有在启用 trap 并且触发标志位为 1 时才会自动上报 trap)
  }
}
//how to do //不做
{
  "ac": "set",
  "op": "link",
  "val": "7F00000000..." // 电表测试的 HEX 指令
}
//to do. what is logic?
{ "ac": "set", "op": "bin", "val": 0 }
//to do 
{
  "ac": "set",
  "op": "update",
  "val": {
    "url": "http://xxxx:xx/xxx/xx.bin", //下载地址172.16.225.25/appname
    "ver": 2, //版本
    "model": "xxxx" // 型号
  }
}
//ok
{ "ac": "set", "op": "restart" }
//to do 
{ "ac": "set", "op": "restore" }//恢复配置文件

```



### MQTT 服务器

```

172.16.225.24:1883

测试账号1
client id: test_f8a27e7f
username: 08818bb8
password: 50ba03ecd0c86bea

测试账号2
client id: test_ba9b06c3
username: 794f1745
password: 47390f94025257dd
```

