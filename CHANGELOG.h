V01.03.12
// Removed unused CONSOLE configuration persistence and bumped CONFIG_VERSION to V01.09.
// Removed unused SYSTEM LED_COUNT/CONFIRM console paths and kept SYSTEM RESET restart handling.

V01.03.11
// Fixed HAL include scoping so Adafruit symbols resolve correctly and restored the Mirror instance to satisfy linker references.

V01.03.10
// Added WS2801 single/dual HAL profiles, aligned the dual WS2812 config, and documented the new options.

V01.03.09
// Added standalone CONFIRM flow for SYSTEM LED_COUNT and introduced SYSTEM RESET command with restart options.

V01.03.08
// Added SYSTEM LED_COUNT placeholder command with confirmation and restart countdown.
// Replaced HAL profiles with simple compile-time configs, added config labels, and documented the new defines.

V01.03.08
// Added configurable HAL profiles, multi-strip hardware support, and moved LED_COUNT management into the HAL layer.

V01.03.07
// Moved effect enablement to TOGGLE EFFECT, removed APPLY console command/docs, and improved settings save logging.

V01.03.06
// Simplified MAIN bridge to push mirror changes immediately and added RGB/RGBW toggle for HSL conversion.

V01.03.05
// Added MAIN mirror translators to sync console updates with CORE config and HomeSpan.

V01.03.04
// Move Mirror to MAIN

V01.03.03
// Some Changes to general architecture.
// reverted implementation from CODEX as they do not work. Inserted ol' trustworthy Implementation from STLv2

V01.03.02
// added homeSpan.poll() back to the loop, to activate HomeSpan activities

V01.03.01
// Fixed LED facade aliases and gradient exports so non-linker files stay decoupled while compiling cleanly.
// bumped Version number to 01.03. with the introduction of MAIN Scaffolding and Implementation of DEVICE

V01.02.39
// Added LED facade wrappers, decoupled CONSOLE/SETTINGS, and introduced MAIN bridge scaffolding.

V01.02.38
// Added DEVICE_EXAMPLE bridge showing two-way HomeSpan/LED synchronization.

V01.02.37
// Added DEVICE namespace with HomeSpan-ready RGB light services and state tracking.

V01.02.36
// Added root-level AGENTS.md describing contributor workflow expectations.

V01.02.35
// Adjusted SETTINGS save logs and ensured staging color updates trigger persistence.

V01.02.34
// Added effect amplitude, evolve/hold range, and enable switches to SET PARAM.

V01.02.33
// Added hold logic to effect()
// added some config items for effect function

V01.02.32
// Changed Name of MiddleInterpolation to InterpolationMode
// Shiften ApplyMiddleInterpolation function into Compute Gradient as Lambda function

V01.02.31
// Adjusted ApplyOutputScaling to clamp scaled colors before applying brightness and on/off factors.

V01.02.30
// Added ShiftScaleChannel helper to mirror legacy register shifting behavior for individual scale channels.
// Removed double-speed logic from ShiftScaleChannel and now reject invalid channel indices.

V01.02.29
// Added Effect Function

V01.02.28
// Added a comprehensive README covering features, setup, console usage, and persistence workflow.

V01.02.27
// Added numeric SET PARAM entries for LED fade increments and timing intervals.

V01.02.26
// added smooth on/off fade factor with console toggle support
// moved gradient inversion toggle to new TOGGLE command and extended help text

V01.02.00
// changed versioning, major releases now increment the second block
// lowest block are to be changed per-upload
// added LINEAR_PADDED as Gradient Mode
// added CONFIG_VERSION as a means to track config changes. old Config is invalid, when CONFIG_VERSION is changed, SKETCH_VERSION does not anymore caus invalid saved configs
// added LED::CORE::Fade() function to fade the Vars towards the staging variables in config struct
// added LOG, but didn't fully implemented it


V01.01.06
// known bug: color reverts back to something else after being set by console
// added LED::CORE::MarkConfig() function 
// added "SAVE"- command to the console
// Version might not compile without errors as there were changes to the LED::CORE::config


V01.01.05
// fixed circular include problem zwischen 200_CONSOLE und 300_SETTINGS
// implemented full config persistance (saving of settings) - not 100% tested!
// Added more branches to the Console (SET PARAM, SET COLOR, HELP) and improved help
// AutoApply logic works


V01.01.04
// deleted some of the old files that were no longer neccessary, renamed others
// Implemented 300_Settings, which saves Config to the EEPROM, currently not working!
// changed version number to be format Vxx.yy.zz


V1.1.3
// Working Example with LED control
// Implemented Namespaces and Structures in LED LINKER and LED CORE
// Working Update, ComputeGradient, LED::SET
// Command Line Interface working
// started Changelog