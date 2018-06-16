#include "Arduino.h"
#include "AZ3166WiFi.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "LIS2MDLSensor.h"
#include "OledDisplay.h"

#define DELTA 30
#define MAGNET_INIT_TIME 5
#define MAGNET_READ_INTERVAL 500
#define INIT_TIME 5
#define SEND_ALERT_TIME 10
#define DEFAULT_TIMER_INTERVAL 500 // 0.5s

enum STATUS
{
  Idle,
  InitMagnet,
  DoorClosed,
  DoorOpened
};

static Timer timer;
static Timer initMagnetTimer;
static Timer sendAlertTimer;

static STATUS status = Idle;
static bool azureConnected = false;
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
  SwitchIdle();
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
    case InitMagnet:
      DoInitMagnet();
      break;
    case DoorClosed:
      DoDoorClosed();
      break;
    case DoorOpened:
      DoDoorOpened();
      break;
    }
    if (digitalRead(USER_BUTTON_B) == LOW)
    {
      SwitchIdle();
    }
  }
}

static bool CheckWiFi()
{
  return WiFi.begin() == WL_CONNECTED;
}

static bool CheckAzureIoT()
{
  return DevKitMQTTClient_Init();
}

static void SwitchIdle()
{
  Screen.clean();
  Screen.print(0, "Idle");
  Screen.print(1, "Press A to init magnetometer", true);
  Screen.print(3, "B to cancel");
  status = Idle;
}

static void DoIdle()
{
  if (digitalRead(USER_BUTTON_A) == LOW)
  {
    SwitchInitMagnet();
  }
}

static void SwitchDoorClosed()
{
  Screen.clean();
  Screen.print(0, "Door closed");
  timer.reset();
  timer.start();
  status = DoorClosed;
}

static void DoDoorClosed()
{
  if (int(timer.read_ms()) > DEFAULT_TIMER_INTERVAL)
  {
    timer.reset();
    lis2mdl->getMAxes(axes);

    char buffer[50];

    sprintf(buffer, "x:  %d", offsets[0] - axes[0]);
    Screen.print(1, buffer);

    sprintf(buffer, "y:  %d", offsets[1] - axes[1]);
    Screen.print(2, buffer);

    sprintf(buffer, "z:  %d", offsets[2] - axes[2]);
    Screen.print(3, buffer);

    if (abs(offsets[0] - axes[0]) > DELTA || abs(offsets[1] - axes[1]) > DELTA || abs(offsets[2] - axes[2]) > DELTA)
    {
      SwitchDoorOpened();
    }
  }
}

static void SwitchDoorOpened()
{
  messageSent = false;
  Screen.print(0, "Door opened!");
  timer.reset();
  timer.start();
  sendAlertTimer.reset();
  sendAlertTimer.start();
  status = DoorOpened;
}

static void DoDoorOpened()
{
  if (timer.read_ms() > DEFAULT_TIMER_INTERVAL)
  {
    timer.reset();
    char buffer[50];
    sprintf(buffer, "Press B in %d seconds to turn off the alarm!", SEND_ALERT_TIME - int(sendAlertTimer.read()));
    Screen.print(1, buffer, true);
  }
  if (sendAlertTimer.read() > SEND_ALERT_TIME)
  {
    if (!messageSent && DevKitMQTTClient_SendEvent("Wild Pikachu appeared"))
    {
      messageSent = true;
      Screen.clean();
      Screen.print(0, "Alert sent!");
      timer.stop();
      timer.reset();
    }
  }
}

static void SwitchInitMagnet()
{
  Screen.clean();
  char buffer[20];
  sprintf(buffer, "Initializing %ds", MAGNET_INIT_TIME);
  Screen.print(0, buffer);
  Screen.print(1, "x:");
  Screen.print(2, "y:");
  Screen.print(3, "z:");
  lis2mdl->getMAxes(axes);
  offsets[0] = axes[0];
  offsets[1] = axes[1];
  offsets[2] = axes[2];
  initMagnetTimer.reset();
  initMagnetTimer.start();
  timer.reset();
  timer.start();
  status = InitMagnet;
}

static void DoInitMagnet()
{
  if (timer.read_ms() > DEFAULT_TIMER_INTERVAL)
  {
    timer.reset();
    lis2mdl->getMAxes(axes);
    if (abs(offsets[0] - axes[0]) < DELTA && abs(offsets[1] - axes[1]) < DELTA && abs(offsets[2] - axes[2]) < DELTA)
    {
      if (int(initMagnetTimer.read()) >= MAGNET_INIT_TIME)
      {
        initMagnetTimer.stop();
        SwitchDoorClosed();
        return;
      }
      char buffer[20];
      sprintf(buffer, "Initializing %ds", MAGNET_INIT_TIME - int(initMagnetTimer.read()));
      Screen.print(0, buffer);
      sprintf(buffer, "x:  %d", offsets[0] - axes[0]);
      Screen.print(1, buffer);

      sprintf(buffer, "y:  %d", offsets[1] - axes[1]);
      Screen.print(2, buffer);

      sprintf(buffer, "z:  %d", offsets[2] - axes[2]);
      Screen.print(3, buffer);
    }
    else
    {
      initMagnetTimer.reset();
      offsets[0] = axes[0];
      offsets[1] = axes[1];
      offsets[2] = axes[2];
    }
  }
}
