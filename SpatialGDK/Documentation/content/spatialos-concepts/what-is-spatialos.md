<%(TOC)%>
# SpatialOS

## What is SpatialOS?

**SpatialOS** is a platform-as-a-service that runs and manages online games in the cloud.

But while it runs your game and manages the infrastructure for you, SpatialOS also enables
something more than that. It runs games in a way that lets them scale further, be more complex,
and have long-living persistence.

## How does it work?

The traditional ways to develop large online games mean that you’re either limited by the capacity
of a single game server, or you have to shard your game world. SpatialOS works differently: it brings together many servers so they’re working as one. But it
does this in a way that makes a single world which looks seamless to players.

## How does it fit with Unreal and the GDK?

The GDK provides a networking integration with SpatialOS, which enables UE4 clients and servers to communicate with the SpatialOS Runtime to synchronize state. 

We’ve created [a version of Unreal Engine](https://github.com/improbableio/UnrealEngine) which provides SpatialOS networking alongside Unreal’s native networking. We maintain Unreal’s networking API, which means you don’t need to rewrite your game to make it work with the GDK.
