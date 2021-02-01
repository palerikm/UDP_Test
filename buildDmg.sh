#!/bin/sh
test -f UDPTest-Installer.dmg && rm UDPTest-Installer.dmg
create-dmg \
  --volname "UDPTest Installer" \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  "UDPTest-Installer.dmg" \
  "release/qtclient/udpqtclient.app/Contents/MacOS/udpqtclient"