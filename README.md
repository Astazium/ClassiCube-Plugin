# ClassiCube-Plugin
A couple of useful (or not) features for ClassiCube in one plugin!
## Components:
|  Name  | Description |
|--------|-------------|
| [AntiAFK](#antiafk)       | Rotates player in specified amount of time                |
| [ArtBuilder](#artbuilder) | Builds an png image from blocks with specified parameters |

## AntiAFK
| Command | Description |
|---------|-------------|
| /client AntiAFK [true/false] | Turn AntiAFK on/off                |
| /client AntiAFK [float value in seconds] | Sets rotating interval |

## ArtBuilder
| Command | Description |
|---------|-------------|
| /client ArtBuilder build [path to png] [posX posY posZ] [dirX dirY] | Builds an [path to png] from blocks at [posX posY posZ] and [dirX dirY] |
| /client ArtBuilder [parameter] [value]                              | Sets [parameter] to [value]                                             |

### Multiplayer mode
ArtBuilder, like other features from this plugin, fully supports multiplayer!

For multiplayer, ArtBuilder provides some customizable parameters to control how ArtBuilder would build your image

| Parameter | Description |
|-----------|-------------|
| /client ArtBuilder build stop                             | Stops building process (only for multiplayer mode!)             |
| /client ArtBuilder [multiplayer/mp] [true/false]          | Turn multiplayer mode on/off, sets automaticly                  |
| /client ArtBuilder [teleportRange/tpRange] [int]          | Distance in blocks before teleport to current building position |
| /client ArtBuilder placeInterval [float value in seconds] | How often blocks will be placed                                 |
