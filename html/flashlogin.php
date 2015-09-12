<?php

	$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
	$policyfile = "trusted.xml";
	socket_bind($socket, "127.0.0.1", 843);
	socket_listen($socket, 90);

	while($client = socket_accept($socket)){
	
		// Print client connections
		if(socket_getpeername($client, $address, $port)) {
			echo "\nClient $address has connected on port $port \n";
		}
	
		// Ensure client is requesting policy as per normal	
		$buf = socket_read($client, 2048);
		echo "Client $address has requested: $buf \n";
		
		// Send cross-domain-policy to client
		// This will need to be researched to be more secure before going live...
		echo "Sending $policyfile contents back to $address \n";
		$crossFile = file($policyfile);
		$crossFile = join('',$crossFile);
		socket_write($client, $crossFile . "\0");
		
		socket_close($client);
	}
	socket_close($socket);
?>
