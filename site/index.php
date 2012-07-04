<?

include("lib/request.inc.php");
include("lib/redirect.inc.php");

function getParam($paramName) {
  return Request::get($paramName);
}

function isAlowedNick($nick) {
  return $nick == '[162]panchenko' || $nick == 'Larry' || preg_match('/^\[ADMIN\]/', $nick) || preg_match('/^\[OP\]/', $nick);
}

$directoryPath = dirname(__FILE__).'/files';

session_start();
if (array_key_exists(session_name(), $_GET)) {
  Redirector::redirect($_SERVER['PHP_SELF']);
  exit;
}
$action = getParam('action');
$isPOST = strcasecmp($_SERVER['REQUEST_METHOD'], 'POST') == 0;
$isAuthenticated = array_key_exists('AUTH', $_SESSION);
if ($action == 'version.xml') {
  $subdirectory = '';
  if (!(isAlowedNick(getParam('NICK')) || preg_match('/^192\.168/', getParam('REMOTE_ADDR')))) {
    $subdirectory = '/soft.cn.ru';
  }
  $latestUpdate = NULL;
  $buildNumber = NULL;
  $directory = @dir($directoryPath.$subdirectory);
  if ($directory) {
    while (false !== ($entry = $directory->read())) {
      if (strlen($entry) > 0 && $entry[0] != '.') {
        if (preg_match('/PeersUpdate(\d+)\.zip/i', $entry, $matches)) {
          if ($entry > $latestUpdate) {
            $latestUpdate = $entry;
            $buildNumber = $matches[1];
          }
        }
      }
    }
    $directory->close();
  }
  header('Content-Type: text/xml; charset=windows-1251');
  print '<?xml version="1.0" encoding="windows-1251" standalone="yes"?>';
  print '<Peers>';
  print '<URL>http://p2p.olmisoft.com/files'.$subdirectory.'/'.$latestUpdate.'</URL>';
  print '<MD5>'.md5_file($directoryPath.$subdirectory.'/'.$latestUpdate).'</MD5>';
  print '<Size>'.filesize($directoryPath.$subdirectory.'/'.$latestUpdate).'</Size>';
  print '<Build>'.$buildNumber.'</Build>';
  print '<Title>New build</Title>';
  print '<Message>Build '.$buildNumber.' is available</Message>';
  print '</Peers>';
  exit;
}
if ($action == 'login' && $isPOST) {
  if (getParam('login') == 'alex' && getParam('password') == 'p2p') {
    $_SESSION['AUTH'] = TRUE;
  }
}
else if ($action == 'upload' && $isPOST) {
  $files = Request::getFiles();
  if (array_key_exists('file', $files)) {
    $file = $files['file'];
    if (!file_exists($directoryPath)) {
      mkdir($directoryPath);
    }
    $file->moveTo($directoryPath);
  }
}
if ($isPOST) {
  Redirector::redirect($_SERVER['PHP_SELF'].'?action=home');
  exit;
}
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">';
print '<html>';
print '<head>';
print '<title>Peers Updates</title>';
print '<style type="text/css">';
print 'body { margin: 0px; background-color: #EEF; }';
print 'body, td, th, input, textarea { font-family: Verdana, Tahoma, sans-serif; font-size: 8pt; }';
print 'div.header { font-size: 18pt; font-weight: bold; color: white; background-color: #88F; padding: 0.75em; margin-bottom: 4px; }';
print 'div.footer { text-align: right; color: white; background-color: #88F; padding: 0.25em; margin-top: 4px; }';
print 'div.login { margin: 0px 4px 4px 4px; text-align: right; }';
print 'div.upload { margin: 0px 4px 4px 4px; padding: 4px; background-color: #ccf; }';
print 'div.filelist { width: 60%; margin-left: auto; margin-right: auto; }';
print 'table.list { border-collapse: collapse; width: 100%; }';
print 'table.list, table.list td, table.list th { border: 1px solid #88f; }';
print 'table.list td, table.list th { padding: 2px 4px; }';
print 'table.list td.group { padding: 4px 8px; font-weight: bold; font-size: 10pt; background-color: #ccf; }';
print '</style>';
print '</head>';
print '<body>';
print '<div class="header">Peers Updates</div>';
if (!$isAuthenticated) {
  print '<div class="login">';
  print '<form action="'.$_SERVER[PHP_SELF].'" method="POST">';
  print 'Login: <input type="text" name="login" size="8"> ';
  print 'Password: <input type="password" name="password" size="8"> ';
  print '<input type="hidden" name="action" value="login">';
  print '<input type="submit" value="Login">';
  print '</form>';
  print '</div>';
}
else {
  print '<div class="upload">';
  print '<form action="'.$_SERVER[PHP_SELF].'" method="POST" enctype="multipart/form-data">';
  print 'File: <input type="file" name="file" size="64" onchange="this.form.submit();"> ';
  print '<input type="hidden" name="action" value="upload">';
  print '<input type="submit" value="Upload">';
  print '</form>';
  print '</div>';
}
print '<div class="filelist">';
print '<table class="list">';
print '<tr><th>File Name</th><th>File Size</th><th>Last Modified</th></tr>';
$fileCount = 0;
$lastBaseFileName = '';
$directory = @dir($directoryPath);
if ($directory) {
  while (false !== ($entry = $directory->read())) {
    if (strlen($entry) > 0 && $entry[0] != '.') {
      $path = $directoryPath.'/'.$entry;
      if (is_file($path) && filesize($path)) {
        if (preg_match('/^(.+?)\d+\./', $entry, $matches)) {
          if ($matches[1] != $lastBaseFileName) {
            $lastBaseFileName = $matches[1];
            print '<tr><td colspan="3" class="group">'.$lastBaseFileName.'</td></tr>';
          }
        }
        print '<tr><td><a href="/files/'.$entry.'">'.$entry.'</a></td><td align="right">'.number_format(filesize($path)).'</td><td>'.date("Y-m-d H:i:s", filemtime($path)).'</td></tr>';
        ++$fileCount;
      }
    }
  }
  $directory->close();
}
if ($fileCount == 0) {
  print '<tr><td colspan="3" class="empty">There are no files</td></tr>';
}
print '</table>';
print '</div>';
print '<div class="footer">(c) 2007 Olmisoft</div>';
print '</body>';
print '</html>';
?>
