#include <Ethernet.h>
#include <SPI.h>
#include <ArduinoHA.h>
#include <ZMPT101B.h>

// Configurações de rede
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 2, 177);
IPAddress mqttServer(192, 168, 2, 75); // Endereço IP do seu servidor MQTT

EthernetClient ethClient;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(ethClient, device);

// Sensor de voltagem
HASensor voltageSensor("voltage");

// Configuração do sensor ZMPT101B
ZMPT101B voltageSensorZMPT(A1, 50.0);
float calibrationFactor = 205.0; // Valor padrão de calibração
char voltageStr[8];

// Declaração do HANumber para calibração
HANumber calibrationNumber("calibration_factor", HANumber::PrecisionP1);

void onNumberCommand(HANumeric number, HANumber* sender) {
    if (number.isSet()) {
        calibrationFactor = number.toFloat();
        voltageSensorZMPT.setSensitivity(calibrationFactor);
        Serial.print("Novo fator de calibração: ");
        Serial.println(calibrationFactor);
    }
    sender->setState(number); // Reporta o valor de volta ao Home Assistant
}

void setup() {
    Serial.begin(115200);
    Ethernet.begin(mac, ip);
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield not found");
        while (true) delay(1); // Trava o loop
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected");
    }
    
    mqtt.begin(mqttServer, "admin", "04060811");

    // Configurações do dispositivo
    device.setName("Arduino Voltage Sensor");
    voltageSensor.setName("Voltage");
    voltageSensor.setUnitOfMeasurement("V");

    // Configuração do número para calibração
    calibrationNumber.onCommand(onNumberCommand);
    calibrationNumber.setIcon("mdi:settings");
    calibrationNumber.setName("Calibration Factor");
    calibrationNumber.setMin(50.0); // Valor mínimo
    calibrationNumber.setMax(500.0); // Valor máximo
    calibrationNumber.setStep(5.0f); // Passo

    // Configuração inicial do sensor ZMPT101B
    voltageSensorZMPT.setSensitivity(calibrationFactor);

    // Conectando ao MQTT
    while (!mqtt.isConnected()) {
        Serial.println("Connecting to MQTT...");
        mqtt.loop();
        delay(1000);
    }

    Serial.println("Connected to MQTT!");
}

void loop() {
    mqtt.loop();

    // Leitura do sensor ZMPT101B
    float voltage = voltageSensorZMPT.getRmsVoltage();

    // Converte o valor de float para string
    dtostrf(voltage, 6, 2, voltageStr);
    
    // Define o valor do sensor
    voltageSensor.setValue(voltageStr);

    // Publica a leitura do sensor a cada 10 segundos
    delay(10000);
}
