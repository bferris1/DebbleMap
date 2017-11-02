var currentPosition;
var lastSentStops;
var routes;
var lastSentStops;
var systemCode;
var baseURL;
if(localStorage.getItem('systemCode')!==null){
  systemCode=localStorage.getItem('systemCode');
}else{
  systemCode = 'citybus';
}
baseURL = 'http://'+systemCode+'.doublemap.com/map/v2/';

Number.prototype.toRadians = function(){
  return this * Math.PI/180;
};

var getStops = function(callback){
  var req = new XMLHttpRequest();
  req.timeout = 5000;
  req.open('GET',baseURL+'stops', true);
  req.onload = function(e) {
  if (req.readyState == 4 && req.status == 200) {
    if(req.status == 200) {
      var response = JSON.parse(req.responseText);
      console.log('got stops');
      callback(response);
    } else { console.log('Error');
           sendError('Error getting data.');
           }
  }
};
  req.ontimeout = function(e){
    sendError('Request timed out.');
  };
  req.onerror = function(e){
    sendError('Unable to retrieve stops.');
  };
  req.send(null);
};

var getRoutes = function(callback){
  var req = new XMLHttpRequest();
  req.open('GET', baseURL+'routes', true);
  req.timeout = 5000;
  req.onload = function(e) {
  if (req.readyState == 4 && req.status == 200) {
    if(req.status == 200) {
      var response = JSON.parse(req.responseText);
      console.log('got routes');
      //console.log('response first route ' + response[0].name);
      callback(response);
    } else { console.log('Error');
           sendError('Error getting data.');
           }
  }
};
  req.ontimeout = function(e){
    sendError('Request timed out.');
  };
  req.onerror = function(e){
    sendError('Unable to retrieve routes.');
  };
  req.send(null);
};

function getETA(stopID, callback){
  var req = new XMLHttpRequest();
  req.open('GET', baseURL+'eta?stop='+stopID, true);
  req.timeout = 5000;
  req.onload = function(e) {
  if (req.readyState == 4 && req.status == 200) {
    if(req.status == 200) {
      var response = JSON.parse(req.responseText);
      console.log('got etas');
      callback(response);
    } else { console.log('Error');
           sendError('Error getting data.');
           }
  }
};
  req.ontimeout = function(e){
    sendError('Request timed out.');
  };
  req.onerror = function(e){
    sendError('Unable to retrieve ETAs.');
  };
  req.send(null);
  
}





function computeDistance(userLat, userLon, stopLat, stopLon){
var R = 6371000; // metres
var φ1 = userLat.toRadians();
var φ2 = stopLat.toRadians();
var Δφ = (stopLat-userLat).toRadians();
var Δλ = (stopLon-userLon).toRadians();

var a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
        Math.cos(φ1) * Math.cos(φ2) *
        Math.sin(Δλ/2) * Math.sin(Δλ/2);
var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));

var d = R * c;
  return d;
  
}



function sortStops(currentLocation, stops){
  return stops.sort(function(stop1,stop2){
    return stop1.distance-stop2.distance;
  });
  
}


// Function to send a message to the Pebble using AppMessage API
function sendMessage(data) {
  Pebble.sendAppMessage(data);
	
	// PRO TIP: If you are sending more than one message, or a complex set of messages, 
	// it is important that you setup an ackHandler and a nackHandler and call 
	// Pebble.sendAppMessage({ /* Message here */ }, ackHandler, nackHandler), which 
	// will designate the ackHandler and nackHandler that will be called upon the Pebble 
	// ack-ing or nack-ing the message you just sent. The specified nackHandler will 
	// also be called if your message send attempt times out.
}

//functions to handle location 
 var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 10000, 
  timeout: 10000
};

function sendError(message){
  var errorMessage = {};
  errorMessage[0]=3;
  errorMessage[1]=message;
  sendMessage(errorMessage);
}

function locationSuccess(pos) {
  //console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  currentPosition = pos;
  
  
  getStops(function(stops){
    //var fullStops = stops;
    
    //compute distance away for each stop
    for (var i = 0;i<stops.length;i++){
    stops[i].distance = computeDistance(currentPosition.coords.latitude, currentPosition.coords.longitude, stops[i].lat, stops[i].lon);
  }
    
    
    //sort stops array by distance
    var sortedStops = sortStops(currentPosition, stops);
    //console.log(JSON.stringify(sortedStops));
    //send names of nearest 10 stops or fewer
    var numStops;
    if(sortedStops.length<10){
      numStops = sortedStops.length;
    }else{
      numStops = 10;
    }
    var stopNames = {};
    stopNames[0]=0;
    stopNames[1]=numStops;
    for(i=0;i<numStops;i++){
      stopNames[i+2]=sortedStops[i].name;
    }
    lastSentStops = sortedStops.slice(0,numStops);
    sendMessage(stopNames);
  });
  
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
  //notify the user on the watch that there was an error
  sendError("Could not obtain current location.");
  
}



// Called when JS is ready
Pebble.addEventListener("ready",function(e) {
 
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);

 
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",function(e) {
  
  var busETAsToSend = {};
  
  //console.log(lastSentStops[e.payload[0]].name);
  //console.log(lastSentStops[e.payload[0]].id);
  var stopID = lastSentStops[e.payload[0]].id;
  getETA(stopID, function(data){
    //console.log(JSON.stringify(data));
    
    //make sure there is at least one eta
    //sometimes the API sends an empty array, sometimes it sends a 1-length array containing an object with an empty etas property
    //sometimes it sends an object with nested etas properties
    //todo: log these out for debugging
    if(data.length!==0&&data.etas.length!==null&&data.etas.length!==0&&data.etas[stopID].etas!==null&&data.etas[stopID].etas.length!==0){
      var stopETAs = data.etas[stopID].etas;
      console.log(stopETAs[0].route + stopETAs[0].color);
      
      getRoutes(function(routes){
          
        //truncate stopETAS if there are more than 10; the c app can currently only handle 10
        
        if (stopETAs.length >10)
          stopETAs.length = 10;
        //loop through array of etas to fetch route information
        for(var etaIndex=0;etaIndex<stopETAs.length;etaIndex++){
          //var currentRouteID = stopETAs[etaIndex].route;
          //loop through routes to find which route the eta referrs to
          var found = false;
          for (var routeIndex=0;routeIndex<routes.length;routeIndex++){
            //check for a match and stop searching if match is found
            if(routes[routeIndex].id===stopETAs[etaIndex].route){
              found = true;
              //console.log("Route ID " + stopETAs[etaIndex].route + " is "+routes[routeIndex].name );
              //console.log(routes[routeIndex].name + " eta: "+stopETAs[etaIndex].avg);
              //format the text to go with the eta number
              switch (stopETAs[etaIndex].avg){
                case 0:
                  busETAsToSend[etaIndex+2]=routes[routeIndex].name + "\nArriving Now";
                  break;
                case 1:
                  busETAsToSend[etaIndex+2]=routes[routeIndex].name + "\nETA: "+stopETAs[etaIndex].avg + " minute";
                  break;
                default:
                  busETAsToSend[etaIndex+2]=routes[routeIndex].name + "\nETA: "+stopETAs[etaIndex].avg + " minutes";
              }
              break;
            }
          }
          if (!found){
            busETAsToSend[etaIndex+2]="Unknown"+ "\nETA: "+stopETAs[etaIndex].avg + " minutes"; // if we can't figure out what route it refers to
          }
        }
        
        busETAsToSend[0]=1;//this indicates the type of data contained in the array
        busETAsToSend[1]=stopETAs.length; //this inidcates how many items are in the array
        
        sendMessage(busETAsToSend);
      
      });
    }
    else{
      busETAsToSend[0]=1;
      busETAsToSend[1]=0;
      sendMessage(busETAsToSend);
    }
    
    
  });
  
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log(JSON.stringify(configData));
  console.log('Selected System: '+configData.system.name+' with ID '+configData.system.code);
  localStorage.setItem('systemName', configData.system.name);
  localStorage.setItem('systemCode', configData.system.code);
  
  //update variables
  systemCode = configData.system.code;
  baseURL = 'http://'+systemCode+'.doublemap.com/map/v2/';
  //this will send nearest stops to watch
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  
});



//event listener to open configuration page
Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://moufee.com/pebble2.php#/?system='+systemCode;

  Pebble.openURL(url);
});






