# Editor console

The editor exposes editing actions through the engine console. The Console panel,
batch scripts, and undo history all use the same command bindings. Start with the
[editor build guide](building.md#editor) for setup and the
[editor architecture](architecture.md#the-editor) for the design.

The editor registers its own families, such as `EditorSelection.*` and
`EditorObject.*`, with `mainConsoleObjects`. The engine's separate `Editor.*`
family belongs to shared `ModuleToolObject` code and remains a recovery frontier;
these editor-only bindings do not implement it.

## Run an edit

With a level open, select a live object, move it one metre upward, and undo the move:

```text
EditorSelection.select 1234
EditorSelection.translate 0/1/0
EditorHistory.undo
```

Replace `1234` with an existing object ID. The console can also resolve object
references written as a template name or `template:n`, where `n` is the zero-based
live root object of that template in ID order. Consult the individual binding
before assuming a command accepts a template reference instead of a numeric ID.

Mutation commands generally return empty text on success. Queries and some edits
return useful output on success, so nonempty output alone is not an error in the
Console panel. Refusals report a reason, including engine parsing errors, unknown
commands, and incorrect argument counts.

For batch operation, `+runConFileAndClose <script>` runs a console file.
Requested `+save...AndClose` operations run after the script. Check the editor's
`+help` output for supported save switches.

## Understand undo

Undo entries contain console scripts. For example, a transform replay activates
an object and sets its full matrix:

```text
EditorObject.activate 1234
Object.absoluteTransformation [1/0/0/0][0/1/0/0][0/0/1/0][10/20/30/1]
```

The undo script stores the old matrix; the redo script stores the new one. Using a
full matrix preserves scale as well as position and rotation. Activation sets
the engine's `Object.*` cursor; failed activation clears it.

History replay uses `runConsoleScript`, which stops on the first editor error or
any nonempty console answer. Replay scripts must therefore use silent setters,
not queries. The interactive runner, `runConsoleLine`, permits successful output.

An action that is neither an editor command nor a property query has an unknown
effect on history. The Console panel and batch runner clear history for such
actions so stale undo entries cannot be replayed. An editor command's presence
in the catalogue does not itself promise undo support: the notes below identify
operations that write immediately or use dedicated replay methods.

## Command catalogue

This catalogue groups commands by workflow. The service interface is declared in
[EditorConsole.h](../Code/Tools/BF2Editor/Framework/EditorConsole.h); the bindings in
[EditorConsole.cpp](../Code/Tools/BF2Editor/Framework/EditorConsole.cpp) are the
current source for exact signatures and additional commands.

These blocks describe syntax, rather than complete scripts. Angle brackets name
arguments; `|` separates alternatives or methods in the same family. After the
first qualified name, a short method name keeps that family unless another
qualified name appears. Parentheses describe behavior. Use quotes for string
arguments containing spaces.

### Objects and selection

Selection commands operate on the open edit session. Pixel coordinates refer to the editor window; vectors use slash-separated world coordinates. Rotations use yaw/pitch/roll in degrees.

```text
EditorObject.activate <id>            the engine's active object ("Object.*" cursor) := object #id

EditorSelection.clear | select <id> | add <id> | pickAt <x> <y> | pickAdd <x> <y> |
                selectTemplate <name> | info

EditorSelection.translate <x/y/z> | rotate <yaw/pitch/roll> | setPosition <x/y/z> |
                setRotation <yaw/pitch/roll>       (undoable; degrees as Object.rotation)

EditorSelection.setLayer <n>                       (undoable: Object.layer)

EditorObject.create <template> <x/y/z> <yaw/pitch/roll> <layer>   (undoable)

EditorObject.placeAt <template> <x> <y>   (undoable: made on the surface under window pixel x, y)

EditorSelection.delete | duplicate | snapToGround                  (undoable)

EditorSelection.box | boxAdd | boxToggle <x0> <y0> <x1> <y1>      (a rectangle of window pixels)

EditorSelection.replace <template> | copyTransform | pasteTransform | snapToGrid <step>

EditorClipboard.copy | cut | paste | pasteAt <x/y/z>

EditorSelection.setProperty <group | disableChildren | notInAI | isOvergrowth | lightSourceMask |
                             controlPointId> <value>

EditorSelection.groupNew | ungroup | selectGroup   (Object.group; the first two undoable)

EditorSelection.anchorInfo | alignStamp | align | raiseByAnchor   (anchor points; the last two
                undoable) | setAnchor <x/y/z> | anchorToBox <bottom|origin|corner0..7> | resetAnchor
                (the template's anchor, undoable) and EditorGizmo.pivot <0 object | 1 anchor> |
                anchorMode <0|1> (the move gizmo moves the anchor)
```

### History and replay

Use the user-facing edit commands to create history entries. Replay methods such as `recreate`, `remove`, `applyChange`, and `applyEdit` are used by stored history scripts.

```text
EditorHistory.undo | redo | clear

EditorObject.recreate <id> <template> <matrix> <layer> | remove <id>   (the history's replays)
```

### Camera, gizmo, and panels

These commands control navigation and the editor shell. The mixed host-command row retains fully qualified names where the command family changes.

```text
EditorCamera.frameSelection

EditorGizmo.mode <0 select | 1 move | 2 rotate> | space <0 world | 1 local>

EditorView.markers <0|1> | combatAreas <0|1>       (the gameplay markers and combat areas)

EditorView.panel <name> <0|1>                      (Log, Engine, Resources, Selection, Objects,
                                                     History, Console, Layers, Profiler)

EditorCamera.mode <0 free | 1 top-down | 2 orbit | 3 follow the terrain> | setPosition <x/y/z> |
             resetPosition | setSpeed <m/s> | resetSpeed | info | addBookmark | toggleBookmark |
             bookmark <1|2> | bookmarks   (the retail host's two camera position bookmarks)

EditorClipboard.pasteInPlace | EditorCamera.fromSelection | EditorLevel.snapAllToGround | saveLists |
             levels | open <level> | EditorView.pauseUpdate <0|1> | levelViewDistance <0|1>   (additional host commands:
             the host's commands the catalogue left out)

EditorTools.customCommands | setCustomCommand <1..3> <line> | setCustomFile <1..3> <file> |
             runCustomCommand <1..3> | runCustomFile <1..3>   (the retail host's custom commands)
```

### Levels and layers

Saving writes level data; packing produces the level archives for a game installation. `saveAs` copies a saved level under a new name and can open the copy. Layer visibility and locking control the editor view.

```text
EditorLayer.hide <n> | show <n> | lock <n> | unlock <n> | select <n>   (the editor's view)

EditorLevel.status | save                          (the level's object files)

EditorLevel.pack <mod folder>                      (the level's client.zip / server.zip for the game)

EditorLevel.saveAs <name> <open 0|1>               (the saved level copied as Levels/<name>, opened after)

EditorLevel.layers | switchLayer <mode> <size> | duplicateLayer <mode> <size>

EditorGameplayLayers.overview | compare <mode> <size> | compareNone   (every layer's control points and
               combat areas; another layer shown beside the open one)

EditorLevelSettings.list <group|all> | get <key> | set <key> <value> | spawnCameraFromView
               (undoable; apply <key> <value> | unset <key> are the replays)

EditorLayer.status | list | add <name> | rename <id> <name> | filters <id> <"modes"> <"players"> | disable <id>
             <0|1> | active <id> | remove <id> [move|delete]   (the retail layer model: Editor/Layer.con and
             Editor/GamePlayObjects.con, the game-mode files made from them at save; applyLayers the replay)
```

### Gameplay and sound

Gameplay commands create or edit level-owned templates, combat areas, ambient sounds, and supply relationships. The catalogue identifies the lower-level methods used for undo replay.

```text
EditorTemplate.set <template> <property> <value> | apply <..same..> | remove <template> <key> |
               lines <template>                    (templates the level files define)

EditorGameplay.create <kind> <name> <x/y/z> | createHere <kind> <name> |
               createEffectArea <name> <effect> <x/y/z> | createEffectAreaHere <name> <effect>
               (undoable: a spawner, spawn point, control point, trigger or effect area
               with a template of its own) | rename <template> <new name>   (undoable; the template
               and its objects - applyRename <from> <to> is the replay)

EditorCombatArea.list | info <name> | create <name> <x/y/z> <size> | createHere <name> <size> |
               delete <name> | movePoint <name> <index> <x> <z> | insertPoint <name> <index> <x> <z> |
               removePoint <name> <index> | set <name> <team|vehicles|layer|usedByPathFinding|inverted>
               <n> | manager <use|timeAllowedOutside|damage> <value> | select <name> <point> |
               deselect     (undoable edits; sync <name> | syncManager are the replays)

EditorSound.list | info <name> | move <name> <x/y/z> | create <name> <file> <x/y/z> |
               createHere <name> <file> | delete <name> | select <name> | deselect
               (undoable edits; apply <name> <x/y/z> | remove <name> | restore <name> are
               the replays)

EditorGameplay.supplyGroups | groupControlPoints | ungroupControlPoints | connectSupply <team> <supply> <group> |
             disconnectSupply <group>   (supply lines and control-point groups on the selected control points'
             templates; undoable, applyGroup / applySupply the replays)
```

### Object templates, materials, and animation

These tools edit their corresponding template, material, animation, or wizard data. Use each domain's save command where listed; individual operations identify their replay commands.

```text
EditorMaterial.status | list | info <id> | cells <id> | cell <a> <b> | set <id> <property> <value> |
             add <name> | addAs <name> <id> <type> | remove <id> | addCell <a> <b> | removeCell <a> <b> |
             setDamageMod <a> <b> <value> | setTemplate <a> <b> <effect|decal|sound> <index> <name|""> |
             save | extract   (the material editor: Common/Material's two files in place and the engine
             live; undoable, applyEdit <id> <before|after> the replay)

EditorAnimation.systems | open <path> | status | tree | list <kind> | select <kind> <name> | selected |
             set <property> <value> | unset <property> | setSystem <property> <value> | unsetSystem <property> |
             addTrigger <type> <name> | addBundle <name> | addValueHolder <name> <"min normal max"> |
             addAnimation <.baf> | addEvent <name> | removeAnimation <.baf> | removeEvent <name> |
             setStart <animation|event> <name> <seconds> | setLength <.baf> <seconds> | link <child|bundle>
             <name> | unlink <child|bundle> <name> | delete | timeline [bundle] | joints <folder> [list] |
             copySystems <template> <source template> | point <template> <1p|3p|seat> <path> | save
             (the animation editor: the .inc files in place, on the selected object; undoable, applyEdit
             <id> <before|after> the replay)

EditorObjectTemplate.files <template> | lines <template> | engineValue <template> <property> |
                set <template> <property[#index]> <value> |
                remove <template> <property[#index]> | children <template> | addChild <template> <child>
                <x/y/z> | moveChild <template> <child[#n]> <x/y/z> <yaw/pitch/roll> | removeChild
                <template> <child[#n]> | components <template> | componentClasses | addComponent
                <template> <class> | removeComponent <template> <class> | show <template> | new <class>
                <name> [folder] | clone <template> <name> [folder] | newChild <parent> <class> <name>
                <x/y/z> | inherit <template> <source> | rename <template> <new name> | changeType
                <template> <class> | fixes <template> (a destroyable object's missing networkable info
                or Armor, as the retail object editor offered them) | save
                (the object editor: in place in the
                template's .tweak / .con; set and remove are undoable, applyEdit <id> <before|after> the replay)

EditorWizard.load | rules | run <category> | items | tick <index|"label"> <0|1> | execute | prompt |
                answer <text> | cancel | check <row> <0|1> | checklistOk  (the object wizard: the retail rule
                files over the selected object's tree; resolutions edit the template files in place)
```

### Terrain and vegetation

Terrain strokes and vegetation painting have dedicated replay records. The commands below distinguish world coordinates from sector indices and serialized configuration.

```text
EditorTerrain.info | height <x> <z> | brush <mode|size|strength|raise|lower|height> <value> |
              stroke <x> <z> <ticks> | strokeSecondary <x> <z> <ticks> | tool <0|1> |
              exportHeightmaps <directory> | sectors | sector <x> <z> <0|1> |
              importSurrounding <raw file> | smoothEdges <width> <amplitude> <falloff> | seaLevel <h>
              (applyChange <id> <before|after>, applySector <x> <z> <0|1> and
              applySeaLevel <h> are the replays)

EditorTerrain.view | technique <n>   (the editable terrain's look: 0 colormap and lightmap, 1 grid,
              2 topographic grid, 3 detail layers, 4 undergrowth overlay, 5 overgrowth materials,
              6 overgrowth shadow, 7 base pass; 5 and 6 need a level with overgrowth)

EditorTerrain.brush mode overgrowth | brush overgrowth <material|shadow> | brush material <1..15> |
              brush shadow <0..255>   (the overgrowth brush: a stroke paints the level's overgrowth
              material or shadow map, the other button erases. Not undoable; the plants change when
              the overgrowth is generated, and saving the painted maps is not wired yet: a level save
              reports them NOT saved)

EditorTerrain.showSector <x> <z> <0|1> | shownSectors   (the surrounding sectors drawn around the
              primary tile, x and z in -1..1; every sector with a tile is shown when the level opens.
              The view only: whether a sector is saved is EditorTerrain.sector)

EditorTerrain.brush mode paint | brush paints <detail|color|both> | brush layer <0..5> |
              brush colorSource <texture|color> | brush color <r/g/b>   (the paint brush; a stroke
              paints the renderer's edit maps) | merge <detail|color|both|-> |
              clearEdits <detail|color|both|->   (keep or drop the strokes made since the last merge;
              - takes the brush's own setting. The level's save writes the painted maps into its
              Editor folder; the game keeps drawing its compiled maps until a terrain compile,
              which this version cannot do yet)

EditorGround.layers | set <layer> <property> <value> | misc <property> <value> | removeLayer <layer>
              (Editor/GroundProperties.con, saved with the level; undoable, apply <layer> <property>
              <value> is the replay. Layer properties: detailTexture, colorTexture, color <r/g/b>,
              material, topTiling, sideTiling <x/y>, yOffset, planeMap <0|1>, colorTiling <x/y>,
              lowDetailType, envMap <0|1>; misc, or layer -1: farTopTilingHi, farTopTilingLow,
              farSideTiling <x/y>, farYOffset, lodSwitchDistance. The material and the far settings
              are written to the file but not yet applied to the renderer. Painting colour from a
              layer's texture, and erasing colour, need the layer's colour texture)

EditorUndergrowth.info | tool <0|1> | material <id> | radius <m> | paint <x> <z> | erase <x> <z>
              (the density map, Undergrowth.raw; applyChange <id> <before|after> is the replay) |
              setting <key> <value> | materialHeight <id> <value> | type <id> <type> <key> <value>
              (Undergrowth.cfg; applyConfig <id> <before|after> is the replay)
```

Both of the following deliberately differ from the retail editor, which loses a stock level's
layers on a terrain save. When a level has no `Editor/GroundProperties.con`, the ground
properties start from the compiled terrain's layers in `terraindata.raw`. Retail started with
none, and its terrain save then wrote a `terraindata.raw` without layer names. When a level has
no editor detail maps, the editor converts the level's compiled detail maps into its staging
folder. Retail started from default maps that put one layer everywhere. Saving painted maps keeps
the heightmap's materials (sounds, effects and grip) as the level's own file has them; retail's
save rebuilt them from the layers' materials, which zeroed a stock level's. The level's own
files change only when it is saved.

### Play testing

Soldier mode walks the open level as the local player. The soldier, its kit and any vehicle it
drives are never part of the level. The client's game logic runs, so nothing takes damage or
respawns.

```text
EditorPlayTest.soldier | leave | info | kit <team> <kit>   (a soldier of the level's kit where the
              camera is; leave returns to the editor camera at the soldier's eyes)

EditorPlayTest.drive | driveTemplate <template>   (the selected object spawner's vehicle, or a vehicle
              made in front of the soldier, with the soldier as the driver) |
              hold <input> <value> <steps>   (holds a player input for a number of steps; for scripts)
```

### Libraries, bundles, and brushes

Bundles and brushes live in the configured editor library. Brush definition edits write their file immediately; placing bundles, stamping, and erasing create undoable edits.

```text
EditorLibrary.info | folder <path | default>       (where bundles and brushes are kept)

EditorBundle.list | info <name> | save <name> | saveOver <name> | place <name> <x/y/z> <yaw> |
             placeHere <name> <yaw> | group <0|1> | delete <name>      (placing is undoable)

EditorBrush.list | info <name> | select <name> | define <name> | addSelection | addItem <template> |
            set <key> <value> | item <index> <key> <value> | removeItem <index> | delete <name> |
            seed <n> | tool <0|1> | eraseRadius <m> | stamp <x/y/z> | erase <x/y/z>
            (brush edits write the brush's file at once; each stamp or erase is undoable)
```

### Lighting and effects

Light presets apply through undoable level settings. Lighting generation writes its output immediately. Effect previews are temporary objects and are never saved with the level.

```text
EditorLight.presets | apply <preset> | save <name> | resetToLevel | sunFromCamera <1|-1> |
             flareFromCamera <1|-1>   (the retail host's light presets, applied as undoable level
             settings; the sun or flare direction from the camera's view)

EditorLighting.groundHemi <resolution> <x/y/z> <size> | minimap <resolution> | envMaps  (the
                ground hemi map, the editor's minimap picture, the environment maps; written at once)

EditorEffect.preview <template> | play | stop | pause | clear | info  (the object editor's effect mode:
                a preview made at the view's centre, never saved; play/stop/pause act on the selected
                effect objects, else on the preview)
```

### Mod and object archives

These commands manage the mod's loose objects and archives. Opening another mod restarts the editor on that mod.

```text
EditorObjects.status | extract | pack <mod folder>  (the mod's object archives and its loose Objects folder)

EditorMod.list | create <name> | open <name>     (the mod manager; open starts the editor again on the mod)
```

## Add or maintain a binding

`IEditorCommands` is the open level's service interface, implemented by the edit
session. `registerEditorConsole` borrows that implementation and registers the
tagged bindings with the engine registry. Keep the implementation alive until
`unregisterEditorConsole`; unregister removes the bindings and clears the session
pointer. Test input, play testing, and shell panel callbacks have separate owners.

Keep command parsing and console registration in `EditorConsole.cpp`, and put
editing behavior in the owning session or module. Report refusals through the
supplied result/error text. Preserve the distinction between an ordinary command
that returns a label or query result and a silent replay command.

For an undoable operation, record scripts that restore the exact before and after
state. Validate both replay directions and failure reporting. Commands that
write files immediately should say so in their documentation and UI. Editor
output must remain loadable by the retail game, as described in the editor
architecture.

This interface is editor-only and engine-dependent. It is not part of the
engine-independent `bf2_editor_checks` target.
