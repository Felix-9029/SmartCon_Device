#!/bin/bash

# Datei, die aktualisiert werden soll
file_to_update=".pio/build/esp32dev/firmware.bin"

# Ihr privater Schlüssel
private_key_path="./private_key.pem"

# Aufforderung zur Eingabe der Serveradresse
read -p "Geben Sie die Serveradresse: " server_base

# Der Endpunkt des Servers wird automatisch ergänzt
server_endpoint="${server_base}/api/update"

# Überprüfen, ob die Firmware-Datei existiert
if [ ! -f "$file_to_update" ]; then
    echo "Firmware-Datei $file_to_update existiert nicht."
    exit 1
fi

# Überprüfen, ob der private Schlüssel existiert
if [ ! -f "$private_key_path" ]; then
    echo "Privater Schlüssel $private_key_path existiert nicht."
    exit 1
fi

# Signieren des binären Hashes mit dem privaten Schlüssel
signature=$(sha256sum "$file_to_update" | openssl dgst -sha256 -sign "$private_key_path" | openssl base64)

# Senden der Datei mit der signierten Hash-Signatur im Header
curl -X POST "$server_endpoint" \
     -H "x-update-hash: $signature" \
     -F "data=@$file_to_update"

echo -e "\nDatei und Signatur gesendet."