// MESSAGE AND ERROR TYPES

var value_type_stop = 0;
var value_type_bus = 1;
var value_type_error = 2;
	
var error_empty = 0;
var error_connection = 1;
var error_unknown = 2;
var error_outdated = 3;
var error_remote = 4;

var stops = [];
var stops_ready = false;

var storage_version_newest = 2;



// EVENT LISTENERS

Pebble.addEventListener('ready', function(e){
	console.log("PebbleKit JS ready!");
	
	// Check storage version
	var storage_version_current = parseInt(localStorage.getItem('version'));
	if(localStorage.length === 0){
		storage_version_current = 0;
	} else if(isNaN(storage_version_current)){
		storage_version_current = 1;
	}
	
	if(storage_version_current != storage_version_newest){
		upgradeStorage(storage_version_current);
	}
	
	// Load stops from storage
	var i = 0;
	var n = parseInt(localStorage.getItem('stops'));
	while(i < n){
		stops.push([
			i,
			parseInt(localStorage.getItem('stop' + i + '_stopId')),
			parseInt(localStorage.getItem('stop' + i + '_lijnId')),
			localStorage.getItem('stop' + i + '_name')
		]);
		++i;
	}
	stops_ready = true;
	console.log("Loaded " + i + " of " + n + " stops from localStorage");
	
	// Send stops to watch
	sendStops();
});

Pebble.addEventListener('showConfiguration', function(e){
	openConfiguration();
});

Pebble.addEventListener('webviewclosed', function(e){
	stops_ready = false;
	
	var stops_new = JSON.parse(decodeURIComponent(e.response));
	if(stops_new.length !== undefined){
		stops = stops_new;
		var n = stops.length;
		console.log("User is saving " + n + " stops.");
		
		// Clear old items
		localStorage.clear();
		
		// Parse and save new items
		var i = 0;
		localStorage.setItem('stops', n);
		localStorage.setItem('version', storage_version_newest);
		while(i < n){
			// Save to localStorage
			localStorage.setItem('stop' + i + '_stopId', stops[i][1]);
			localStorage.setItem('stop' + i + '_lijnId', stops[i][2]);
			localStorage.setItem('stop' + i + '_name', stops[i][3]);
			
			// Parse integers in new stops array
			stops[i][0] = i;
			stops[i][1] = parseInt(stops[i][1]);
			stops[i][2] = parseInt(stops[i][2]);
			
			++i;
		}
		
		// Reload stops on Pebble
		sendError(error_outdated, function(){
			sendStops();
		});
	} else{
		console.log("Configuration screen cancelled");
	}
	
	stops_ready = true;
});

Pebble.addEventListener('appmessage', function(e){
	console.log("AppMessage received!");
	switch(e.payload.KEY_TYPE){
		case value_type_stop: // Get schedule for a certain stop
			console.log("Type: stop -> Get schedule");
			var stop = stops[e.payload.KEY_STOP_ID];
			if(stop !== null){
				sendStop(stop[0], false, function(){
					sendSchedule(stop, 10);
				});
			} else{
				console.log("No such stop");
				sendError(error_unknown);
			}
			break;
		
		default:
			console.log("Unknown message type");
	}
});



// FUNCTIONS

function upgradeStorage(version){
	console.log("Upgrading localStorage from version " + version + " to " + storage_version_newest);
	switch(version){
		case(0): // New users
			localStorage.setItem('stops', 0);
			localStorage.setItem('version', storage_version_newest);
			break;
		case(1): // Storage version 1
			var n = localStorage.length/3;
			localStorage.setItem('stops', n);
			localStorage.setItem('version', storage_version_newest);
			break;
		default: // Unknown storage version
			localStorage.clear();
			localStorage.setItem('stops', 0);
			localStorage.setItem('version', storage_version_newest);
			Pebble.showSimpleNotificationOnPebble("Haltes verwijderd", "Door een fout zijn je opgeslagen haltes verwijderd. Voeg ze opnieuw toe via de Pebble app.");
	}
}

function openConfiguration(){
	// Open configuration screen if the stops are loaded
	if(stops_ready){
		console.log("Opened configuration screen on phone");
		Pebble.openURL('http://nexworx.com/pebble/delijn/configure.html?' + encodeURIComponent(JSON.stringify(stops)));
	} else{
		setTimeout(function(){ openConfiguration(); },100);
	}
}

var xhrRequest = function (url, type, callback){
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.timeout = 10000;
	xhr.ontimeout = function () {
		console.log("Connection to De Lijn timed out");
		sendError(error_connection);
	};
	xhr.onerror = function() {
		console.log("De Lijn web API is down");
		sendError(error_remote);
	};
	xhr.open(type, url);
	xhr.send();
};

function sendStops(){
	if(stops.length > 0){
		sendStop(0, true);
	} else{
		sendError(error_empty);
	}
}

function sendSchedule(stop, request_quantity){
	console.log('Requesting ' + request_quantity + ' buses for stop ' + stop[1]);
	var url = 'http://www.delijn.be/rise-api-web/haltes/vertrekken/' + stop[1] + '/' + request_quantity;
	xhrRequest(url, 'GET', function(responseText){
		var json = JSON.parse(responseText);
		// De Lijn data received, parse and send data
		if(json !== null){
			console.log('De Lijn returned ' + json.lijnen.length + ' upcoming buses.');
			
			// Filter stops
			if(stop[2] != -1){
				json = filterSchedule(json, stop[2]);
			}
			
			// Send if enough stops for requested line
			if(json.lijnen.length < 4 && request_quantity < 40){
				console.log("Only fetched " + json.lijnen.length + " buses of interest. Requesting more.");
				sendSchedule(stop, request_quantity+10);
			} else if(json.lijnen.length === 0){
				sendError(error_empty);
			} else{
				console.log("Fetched " + json.lijnen.length + " buses of interest. Sending to Pebble.");
				sendBus(json, 0, true);
			}
		} else{
			sendError(error_empty);
		}
	});
}

function filterSchedule(json, filter){
	var i = json.lijnen.length;
	// Delete irrelevant buses (iterate in reverse!)
	while(i--){
		if(parseInt(json.lijnen[i].lijnNummerPubliek) != filter){
			json.lijnen.splice(i, 1);
		}
	}
	return json;
}

function sendStop(i, next, callback_ack, callback_nack){
	var dictionary = {
		"KEY_TYPE": value_type_stop,
		"KEY_STOP_ID": stops[i][0],
		"KEY_STOP_LIJNID": stops[i][2],
		"KEY_STOP_NAME": stops[i][3]
	};
	Pebble.sendAppMessage(dictionary, function(e){
		// Pebble ACK, send next stop
		console.log("Pebble ACK (saved stop), transactionId=" + e.data.transactionId);
		if(i < stops.length-1 && next){
			sendStop(++i, true);
		}
		if(callback_ack && typeof(callback_ack) === 'function'){
			callback_ack();
		}
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (saved stop), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
		if(callback_nack && typeof(callback_nack) === 'function'){
			callback_nack();
		}
	});
}

function sendBus(json, i, next, callback_ack, callback_nack){
	// Parse JSON
	var lijnId = json.lijnen[i].lijnNummerPubliek.trim();
	var destination = toTitleCase(json.lijnen[i].bestemming);
	var eta = json.lijnen[i].vertrekTijd;

	// Send bus
	var dictionary = {
		"KEY_TYPE": value_type_bus,
		"KEY_BUS_LIJNID": lijnId,
		"KEY_BUS_DESTINATION": destination,
		"KEY_BUS_ETA": eta
	};
	Pebble.sendAppMessage(dictionary, function(e){
		// Pebble ACK, send next bus
		console.log("Pebble ACK (bus), transactionId=" + e.data.transactionId);
		if(i < json.lijnen.length-1 && next){
			sendBus(json, ++i, true);
		}
		if(callback_ack && typeof(callback_ack) === 'function'){
			callback_ack();
		}
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (bus), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
		if(callback_nack && typeof(callback_nack) === 'function'){
			callback_nack();
		}
	});
}

function sendError(errorCode, callback_ack, callback_nack){
	var dictionary = {
		"KEY_TYPE": value_type_error,
		"KEY_ERROR": errorCode
	};
	Pebble.sendAppMessage(dictionary, function(e){
		// Pebble ACK
		console.log("Pebble ACK (stop), transactionId=" + e.data.transactionId);
		if(callback_ack && typeof(callback_ack) === 'function'){
			callback_ack();
		}
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (stop), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
		if(callback_nack && typeof(callback_nack) === 'function'){
			callback_nack();
		}
	});
}

function toTitleCase(str){
    return str.replace(/\w\S*/g, function(txt){return txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase();});
}