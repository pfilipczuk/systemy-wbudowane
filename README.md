# Założenia
Projekt jest prostą implementacją podstawowego systemu antywłamaniowego. Pomysł działa w oparciu o magnetometr. Układ miałby być wbudowany we framugę drzwi, a do samych drzwi byłby przyczepiony magnes. System wykrywa znaczące zmiany natężenia pola magnetycznego i stwierdza że nastąpiło otwarcie drzwi. Następnie użytkownik ma określony czas (10s) na wyłączenie alarmu, w przeciwnym razie system wysyła maila o możliwym włamaniu.

# Jak to działa
Całość składa się z kilku płaszczyzn:
* Azure DevKit (Arduino + biblioteki ustawiające connectionString do Azure IoT Hub)
* Azure IoT Hub
* Azure Function
* SendGrid

![flow](https://github.com/sibirov/systemy-wbudowane/raw/master/img/flow.jpg)

Plik .bin/azuredeploy.json może zostać użyty do wdrożenia potrzebnych zasobów na Azure. SendGrid należy dodać osobno.
Kod funkcji (serverless) znajduje się w azure/alarmFunction/run.csx, konfiguracja Trigger + Output w function.json
