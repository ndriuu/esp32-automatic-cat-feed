#include <Arduino.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
// Insert your network credentials
#define WIFI_SSID "____"
#define WIFI_PASSWORD "____"

// Insert Firebase project API Key
#define API_KEY "____"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "____" 
const int trigPin = 13;
const int echoPin = 12;
long duration;
float distanceCm;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

//Define Servo
Servo myservo;

//Define Firebase Data object
FirebaseData fbdo;
FirebaseJsonData result;
FirebaseAuth auth;
FirebaseConfig config;
int stok,servo;
String servo1;
bool signupOK = false;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7*60*60;
const int   daylightOffset_sec = 7*60*60;
void resultFirebase(FirebaseData &data);
void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  //===========================
  myservo.attach(14);
  myservo.write(115);
  //==========================
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  //init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Connected");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(1024, 1024);
  fbdo.setResponseSize(1024);
  if (!Firebase.RTDB.beginStream(&fbdo, "/")){
    Serial.println(fbdo.errorReason());
  }
}

void loop() {
  if (!Firebase.RTDB.readStream(&fbdo)){
    Serial.println(fbdo.errorReason());
  }
  if (fbdo.streamTimeout()){
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
  }
  if (fbdo.streamAvailable()){
      resultFirebase(fbdo);
  }
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  // Convert to inches
  Serial.print("Servo: ");
  Serial.println(servo1.toInt());
  Serial.print("Stok: ");
  Serial.println(stok);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);

  if(!(servo1.toInt()==0) && !(servo==1)){
    myservo.write(20);
    Serial.println("Servo Terbuka");
    servo=1;
    servo1="0";
    delay(1000);
    myservo.write(115);
    Firebase.RTDB.setString(&fbdo, "test/servo", servo1);
    
  }else{
    myservo.write(115);
    Serial.println("Servo tertutup");
    servo=0;
  }
  if(distanceCm >= 12 && !(stok==0)){
    Serial.println("Stok Habis");
    stok = 0;
    Firebase.RTDB.setInt(&fbdo, "test/stok", stok);
  }
  if(distanceCm > 8 && distanceCm < 12 && !(stok==1)){
    Serial.println("Stok Sedang");
    stok = 1;
    Firebase.RTDB.setInt(&fbdo, "test/stok", stok);
  }
  if(distanceCm <7 && !(stok==2)){
    Serial.println("Stok Full");
    stok = 2;
    Firebase.RTDB.setInt(&fbdo, "test/stok", stok);
  }
}

void resultFirebase(FirebaseData &data){
  if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_integer)
    stok = fbdo.to<int>();
  else if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_string){
    servo1 = fbdo.to<String>();
  }
  else if (fbdo.dataTypeEnum() == firebase_rtdb_data_type_json)
  {
    FirebaseJson *json = fbdo.to<FirebaseJson *>();
    json->get(result, "test/stok");
    if (result.success){
    stok = result.to<int>();
    }
    json->get(result, "test/servo");
    if (result.success){
    servo1 = result.to<String>();
    }
  }
}
