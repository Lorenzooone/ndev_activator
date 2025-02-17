#!/bin/bash
docker image rm lorenzooone/ndev_activator:builder32
docker build --target builder32 . -t lorenzooone/ndev_activator:builder32
docker image rm lorenzooone/ndev_activator:builder64
docker build --target builder64 . -t lorenzooone/ndev_activator:builder64
docker image rm lorenzooone/ndev_activator:builderarm32
docker build --target builderarm32 . -t lorenzooone/ndev_activator:builderarm32
docker image rm lorenzooone/ndev_activator:builderarm64
docker build --target builderarm64 . -t lorenzooone/ndev_activator:builderarm64
