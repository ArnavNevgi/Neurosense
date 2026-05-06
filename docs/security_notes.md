# Security Notes

## Secrets

- Real Wi-Fi, Azure IoT Hub, SAS token, and ThingSpeak values belong in `include/secrets.h`.
- `include/secrets.h` is ignored by Git.
- `include/secrets.h.example` contains placeholders only and is safe to commit.
- Do not commit SAS tokens, ThingSpeak write keys, Wi-Fi passwords, certificates, private keys, or `.env` files.

For PlatformIO:

```powershell
Copy-Item include\secrets.h.example include\secrets.h
```

For Arduino IDE, if the nested biometric sketch cannot find the repo-level `include/` directory, copy the local secrets file into the sketch folder as `secrets.h`.

## Azure SAS Tokens

Azure SAS tokens expire. Regenerate them as needed and store the current token only in your ignored local secrets file.

## TLS Limitation

The ESP8266 biometric monitor currently uses `espClient.setInsecure()`. That means the development build does not validate the Azure IoT Hub server certificate. CA certificate validation is required before production use.
