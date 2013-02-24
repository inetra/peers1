/* 
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "SettingsManager.h"
#include "ResourceManager.h"

#include "SimpleXML.h"
//[-]PPA [Doxygen 1.5.1] #include "Util.h"
#ifdef PPA_INCLUDE_SSL
#include "File.h"
#endif
#include "CID.h"
#ifdef PPA_INCLUDE_SSL
 #include "Socket.h" // !SMT!-IP
#endif
#include "../../utils/SystemUtils.h"
#include "../peers/PeersVersion.h"
#define SENTRY "SENTRY"
StringList SettingsManager::connectionSpeeds;

const string SettingsManager::settingTags[] =
{
	// Strings //

	"Nick", "UploadSpeed", "Description", "DownloadDirectory", "DownloadDirectoryShared", "EMail", "ExternalIp",
    
	"TextFont", "MainFrameOrder", "MainFrameWidths", "HubFrameOrder", "HubFrameWidths",
    "LanguageFile", "SearchFrameOrder", "SearchFrameWidths", "FavoritesFrameOrder", "FavoritesFrameWidths", "FavoritesFrameVisible",
    "HublistServers", "QueueFrameOrder", "QueueFrameWidths", "PublicHubsFrameOrder", "PublicHubsFrameWidths", "PublicHubsFrameVisible",
    "UsersFrameOrder", "UsersFrameWidths", "UsersFrameVisible", "HttpProxy", "LogDirectory", "NotepadText", "LogFormatPostDownload",
	
	"LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat", "FinishedOrder", "FinishedWidths",	 
	"TempDownloadDirectory", "BindAddress", "SocksServer", "SocksUser", "SocksPassword", "ConfigVersion", 
    
	"DefaultAwayMessage", "TimeStampsFormat", "ADLSearchFrameOrder", "ADLSearchFrameWidths", "ADLSearchFrameVisible",
    "FinishedULWidths", "FinishedULOrder", "CID", "SpyFrameWidths", "SpyFrameOrder", "SpyFrameVisible",
	
	"BeepFile", "BeginFile", "FinishedFile", "SourceFile", "UploadFile", "FakerFile", "ChatNameFile", "WinampFormat",
	"KickMsgRecent01", "KickMsgRecent02", "KickMsgRecent03", "KickMsgRecent04", "KickMsgRecent05", 
	"KickMsgRecent06", "KickMsgRecent07", "KickMsgRecent08", "KickMsgRecent09", "KickMsgRecent10", 
	"KickMsgRecent11", "KickMsgRecent12", "KickMsgRecent13", "KickMsgRecent14", "KickMsgRecent15", 
	"KickMsgRecent16", "KickMsgRecent17", "KickMsgRecent18", "KickMsgRecent19", "KickMsgRecent20",
	"Toolbar", "ToolbarImage", "ToolbarHot", "UserListImage",
	"UploadQueueFrameOrder", "UploadQueueFrameWidths", "DownSpeed", "UpSpeed",
    
	"UpdateURL", "SoundTTH", "SoundException", "SoundHubConnected", "SoundHubDisconnected", "SoundFavUserOnline", "SoundFavUserOffline", "SoundTypingNotify",

	"WebServerLogFormat" , "WebServerUser", "WebServerPass", "LogFileMainChat", 
	"LogFilePrivateChat", "LogFileStatus", "LogFileUpload", "LogFileDownload", "LogFileSystem", "LogFormatSystem", 
	"LogFormatStatus", "LogFileWebServer", "DirectoryListingFrameOrder", "DirectoryListingFrameWidths", 
	"MainFrameVisible", "SearchFrameVisible", "QueueFrameVisible", "HubFrameVisible", "UploadQueueFrameVisible", 
	"EmoticonsFile", "TLSPrivateKeyFile", "TLSCertificateFile", "TLSTrustedCertificatesPath",
	"FinishedVisible", "FinishedULVisible", "BetaUser", "BetaPass", "AuthPass", "SkiplistShare", "DirectoryListingFrameVisible",

	"RecentFrameOrder", "RecentFrameWidths", "RecentFrameVisible", "HighPrioFiles", "LowPrioFiles", "SecondaryAwayMsg", "ProtectedPrefixes", "WMPFormat",
	"iTunesFromat", "MPCFormat", "RawOneText", "RawTwoText", "RawThreeText", "RawFourText", "RawFiveText", "PopupFont", "PopupTitleFont", "PopupFile",
    
	"BanMessage","SlotAsk", // !SMT!-S
	"UrlGetIp", //[+]PPA
    "Password", "PasswordHint", // !SMT!-PSW
	"WebMagnet",//[+]necros
	"RatioTemplate",//[+] WhiteD. Custom ratio message
    "UrlTestIp", //[+]PPA
	"CustomVideoPlayer",
    SENTRY, 

	
	// Ints //

	"IncomingConnections", "InPort", "Slots", "Rollback", "AutoFollow", "ClearSearch",
	"BackgroundColor", "TextColor", "ShareHidden", "FilterMessages", "MinimizeToTray",
	"AutoSearch", "TimeStamps", "ConfirmExit", "PopupHubPms", "PopupBotPms", "IgnoreHubPms", "IgnoreBotPms",
	"BufferSize", "DownloadSlots", "MaxDownloadSpeed", "LogMainChat", "LogPrivateChat",
	"LogDownloads", "LogUploads", "StatusInChat", "ShowJoins", "PrivateMessageBeep", "PrivateMessageBeepOpen",
	"UseSystemIcons", "PopupPMs", "MinUploadSpeed", "GetUserInfo", "UrlHandler", "MainWindowState", 
	"MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX", "MainWindowPosY", "AutoAway",
	"SocksPort", "SocksResolve", "KeepLists", "AutoKick", "QueueFrameShowTree", 
	"CompressTransfers", "ShowProgressBars", "MaxTabRows",
	"MaxCompression", "AntiFragMethod", "MDIMaxmimized", "NoAwayMsgToBots",
	"SkipZeroByte", "AdlsBreakOnFirst",
	"HubUserCommands", "AutoSearchAutoMatch", "DownloadBarColor", "UploadBarColor", "LogSystem",
	"LogFilelistTransfers", "ShowStatusbar", "BandwidthSettingMode", "ShowToolbar", "ShowTransferview", 
	"SearchPassiveAlways", "SetMinislotSize", "ShutdownInterval", "DontAnnounceNewVersions", 
	"CzertHiddenSettingA", "CzertHiddenSettingB", "ExtraSlots", 
	"TextGeneralBackColor", "TextGeneralForeColor", "TextGeneralBold", "TextGeneralItalic", 
	"TextMyOwnBackColor", "TextMyOwnForeColor", "TextMyOwnBold", "TextMyOwnItalic", 
	"TextPrivateBackColor", "TextPrivateForeColor", "TextPrivateBold", "TextPrivateItalic", 
	"TextSystemBackColor", "TextSystemForeColor", "TextSystemBold", "TextSystemItalic", 
	"TextServerBackColor", "TextServerForeColor", "TextServerBold", "TextServerItalic", 
	"TextTimestampBackColor", "TextTimestampForeColor", "TextTimestampBold", "TextTimestampItalic", 
	"TextMyNickBackColor", "TextMyNickForeColor", "TextMyNickBold", "TextMyNickItalic", 
	"TextFavBackColor", "TextFavForeColor", "TextFavBold", "TextFavItalic", 
	"TextOPBackColor", "TextOPForeColor", "TextOPBold", "TextOPItalic", 
	"TextURLBackColor", "TextURLForeColor", "TextURLBold", "TextURLItalic", 
	"BoldAuthorsMess", "UploadLimitNormal", "ThrottleEnable", "HubSlots", "DownloadLimitNormal", 
	"UploadLimitTime", "DownloadLimitTime", "TimeThrottle", "TimeLimitStart", "TimeLimitEnd",
	"RemoveForbidden", "ProgressTextDown", "ProgressTextUp", "ShowInfoTips", "ExtraDownloadSlots",
	"MinimizeOnStratup", "ConfirmDelete", "DefaultSearchFreeSlots", 
	"ExtensionDownTo", "ErrorColor", "ExpandQueue", "TransferSplitSize", "IDownSpeed", "HDownSpeed", "DownTime", 
	"ProgressOverrideColors", "Progress3DDepth", "ProgressOverrideColors2",
	"MenubarTwoColors", "MenubarLeftColor", "MenubarRightColor", "MenubarBumped", 
	"DisconnectingEnable", "MinFileSize", "UploadQueueFrameShowTree",
	"SegmentsManual", "NumberOfSegments", "PercentFakeShareTolerated",
	"SendUnknownCommands", "Disconnect",
	"AutoConfigureNetwork", "MaxHashSpeed", "GetUserCountry", "DisableCZDiacritic",
	"DebugCommands", "UseAutoPriorityByDefault", "UseOldSharingUI", "ShowDescriptionSpeed",
	"FavShowJoins", "LogStatusMessages", "PMLogLines", "SearchAlternateColour", "SoundsDisabled",
	"ReportFoundAlternates", "CheckNewUsers", "GarbageIn", "GarbageOut", 
	"SearchTime", "DontBeginSegment", "DontBeginSegmentSpeed", "PopunderPm", "PopunderFilelist",
	"DropMultiSourceOnly", "DisplayCheatsInMainChat", "MagnetAsk", "MagnetAction", "MagnetRegister",
	"DisconnectRaw", "TimeoutRaw", "FakeShareRaw", "ListLenMismatch", "FileListTooSmall", "FileListUnavailable",
	"AddFinishedInstantly", "Away", "UseCTRLForLineHistory",

	"PopupHubConnected", "PopupHubDisconnected", "PopupFavoriteConnected", "PopupFavoriteDisconnected", "PopupCheatingUser", "PopupDownloadStart",

	"PopupDownloadFailed", "PopupDownloadFinished", "PopupUploadFinished", "PopupPm", "PopupNewPM", 
	"PopupType", "WebServer", "WebServerPort", "WebServerLog", "ShutdownAction", "MinimumSearchInterval",
	"PopupAway", "PopupMinimized", "ShowShareCheckedUsers", "MaxAutoMatchSource",
    "ReservedSlotColor", "IgnoredColor", "FavoriteColor",
	"NormalColour", "ClientCheckedColour", "FileListCheckedColour",
	"FireballColor", "ServerColor", "PasiveColor", "OpColor", 
	"FileListAndClientCheckedColour", "BadClientColour", "BadFilelistColour", "DontDLAlreadyShared", "RealTimeQueueUpdate",
	"ConfirmHubRemoval", "SuppressMainChat", "ProgressBackColor", "ProgressCompressColor", "ProgressSegmentColor",
	"UseVerticalView", "OpenNewWindow", "FileSlots",  "UDPPort", "MultiChunk",
 	"UserListDoubleClick", "UserListChatDoubleClick", "TransferListDoubleClick", "ChatDoubleClick", "AdcDebug",
	"ToggleActiveWindow", "ProgressbaroDCStyle", "SearchHistory", 
	"BadSoftDetections", "DetectBadSoft", "AdvancedResume", "AcceptedDisconnects", "AcceptedTimeouts",
	"OpenPublic", "OpenFavoriteHubs", "OpenFavoriteUsers", "OpenQueue", "OpenFinishedDownloads",
	"OpenFinishedUploads", "OpenSearchSpy", "OpenNetworkStatistics", "OpenNotepad", "OutgoingConnections",
	"OpenAdvice12",
	"OpenSubscriptions",
	"NoIPOverride", "GroupSearchResults", "BoldFinishedDownloads", "BoldFinishedUploads", "BoldQueue", 
	"BoldHub", "BoldPm", "BoldSearch", "TabsOnTop", "SocketInBuffer", "SocketOutBuffer", 
	"ColorRunning", "ColorDownloaded", "ColorVerified", "ColorAvoiding", "AutoRefreshTime", "UseTLS", "OpenWaitingUsers",
	"BoldWaitingUsers", "AutoSearchLimit", "AutoKickNoFavs", "PromptPassword", "SpyFrameIgnoreTthSearches",
 	"AllowUntrustedHubs", "AllowUntrustedClients", "TLSPort", "FastHash", "DownConnPerSec",
	"HighestPrioSize", "HighPrioSize", "NormalPrioSize", "LowPrioSize", "LowestPrio", 
	"ShowDescriptionSlots", "ShowDescriptionLimit", "ProtectTray", "ProtectStart", "ProtectClose",
	"StripTopic", "TbImageSize", "TbImageSizeHot", "OpenCdmDebug", "ShowWinampControl", "HubThreshold", "PGOn", "GuardUp", "GuardDown", "GuardSearch", "PGLog",
	"PreviewPm", "FilterEnter", "PopupTime", "AwayThrottle", "AwayStart", "AwayEnd", "OdcStyleBumped", "TopSpeed", "StealthyStyle", "PSRDelay",
	"IpInChat", "CountryInChat", "TopUpSpeed", "Broadcast", "SettingsState", "Page", "ChatBufferSize", "EnableHubTopic", "HubSmall", "PgLastUp", "FormatBIU",
	"MediaPlayer", "ProtFavs", "MaxMsgLength", "PopupBackColor", "PopupTextColor", "PopupTitleTextColor", "SortFavUsersFirst", "ShowShellMenu", "OpenLogsInternal",
	"NoEmotesLinks",

	"NsLookupMode", "NsLookupDelay", // !SMT!-IP
	"BanSlots", "BanShare", "BanLimit", "BanmsgPeriod", "BanStealth", "BanForcePM", "BanSkipOps", "ExtraSlotToDl", // !SMT!-S
    "BanColor", "DupeColor", "MultilineChatInput", "ExternalPreview", "SendSlotgrantMsg", "FavUserDblClick",// !SMT!-UI
    "ProtectPrivate", // !SMT!-PSW
    "UploadTransfersColors", // [+] Drakon
	"StartupBackup", // [+] Drakon
	
	"GlobalHubFrameConf", "UseAntiVir", "IgnoreUseRegExpOrWc", "StealthyIndicateSpeeds",
	"OldTrayBehav", "OldNamingStyle", "OldIconsMode", "ColourDupe", "NoTigerTreeHashCheat", "OldClientRaw",
	"DeleteChecked", "Topmost", "LockToolbars", "AutoCompleteSearch", "KeepDLHistory", "KeepULHistory", "ShowQSearch",
	"SearchDetectHash", "FullFileListNfo", "ZionTabs", "NonHubsFront", "GlobalMiniTabs", "BlendOffline", "MaxResizeLines",
    "UseCustomListBackground",
    "ExpertMode",
    "SearchExpandToolbarOnActivate",
    "SearchRestoreConditionOnActivate",
    "MenuAdvanced",
    "Provider",
    "OpenHubChatOnConnect",
	"NickAddUniqueSuffix",
	"DownloadDirectoryShortcut",
	"UseCustomVideoPlayer",
	"MinimizeOnClose",
	"DefaultDSCPmark",
	"HubDSCPmark",
	"PeerDSCPmark",
	SENTRY,
	
	
	// Int64 //
	
	"TotalUpload", "TotalDownload",
	SENTRY
};

const string SettingsManager::speeds[] = {"64K","128K","150K","192K",
"256K","384K","400K","512K","600K","768K","1M","1.5M","2M","4M","6M","8M","10M+" };

SettingsManager::SettingsManager(const string& version):
m_version(version)
{
connectionSpeeds.push_back("0.005");
	connectionSpeeds.push_back("0.01");
	connectionSpeeds.push_back("0.02");
	connectionSpeeds.push_back("0.05");
	connectionSpeeds.push_back("0.1");
	connectionSpeeds.push_back("0.2");
	connectionSpeeds.push_back("0.5");
	connectionSpeeds.push_back("1");
	connectionSpeeds.push_back("2");
	connectionSpeeds.push_back("5");
	connectionSpeeds.push_back("10");
	connectionSpeeds.push_back("20");
	connectionSpeeds.push_back("50");
	connectionSpeeds.push_back("100");

	for(int i=0; i<SETTINGS_LAST; i++)
		isSet[i] = false;

	for(int j=0; j<INT_LAST-INT_FIRST; j++) {
		intDefaults[j] = 0;
		intSettings[j] = 0;
	}
	for(int k=0; k<INT64_LAST-INT64_FIRST; k++) {
		int64Defaults[k] = 0;
		int64Settings[k] = 0;
	}
        setDefaults();
}

static pair<string,string> getDownloadsPath() {
	tstring path;
	if (Util::isVista()) {
		path = SystemUtils::getProfileFolder();
	}
	else {
		path = SystemUtils::getDocumentFolder();
	}
	string downloads = Text::fromT(path);
	if (!downloads.empty() && downloads[downloads.length() - 1] != PATH_SEPARATOR) {
		downloads += PATH_SEPARATOR_STR;
	}
	downloads += Text::fromT(_T("Загрузки ") _T(APPNAME) _T(PATH_SEPARATOR_STR));
	return make_pair(downloads, downloads + Text::fromT(_T("Incomplete") _T(PATH_SEPARATOR_STR)));
}

void SettingsManager::setDefaults()
{
	pair<string,string> downloadPaths = getDownloadsPath();
	setDefault(DOWNLOAD_DIRECTORY, downloadPaths.first);
	setDefault(TEMP_DOWNLOAD_DIRECTORY, downloadPaths.second);
	setDefault(DOWNLOAD_DIRECTORY_SHARED, Util::emptyString);
	setDefault(SLOTS, 15); //[!] PPA 2->15
	setDefault(TCP_PORT, 0);
	setDefault(UDP_PORT, 0);
	setDefault(TLS_PORT, 0);
	setDefault(INCOMING_CONNECTIONS, INCOMING_FIREWALL_PASSIVE);
	setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
	setDefault(ROLLBACK, 4096);
//		setDefault(BAN_FEW_HUB, false);//[+]necros
	setDefault(AUTO_FOLLOW, true);
	setDefault(CLEAR_SEARCH, true);
	setDefault(SHARE_HIDDEN, false);
	setDefault(FILTER_MESSAGES, true);
	setDefault(MINIMIZE_TRAY, true);
	setDefault(AUTO_SEARCH, true);
	setDefault(TIME_STAMPS, true);
	setDefault(CONFIRM_EXIT, true);
	setDefault(POPUP_HUB_PMS, true);
	setDefault(POPUP_BOT_PMS, true);
	setDefault(IGNORE_HUB_PMS, false);
	setDefault(IGNORE_BOT_PMS, false);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, ""); //[-]PPA "http://nis.edu.ru/PublicHubList.xml.bz2;http://nis.edu.ru/hublist.xml.bz2"
	setDefault(DOWNLOAD_SLOTS, 0); //[+]PPA
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(LOG_DIRECTORY, Text::fromT(SystemUtils::getAppDataFolder()) + "Logs" PATH_SEPARATOR_STR);
	setDefault(LOG_UPLOADS, false);
	setDefault(LOG_DOWNLOADS, false);
	setDefault(LOG_PRIVATE_CHAT, false);
	setDefault(LOG_MAIN_CHAT, false);
	setDefault(STATUS_IN_CHAT, true);
	setDefault(SHOW_JOINS, false);
	setDefault(UPLOAD_SPEED, connectionSpeeds[0]);
	setDefault(PRIVATE_MESSAGE_BEEP, false);
	setDefault(PRIVATE_MESSAGE_BEEP_OPEN, false);
	setDefault(USE_SYSTEM_ICONS, true);
	setDefault(POPUP_PMS, true);
	setDefault(MIN_UPLOAD_SPEED, 0);
	setDefault(LOG_FORMAT_POST_DOWNLOAD, "%Y-%m-%d %H:%M: %[target]" + STRING(DOWNLOADED_FROM) + "%[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_POST_UPLOAD, "%Y-%m-%d %H:%M: %[source]" + STRING(UPLOADED_TO) + "%[userNI] (%[userCID]), %[fileSI] (%[fileSIchunk]), %[speed], %[time]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M%[extra]] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M%[extra]] %[message]");
	setDefault(LOG_FORMAT_STATUS, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_SYSTEM, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FILE_MAIN_CHAT, "%B - %Y\\%[hubURL].log");
	setDefault(LOG_FILE_STATUS, "%B - %Y\\%[hubURL]_status.log");
	setDefault(LOG_FILE_PRIVATE_CHAT, "PM\\%B - %Y\\%[userNI].log");
	setDefault(LOG_FILE_UPLOAD, "Uploads.log");
	setDefault(LOG_FILE_DOWNLOAD, "Downloads.log");
	setDefault(LOG_FILE_SYSTEM, "system.log");
	setDefault(LOG_FILE_WEBSERVER, "Webserver.log");
	setDefault(GET_USER_INFO, true);
	setDefault(URL_HANDLER, true);
	setDefault(AUTO_AWAY, false);
	setDefault(BIND_ADDRESS, "0.0.0.0");
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, 1);
	setDefault(CONFIG_VERSION, "0.181");		// 0.181 is the last version missing configversion
	setDefault(KEEP_LISTS, false);
	setDefault(AUTO_KICK, false);
	setDefault(QUEUEFRAME_SHOW_TREE, true);
	setDefault(COMPRESS_TRANSFERS, true);
	setDefault(SHOW_PROGRESS_BARS, true);
	setDefault(DEFAULT_AWAY_MESSAGE, "I'm away. State your business and I might answer later if you're lucky.");
	setDefault(TIME_STAMPS_FORMAT, "%H:%M:%S");
	setDefault(MAX_TAB_ROWS, 4);
	setDefault(MAX_COMPRESSION, 5);
	setDefault(ANTI_FRAG, true);
	setDefault(NO_AWAYMSG_TO_BOTS, true);
	setDefault(SKIP_ZERO_BYTE, false);
	setDefault(ADLS_BREAK_ON_FIRST, false);
	setDefault(HUB_USER_COMMANDS, true);
	setDefault(AUTO_SEARCH_AUTO_MATCH, false);
	setDefault(LOG_FILELIST_TRANSFERS, false);
	setDefault(LOG_SYSTEM, false);
	setDefault(SEND_UNKNOWN_COMMANDS, false);
	setDefault(MAX_HASH_SPEED, 0);
//[-]PPA	setDefault(GET_USER_COUNTRY, true); // [*]PPA
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_TRANSFERVIEW, true);
	setDefault(SHOW_STATUSBAR, true);
	setDefault(SHOW_TOOLBAR, true);
	setDefault(POPUNDER_PM, false);
	setDefault(POPUNDER_FILELIST, false);
	setDefault(MAGNET_REGISTER, true);
	setDefault(MAGNET_ASK, false);
	setDefault(MAGNET_ACTION, MAGNET_AUTO_SEARCH);
	setDefault(ADD_FINISHED_INSTANTLY, true); //[*]PPA = true
	setDefault(DONT_DL_ALREADY_SHARED, false);
	setDefault(CONFIRM_HUB_REMOVAL, true);
	setDefault(USE_CTRL_FOR_LINE_HISTORY, true);
	setDefault(JOIN_OPEN_NEW_WINDOW, false);
	setDefault(SHOW_LAST_LINES_LOG, 10);
	setDefault(CONFIRM_DELETE, true);
	setDefault(ADVANCED_RESUME, true);
	setDefault(ADC_DEBUG, false);
	setDefault(UNUSED_TOGGLE_ACTIVE_WINDOW, false);
	setDefault(SEARCH_HISTORY, 10);
	setDefault(SET_MINISLOT_SIZE, 1024); //[+]PPA
	setDefault(PRIO_HIGHEST_SIZE, 64);
	setDefault(PRIO_HIGH_SIZE, 0);
	setDefault(PRIO_NORMAL_SIZE, 0);
	setDefault(PRIO_LOW_SIZE, 0);
	setDefault(PRIO_LOWEST, false);
	setDefault(OPEN_PUBLIC, false);
	setDefault(OPEN_FAVORITE_HUBS, false);
	setDefault(OPEN_FAVORITE_USERS, false);
	setDefault(OPEN_QUEUE, false);
	setDefault(OPEN_FINISHED_DOWNLOADS, false);
	setDefault(OPEN_FINISHED_UPLOADS, false);
	setDefault(OPEN_SEARCH_SPY, false);
	setDefault(OPEN_NETWORK_STATISTICS, false);
	setDefault(OPEN_NOTEPAD, false);
	setDefault(OPEN_ADVICE, true);
	setDefault(OPEN_SUBSCRIPTIONS, true);
	setDefault(OPEN_WAITING_USERS, false);
	setDefault(NO_IP_OVERRIDE, true);  //[+] PPA
	setDefault(SOCKET_IN_BUFFER, 64*1024);
	setDefault(SOCKET_OUT_BUFFER, 64*1024);
#ifdef PPA_INCLUDE_SSL
	setDefault(TLS_TRUSTED_CERTIFICATES_PATH, Util::getConfigPath() + "Certificates" PATH_SEPARATOR_STR);
	setDefault(TLS_PRIVATE_KEY_FILE, Util::getConfigPath() + "Certificates" PATH_SEPARATOR_STR "client.key");
	setDefault(TLS_CERTIFICATE_FILE, Util::getConfigPath() + "Certificates" PATH_SEPARATOR_STR "client.crt");
#endif
	setDefault(BOLD_FINISHED_DOWNLOADS, true);
	setDefault(BOLD_FINISHED_UPLOADS, true);
	setDefault(BOLD_QUEUE, true);
	setDefault(BOLD_HUB, true);
	setDefault(BOLD_PM, true);
	setDefault(BOLD_SEARCH, true);
	setDefault(BOLD_WAITING_USERS, true);
	setDefault(AUTO_REFRESH_TIME, 60);
	setDefault(USE_TLS, false); //[+]PPA
	setDefault(AUTO_SEARCH_LIMIT, 15);
	setDefault(AUTO_KICK_NO_FAVS, false);
	setDefault(PROMPT_PASSWORD, true);
	setDefault(SPY_FRAME_IGNORE_TTH_SEARCHES, true);
	setDefault(ALLOW_UNTRUSTED_HUBS, true);
#ifdef PPA_INCLUDE_SSL
	setDefault(ALLOW_UNTRUSTED_CLIENTS, true);		
#endif
	setDefault(FAST_HASH, true);
	setDefault(SORT_FAVUSERS_FIRST, false);
	setDefault(SHOW_SHELL_MENU, false);
	setDefault(NUMBER_OF_SEGMENTS, 200); //[!]PPA
	setDefault(SEGMENTS_MANUAL, true); //[!]PPA
	setDefault(HUB_SLOTS, 0);
    setDefault(TEXT_FONT, "Tahoma,8,400,0"); // !SMT!-F
	setDefault(DROP_MULTISOURCE_ONLY, true);
	setDefault(DEBUG_COMMANDS, false);
	setDefault(UPDATE_URL, "http://www.flylinkdc.ru/"); 
	setDefault(EXTRA_SLOTS, 10); //[+]PPA
	setDefault(SHUTDOWN_TIMEOUT, 150);
	setDefault(SEARCH_PASSIVE,false);  //[+] PPA
	setDefault(MAX_UPLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_NORMAL, 0);
	setDefault(MAX_UPLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(MAX_DOWNLOAD_SPEED_LIMIT_TIME, 0);
	setDefault(TOOLBAR, "0,-1,1,2,-1,3,4,5,-1,6,7,8,9,-1,10,11,12,13,-1,14,15,16,17,-1,18,19,20,21");
	setDefault(SEARCH_ALTERNATE_COLOUR, RGB(255,200,0));
	setDefault(WEBSERVER, false);
	setDefault(WEBSERVER_PORT, (int)Util::rand(80, 1024));
	setDefault(WEBSERVER_FORMAT,"%Y-%m-%d %H:%M: %[ip] tried getting %[file]");
	setDefault(LOG_WEBSERVER, false);
	setDefault(WEBSERVER_USER, "flylinkdc");
	setDefault(WEBSERVER_PASS, "flylinkdc");
	setDefault(AUTO_PRIORITY_DEFAULT, true);
	setDefault(TOOLBARIMAGE,"");
	setDefault(TOOLBARHOTIMAGE,"");
	setDefault(TIME_DEPENDENT_THROTTLE, false);
	setDefault(BANDWIDTH_LIMIT_START, 0);
	setDefault(BANDWIDTH_LIMIT_END, 0);
	setDefault(REMOVE_FORBIDDEN, true);
	setDefault(THROTTLE_ENABLE, false);
	setDefault(EXTRA_DOWNLOAD_SLOTS, 3);

	setDefault(BACKGROUND_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_COLOR, RGB(67, 98, 154));

	setDefault(TEXT_GENERAL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_GENERAL_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_GENERAL_BOLD, false);
	setDefault(TEXT_GENERAL_ITALIC, false);

	setDefault(TEXT_MYOWN_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_MYOWN_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_MYOWN_BOLD, false);
	setDefault(TEXT_MYOWN_ITALIC, false);

	setDefault(TEXT_PRIVATE_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_PRIVATE_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_PRIVATE_BOLD, false);
	setDefault(TEXT_PRIVATE_ITALIC, true);

	setDefault(TEXT_SYSTEM_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SYSTEM_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_SYSTEM_BOLD, true);
	setDefault(TEXT_SYSTEM_ITALIC, false);

	setDefault(TEXT_SERVER_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_SERVER_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_SERVER_BOLD, true);
	setDefault(TEXT_SERVER_ITALIC, false);

	setDefault(TEXT_TIMESTAMP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_TIMESTAMP_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_TIMESTAMP_BOLD, false);
	setDefault(TEXT_TIMESTAMP_ITALIC, false);

	setDefault(TEXT_MYNICK_BACK_COLOR, RGB(255, 0, 0));
	setDefault(TEXT_MYNICK_FORE_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_MYNICK_BOLD, true);
	setDefault(TEXT_MYNICK_ITALIC, false);

	setDefault(TEXT_FAV_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_FAV_FORE_COLOR, RGB(67, 98, 154));
	setDefault(TEXT_FAV_BOLD, true);
	setDefault(TEXT_FAV_ITALIC, false);

	setDefault(TEXT_OP_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_OP_FORE_COLOR, RGB(0, 128, 64));
	setDefault(TEXT_OP_BOLD, true);
	setDefault(TEXT_OP_ITALIC, false);

	setDefault(TEXT_URL_BACK_COLOR, RGB(255, 255, 255));
	setDefault(TEXT_URL_FORE_COLOR, RGB(0,0,255));
	setDefault(TEXT_URL_BOLD, false);
	setDefault(TEXT_URL_ITALIC, false);

	setDefault(BOLD_AUTHOR_MESS, true);
	setDefault(KICK_MSG_RECENT_01, "");
	setDefault(KICK_MSG_RECENT_02, "");
	setDefault(KICK_MSG_RECENT_03, "");
	setDefault(KICK_MSG_RECENT_04, "");
	setDefault(KICK_MSG_RECENT_05, "");
	setDefault(KICK_MSG_RECENT_06, "");
	setDefault(KICK_MSG_RECENT_07, "");
	setDefault(KICK_MSG_RECENT_08, "");
	setDefault(KICK_MSG_RECENT_09, "");
	setDefault(KICK_MSG_RECENT_10, "");
	setDefault(KICK_MSG_RECENT_11, "");
	setDefault(KICK_MSG_RECENT_12, "");
	setDefault(KICK_MSG_RECENT_13, "");
	setDefault(KICK_MSG_RECENT_14, "");
	setDefault(KICK_MSG_RECENT_15, "");
	setDefault(KICK_MSG_RECENT_16, "");
	setDefault(KICK_MSG_RECENT_17, "");
	setDefault(KICK_MSG_RECENT_18, "");
	setDefault(KICK_MSG_RECENT_19, "");
	setDefault(KICK_MSG_RECENT_20, "");
	setDefault(WINAMP_FORMAT, "+me playing: %[title] (%[bitrate])");
	setDefault(PROGRESS_TEXT_COLOR_DOWN, RGB(0, 51, 0));
	setDefault(PROGRESS_TEXT_COLOR_UP, RGB(98, 0, 49));
	setDefault(SHOW_INFOTIPS, true);
	setDefault(MINIMIZE_ON_STARTUP, false);
	setDefault(FREE_SLOTS_DEFAULT, false);
	setDefault(USE_EXTENSION_DOWNTO, true);
	setDefault(ERROR_COLOR, RGB(255, 0, 0));
	setDefault(EXPAND_QUEUE, true);
	setDefault(TRANSFER_SPLIT_SIZE, 8000);
	setDefault(I_DOWN_SPEED, 5);
	setDefault(H_DOWN_SPEED, 10);
	setDefault(DOWN_TIME, 20);
	setDefault(DISCONNECTING_ENABLE, true);
	setDefault(MIN_FILE_SIZE, 10);
    setDefault(DISCONNECT, 2);
    setDefault(FILE_SLOTS, 0); //[+]PPA
	setDefault(MENUBAR_TWO_COLORS, true);
	setDefault(MENUBAR_LEFT_COLOR, RGB(0, 0, 128));
	setDefault(MENUBAR_RIGHT_COLOR, RGB(0, 64, 128));
	setDefault(MENUBAR_BUMPED, true);

	setDefault(PERCENT_FAKE_SHARE_TOLERATED, 20);
//[-]PPA	setDefault(CZCHARS_DISABLE, false);
	setDefault(REPORT_ALTERNATES, true);	

	setDefault(SHOW_DESCRIPTION_SPEED, false);
	setDefault(SOUNDS_DISABLED, false);
	setDefault(CHECK_NEW_USERS, false);
	setDefault(GARBAGE_COMMAND_INCOMING, false);
	setDefault(GARBAGE_COMMAND_OUTGOING, false);
	setDefault(UPLOADQUEUEFRAME_SHOW_TREE, true);	
	setDefault(DONT_BEGIN_SEGMENT, false); //[!] PPA
	setDefault(DONT_BEGIN_SEGMENT_SPEED, 1024);

	setDefault(DISCONNECT_RAW, 0);
	setDefault(TIMEOUT_RAW, 0);
	setDefault(FAKESHARE_RAW, 0);
	setDefault(LISTLEN_MISMATCH, 0);
	setDefault(FILELIST_TOO_SMALL, 0);
	setDefault(FILELIST_UNAVAILABLE, 0);
	setDefault(USE_VERTICAL_VIEW, true);
	setDefault(DISPLAY_CHEATS_IN_MAIN_CHAT, true);	
	setDefault(SEARCH_TIME, 5); //[.] PPA
	setDefault(REALTIME_QUEUE_UPDATE, true);
	setDefault(SUPPRESS_MAIN_CHAT, false);
	
	// default sounds
	setDefault(BEGINFILE, Util::emptyString);
	setDefault(BEEPFILE, Util::emptyString);
	setDefault(FINISHFILE, Util::emptyString);
	setDefault(SOURCEFILE, Util::emptyString);
	setDefault(UPLOADFILE, Util::emptyString);
	setDefault(FAKERFILE, Util::emptyString);
	setDefault(CHATNAMEFILE, Util::emptyString);
	setDefault(SOUND_TTH, Util::emptyString);
	setDefault(SOUND_EXC, Util::emptyString);
	setDefault(SOUND_HUBCON, Util::emptyString);
	setDefault(SOUND_HUBDISCON, Util::emptyString);
	setDefault(SOUND_FAVUSER, Util::emptyString);
	setDefault(SOUND_FAVUSER_OFFLINE, Util::emptyString); // !SMT!-UI
	setDefault(SOUND_TYPING_NOTIFY, Util::emptyString);

	setDefault(POPUP_HUB_CONNECTED, false);
	setDefault(POPUP_HUB_DISCONNECTED, false);
	setDefault(POPUP_FAVORITE_CONNECTED, true);
	setDefault(POPUP_FAVORITE_DISCONNECTED, true); // !SMT!-UI
	setDefault(POPUP_CHEATING_USER, true);
	setDefault(POPUP_DOWNLOAD_START, true);
	setDefault(POPUP_DOWNLOAD_FAILED, false);
	setDefault(POPUP_DOWNLOAD_FINISHED, true);
	setDefault(POPUP_UPLOAD_FINISHED, false);
	setDefault(POPUP_PM, false);
	setDefault(POPUP_NEW_PM, true);
	setDefault(POPUP_TYPE, 0);
	setDefault(POPUP_AWAY, false);
	setDefault(POPUP_MINIMIZED, true);

	setDefault(AWAY, false);
	setDefault(SHUTDOWN_ACTION, 0);
	setDefault(MINIMUM_SEARCH_INTERVAL, 20);
	setDefault(PROGRESSBAR_ODC_STYLE, true);

	setDefault(PROGRESS_3DDEPTH, 4);
	setDefault(PROGRESS_OVERRIDE_COLORS, true);
	setDefault(PROGRESS_OVERRIDE_COLORS2, true);
	setDefault(MAX_AUTO_MATCH_SOURCES, 5);
	setDefault(MULTI_CHUNK, true);
	setDefault(USERLIST_DBLCLICK, 0);
	setDefault(USERLIST_CHAT_DBLCLICK, 1);
	setDefault(TRANSFERLIST_DBLCLICK, 0);
	setDefault(CHAT_DBLCLICK, 0);	
	setDefault(NORMAL_COLOUR, RGB(67, 98, 154));
	setDefault(RESERVED_SLOT_COLOR, RGB(255,0,128));
	setDefault(IGNORED_COLOR, RGB(192, 192, 192));	
	setDefault(FAVORITE_COLOR, RGB(67, 98, 154));	
	setDefault(FIREBALL_COLOR, RGB(255,0,0));
 	setDefault(SERVER_COLOR, RGB(128,128,255));
	setDefault(PASIVE_COLOR, RGB(67, 98, 154));
	setDefault(OP_COLOR, RGB(0, 128, 64));
	setDefault(CLIENT_CHECKED_COLOUR, RGB(160, 160, 160));
	setDefault(FILELIST_CHECKED_COLOUR, RGB(0, 160, 255));
	setDefault(FULL_CHECKED_COLOUR, RGB(0, 160, 0));
	setDefault(BAD_CLIENT_COLOUR, RGB(204,0,0));
	setDefault(BAD_FILELIST_COLOUR, RGB(204,0,204));	
	setDefault(HUBFRAME_VISIBLE, "1,1,0,1,0,1,0,0,0");
	setDefault(DIRECTORYLISTINGFRAME_VISIBLE, "1,1,0,1,1");	
	setDefault(FINISHED_VISIBLE, "1,1,1,1,1,1,1,1");
	setDefault(FINISHED_UL_VISIBLE, "1,1,1,1,1,1,1");
	setDefault(ACCEPTED_DISCONNECTS, 5);
	setDefault(ACCEPTED_TIMEOUTS, 10);
    setDefault(EMOTICONS_FILE, "FlylinkSmiles");
	setDefault(GROUP_SEARCH_RESULTS, true);
	setDefault(BWSETTING_MODE, BWSETTINGS_DEFAULT);
	setDefault(DOWNCONN_PER_SEC, 2);
	// ApexDC++
	setDefault(SHOW_DESCRIPTION_SLOTS, true);
	setDefault(SHOW_DESCRIPTION_LIMIT, true);
	setDefault(PROTECT_TRAY, false);
	setDefault(PROTECT_START, false);
	setDefault(PROTECT_CLOSE, false);
	setDefault(STRIP_TOPIC, false);
	setDefault(SKIPLIST_SHARE, "");
	setDefault(TB_IMAGE_SIZE, 20);
	setDefault(TB_IMAGE_SIZE_HOT, 20);
	setDefault(SHOW_WINAMP_CONTROL, false);
	setDefault(USER_THERSHOLD, 1000);
	setDefault(PM_PREVIEW, true);
	setDefault(FILTER_ENTER, false);
	setDefault(HIGH_PRIO_FILES, "*.sfv;*.nfo;*sample*;*cover*;*.pls;*.m3u");
	setDefault(LOW_PRIO_FILES, "");
	setDefault(POPUP_TIME, 5);
	setDefault(AWAY_START, 0);
	setDefault(AWAY_END, 0);
	setDefault(AWAY_TIME_THROTTLE, false);
	setDefault(SECONDARY_AWAY_MESSAGE, "");
	setDefault(PROGRESSBAR_ODC_BUMPED, true);
	setDefault(TOP_SPEED, 100);
	setDefault(TOP_UP_SPEED, 50);
	setDefault(STEALTHY_STYLE, true);
	setDefault(PSR_DELAY, 30);
	setDefault(IP_IN_CHAT, false);
	setDefault(COUNTRY_IN_CHAT, false);
	setDefault(BROADCAST, 0);
	setDefault(SETTINGS_STATE, false);
	setDefault(PAGE, 0);
	setDefault(CHATBUFFERSIZE, 25000);
	setDefault(ENABLE_HUBTOPIC, false);
	setDefault(HUB_SMALL, false);
	setDefault(FORMAT_BIU, false);
	setDefault(PROT_USERS, "");
	setDefault(MEDIA_PLAYER, 0);
	setDefault(WMP_FORMAT, "+me playing: %[title] at %[bitrate] <Windows Media Player %[version]>");
	setDefault(ITUNES_FORMAT, "+me playing: %[title] at %[bitrate] <iTunes %[version]>");
	setDefault(MPLAYERC_FORMAT, "+me playing: %[title] <Media Player Classic>");
	setDefault(IPUPDATE, true); //[+] PPA
	setDefault(PROT_FAVS, false);
	setDefault(RAW1_TEXT, "Raw 1");
	setDefault(RAW2_TEXT, "Raw 2");
	setDefault(RAW3_TEXT, "Raw 3");
	setDefault(RAW4_TEXT, "Raw 4");
	setDefault(RAW5_TEXT, "Raw 5");
	setDefault(MAX_MSG_LENGTH, 120);
	setDefault(POPUP_FONT, "Arial,-11,400,0"); // !SMT!-F
	setDefault(POPUP_TITLE_FONT, "Arial,-11,400,0"); // !SMT!-F
	setDefault(POPUP_BACKCOLOR, RGB(58, 122, 180));
	setDefault(POPUP_TEXTCOLOR, RGB(255, 255, 255));
	setDefault(POPUP_TITLE_TEXTCOLOR, RGB(255, 255, 255));
	setDefault(OPEN_LOGS_INTERNAL, true); //[+]PPA
//[-]PPA	setDefault(NO_EMOTES_LINKS, true); //[+]PPA
	// ApexDC++
#ifdef PPA_INCLUDE_DNS
    setDefault(NSLOOKUP_MODE, Socket::DNSCache::NSLOOKUP_DELAYED); // !SMT!-IP
    setDefault(NSLOOKUP_DELAY, 100); // !SMT!-IP
#endif
        // !SMT!-S
        setDefault(BAN_SLOTS, 0);
        setDefault(BAN_SHARE, 0);
        setDefault(BAN_LIMIT, 0);
        setDefault(BAN_MESSAGE, "You've banned. Share more files or open more slots");
        setDefault(SLOT_ASK, STRING(ASK_SLOT_TEMPLATE));
        setDefault(BANMSG_PERIOD, 60);
        setDefault(BAN_STEALTH, 0);
        setDefault(BAN_FORCE_PM, 0);
        setDefault(BAN_SKIP_OPS, 1);
        setDefault(EXTRASLOT_TO_DL, 0);

        // !SMT!-UI
        setDefault(BAN_COLOR, 0xF49AB3);
        setDefault(DUPE_COLOR, 0x73F7E6);
        setDefault(MULTILINE_CHAT_INPUT, 0);
        setDefault(EXTERNAL_PREVIEW, 0);
        setDefault(SEND_SLOTGRANT_MSG, 0);
        setDefault(FAVUSERLIST_DBLCLICK, 0);

        // !SMT!-PSW
        setDefault(PROTECT_PRIVATE, 0);
        setDefault(PM_PASSWORD, STRING(DEF_PASSWORD));
        setDefault(PM_PASSWORD_HINT, STRING(DEF_PASSWORD_HINT));

		//[+]necros
		setDefault(COPY_WMLINK, "<a href=\"%[magnet]\" title=\"%[name]\" target=\"_blank\">%[name] (%[size])</a>");
		//[+] M.S.A

	setDefault(UP_TRANSFER_COLORS, 1); // By Drakon
	setDefault(STARTUP_BACKUP,0); // by Drakon
	setDefault(GLOBAL_HUBFRAME_CONF, false);
	setDefault(USE_ANTIVIR, false);
//	setDefault(ANTIVIR_PATH, "");
//	setDefault(ANTIVIR_PARAMS, "%[file]");
	setDefault(IGNORE_USE_REGEXP_OR_WC, false);
	setDefault(STEALTHY_INDICATE_SPEEDS, false);
	setDefault(OLD_TRAY_BEHAV, false);
	setDefault(OLD_NAMING_STYLE, false);
	setDefault(OLD_ICONS_MODE, 0);
	setDefault(COLOUR_DUPE, RGB(0, 174, 87));
	setDefault(NO_TTH_CHEAT, false);
	setDefault(OLD_CLIENT_RAW, 0);
	setDefault(DELETE_CHECKED, false);
	setDefault(TOPMOST, false);
	setDefault(LOCK_TOOLBARS, false);
	setDefault(AUTO_COMPLETE_SEARCH, true);
	setDefault(KEEP_DL_HISTORY, false);
	setDefault(KEEP_UL_HISTORY, false);
	setDefault(UNUSED_SHOW_QUICK_SEARCH, false);
	setDefault(SEARCH_DETECT_TTH, true);
	setDefault(FULL_FILELIST_NFO, false);
	setDefault(NON_HUBS_FRONT, false);
	setDefault(BLEND_OFFLINE_SEARCH, true);
	setDefault(MAX_RESIZE_LINES, 1);
//	setDefault(PG_LOG_FILE, "PeerGuardian.log");
//	setDefault(PG_LOG_FORMAT, "Type: %[type] From: %[userI4] (Nick: %[userNI] Hub: %[hubURL]) Because: %[company]");
//	setDefault(PG_FILE, Util::getConfigPath() + "guarding.p2p");
//	setDefault(PG_UPDATE_URL, "http://www.apexdc.net/updater/");
	setDefault(USE_CUSTOM_LIST_BACKGROUND, true);
	setDefault(SEARCHFRAME_VISIBLE, "1,1,1,1,1,1,1,0,0,0,0,0,0");
	setDefault(SEARCHFRAME_ORDER, "1,6,0,4,3,2,5,7,8,9,10,11,12");
	setDefault(SEARCHFRAME_WIDTHS, "210,346,100,153,80,100,211,150,80,105,100,100,150");

	setDefault(QUEUEFRAME_VISIBLE, "1,0,0,1,1,1,1,0,1,0,0,0,0,0");
	setDefault(QUEUEFRAME_ORDER, "0,6,7,1,2,3,4,8,5,9,10,11,12,13");
#ifdef _WIN32
	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);
	setDefault(MDI_MAXIMIZED, true);
	setDefault(UPLOAD_BAR_COLOR, RGB(205, 60, 55));
	setDefault(DOWNLOAD_BAR_COLOR, RGB(55, 170, 85));
	setDefault(PROGRESS_BACK_COLOR, RGB(95, 95, 95));
	setDefault(PROGRESS_COMPRESS_COLOR, RGB(222, 160, 0));
	setDefault(PROGRESS_SEGMENT_COLOR, RGB(49, 106, 197));
	setDefault(COLOR_RUNNING, RGB(0, 0, 100));
	setDefault(COLOR_DOWNLOADED, RGB(255, 255, 100));
	setDefault(COLOR_VERIFIED, RGB(0, 255, 0));

	setDefault(SEARCH_EXPAND_TOOLBAR_ON_ACTIVATE, true);
	setDefault(SEARCH_RESTORE_CONDITION_ON_ACTIVATE, true);
	setDefault(OPEN_HUB_CHAT_ON_CONNECT, false);
	setDefault(NICK_ADD_UNIQUE_SUFFIX, true);
	setDefault(DOWNLOAD_DIRECTORY_SHORTCUT, Util::readRegistryBoolean(settingTags[DOWNLOAD_DIRECTORY_SHORTCUT], false));
	setDefault(MINIMIZE_ON_CLOSE, true);
	setDefault(DEFAULT_DSCP_MARK, 0);
	setDefault(HUB_DSCP_MARK, 0);
	setDefault(PEER_DSCP_MARK, 0);

    //make sure the total of the following and PROGRESS_BACK_COLOR are under 255,255,255, since they are added together
	setDefault(COLOR_AVOIDING, RGB(100, 0, 0));

	OSVERSIONINFO ver;
	memzero(&ver, sizeof(OSVERSIONINFO));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&ver);

	setDefault(USE_OLD_SHARING_UI, (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) ? false : true);
#endif
//[+] WhiteD. Custom ratio message
	setDefault(RATIO_TEMPLATE, "/me ratio: %[ratio] (Uploaded: %[up] | Downloaded: %[down])");
// End of addition.
}

void SettingsManager::load(string const& aFileName, bool main)
{
	try {
		SimpleXML xml;
		xml.fromXML(File(aFileName, File::READ, File::OPEN).read());
		xml.resetCurrentChild();
		xml.stepIn();
		if(xml.findChild("Settings")) {
			xml.stepIn();
			for(int i=STR_FIRST; i<STR_LAST; i++) {
				const string& attr = settingTags[i];
				dcassert(attr.find(SENTRY) == string::npos);
				
				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(int i=INT_FIRST+1; i<INT_LAST; i++) {
				const string& attr = settingTags[i];
				dcassert(attr.find(SENTRY) == string::npos);
				
				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(int i=INT64_FIRST; i<INT64_LAST; i++) {
				const string& attr = settingTags[i];
				dcassert(attr.find(SENTRY) == string::npos);
				
				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}			
			xml.stepOut();
		}

		if (main) {
			double v = Util::toDouble(SETTING(CONFIG_VERSION));
			// if(v < 0.x) { // Fix old settings here }

			if(v <= 0.674 || SETTING(PRIVATE_ID).length() != 39 || CID(SETTING(PRIVATE_ID)).isZero()) {
				set(PRIVATE_ID, CID::generate().toBase32());

				// Formats changed, might as well remove these...
				set(LOG_FORMAT_POST_DOWNLOAD, Util::emptyString);
				set(LOG_FORMAT_POST_UPLOAD, Util::emptyString);
				set(LOG_FORMAT_MAIN_CHAT, Util::emptyString);
				set(LOG_FORMAT_PRIVATE_CHAT, Util::emptyString);
				set(LOG_FORMAT_STATUS, Util::emptyString);
				set(LOG_FORMAT_SYSTEM, Util::emptyString);
				set(LOG_FILE_MAIN_CHAT, Util::emptyString);
				set(LOG_FILE_STATUS, Util::emptyString);
				set(LOG_FILE_PRIVATE_CHAT, Util::emptyString);
				set(LOG_FILE_UPLOAD, Util::emptyString);
				set(LOG_FILE_DOWNLOAD, Util::emptyString);
				set(LOG_FILE_SYSTEM, Util::emptyString);
			}

			if(v <= 16) {
				set(HUBLIST_SERVERS, Util::emptyString);
			}
#ifdef _DEBUG
			set(PRIVATE_ID, CID::generate().toBase32());
#endif
			setDefault(UDP_PORT, SETTING(TCP_PORT));

#ifdef PPA_INCLUDE_SSL
			File::ensureDirectory(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));
#endif

			fire(SettingsManagerListener::Load(), xml);
		}

		xml.stepOut();

	} catch(const Exception&) {
		if (main) {
			if(CID(SETTING(PRIVATE_ID)).isZero())
				set(PRIVATE_ID, CID::generate().toBase32());
		}
	}

	if (main) {
		if(SETTING(TCP_PORT) == 0) {
			set(TCP_PORT, (int)Util::rand(1025, 8000));
		}
		if(SETTING(UDP_PORT) == 0) {
			set(UDP_PORT, (int)Util::rand(1025, 8000));
		}
		if(SETTING(TLS_PORT) == 0) {
			set(TLS_PORT, (int)Util::rand(1025, 32000));
		}
	}

}

void SettingsManager::save(string const& aFileName) {

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	int i;
	string type("type"), curType("string");
	
	for(i=STR_FIRST; i<STR_LAST; i++)
	{
		if (i == INCOMING_CONNECTIONS) continue;
		if(i == CONFIG_VERSION) {
			xml.addTag(settingTags[i], m_version);
			xml.addChildAttrib(type, curType);
		} else if(isSet[i]) {
			xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}

	curType = "int";
	for(i=INT_FIRST; i<INT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	curType = "int64";
	for(i=INT64_FIRST; i<INT64_LAST; i++)
	{
		if(isSet[i])
		{
			xml.addTag(settingTags[i], get(Int64Setting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	
	fire(SettingsManagerListener::Save(), xml);

	try {
		File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(const FileException&) {
		// ...
	}
}

/**
 * @file
 * $Id: SettingsManager.cpp,v 1.21.2.9 2008/12/10 19:53:02 alexey Exp $
 */
