# D2MOO Context Memory (LOD)

Date de mise a jour: 2026-04-10
Workspace: c:\src\D2MOO
Scope: Diablo II LOD 1.10f/1.14d (32-bit), pas D2R.

## 1) Orientation rapide

- D2MOO est une reconstruction C++ du moteur D2 LOD (D2Common, D2Game, D2Client, Fog, etc.).
- Le coeur generation de niveau est DRLG (rooms, levels, presets, warps).
- Unite de position gameplay: subtile (1 tile = 5 subtiles).
- Priorite de verite pour signatures: code D2MOO > heuristiques decompilation.
- La reconstruction de D2Client est en cours, les autres DLL sont déjà bien entammées

## 2) Arborescence utile

- source/D2Common/: logique partagee (DRLG, tables, path, units)
- source/D2Game/: logique serveur (quests, spawn, AI)
- source/D2Client/: rendu client/automap
- D2.Detours.patches/: mapping des patches par DLL/version
- automap_outdoor_line.cpp: utilitaires custom outdoor/warp/quest POI
- automap_ds1_warp_scan.cpp/.h: scan DS1 a la demande pour rooms dwScan=0

## 3) Coordonnees (a ne pas melanger)

- Tile: unite DRLG (rooms, lvl defs, vertices)
- Subtile: unite gameplay (positions d'unites)
- Client: cX = 16*(sX-sY), cY = 8*(sX+sY)
- Plus d'information dans doc/CoordinateSystems.md et doc/CoordinateSystems_FAQ.md
- Minimap: mapX = cX/10 + 1, mapY = cY/10 - 3
	- En tile direct: mapX = 8*(tX-tY)+1, mapY = 4*(tX+tY)-3

## 4) DRLG: structures et invariants

Chaine principale:

- D2DrlgActStrc
- D2DrlgStrc
- D2DrlgLevelStrc
- D2DrlgRoomStrc

Types de room (D2DrlgRoomStrc::nType):

- DRLGTYPE_MAZE (1)
- DRLGTYPE_PRESET (2)
- DRLGTYPE_OUTDOOR (3)

Point critique:

- pMaze et pOutdoor partagent la meme union.
- Pour DRLGTYPE_MAZE/outdoor, lire pMaze->pMap sans garde peut crash.
- Acces safe pour pMap: verifier room PRESET + pointeurs non nuls.

Pattern robuste:

```cpp
if (pRoom->nType != DRLGTYPE_PRESET || !pRoom->pMaze || !pRoom->pMaze->pMap)
		continue;
```

## 5) Cycle de vie des preset units (source majeure de bugs)

Build time:

- DRLGPRESET_BuildPresetArea peut charger DS1 puis copier des units dans pMap->pPresetUnit.
- Ces coordonnees dans pMap sont ABSOLUES en subtile.

Activation room:

- DRLGACTIVATE_RoomEx_EnsureHasRoom -> DRLGPRESET_InitPresetRoomGrids.
- Les units sont migrees de pMap->pPresetUnit vers pRoom->pPresetUnits.
- Dans pRoom->pPresetUnits, les coordonnees deviennent ROOM-RELATIVE.

Consequence pratique:

- Chercher d'abord pRoom->pPresetUnits (actif) puis pMap->pPresetUnit (non actif).
- Reconstituer absolu depuis room-relative: + 5 * nTileXPos/Y.

## 6) Outdoor + vertices + warps

- pLevel->pOutdoors contient grilles, dimensions et vertices de sorties.
- D2DrlgVertexStrc stocke positions en tile + direction cardinale.
- Act 1/5 utilisent plus souvent les vertices de chemins; Act 2/3/4 peuvent avoir nVertices=0.

Warp system:

- pDrlg->pWarp est la source fiable des connexions level->level (vis + warp arrays).
- Attention: DRLGROOMFLAG_HAS_WARP_* signifie surtout passages a pied (warp[i] == -1),
	pas tous les portals.
- Pour identifier un portal DS1: tile type sortie + style index correspondant au slot vis/warp.

## 7) Quest/special POI notes

- Stony Field: cairn alpha stone via preset lookup (OBJECT_STONEALPHA).
- Arcane Sanctuary: room summoner via LVLPREST_*, position souvent centre de room.
- Act 5 Anya: marqueurs town/outside town distincts (OBJECT_DREHYA_START_*).

## 8) Patching/Detours (comportement runtime)

- Les DLL reconstruites sont compilees avec exports/ordinals alignes (.def).
- D2.DetoursLauncher injecte/route les appels vers patches.
- Les actions d'ordinals sont dans D2.Detours.patches/<version>/<DLL>.patch.cpp.
- Pour verifier un comportement en jeu, regarder la map de patch ordinal avant de conclure.

## 9) Workflow reverse/implementation valide

Regles de travail:

- Decompiler en IDA d'abord, puis coder 1:1 logique.
- Si decompilation ambigue, revenir a l'asm pour lever les doutes.
- Pour appel vers D2Common/D2Game/etc., preferer signatures D2MOO.
- Si callee inconnu:
	- le reconstruire si simple,
	- sinon shell + fallback original via FunctionReplacePatchByOriginal.

Mapping adresses (indispensable):

- RVA = VA_D2MOO - ImageBase_DLL
- VA_IDA = Base_IDA + RVA

Valeurs verifiees dans cette base:

- D2Common base IDA effective: 0x09B10000
- Exemple DRLG_GetLevel:
	- VA D2MOO: 0x6FD749A0
	- ImageBase D2Common: 0x6FD40000
	- RVA: 0x349A0
	- VA IDA: 0x09B449A0

## 10) Fonctions et fichiers pivots

Fonctions importantes:

- DRLG_GetLevel
- DRLGACTIVATE_RoomEx_EnsureHasRoom
- DRLGPRESET_AddPresetUnitToDrlgMap
- DRLGPRESET_InitPresetRoomGrids
- DRLGPRESET_LoadDrlgFile / DRLGPRESET_FreeDrlgFile

Fichiers de reference:

- source/D2Common/include/Drlg/D2DrlgDrlg.h
- source/D2Common/include/Drlg/D2DrlgPreset.h
- source/D2Common/include/Drlg/D2DrlgOutdoors.h
- source/D2Common/src/Drlg/DrlgPreset.cpp
- source/D2Common/src/Drlg/DrlgActivate.cpp
- source/D2Common/src/Drlg/DrlgRoomTile.cpp
- source/D2Game/src/QUESTS/Quests.cpp
- source/D2Game/src/QUESTS/ACT5/A5Q3.cpp

## 11) Gotchas a garder en tete

- Ne jamais dereferencer pMaze->pMap sans filtrer nType.
- pRoom->pPresetUnits != pMap->pPresetUnit (room-relative vs absolu).
- Les rooms dwScan=0 peuvent avoir pMap->pFile = null avant activation.
- pRoomData outdoor peut etre incomplet selon acte/phase; preferer pDrlg->pWarp.

## 12) Convention de session (memoire persistante)

- Ce repo est traite comme D2 LOD 32-bit uniquement.
- D2R peut inspirer des patterns (automap/AVL), mais ne doit pas etre confondu avec la logique LOD.
- Si un assert/reverse revele un path source original, privilegier une organisation de fichiers D2MOO qui le reflete.
