#include "AZ3166WiFi.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "LIS2MDLSensor.h"
#include "OledDisplay.h"

#define DELAY 1000
#define EXPECTED_COUNT 5

// The magnetometer sensor
static DevI2C *i2c;
static LIS2MDLSensor *lis2mdl;

// Data from magnetometer sensor
static int axes[3];
static int base_x;
static int base_y;
static int base_z;

static bool isOpened = false;
static bool messageSent = false;

// The open / close status of the door
static bool preOpened = false;

// Indicate whether DoorMonitorSucceed event has been logged
static bool telemetrySent = false;

// Indicate whether WiFi is ready
static bool hasWifi = false;

// Indicate whether IoT Hub is ready
static bool hasIoTHub = false;

static bool CheckWiFi()
{
  return WiFi.begin() == WL_CONNECTED);
}

// Utilities
static void InitWiFi()
{
  Screen.print(2, "Connecting...");

  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
    Screen.print(2, "Running... \r\n");
  }
  else
  {
    hasWifi = false;
    Screen.print(1, "No Wi-Fi\r\n ");
  }
}

static void InitMagnetometer()
{
  Screen.print(2, "Initializing...");
  i2c = new DevI2C(D14, D15);
  lis2mdl = new LIS2MDLSensor(*i2c);
  lis2mdl->init(NULL);

  lis2mdl->getMAxes(axes);
  base_x = axes[0];
  base_y = axes[1];
  base_z = axes[2];

  int count = 0;
  int delta = 10;
  char buffer[20];
  while (true)
  {
    delay(DELAY);
    lis2mdl->getMAxes(axes);

    // Waiting for the data from sensor to become stable
    if (abs(base_x - axes[0]) < delta && abs(base_y - axes[1]) < delta && abs(base_z - axes[2]) < delta)
    {
      count++;
      if (count >= EXPECTED_COUNT)
      {
        // Done
        Screen.print(0, "Monitoring...");
        break;
      }
    }
    else
    {
      count = 0;
      base_x = axes[0];
      base_y = axes[1];
      base_z = axes[2];
    }
    sprintf(buffer, "      %d", EXPECTED_COUNT - count);
    Screen.print(1, buffer);
  }
}

void CheckMagnetometerStatus()
{
  char *message;
  int delta = 300;
  bool curOpened = false;
  if (abs(base_x - axes[0]) < delta && abs(base_y - axes[1]) < delta && abs(base_z - axes[2]) < delta)
  {
    Screen.print(0, "Door closed");
    message = "Door closed";
    curOpened = false;
  }
  else
  {
    Screen.print(0, "Door opened");
    message = "Door opened";
    curOpened = true;
  }
  // send message when status change
  if (curOpened != preOpened)
  {
    if (DevKitMQTTClient_SendEvent(message))
    {
      if (!telemetrySent)
      {
        telemetrySent = true;
      }
    }
    preOpened = curOpened;
  }
}

void setup()
{
  Serial.begin(115200);

  Screen.init();
  Screen.print(0, "Setup...");
  Screen.print(1, "Checking WiFi...");
  if (!CheckWiFi())
  {
    return;
  }

  Screen.print(1, "WiFi OK");
  IPAddress ip = WiFi.localIP();
  Screen.print(2, ip.get_address());
  
  // IoT hub
  Screen.print(3, " > IoT Hub");
  DevKitMQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "DoorMonitor");
  if (!DevKitMQTTClient_Init())
  {
    Screen.clean();
    Screen.print(0, "DoorMonitor");
    Screen.print(2, "No IoT Hub");
    hasIoTHub = false;
    return;
  }
  hasIoTHub = true;

  Screen.print(3, " > Magnetometer");
  InitMagnetometer();
}

void loop()
{
  if (hasWifi && hasIoTHub)
  {
    // Get data from magnetometer sensor
    lis2mdl->getMAxes(axes);
    Serial.printf("Axes: x - %d, y - %d, z - %d\r\n", axes[0], axes[1], axes[2]);

    char buffer[50];

    sprintf(buffer, "x:  %d", axes[0]);
    Screen.print(1, buffer);

    sprintf(buffer, "y:  %d", axes[1]);
    Screen.print(2, buffer);

    sprintf(buffer, "z:  %d", axes[2]);
    Screen.print(3, buffer);

    CheckMagnetometerStatus();
  }
  delay(DELAY);
}
