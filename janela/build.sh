#!/bin/sh
# Compila a janela da central (janela.swift) com o swiftc do Command Line Tools: sem Xcode e sem
# baixar nada. Roda no Mac, de qualquer pasta. Saída em build/: o pacote "Central MOI3.app", com
# o programa central-moi3 dentro.
set -e
D=$(cd "$(dirname "$0")" && pwd); APP="$D/build/Central MOI3.app"
command -v swiftc >/dev/null || { echo "falta o swiftc: instale o Command Line Tools (xcode-select --install)"; exit 1; }
mkdir -p "$APP/Contents/MacOS"
swiftc -O -o "$APP/Contents/MacOS/central-moi3" "$D/janela.swift"
cat > "$APP/Contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>Central MOI3</string>
  <key>CFBundleDisplayName</key><string>Central MOI3</string>
  <key>CFBundleIdentifier</key><string>local.vagcoding.central-moi3</string>
  <key>CFBundleExecutable</key><string>central-moi3</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSPrincipalClass</key><string>NSApplication</string>
</dict>
</plist>
EOF
echo "pronto: $APP"
