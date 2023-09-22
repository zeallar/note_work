/* 热键代码注释：
  #==win，!==Alt, ^==ctrl, +==shfit, XButton1==鼠标侧键后退键， XButton2==鼠标侧键前进键，mbutton==鼠标中键
  可以将AHK脚本放置在启动文件夹（win+R运行，输入：shell:startup，就可以打开启动文件夹），每次开机自动运行。
  当然前提是需要安装好AHK程序，这个自行官网下载
*/
;文件说明
::brief::
(
/************************************************* 
Author:zhouBL
Version: 
Description: 
Others: 
created date:
modified date:
*************************************************/
)
;函数说明
::param::
(
/* 
* @Description: 
* @param1- 参数:
* @param2- 参数:
* @return-
*/
)
;行注释
::ex::/**/


;_____________________function___________________________

;获取当前日期ctrl+alt+d
^!d:: 
FormatTime, CurrentDateTime,, yyyy/M/d h:mm tt
SendInput %CurrentDateTime%
return



