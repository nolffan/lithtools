# Bugs in the retail tools

Bugs in the Talon-era tools as they shipped (AvP2 v1.0's and Tools
Update 2's, NOLF Tools 001's), found while rebuilding them. Each is a
mistake in the code that a source change would fix. This is a living list:
add a bug when it is found, in the format below, whether or not it is
fixed yet.

Our sources reproduce every one of them, on purpose: compiling to the
retail machine code means compiling its mistakes too. The tools are to be
re-implemented later with these fixes applied (unlike the games, which fix
theirs in a modern build of the same sources); this list is what that work
starts from. An entry that is fixed then says so under its heading, with
where the fix is.

Every binary here is a reconstruction (`docs/PLAN.md`), so an entry is
read off the retail bytes at the address it names, never guessed, and says
how well our function matches the retail one there (`tools/exematch.py`,
`tools/fndiff.py`): identical, or near with the bug reproduced. Addresses
are as in the retail images (`build/reference.sh` extracts them into
`out/reference/`).

## Format

```
### <What goes wrong, as a level designer or a programmer would put it>

<binary> <address> (<function>, <source file>; identical or near). What the
code does, and what it should do; how it shows, if it does.

Not fixed yet.
```

## The bugs

### The Processor's ray selection count covers only the last world model

AvP2's Processor.exe 0x4450c0 (DoRaySelect,
src/LT2/tools/PreProcessor/RaySelecter.cpp; near: size-exact, one
multiply's operand order differs). Each RaySelecter object casts a ray at
every world model's polies and selects the brush of the last poly it hits.
The count of selections is cleared at the start of each world model
(0x4450e7) instead of once before the loop, so the function returns how
many rays selected something in the last world model only. Every selection
is still made; only the count is wrong, and it goes no further than the
log's "RaySelect: Did %d ray selections" (Processing, 0x440400).

Not fixed yet.

### Merging terrain surfaces skips a poly after each one it takes out

AvP2's Processor.exe 0x445b80 (MergeSurfaces_RS, the name ours,
src/LT2/tools/PreProcessor/RaySelecter.cpp; near: the same instructions,
one register spilled). MakeTerrain calls it (0x446714) to give polies with
the same surface one surface: the polies are hashed into 512 lists, and
each list's poly at i is taken out (0x445c67) and compared with the polies
left. Taking poly i out moves the next one into slot i, and the loop then
goes on to i + 1, so that poly is never compared as the first of a group.
Polies with the same surface can keep surfaces of their own: the terrain
keeps more surfaces than it needs. Nothing breaks.

Not fixed yet.
