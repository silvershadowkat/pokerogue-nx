# Debug menu navigation and setting timing

The local Debug overlay now retains its selected category and each category's
selected setting plus exact scroll offset. Applying or cancelling a value returns
to the same setting. Back returns to the previously selected category, and closing
and reopening Debug Tools retains the position for that UI session. Title-screen
and active-run positions are independent; UI replacement/relaunch clears them.
No cursor preferences are written into saves or the Online/Offline profiles.

The generic option handler gains opt-in initialScrollCursor/onCursorChange hooks.
Restoration clamps bounds, skips disabled rows, and clears intermediate repeat
state so returning across a disabled row cannot repeat the next action. Existing
menus without those hooks keep their navigation behavior.

The migrated settings metadata marks 60 Starter Points and Allow Duplicate
Starters as requireReload. Both show (restart required), are gray/unselectable
while a run has a party, and reject direct handler invocation in that state.
Change these at the title screen; saving a value shows a reminder to restart
Debug mode before starting a run. The client never automatically restarts or
abandons a run. The metadata remains conservative even though their consumers
read overrides dynamically; rebuilding starter UI/state is the safe boundary.

Starting Level and All Starters Pokerus are labeled (new run), with help explaining
that current team members are unchanged. Their consumers initialize new starters;
other cheat hooks read settings during subsequent gameplay actions. No existing
Pokemon, rule hooks, random generation or save formats are changed by this UI fix.

Regression coverage exercises title/run Back, Cancel, Apply, close/reopen,
selected-category and scrolled-row restoration, disabled-row input repeat, and
restart/new-run labels and guards. Debug gameplay and assets remain pinned to
the reviewed official revision. The update is a small Debug overlay only; the
0.3.0-beta.3 native APK/NRO and stock base archive are unchanged.
