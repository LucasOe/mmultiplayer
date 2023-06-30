#!/bin/bash

set -ex
buildDir='build'
mkdir -p "${buildDir}"

(
    cd Client
    MSBuild.exe Client.vcxproj -property:Configuration=Release -property:Platform=x86 -t:Build
    cp Release/mmultiplayer.dll ../${buildDir}/app/
)
(
    cd Launcher
    MSBuild.exe Launcher.vcxproj -property:Configuration=Release -property:Platform=x86 -t:Build
    cp Release/Launcher.exe ../${buildDir}/app/
)
