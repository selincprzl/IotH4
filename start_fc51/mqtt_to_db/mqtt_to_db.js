const mqtt = require('mqtt');
const sql = require('mssql');

// MQTT credentials     
const MQTT_USER = 'hivemq.webclient.1732612694947'; // Same as in main.cpp
const MQTT_PASSWORD = 'e&D2?FR5qh0Tod<1VH>m'; // Same as in main.cpp

// MQTT setup
const client = mqtt.connect('mqtts://f08089bc943a49cd9c62cb94a58e946d.s1.eu.hivemq.cloud', {
    username: MQTT_USER,  // Include username here
    password: MQTT_PASSWORD,  // Include password here
});

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    client.subscribe('iot/temperature', (err, granted) => {
        if (err) {
            console.error('Subscription error:', err);
        } else {
            console.log('Subscribed to topic: iot/temperature');
            console.log('Granted topics:', granted); // Added to show what topics are granted
        }
    });
});



// SQL Server connection string
sqlConfig = "Data Source=192.168.0.225;Initial Catalog=Mqtt;User ID=sa;Password=passw0rd;Connect Timeout=30;Encrypt=True;TrustServerCertificate=True;Application Intent=ReadWrite;Multi Subnet Failover=False";
//you need to connect to your PC ip not local host cause it's runnin in your ESP not local machine - how we do that
// Function to insert data into SQL Server
async function saveDataToDb(temperature) {
    try {
        console.log('Connecting to database...');
        const pool = await sql.connect(sqlConfig); // Use connectionString here
      
        console.log('Connected to database');

        //i think i found the problem

        // Log the query being executed
        const result = await pool.request()
            .input('temperature', sql.Float, temperature)  // Ensure 'temperature' is passed as a Float
            .query("USE Mqtt; INSERT INTO TemperatureReadings (temperature) VALUES (@temperature)");
    
        
            //this is wrong how why
        console.log('Data inserted successfully:', result);
    } catch (err) {
        console.error('Error saving data to DB:', err);
    }
    //restart ESP

}

// MQTT message handling with duplicate and frequency filtering
let lastSavedTemperature = null;
let lastSaveTime = 0;

client.on('message', (topic, message) => {
    console.log('Received message on topic:', topic); // Log topic
    console.log('Message content:', message.toString()); // Log message content

    if (topic === 'iot/temperature') {
        const temperature = parseFloat(message.toString());
        const currentTime = Date.now();

        console.log('Parsed temperature:', temperature);

        // Log the message and temperature value
        console.log('Temperature data to insert into database:', temperature);

        // Avoid saving duplicate or too frequent data
        if (temperature !== lastSavedTemperature || currentTime - lastSaveTime > 5000) {
            console.log('Saving data to DB...');
            saveDataToDb(temperature);
            lastSavedTemperature = temperature;
            lastSaveTime = currentTime;
        } else {
            console.log('Duplicate or frequent data ignored:', temperature);
        }
    }
});

// Error handler for MQTT client
client.on('error', (err) => {
    console.error('MQTT error:', err);
});
