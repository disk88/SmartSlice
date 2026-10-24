# SmartSlice AI — Project checkpoint 01

**Date:** 2026-10-24  
**Repository:** `disk88/SmartSlice` (fork of `OrcaSlicer/OrcaSlicer`)  
**Working branch:** `dev/smartslice-v0.1`  
**Pilot machine:** Anycubic Kobra 3 V2  
**Status:** Architecture baseline documented. No successful custom build, printer connection, or live camera integration has yet been verified.

## Product objective

Build an independent desktop-first FDM slicer, derived from OrcaSlicer, with an AI-assisted "simple mode" that transforms high-level goals into validated slicer settings. Keep an expert mode for full control. Prioritize quality, safety, explainability, and reversible configuration changes.

Initial user choices: nozzle, material/color, infill target, desired detail, and optional areas requiring reinforcement. AI proposes supports, orientation, walls, speed, temperature within confirmed machine/material limits, and local modifiers.

## Selected source strategy

- **Base:** current OrcaSlicer fork, preserving its slicing engine, 3D viewer, printer profiles, and upstream license notices.
- **Reference only:** public `ANYCUBIC-3D/AnycubicSlicerNext` sources. Port individual Anycubic-related changes only after comparing provenance, version, dependencies, applicability, and licensing.
- **No wholesale file copying** between different versions of the slicer.
- **Licensing:** derived project remains subject to AGPL-3.0 and applicable third-party notices. Review logos, service terms, and redistributable components separately.

## Technical layout (proposed)

1. **Slicer adapter:** safe mapping from typed AI settings proposals into existing Orca configuration/presets, with validation and a human-readable diff.
2. **AI provider adapter:** local-model support as default option; optional user-configured online provider. Never commit credentials, camera URLs with tokens, or printer access codes.
3. **Calibration service:** repeatable, parameter-isolated calibration prints; images and user measurements; before/after comparisons; versioned profiles with user approval before saving.
4. **Vision service:** camera access (subject to Kobra 3 V2 firmware and API verification), OpenCV-based camera/bed calibration, image capture, defect detection and confidence levels.
5. **Device integration:** discover transport and monitor capabilities before introducing automatic printing/pausing or camera access.
6. **UI:** desktop fork first, mobile companion only after core workflows stabilize.

## Safety constraints

- No model-written G-code sent directly to the printer without established validation.
- Validate settings against the selected machine/nozzle/material limits; reject out-of-range proposals.
- Do not auto-save printer/filament profiles without user confirmation.
- Camera-only observations cannot establish precise dimensional accuracy or mechanical strength.
- For uncertain defects, notify rather than auto-pause; test automatic actions separately.
- Preserve original presets and provide undo/restore functions.

## V0.1 milestones

1. **Baseline build:** compile the existing Orca fork on Windows, record toolchain and build instructions.
2. **Branding separation:** distinct app name/identity while preserving attribution, upstream tracking, and license information.
3. **AI Settings Proposal MVP:** an initial deterministic rules/validation layer plus a replaceable AI provider; show proposed differences to the user.
4. **First pilot workflow:** import a test STL, select Kobra 3 V2 and filament, choose quality and infill, apply a proposal, produce G-code and compare against an unmodified Orca baseline.
5. **Hardware verification:** user prints a supervised test and reports outcomes.

## Deferred until proven

- Anycubic camera feed and authenticated network control.
- Automatic camera calibration for a moving Y-bed.
- Autonomous calibration adjustments and printer commands.
- Local strength-region painting and physical strength validation.
- Mobile slicing directly on phone.

## Repository workflow

- Keep `main` at known-good revisions and use feature branches / PRs.
- Track the upstream Orca base revision explicitly.
- Add regression tests for any changed preset and G-code functionality.
- Never modify `disk88/trading-desk`.

## Next engineering task

Identify and document the exact Orca build/toolchain steps and configuration API integration points, then compile an unchanged Windows baseline before modifying slicer behavior.

**Resume key:** `SMARTSLICE-AI-01`
