#!/bin/bash

# Example
# $ VERSION=0.0.1 ./package.sh

if [[ -z "${VERSION}" ]]
then
    echo 'the VERSION environment variable must be set'
    exit 1
fi

set -eux

buildDir='build'
go run resources/installer/xor/main.go Launcher/Release/Launcher.exe > resources/installer/squibbles/launcher.buh
go run resources/installer/xor/main.go Client/Release/mmultiplayer.dll > resources/installer/squibbles/dll.buh
go build -o ${buildDir}/app/squibbles.exe resources/installer/squibbles/main.go

# TODO: Super hack because InnoSetup does not support relative paths.
cp -v resources/installer/installer.iss "${buildDir}/"

# TODO: Add license for Windows installer.
# '/DLicenseFileOverride=license_en_US.txt'
innosetupArgs=$(cat <<-END
/DApplicationFilesPath=app
/DOutputPath=.
/DAppNameOverride=Mirrors Edge Multiplayer
/DAppPublisherOverride=Buh
/DAppURLOverride=https://github.com/Toyro98/mmultiplayer
/DAppExeNameOverride=Launcher.exe
/DAppVersionOverride=${VERSION}
/DOutputBaseFilenameOverride=mirrors-edge-multiplayer-${VERSION}
installer.iss
END
)

innosetupArgs="\"${innosetupArgs//$'\n'/\" \"}\""

batFilePath="${buildDir}/package.bat"
echo "ISCC.exe ${innosetupArgs}" > "${batFilePath}"
(cd "${buildDir}" && cmd //c "${batFilePath##*/}")
