;文件说明
:*:brief.::
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
:*:param.::
(
/* 
* @Description: 
* @param1- 参数:
* @param2- 参数:
* @return-
*/
)
;行注释
:*:ex.::/**/
;_____________________function___________________________

;获取当前日期ctrl+alt+d
^d::  ; 此热串通过下面的函数用当前日期和时间替换 "]d".
{
SendInput FormatTime(, "M/d/yyyy h:mm tt")  ;  看起来会像 9/1/2005 3:53 PM 这样
}
;运行git
!g::{
    run "E:\myNote\note_work\autogit.bat"
}