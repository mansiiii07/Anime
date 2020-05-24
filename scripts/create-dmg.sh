#!/bin/sh

# Puts the app package into a dmg file

# Requires PROJECT_DIR, BUILT_PRODUCTS_DIR, EXECUTABLE_PATH, FULL_PRODUCT_NAME, 
# CODE_SIGN_IDENTITY, EXPANDED_CODE_SIGN_IDENTITY
# Requires appdmg
# Requires envsubst from gettext

alias animera="$BUILT_PRODUCTS_DIR/$EXECUTABLE_PATH"
BACKGROUND="$PROJECT_DIR/Resources/Sprites/dmg background.animera"

animera export --name=background --scale=4 "$BACKGROUND"
animera export --name=background@2x --scale=8 "$BACKGROUND"

cat "$PROJECT_DIR/scripts/dmg.json" | envsubst > expanded.json
appdmg expanded.json "$PROJECT_DIR/Animera.dmg"

rm expanded.json
rm background.png
rm background@2x.png