<?php
$servername = htmlspecialchars($_GET["servername"]);
$username = htmlspecialchars($_GET["username"]);
$password = htmlspecialchars($_GET["password"]);
$database = htmlspecialchars($_GET["database"]);
$temperature = htmlspecialchars($_GET["temperature"]);
$pressure = htmlspecialchars($_GET["pressure"]);
$humidity = htmlspecialchars($_GET["humidity"]);

// if correct method and db credentials not empty, try to connect
if ( !empty($servername) && !empty($username) && !empty($password) && !empty($database) && !empty($temperature) && !empty($pressure) && !empty($humidity)){
    try {
        $db = new mysqli($servername, $username, $password, $database);
    
        if (mysqli_connect_errno()) {
            http_response_code(500);
            echo json_encode(array("message" => "Connection to database failed."));
        }
        else {
            $query = "INSERT INTO tbl_weather (temperature, pressure, humidity) VALUES ('$temperature', '$pressure', '$humidity')";
            $result = mysqli_query($db, $query);

            if ($result) {
                http_response_code(200);
                echo json_encode(array("message" => "Data has been saved."));
            } else {
                http_response_code(500);
                echo json_encode(array("message" => "Error occurred while saving data."));
            }
        }
    } catch (Exception $e) {
        throw new Exception($e->getMessage());
    }
}
else {
    http_response_code(400);
    echo json_encode(array("message" => "Servername, username, password, database, temperature, pressure and humidity are required."));
}
