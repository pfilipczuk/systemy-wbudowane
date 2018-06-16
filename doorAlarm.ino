#include "Arduino.h"
#include "AZ3166WiFi.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "LIS2MDLSensor.h"
#include "OledDisplay.h"

#define DELTA 300
#define DELAY 500
#define INIT_TIME 5
#define TIME_LIMIT 10

static Timer timer;

enum STATUS
{
  Idle,
  DoorClosed,
  DoorOpened
};

static STATUS status = Idle;
static bool azureConnected = false;
static bool doorOpened = false;
static bool messageSent = false;

static DevI2C *i2c;
static LIS2MDLSensor *lis2mdl;

static int offsets[3] = {0, 0, 0};
static int axes[3];

void setup()
{
  Serial.begin(115200);

  Screen.init();
  Screen.print(0, "Setup...");

  Screen.print(1, "WiFi...");
  if (!CheckWiFi())
    return;
  Screen.print(1, "WiFi OK");

  Screen.print(2, "Azure IoT...");
  if (!CheckAzureIoT())
    return;
  Screen.print(2, "Azure IoT OK");
  azureConnected = true;

  pinMode(USER_BUTTON_A, INPUT);
  pinMode(USER_BUTTON_B, INPUT);

  Screen.print(3, "Magnetometer...");
  i2c = new DevI2C(D14, D15);
  lis2mdl = new LIS2MDLSensor(*i2c);
  if (lis2mdl->init(NULL) != MAGNETO_OK)
    return;
  Screen.print(3, "Magnetometer OK");
  Screen.clean();
}

void loop()
{
  if (azureConnected)
  {
    switch (status)
    {
    case Idle:
      DoIdle();
      break;
    case DoorClosed:
      DoDoorClosed();
      break;
    case DoorOpened:
      DoDoorOpened();
      break;
    }
  }
  delay(DELAY);
}

static bool CheckWiFi()
{
  return WiFi.begin() == WL_CONNECTED;
}

static bool CheckAzureIoT()
{
  return DevKitMQTTClient_Init();
}

static void DoIdle()
{
  Screen.print(0, "Idle");
  Screen.print(1, "Press A to init magnetometer", true);
  if (digitalRead(USER_BUTTON_A) == LOW)
  {
    InitMagnetometer();
    status = DoorClosed;
  }
}

static void DoDoorClosed()
{
  Screen.print(0, "Door closed");
  lis2mdl->getMAxes(axes);

  char buffer[50];
  
  sprintf(buffer, "x:  %d", offsets[0] - axes[0]);
  Screen.print(1, buffer);

  sprintf(buffer, "y:  %d", offsets[1] - axes[1]);
  Screen.print(2, buffer);

  sprintf(buffer, "z:  %d", offsets[2] - axes[2]);
  Screen.print(3, buffer);

  CheckMagnetometerStatus();
}

static void DoDoorOpened()
{
  if (digitalRead(USER_BUTTON_B) == LOW)
  {
    Screen.clean();
    timer.stop();
    status = Idle;
    return;
  }

  Screen.print(0, "Door opened!");
  if (timer.read() <= TIME_LIMIT)
  {
    char buffer[50];
    sprintf(buffer, "Press B in %d seconds to turn off the alarm!", TIME_LIMIT - int(timer.read());
    Screen.print(1, buffer, true);
  }
  else
  {
    if (!messageSent && DevKitMQTTClient_SendEvent("Wild Pikachu appeared"))
    {
      messageSent = true;
      Screen.print(1, "Alert sent");
      timer.stop();
    }
  }
}

static void InitMagnetometer()
{
  Screen.clean();
  lis2mdl->getMAxes(axes);
  offsets[0] = axes[0];
  offsets[1] = axes[1];
  offsets[2] = axes[2];

  timer.reset();
  timer.start();
  char buffer[50];
  int delta = 10;
  while (true)
  {
    sprintf(buffer, "Initializing %d", INIT_TIME - int(timer.read()));
    Screen.print(0, buffer);
    sprintf(buffer, "x:  %d", offsets[0] - axes[0]);
    Screen.print(1, buffer);

    sprintf(buffer, "y:  %d", offsets[1] - axes[1]);
    Screen.print(2, buffer);

    sprintf(buffer, "z:  %d", offsets[2] - axes[2]);
    Screen.print(3, buffer);
    delay(DELAY);
    lis2mdl->getMAxes(axes);

    if (abs(offsets[0] - axes[0]) < delta && abs(offsets[1] - axes[1]) < delta && abs(offsets[2] - axes[2]) < delta)
    {
      if (int(timer.read()) >= INIT_TIME)
      {
        timer.stop();
        break;
      }
    }
    else
    {
      offsets[0] = axes[0];
      offsets[1] = axes[1];
      offsets[2] = axes[2];
    }
  }
}

void CheckMagnetometerStatus()
{
  if (abs(offsets[0] - axes[0]) < DELTA && abs(offsets[1] - axes[1]) < DELTA && abs(offsets[2] - axes[2]) < DELTA)
  {
    Screen.print(0, "Door closed");
    doorOpened = false;
    messageSent = false;
  }
  else
  {
    status = DoorOpened;
    timer.reset();
    timer.start();
  }
}
