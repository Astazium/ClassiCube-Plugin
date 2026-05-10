# ClassiCube-Plugin
A couple of useful (or not) features for [ClassiCube](https://github.com/ClassiCube/ClassiCube) in one plugin!
## Components:
|  Name  | Description |
|--------|-------------|
| [AntiAFK](#antiafk)       | Rotates player with specified interval in seconds         |
| [ArtBuilder](#artbuilder) | Builds an png image from blocks with specified parameters |

## AntiAFK
| Command | Description |
|---------|-------------|
| /client AntiAFK [true/false] | Turn AntiAFK on/off                             |
| /client AntiAFK [float value in seconds] | Sets rotating interval (in seconds) |

## ArtBuilder
| Command | Description |
|---------|-------------|
| /client ArtBuilder build [path to png] [posX posY posZ] [degX degY] | Builds an [path to png] from blocks at [posX posY posZ] and degree [degX degY]. [path to png] must be relative to ClassiCube executable |
| /client ArtBuilder [parameter] [value]                              | Sets [parameter] to [value] |

### Multiplayer mode
ArtBuilder, like other features from this plugin, fully supports multiplayer!

For multiplayer, ArtBuilder provides some customizable parameters to control how ArtBuilder would build your image.

| Parameter | Description |
|-----------|-------------|
| /client ArtBuilder build stop                             | Stops building process (only for multiplayer mode!)             |
| /client ArtBuilder build eta                              | Prints remaining time of building                               |
| /client ArtBuilder [multiplayer/mp] [true/false]          | Turn multiplayer mode on/off, sets automaticly                  |
| /client ArtBuilder exitOnFinish [true/false]              | Turn exit game on finish on/off                                 |
| /client ArtBuilder [teleportRange/tpRange] [int]          | Distance in blocks before teleport to current building position |
| /client ArtBuilder placeInterval [float value in seconds] | How often (in seconds) blocks will be placed                    |

## Setting up for Visual Studio
1. Open console and clone ClassiCube repo: `git clone https://github.com/ClassiCube/ClassiCube.git`
2. Go to ClassiCube directory: `cd ClassiCube`
3. Clone ClassiCube-Plugin repo: `git clone https://github.com/Astazium/ClassiCube-Plugin.git`
4. Open ClassiCube solution in Visual Studio and create new project in it
6. Open your project properties and in "Configuration Type" choose "Dynamic Library (.dll)". Don`t forget to change it for all configurations!
7. Click on project, Add -> Existing Item, go to ClassiCube-Plugin directory and select all files
