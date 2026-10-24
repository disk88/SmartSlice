# SmartSlice AI v0.1 — Windows local AI smoke test

Branch: `dev/smartslice-v0.1`. This experimental code does **not** change slicing settings, G-code or printer commands.

## What is implemented

The desktop **SmartSlice AI → Test local AI...** menu opens a wxWidgets dialog that:
- Queries `http://127.0.0.1:11434/api/tags` to discover locally installed Ollama models.
- Sends a user-authored prompt to `http://127.0.0.1:11434/api/generate` with `stream: false`.
- Displays the local model's response, without transmitting model data to an online AI provider.
- Handles missing Ollama, missing models, HTTP errors, and unexpected responses without applying print settings.

No AI model is included in the Orca-derived application. The planned release installer may offer **opt-in first-run setup** for Ollama and a model after confirming download sizes, supported OS, licenses and user approval. Never silently install a model or bundle the Ollama binaries without a license/packaging review.

## Windows developer setup and test

1. Follow the upstream OrcaSlicer [build instructions](https://github.com/OrcaSlicer/OrcaSlicer) for the fork's exact base revision; compile `Release` on Windows. The build is not yet verified for this branch.
2. Install Ollama from [ollama.com/download/windows](https://ollama.com/download/windows).
3. In PowerShell or Command Prompt, run `ollama pull qwen3:4b` (illustrative starting model; verify RAM/VRAM availability and model compatibility before relying on it).
4. Verify `http://127.0.0.1:11434/api/tags` responds, then start the compiled application.
5. Open **SmartSlice AI → Test local AI...**, click **Refresh**, choose an available model and ask a test question.
6. Confirm that closing the dialog during a request does not cause a crash; repeat with Ollama stopped and with zero downloaded models.
7. Independently confirm existing OrcaSlicer STL import, slice and export functions are unchanged.

## Limitations and next tasks

- Code has been pushed but **not compiled, packaged or tested on Windows**.
- AI response is plain advice only. There is no model geometry context, preset validation, print settings writing or learned calibration.
- User needs to install Ollama and choose/download a model separately for this initial developer test.
- Next: compile, fix any compiler issues, establish repeatable Windows CI, validate packaging and app data separation, then add typed AI parameter proposals with safety validation.
- Preserve AGPL-3.0 notices and third-party credits. The fork's application branding and persistence directories have not yet been separated from Orca.
