<?
# ������ ����������� IP ������ ����������� ������������ ���� Domolink
# ����� �������:
# http://www.flylinkdc.ru/getip.php
# �����: SkazochNik (6.01.07) skazochnik97@lipetsk.ru

ignore_user_abort(1);
$host=$_SERVER['REMOTE_ADDR'];

print "<html><head><title>Current IP Check</title></head><body>Current IP Address: $host</body></html>
";

?>