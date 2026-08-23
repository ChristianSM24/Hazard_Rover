# Hazard Rover — IoT-baseret overvågningsrover

Soloprojekt · 2. semester eksamensprojekt · AK IT-teknolog, Zealand Erhvervsakademi

## Om projektet

Hazard Rover er en selvstændigt designet og bygget IoT-rover, udviklet som eksamensprojekt
på 2. semester. Roveren er bygget til at kunne bevæge sig rundt i et miljø, indsamle sensordata
i realtid og streame et live kamerabillede tilbage til en bruger, samtidig med at data logges
til efterfølgende analyse.

Projektet er bygget alene, fra kravspecifikation og komponentvalg til hardwareopbygning,
softwareudvikling og test.

## Hardware

- **Arduino Mega 2560** — styring af sensorer, motorer og robotarm
- **Raspberry Pi 3B+** — kørsel af Flask-server, kamerastreaming og overordnet systemstyring
- **Sensorer:**
  - DHT22 (temperatur & luftfugtighed)
  - MQ135 (luftkvalitet)
  - MQ2 (gas)
  - IR-sensorer (forhindringsregistrering)
  - TF-Luna LiDAR (afstandsmåling)
- **Robotarm** til fysisk interaktion med omgivelserne
- Kamera til live videostreaming

## Software

- **Flask-server** (Python) til styring af roveren og formidling af sensordata/videostream
- **SQLite** til lokal logning af indsamlede sensordata
- Arduino-firmware (C/C++) til sensor- og motorstyring

## Funktioner

- Fjernstyret bevægelse
- Live kamerastreaming
- Kontinuerlig indsamling og logning af miljø- og afstandsdata
- Styring og betjening af robotarm

## Status

Projektet er afleveret og bestået som eksamensprojekt. Kildekode og eksamensrapport
findes i dette repository.

## Mulige forbedringer

- Overgang til trådløs styring i stedet for kabelforbundet opsætning
- Webbaseret dashboard til visualisering af sensordata i realtid
- Alarmering ved kritiske sensorværdier (fx luftkvalitet over grænseværdi)
