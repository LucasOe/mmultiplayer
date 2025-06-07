#!/bin/bash

set -ex

MSBuild.exe Client/Client.vcxproj -property:Configuration=Release -property:Platform=x86 -property:OutDir=../Release/ -t:Build
MSBuild.exe Launcher/Launcher.vcxproj -property:Configuration=Release -property:Platform=x86 -property:OutDir=../Release/ -t:Build
