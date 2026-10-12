const mqtt = require("mqtt"); //load the mqtt library 

const postgres = require("pg"); //loag the postgres library
const Pool = postgres.Pool; //get the Pool property

const db = new Pool({
	
	max: 2, //at most 2 database connections in this pool
	connectionTimeoutMillis: 5000, // wait up to 5 secs when acquiring a connection

});

const topic = "greenhouse/01/telemetry"; //choose the topic

const client = mqtt.connect("mqtt://10.42.0.1:1883"); //and th connection



db.on("error", function(error){

	console.error("Database connection error:", error.message);
	
});


async function checkDatabase(){//async allows to use the await feature
	
	try{
		const result = await db.query("SELECT 1 AS test");//awaits mens to wait for this sql result

		console.log("Database connected. Test result: ", result.rows[0].test);

	}catch(error){
		console.error("Database connection failed:", error.message)
		
	}

}


async function saveReading(data){

	if( !Number.isSafeInteger(data.sampled_at_ms) || data.sampled_at_ms < 0){
		throw new Error("sampled_at_ms must be a non-negative integer");
	}

	if(typeof data.test !== "boolean"){
		throw new Error("test must be boolean");
	}
	
	const values = [

		"01",
		data.sampled_at_ms,
		measurement(data.temperature_c, "temperature_c", -100, 150),
		measurement(data.humidity_pct, "humidity_pct", 0, 100),
		booleanValue(data.water_low, "water_low"),
		measurement(data.light_pct, "light_pct", 0, 100),
		measurement(data.moisture_pct, "moisture_pct", 0, 100),
		booleanValue(data.pump_on, "pump_on"),
		data.test

	];


	const result = await db.query(

		`INSERT INTO readings (

			device_id,
			sampled_at_ms,
			temperature_c,
			humidity_pct,
			water_low,
			light_pct,
			moisture_pct,
			pump_on,
			is_test
			)
		VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
		RETURNING id`,
		values
	);

	console.log("Saved reading with ID: ", result.rows[0].id);

}

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


function booleanValue(value, name){
	
	if(value === null || typeof value === "boolean"){
		return value;
	}

	throw new Error(name + " must be a boolean or null");

}

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
client.on("message", async function(receivedTopic, payload) {// lets the func use await

	if(receivedTopic !== topic){
		return;
	}

	const text = payload.toString("utf8");

	try{
		const data = JSON.parse(text);

		if(data === null || typeof data !== "object" || Array.isArray(data)){
			throw new Error("Expected a JSON object");
		}

		await saveReading(data);

	} catch(error){
		console.error("Rejected message: ", error.message);
	}


});



//if an error occurs, enter the function, which logs it
client.on("error", function(error){

	console.error("MQTT Error: ", error.message);

});


checkDatabase(); //execute the func
