# BF2Rebuilt

BF2Rebuilt reconstructs the Battlefield 2 1.50 engine from the shipped Windows and Linux
binaries. The goal is a stable dedicated server that an unmodified retail client can join,
play on, reconnect to, and follow through a map reload. The repository also contains a
client reconstruction, native 64-bit server ports, and a new level editor.

This is an independent reconstruction in progress. Parts of the engine remain incomplete;
successful compilation or a diagnostic executable does not establish a playable game.

## Start here

- [Getting started](docs/getting-started.md): toolchains, build targets, and runtime setup.
- [Engine architecture](docs/architecture.md): image boundaries, source layout, and lifecycle.
- [Development guide](docs/development.md): recovery evidence, code conventions, and validation.
- [Documentation index](docs/README.md): public engineering guides and contribution workflow.

The default Windows build produces the `bf2_recovered` static archive. The `msvc-x86`
configuration is the retail-layout reference; `msvc-x64` and `linux-x64` are native server
ports with their own object layouts. Game content is loaded from a separate Battlefield 2
installation. Keep rebuilt binaries and reference binaries in different directories.

## Contributing

Read [AGENTS.md](AGENTS.md) before changing the tree; it contains the repository's working
rules and validation requirements. Reconstructed code defaults to the observed engine behavior;
record uncertain boundaries and validate the image you changed. Keep concise contracts and
important quirks beside the code. Engineering guides live under [docs](docs/README.md).

Non-retail fixes, improvements and features are welcome only when a linked issue already
contains discussion of the proposed approach before implementation begins. This includes
editor-only work. AI may assist with these changes. Link the issue in the change description
or documentation, and in source comments where needed to explain an intentional departure.
Clearly distinguish the new behavior from recovered code.

Agents working on recovered engine implementations must have IDA or Ghidra connected and
accessible with the relevant original Battlefield 2 1.50 binaries loaded. If the tool or
required evidence is missing, stop that recovery work rather than guess. Intentional changes
can follow the issue-discussed design, with original-binary verification where needed for
affected behavior and compatibility contracts.

Agents must not create or submit pull requests or reply to reviewer questions. The human
author must fully understand every submitted code change, its evidence and its validation.
The human submits the pull request, answers review questions and takes responsibility for
the contribution. See the [development guide](docs/development.md) for the full workflow.
