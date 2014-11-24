// HARDCODED STOPS

/*var stops = [
	[0, 401935, 306, 'Geel Station Perron 5'],
	[1, 306086, 306, 'Leuven Station Perron 11'],
	[2, 104718 , 3, 'Antwerpen Premetrostation Astrid']
];*/



// MESSAGE AND ERROR TYPES

var value_type_stop = 0;
var value_type_bus = 1;
var value_type_error = 2;
	
var error_empty = 0;
var error_connection = 1;
var error_unknown = 2;

var stops = [];



// EVENT LISTENERS

Pebble.addEventListener('ready', function(e){
	console.log("PebbleKit JS ready!");
	
	// Load stops from storage
	var i = 0;
	while(localStorage.getItem('stop' + i + '_stopId') !== null){
		stops.push([
			i,
			parseInt(localStorage.getItem('stop' + i + '_stopId')),
			parseInt(localStorage.getItem('stop' + i + '_lijnId')),
			localStorage.getItem('stop' + i + '_name')
		]);
		++i;
	}
	console.log("Loaded " + i + " stops from localStorage");
	
	// Send stops to watch
	sendStops();
});

Pebble.addEventListener('showConfiguration', function(e){
	console.log("Opened configuration screen on phone");
	Pebble.openURL('http://nexworx.com/pebble/delijn/configure.html?' + encodeURIComponent(JSON.stringify(stops)));
});

Pebble.addEventListener('webviewclosed', function(e){
	var stops_new = JSON.parse(decodeURIComponent(e.response));
	if(stops_new.length !== undefined){
		stops = stops_new;
		console.log("User is saving " + stops.length + " stops.");
		
		// Clear old items
		var i = 0;
		while(i < localStorage.length/3){
			localStorage.removeItem('stop' + i + '_stopId');
			localStorage.removeItem('stop' + i + '_lijnId');
			localStorage.removeItem('stop' + i + '_name');
			++i;
		}
		
		// Save new items
		i = 0;
		while(i < stops.length){
			localStorage.setItem('stop' + i + '_stopId', stops[i][1]);
			localStorage.setItem('stop' + i + '_lijnId', stops[i][2]);
			localStorage.setItem('stop' + i + '_name', toTitleCase(stops[i][3]));
			++i;
		}
	} else{
		console.log("Configuration screen cancelled");
	}
});

Pebble.addEventListener('appmessage', function(e){
	console.log("AppMessage received!");
	switch(e.payload.KEY_TYPE){
		case value_type_stop: // Get schedule for a certain stop
			console.log("Type: stop -> Get schedule");
			var stop = stops[e.payload.KEY_STOP_ID];
			if(stop !== null){
				sendSchedule(stop);	
			} else{
				console.log("No such stop");
				sendError(error_unknown);
			}
			break;
		default:
			console.log("Type invalid: " + e.payload.KEY_TYPE);
			break;
	}
});



// FUNCTIONS

var xhrRequest = function (url, type, callback){
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
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

function sendSchedule(stop){
	// Send name of stop
	sendStop(stop[0], false);
	
	// Call Kimono API
	console.log('Initializing AJAX connection for line ' + stop[2] + ' at stop ' + stop[1]);
	var url = 'https://www.kimonolabs.com/api/be9t80fy?apikey=WGySCl9CSlXC0Vj0lq5LeuOQ2iEhwN5A&kimpath3=' + stop[1] + '&kimpath4=' + stop[2];
	xhrRequest(url, 'GET', function(responseText){
		var json = JSON.parse(responseText);
		if(json.lastrunstatus === 'success'){
			// Kimono data received, parse and send data
			if(json.results.buses !== undefined){
				console.log('Kimono returned ' + json.results.buses.length + ' upcoming buses.');
				sendBus(json, 0, true);
			} else{
				sendError(error_empty);
			}
		} else{
			// Kimono could not fetch data, show error message
			console.log('Kimono was unable to fetch the data.');
			sendError(error_connection);
		}	
	});
}

function sendStop(i, next){
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
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (saved stop), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
	});
}

function sendBus(json, i, next){
	// Parse JSON
	var lijnId = json.results.buses[i].lijnId;
	var destination = toTitleCase(json.results.buses[i].destination);
	var eta = json.results.buses[i].eta;
	
	if(eta === ''){
		eta = "Arriveert nu!";
	}
	
	if(eta.charAt(2) !== ':' && eta.length !== 0){
		eta += ' (real-time)';
	}
	
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
		if(i < json.results.buses.length-1 && next){
			sendBus(json, ++i, true);
		}
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (bus), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
	});
}

function sendError(errorCode){
	var dictionary = {
		"KEY_TYPE": value_type_error,
		"KEY_ERROR": errorCode
	};
	Pebble.sendAppMessage(dictionary, function(e){
		// Pebble ACK
		console.log("Pebble ACK (stop), transactionId=" + e.data.transactionId);
	}, function(e){
		// Pebble NACK
		console.log("Pebble NACK (stop), transactionId=" + e.data.transactionId + ", error: " + e.error.message);
	});
}

function toTitleCase(str){
    return str.replace(/\w\S*/g, function(txt){return txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase();});
}