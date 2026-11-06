# 0.3.0-beta.2 input fixes

Long-list paging now counts either scrolling or cursor movement as a successful
selection move. Previously, a page could scroll while the highlight stayed on
the same visible row and the handler returned false, suppressing its normal
selection sound. A clamped press at the true end of the list still does not move
or play an extra selection sound. The shared Debug handler covers Daily/date and
editor lists; Online remains the official game.

Touch pad Always/Fade now exclusively owns game touch gestures, including taps
outside the buttons and while fully faded. Touch pad Off enables direct game
menu/move taps, regardless of an older independent directtouch preference. The
redundant Tap menus toggle is removed. Physical controller input, neutral capture,
continuous thumb sliding, contextual buttons and native dialogs/keyboards remain
on their existing paths.

The Debug pack version changes to `ss-0.3.0-beta.2-f045bef`; its stock asset base
and official gameplay revision remain unchanged. App identity/signing stay the
same; Android versionCode is 11. An in-place update preserves local profiles.

Checks: 91 Debug editor/seeded-run tests, TypeScript and Vite pass. The native
headless GLES overlay regression passes uninterrupted 30/60/120 Hz thumb motion,
zero D-pad texture uploads, capture/release, faded/always input swallowing and
Off-only direct touch. Hardware verification and packaging receipts are retained
separately with the candidate artifacts.

Heavy build/source intermediates can be placed off C: using
`PKR_ANDROID_BUILD_DIR` for both Android build and packaging, and
`PKR_SWITCH_BUILD_DIR` for the Switch shell build. Defaults remain unchanged.
These paths hold generated files only; private signing keys stay in their
existing protected location. A separate source checkout for `build_debug.py`
may also live on E:. Remove disposable build trees after delivery verification.

The separate [4:3 investigation](aspect-ratio-investigation.md) does not change
the shipped aspect ratio in this input update.
