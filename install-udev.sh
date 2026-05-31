#!/bin/sh
set -e

DEST=/etc/udev/rules.d/99-logitech-hub.rules
SRC="$(dirname "$0")/udev/99-logitech-hub.rules"

echo "Copying rules to $DEST ..."
sudo cp "$SRC" "$DEST"

echo "Reloading udev rules ..."
sudo udevadm control --reload-rules

echo "Applying rules to hidraw devices ..."
sudo udevadm trigger --action=add --subsystem-match=hidraw

echo ""
echo "Done. The mouse should now be accessible."
echo "If you still see 'No permission', unplug and replug the mouse."
