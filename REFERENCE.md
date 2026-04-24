# AI Agent Reference Guide: Skyrim Plugin Development

This document serves as a roadmap for AI agents working on this SKSE plugin. Before implementing features, you should familiarize yourself with the following three resources located in the project root.

---

## 1. CommonLibSSE-NG
**Path:** `CommonLibSSE-NG/`

### What it is
The modern, comprehensive C++ library for Skyrim Special Edition/Anniversary Edition. It contains reverse-engineered headers for almost every game class and provides a type-safe way to interact with the game engine.

### Where to look
- **Game Classes:** `include/RE/`
  - Organized by the first letter of the class (e.g., `RE/A/Actor.h`, `RE/T/TESForm.h`).
  - **Pro-tip:** Start with `include/RE/Skyrim.h` for a broad overview of included headers.
- **SKSE API:** `include/SKSE/`
  - Use this for logging (`SKSE/log.h`), messaging between plugins (`SKSE/Interfaces.h`), and serialization.
- **Offsets & RTTI:** `include/RE/Offsets_RTTI.h` and `include/RE/Offsets_VTABLE.h`.
  - Used for dynamic casting and finding functions in memory across different game versions.

### Troubleshooting
- If a class member seems missing, check if it's named differently in the `RE` namespace compared to old SKSE headers.
- Use `CommonLibSSE-NG` as your **primary** source for code implementation.

---

## 2. skse64
**Path:** `skse64/`

### What it is
The original source code for the Skyrim Script Extender. While `CommonLibSSE-NG` is better for plugin development, the original `skse64` source is the "ground truth" for how SKSE itself works.

### Where to look
- **Internal Logic:** `skse64/skse64/`
  - `PluginAPI.h`: The low-level interface SKSE provides to plugins.
  - `Papyrus*.h/cpp`: Examples of how to expose new C++ functions to the Papyrus scripting language.
  - `Game*.h`: Older definitions of game classes. Use these for comparison if `CommonLib` is ambiguous.

### Troubleshooting
- Use this if you need to understand the **lifecycle** of a plugin (loading, initialization sequence) or how SKSE handles binary serialization (`Serialization.h`).

---

## 3. skyrim_mod (Tutorials & Analysis)
**Path:** `skyrim_mod/`

### What it is
A collection of local analysis notes and tutorials specifically curated for this project.

### Where to look
- **Tutorials:** `skyrim_mod/tutorial/`
  - Look for files like `Systems_20_Dynamic_Form_Creation.md` for high-level implementation logic (e.g., how to safely create items or spells at runtime without bloating save files).
- **Project Context:** Any `.md` files here explain the *why* and *how* behind specific architectural choices in this mod.

### Troubleshooting
- **READ THESE FIRST.** Before diving into raw headers, check if there is a tutorial for the system you are trying to implement. It will save you from common pitfalls like "Save Bloat."

---

## General Workflow for Agents
1. **Identify the Goal:** (e.g., "Add a new shout").
2. **Consult `skyrim_mod/`:** Check if there's a tutorial for Shouts or Dynamic Forms.
3. **Search `CommonLibSSE-NG/include/RE/`:** Find the relevant classes (e.g., `TESShout`, `SpellItem`).
4. **Reference `skse64/`:** If you need to see how SKSE handles specific data types or Papyrus bindings.
5. **Verify with `src/util.h`:** This project has many local helpers. Check if a helper already exists for what you're doing.
