const mqtt = require("mqtt"); //load the mqtt library

const topic = "greenhouse/01/telemetry"; //choose the topic

const client = mqtt.connect("mqtt://10.42.0.1:1883"); //and th connection


//if client givees connect, execute this functions, which logs connection
//if it manages to subscribe to the topic, log that it is Subscribed
//if it fails, log the error
client.on("connect", function(){

	console.log("Connected to MQTT broker");

	client.subscribe(topic, { qos: 1}, function (error){

		if(error){
			
			console.error("Subscription failed: ", error.message);
			return;
		}

		console.log("Subscribed to: ", topic);

	});

});

function measurement(value, name, min, max){

	if(value === null){
		return null;
	}

	if(typeof value !== "number"){
		throw new Error(name + " must be a number or null!!");
	}

	if(!Number.isFinite(value)){
		throw new Error(name + " must be a finite number");
	}

	if(value < min || value > max){
		throw new Error(name + " is outside of the allowed range");
	}

	return value;


}


//if client receives a message, enter the function, with pars about the topic and the payload of this message
//check if the json is valid, if not log the error, if yes, display the info
client.on("message", function(receivedTopic, payload) {

	const text = payload.toString("utf8");

	try{
		const data = JSON.parse(text);

		if(data === null || typeof data !== "object" || Array.isArray(data)){
			throw new Error("Expected a JSON object");
		}

		const temperature = measurement(data.temperature_c, "temperature_c", -100, 150);

		const humidity = measurement(data.humidity_pct, "humidity_pct", 0, 100);

		console.log("Accepted Temperature:", data.temperature_c);
		console.log("Accepted Humdity:", data.humidity_pct);


	} catch(error){
		console.error("Rejected message: ", error.message);
	}


});



//if an error occurs, enter the function, which logs it
client.on("error", function(error){

	console.error("MQTT Error: ", error.message);

});
