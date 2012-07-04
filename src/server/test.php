<?
# Скрипт тестирования портов для программы DomolinkDC++
# Форма запроса
# http://flylinkdc.ru/test.php?port_IP=16211&port_PI=16212&ver=192
# где
# - port_IP - номер тестируемого TCP-порта, например, 16211
# - port_PI - номер тестируемого UDP-порта, например, 16212
# - ver - версия программы или что-то ещё
#
# В случае, если параметр ver не указан, будет проведена проверка
# только TCP порта.
#
# Автор: SkazochNik (11.01.07) skazochnik97@lipetsk.ru
#
# Правка от 04.06.07
# [-] Убрал запись в counter.dat (кстати, зачем она была нужна?)
# [*] Пофиксил системный вывод ошибок для fsockopen, теперь
#  ошибку ловим только переменными

# Настройка
$log_sc=0; // Вести лог (0 - нет, 1 - да) Отключен по дефолту


ignore_user_abort(1);
$port = $_POST['port'];

$ver = $_GET["ver"];
if($port == "" && isset($ver)){$port=$_GET["port_IP"];$port_udp =$_GET["port_PI"];}
$host=$_SERVER['REMOTE_ADDR'];

ob_start(); // Открыть буфер

echo "<html><head><title>Тест портов для программы FlylinkDC++</title>
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1251\">";
echo "<style type=\"text/css\">
<!--
table {
	font-family: Verdana, Arial, Helvetica, sans-serif;
	font-size: 10px;
}
button {
	font-family: Verdana, Arial, Helvetica, sans-serif;
	font-size: 10px;
}
-->
</style>";

echo "</head>";
echo"<body link=\"#000000\" BGCOLOR=\"#FFFFFF\"><br><br>";

echo "<table align=center width=600><tr><td>";
if (isset($port) && intval($port)>1000 && intval($port)<32767)
{
echo "Идёт тест для <b>".$host."</b> порт TCP <b>".$port."</b> порт UDP <b>".$port_udp . "</b>...<br><br>";
flush(); // Отдать буфер :)
$fp = @fsockopen ($host, $port, &$errno, &$errstr, 5); # Правка от 04.06.07

if (!$fp)
	{
	if ($port==$host){$errstr="Ошибка DNK. Неизлечимо.";$errno="DNK";}
	echo "</td></tr><tr><td bgcolor=#eeeeee><b>Тест TCP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> <font color=#990000>";

if ($errno=="60"){echo "TCP не прошёл проверку. Конечный компьютер не ответил на соединение. Может следует почитать как настроить портфорвардиг?";}
if ($errno=="10061"){echo "TCP не прошёл проверку. Кончились слоты? :Р";}
if ($errno=="61"){echo "TCP не прошёл проверку. Соединение отклонено. Может следует почитать как настроить портфорвардиг?";}

echo "</font><br>Тест UDP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> Нет необходимости тестировать UDP-порт.</td></tr><tr><td><br>\n";
	$isok = $errno; // флаг состояния
	}
	else
	{
	echo "</td></tr><tr><td bgcolor=#eeeeee><b>Тест TCP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#009900\">&nbsp;&nbsp;&nbsp;</span><font color=#009900> Проверка TCP-порта пройдена.</font>";


if (isset($ver))
	{
echo"<br>Teст UDP:<br>";
#"Очевидно предположить, что UDP-порт настроен верно. Поздравляю :)"
$fa = @fsockopen ($host, $port_udp, &$errnov, &$errstrv, 5); # Правка от 04.06.07

if ($errnov=="60"){echo "<span style=\"height:8px; width:8px; font-size:10px; background-color:#009900\">&nbsp;&nbsp;&nbsp;</span> <font color=#009900>Очевидно предположить, что UDP-порт настроен верно. Поздравляю :)</font>";}
if ($errnov=="61"){echo "<span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> <font color=#990000>UDP не прошёл проверку. Может следует почитать как настроить портфорвардиг?</font>";}

	}
echo"</b></td></tr><tr><td><br>";
	$isok = "ok"; // флаг состояния
	}


echo "Что можно ещё сделать:<ul><li><a href=\"javascript:window.close();\">Закрыть страницу</a></li>";
echo "<li><a href=http://flylinkdc.ru/portforward.htm target=_blank>Почитать о портфорвардинге</a></li></ul><br>";
if($port==$host){$port="nOOb";};


if ($log_sc) {$all = "$host:$port:$port_udp:$ver:$isok\n";$fp = fopen("test.log","a");fputs($fp,$all);fclose($fp);}
}

if(!intval($port)>1000 && !intval($port)<32767 || intval($port)=="" && isset($port))
{
echo"<font color=#FF0000>Не введён номер порта.</font>";
}

if (!isset($ver)){
echo "<br>Введите номер TCP порта для проверки (999 &#8249; PORT &#8249; 32768):
<table width=100%><tr><td>
<form action=\"$PHP_SELF\" method=\"post\">\n";
echo "<b>Порт:</b></td><td><input type=\"text\" name=\"port\" value=\"$port\" maxlength=\"5\"></td></tr>\n";
echo "<tr><td></td><td><input type=\"submit\" name=\"Submit\" value=\"Нажать\">";
echo "</form></td></tr></table>";
}
echo "</td></tr><tr><td bgcolor=#eeeeee>Кодинг: SkazochNik (skazochnik97@lipetsk.ru)<br>Мысли: PPA (ppa@lipetsk.ru) </td></tr></table>";
echo "</body></html>";
?>