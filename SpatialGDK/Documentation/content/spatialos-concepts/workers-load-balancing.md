<%(TOC)%>
# Workers and load balancing

Before you read this page, you should read
[What is SpatialOS?]({{urlRoot}}/shared/concepts/spatialos) and
[World, entities, components]({{urlRoot}}/shared/concepts/world-entities-components).

## Worker instances and worker types

SpatialOS uses **server-worker instances** to simulate your game world. 

Players use **client-worker instances** to connect to your game and interact with the world.

You can think of a worker type as a template. Each worker type has a worker configuration file (<worker_type>.worker.json) where you define how SpatialOS should build, launch, and interact with instances of this worker type.

For example, you could create a server-worker type called `UnrealWorker`, and your deployment could have many instances of this worker type simulating the game world.

## Server-worker instances

SpatialOS uses the combined computation of multiple server-worker instances to simulate the game world. This means that the server-worker instances don’t know anything about each other - they might not even be on the same machine in the cloud. 

So when you’re writing the code for server-workers, you need to write code that can cope with only knowing about one part (or multiple non-contiguous parts) of the world.

All of these server-worker instances run using a copy of the same binary.

## Splitting up the world, or “load balancing”

One of the decisions you need to make as a developer is, “How many server-worker instances does my world need?”

To decide this, you’re working out how much computation your world needs, and how many server-worker instances you need to do that work. When you decide this, you split the world up into regions; each region is the area of authority of a server-worker instance. 

For a very small world, one instance might be enough. However, if you’re planning to use multiple server-worker instances to simulate your world, we definitely recommend trying to scale your game early. You should test early on with at least two server-worker instances running your world. It can be hard to reason about how to architect your game properly to deal with re-assignments of authority from one server-worker instance to another. And some problems won’t be obvious until you have multiple server-worker instances behind your game world.

You can use the networking switch to do this… iterate with Unreal, quickly, and then flip the switch and see what breaks.

## Deployments

SpatialOS hosts your games for you. We call an instance of a running game a **deployment**.

As you learned above, you decide how many server-worker instances your world needs, and how to organize them across the world. In a deployment, SpatialOS starts those server-worker instances for you, running them on machines in the cloud (and orchestrating this for you - you don’t need to interact with the machines directly).

SpatialOS also mediates client-worker instance connections.

## Client-worker instances

Because each client-worker instance is tied to a player, and runs on the player's local machine, SpatialOS doesn't manage a client-worker instance's workload in the same way as it manages a server-worker instance's workload. This means that during game development, you set up client-worker types and server-worker types differently. The main difference is around how you synchronize data to and from the game world.

Like server-worker instances, client-worker instances can only see a part of the world. However, client-worker instances can see across server-worker instance boundaries.
