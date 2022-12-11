# EasyLED_Server
LED Server for Smart Home Light Controller

Zum erstellen der CMakeListsPrivate.txt:<br />
``pio init --ide clion``

upload_port in platformio.ini bearbeiten, damit der Pfad zum controller passt(`pio device list`)<br />
platformio.ini - Rechtsklick > PlatformIO > Re-Init

[File | Settings | Build, Execution, Deployment | CMake](jetbrains://CLion/settings?name=Build%2C+Execution%2C+Deployment--CMake)<br />
remove default; add new