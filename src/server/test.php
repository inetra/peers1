<?
# ������ ������������ ������ ��� ��������� DomolinkDC++
# ����� �������
# http://flylinkdc.ru/test.php?port_IP=16211&port_PI=16212&ver=192
# ���
# - port_IP - ����� ������������ TCP-�����, ��������, 16211
# - port_PI - ����� ������������ UDP-�����, ��������, 16212
# - ver - ������ ��������� ��� ���-�� ���
#
# � ������, ���� �������� ver �� ������, ����� ��������� ��������
# ������ TCP �����.
#
# �����: SkazochNik (11.01.07) skazochnik97@lipetsk.ru
#
# ������ �� 04.06.07
# [-] ����� ������ � counter.dat (������, ����� ��� ���� �����?)
# [*] �������� ��������� ����� ������ ��� fsockopen, ������
#  ������ ����� ������ �����������

# ���������
$log_sc=0; // ����� ��� (0 - ���, 1 - ��) �������� �� �������


ignore_user_abort(1);
$port = $_POST['port'];

$ver = $_GET["ver"];
if($port == "" && isset($ver)){$port=$_GET["port_IP"];$port_udp =$_GET["port_PI"];}
$host=$_SERVER['REMOTE_ADDR'];

ob_start(); // ������� �����

echo "<html><head><title>���� ������ ��� ��������� FlylinkDC++</title>
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
echo "��� ���� ��� <b>".$host."</b> ���� TCP <b>".$port."</b> ���� UDP <b>".$port_udp . "</b>...<br><br>";
flush(); // ������ ����� :)
$fp = @fsockopen ($host, $port, &$errno, &$errstr, 5); # ������ �� 04.06.07

if (!$fp)
	{
	if ($port==$host){$errstr="������ DNK. ����������.";$errno="DNK";}
	echo "</td></tr><tr><td bgcolor=#eeeeee><b>���� TCP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> <font color=#990000>";

if ($errno=="60"){echo "TCP �� ������ ��������. �������� ��������� �� ������� �� ����������. ����� ������� �������� ��� ��������� �������������?";}
if ($errno=="10061"){echo "TCP �� ������ ��������. ��������� �����? :�";}
if ($errno=="61"){echo "TCP �� ������ ��������. ���������� ���������. ����� ������� �������� ��� ��������� �������������?";}

echo "</font><br>���� UDP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> ��� ������������� ����������� UDP-����.</td></tr><tr><td><br>\n";
	$isok = $errno; // ���� ���������
	}
	else
	{
	echo "</td></tr><tr><td bgcolor=#eeeeee><b>���� TCP:<br><span style=\"height:8px; width:8px; font-size:10px; background-color:#009900\">&nbsp;&nbsp;&nbsp;</span><font color=#009900> �������� TCP-����� ��������.</font>";


if (isset($ver))
	{
echo"<br>Te�� UDP:<br>";
#"�������� ������������, ��� UDP-���� �������� �����. ���������� :)"
$fa = @fsockopen ($host, $port_udp, &$errnov, &$errstrv, 5); # ������ �� 04.06.07

if ($errnov=="60"){echo "<span style=\"height:8px; width:8px; font-size:10px; background-color:#009900\">&nbsp;&nbsp;&nbsp;</span> <font color=#009900>�������� ������������, ��� UDP-���� �������� �����. ���������� :)</font>";}
if ($errnov=="61"){echo "<span style=\"height:8px; width:8px; font-size:10px; background-color:#990000\">&nbsp;&nbsp;&nbsp;</span> <font color=#990000>UDP �� ������ ��������. ����� ������� �������� ��� ��������� �������������?</font>";}

	}
echo"</b></td></tr><tr><td><br>";
	$isok = "ok"; // ���� ���������
	}


echo "��� ����� ��� �������:<ul><li><a href=\"javascript:window.close();\">������� ��������</a></li>";
echo "<li><a href=http://flylinkdc.ru/portforward.htm target=_blank>�������� � ���������������</a></li></ul><br>";
if($port==$host){$port="nOOb";};


if ($log_sc) {$all = "$host:$port:$port_udp:$ver:$isok\n";$fp = fopen("test.log","a");fputs($fp,$all);fclose($fp);}
}

if(!intval($port)>1000 && !intval($port)<32767 || intval($port)=="" && isset($port))
{
echo"<font color=#FF0000>�� ����� ����� �����.</font>";
}

if (!isset($ver)){
echo "<br>������� ����� TCP ����� ��� �������� (999 &#8249; PORT &#8249; 32768):
<table width=100%><tr><td>
<form action=\"$PHP_SELF\" method=\"post\">\n";
echo "<b>����:</b></td><td><input type=\"text\" name=\"port\" value=\"$port\" maxlength=\"5\"></td></tr>\n";
echo "<tr><td></td><td><input type=\"submit\" name=\"Submit\" value=\"������\">";
echo "</form></td></tr></table>";
}
echo "</td></tr><tr><td bgcolor=#eeeeee>������: SkazochNik (skazochnik97@lipetsk.ru)<br>�����: PPA (ppa@lipetsk.ru) </td></tr></table>";
echo "</body></html>";
?>