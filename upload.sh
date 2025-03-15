#!/bin/bash

# Datei, die aktualisiert werden soll
file_to_update=".pio/build/esp32dev/firmware.bin"

# Ihr privater Schlüssel
private_key_path="./private_key.pem"

# Aufforderung zur Eingabe der Serveradresse
#read -p "Geben Sie die Serveradresse: " server_base

# Der Endpunkt des Servers
server_base="10.10.30.99"

# Der Endpunkt des Servers wird automatisch ergänzt
server_endpoint="${server_base}/api/update"

pio run -e esp32dev

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

# MD5-Hash der Datei berechnen
md5_hash=$(md5sum "$file_to_update" | awk '{ print $1 }')

# MD5-Hash ausgeben
echo "MD5-Hash:"
for ((i=0; i<${#md5_hash}; i+=2)); do
    printf "%02x" "0x${md5_hash:$i:2}"
done
echo -e "\n"

# Signieren des binären Hashes mit dem privaten Schlüssel
signature=$(openssl dgst -sign "$private_key_path" -keyform PEM -md5 -binary "$file_to_update" | openssl base64)
echo -e "Signatur:\n$signature"


# Senden der Datei mit der signierten Hash-Signatur im Header
curl -X POST "$server_endpoint" \
     -H "x-update-hash: $signature" \
     -F "data=@$file_to_update"

echo -e "\nDatei und Signatur gesendet."