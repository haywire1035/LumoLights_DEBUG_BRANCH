# AGENT WORKFLOW GUIDANCE

## Scope
These instructions apply to the entire repository unless a nested `AGENTS.md` overrides specific sections.

## Repository Etiquette
- Read `_CHANGELOG.h`, version defines, and any request-specific notes before editing.
- Favor `rg`, `fd`, or `ls` for discovery; avoid recursive `ls -R`/`grep -R` to keep output manageable.
- Keep commits minimal, readable, and justified by the request. Never introduce unrequested changes.

## Coding & Documentation Standards
- Maintain clarity: straightforward logic, descriptive names, and minimal side effects per function.
- Place API/core routines before helpers within each translation unit and declare prototypes at the top.
- When editing documentation, mirror the existing tone and structure in that file.

## Versioning & Changelog
- Obey the bespoke `SKETCH_VERSION`, `CONFIG_VERSION`, and `_CHANGELOG.h` policies described in the repository and user instructions. Every code change must increment the proper version(s) exactly once and log the update with a summary.
- When creating the pull request for github, always begin the commit message with the current SKETCH_VERSION

## Testing & Verification
- Run only the tests relevant to touched areas; capture command output for reporting. If no automated tests exist, note that explicitly in the final summary.

## PR & Reporting
- Summaries must reference changed files and cite relevant paths/commands when responding to questions.
- Keep final responses in English unless the request explicitly asks for another language.
