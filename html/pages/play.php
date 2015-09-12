<!-- PHP Handler -->
<?php
	if(!defined('INITIALIZED')) {
		exit;
	}
	
	$player = new Player();
	$player->loadByName(htmlspecialchars($_REQUEST['name']));
	if($player->isLoaded()) {
		$account = array(
			'sessionKey' => Website::generateSessionKey()
		);
		
		$server = array(
			'WorldID' => 1, // Keep since there are plans to add multi-worlds to TFS 1.x
			'ip' => $_SERVER['SERVER_ADDR'],
			'port' => 7172
		);
		
		$accountCharacters = "";
		$selected_character = "";
		foreach($account_logged->getPlayersList() as $character) {
			if(strtolower($player->getName()) == strtolower($character->getName())) {
				$selected_character = "<character name='" . htmlspecialchars($character->getName()) . "' worldid='" . $server['WorldID'] . "' selected='true' />";
			} else {
				$accountCharacters .= "<character name='" . htmlspecialchars($character->getName()) . "' worldid='" . $server['WorldID'] . "' selected='false' />";
			}
		}
		
		$accountCharacters .= $selected_character;
		$SQL->query("UPDATE `accounts` SET `authToken` = '" . $account['sessionKey'] . "' WHERE `name` = '" . $account_logged->getName() . "'");
	}
?>

<html>
	<head>
		<title><?php echo htmlspecialchars($config['server']['serverName']); ?> Flash Client</title>
		<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
		<meta http-equiv="content-language" content="en" />
		<link rel="shortcut icon" href="/flash-regular-bin/images/favicon.ico" type="image/x-icon" />
		<link rel="icon" href="/flash-regular-bin/images/favicon.ico" type="image/x-icon" />
		<link rel="stylesheet" href="flash-regular-bin/common/style.css" />
		
		<script type="text/javascript" src="/flash-regular-bin/js/jquery.js"></script>
		<script type="text/javascript" src="/flash-regular-bin/js/flashclienthelper.js"></script>
		<script type="text/javascript" src="/flash-regular-bin/js/swfobject.js"></script>
		<script type="text/javascript">
			window.onresize = function(e) {
				var w = e.currentTarget.innerWidth, h = e.currentTarget.innerHeight;
				if (w > 1.25 * h) {
					$('#BackgroundContainer').css('background-size', 'auto 100%');
				} else {
					$('#BackgroundContainer').css('background-size', '100% auto');
				}
			}
			
			function ShowSWFLoadingError() {
				document.getElementById("placeholder2").style.display = "block";
				document.getElementById("placeholder2").style.visibility = "visible";
				if (document.getElementById('placeholder1')) {
					document.getElementById("placeholder1").style.display = "none";
					document.getElementById("placeholder1").style.visibility = "hidden";
				}
			}
			
			function SWFStatusAction(e) {
				if (e.success == false) {
					ShowSWFLoadingError();
				}
			}
		</script>
		<style type="text/css" media="screen">#placeholder1 {visibility:hidden}</style>
	</head>
	
	<body>
		<?php
			if(!$logged) {
				echo '
					<div id="BackgroundContainer" style="background-image: url(/flash-regular-bin/images/play-background_artwork.png);">
						<div id="FlashClientBackgroundOverlay"></div>
						<div id="FlashClientErrorBox" class="FlashClientBox" style="background-image: url(/flash-regular-bin/images/flashclient_error_box.png);">
							<div class="FlashClientBoxHeadline">Error</div>
							<div id="FlashClientErrorBoxText">You are not logged into your account.<br/>Please log in at the website first.<br/><br/>(The authenticator login was not successful!)</div>
							<a href="index.php?subtopic=accountmanagement">
								<div id="FlashClientErrorBoxButton">Close</div>
							</a>
						</div>
					</div>
				';
				return;
			}
		?>
		
		<div id="BackgroundContainer" style="background-image: url(/flash-regular-bin/images/play-background_artwork.png);">
			<div id="placeholder1">
				<div id="FlashClientBackgroundOverlay"></div>
				<div id="FlashClientErrorBox" class="FlashClientBox" style="background-image: url(/flash-regular-bin/images/flashclient_error_box.png);">
					<div class="FlashClientBoxHeadline">Error</div>
					<div id="FlashClientErrorBoxText">Adobe Flash Player is missing.<br/><br/>You need the Adobe Flash Player (version 11.2.0 or higher) installed to use the Flash client.</div>
					<a href="index.php?subtopic=accountmanagement">
						<div id="FlashClientErrorBoxButton">Close</div>
					</a>
				</div>
			</div>
			
			<div id="placeholder2">
				<div id="FlashClientBackgroundOverlay"></div>
				<div id="FlashClientErrorBox" class="FlashClientBox" style="background-image: url(/flash-regular-bin/images/flashclient_error_box.png);">
					<div class="FlashClientBoxHeadline">Error</div>
					<div id="FlashClientErrorBoxText">The Flash client could not be loaded!<br />Please try again later.</div>
					<a href="index.php?subtopic=accountmanagement">
						<div id="FlashClientErrorBoxButton">Close</div>
					</a>
				</div>
			</div>
		</div>
		
		<script>
			var l_Request = false;
			if (window.XMLHttpRequest) {
				l_Request = new XMLHttpRequest;
			} else if (window.ActiveXObject) {
				l_Request = new ActiveXObject("Microsoft.XMLHttp");
			}
			
			try {
				l_Request.open("HEAD", "flash-regular-bin/preloader.swf", true);
				l_Request.onreadystatechange=function() {
					if (l_Request.readyState == 4) {
						if (l_Request.status == 200 || l_Request.status == 304) {
							swfobject.embedSWF(
								"flash-regular-bin/preloader.swf",
								"placeholder1",
								"100%",
								"100%",
								"11.2.0",
								null,
								{
									tutorial: false,
									sessionKey: "<?php echo $account['sessionKey']; ?>",
									sessionRefreshURL: "/?subtopic=refresh",
									accountData: "<accountData><world id='<?php echo $server['WorldID']; ?>' name='<?php echo htmlspecialchars($config['server']['serverName']); ?>' address='<?php echo $server['ip']; ?>' port='<?php echo $server['port']; ?>' previewstate='0' /><?php echo $accountCharacters; ?></accountData>",
									backgroundColor: 0x051122,
									backgroundImage: "/flash-regular-bin/images/play-background_artwork.png",
									closeClientURL: "index.php?subtopic=accountmanagement"
								},
								{
									allowscriptaccess: "sameDomain",
									wmode: "direct"
								},
								{
									id: "FlashClient",
									name: "FlashClient"
								},
								SWFStatusAction
							);
						} else {
							ShowSWFLoadingError();
						}
					}
				}
				l_Request.send(null);
			} catch (er) {
				ShowSWFLoadingError();
			}
		</script>
	</body>
</html>
