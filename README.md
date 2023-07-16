# Krazy Karts

Basic car project I made as part of the [Unreal Multiplayer Master: Video Game Dev In C++ Course](https://www.gamedev.tv/p/unrealmultiplayer) to learn how to do multiplayer replication and interpolation in a good way in Unreal Engine.

## Setup Development Environment

Project has only default requirements of a UE project.
It was created using UE 4.26.2, so the requirements are as following:
• UE 4.26.2
• Visual Studio 2019 with Visual C++ installed.

Only the default steps are required to setup the development environment:
• RMB on **KrazyKarts.uproject**, then choose **Generate Visual Studio project files**.
• Open **KrazyKarts.uproject**.
• Open **KrazyKarts.sln**.

## Components

The main components to check out in this project are `GoKart`, `GoKartMovementComponent` and `GoKartMovementReplicator`.
`GoKart` is the main actor of the project. It holds all the inner actor components: mesh, movement and the replicator components.
`GoKartMovementComponent` is, as the name tells us, the movement component for `GoKart` actor.
`GoKartMovementReplicator` is the actor component that is responsible for online replication and smooth interpolation of the `GoKart`.
