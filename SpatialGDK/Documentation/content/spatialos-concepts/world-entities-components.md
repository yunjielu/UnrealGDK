<%(TOC)%>
# World, entities, components

Before you read this page, you should read
[What is SpatialOS?]({{urlRoot}}/shared/concepts/spatialos)

## The SpatialOS world

In a game running on SpatialOS, the **game world** is a core concept. By this we mean the SpatialOS
world: the canonical source of truth about things in your game.

## Entities and components

Entities are the objects in your game. All of the data that you want to share between worker
instances has to be stored in entities. Each entity is made up of components; it's the components which store this data, in their properties.

<%(Callout type="tip" message="
* Entities in SpatialOS approximate to Actors in Unreal.
* Components’ properties in SpatialOS approximate to replicated properties in Unreal
 ")%>

For example, in a world with rabbits and lettuces, you'd have `Rabbit` entities and `Lettuce` entities, each with certain components. These components in turn would have certain properties:

![Entities example]({{assetRoot}}assets/shared/concepts/component-details.png)

## Why is this necessary?

The reason for having this separately-defined world is to store the state of the game world in
such a way that many instances of server-workers and client-workers can access and change it, without needing to
communicate with each other.

A major reason to use SpatialOS is to exceed those limits: instead of one server looking after the
server-side of the game world, SpatialOS coordinates multiple programs (server-worker instances) to do that.