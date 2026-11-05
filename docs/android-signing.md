# Android release signing

The maintainer explicitly authorized a new Android signing identity for
0.3.0-beta.1 after the previous private key could not be located. Package ID
remains `com.silvershadow.pokeroguenx`; app name is **PokeRogue**.

Public certificate SHA-256:
`d9ead44887d9cf27ae29c24a19ddae6ea97ee912c20204c6e95d370513a8a5a1`

RC7 and earlier used a different certificate. Android requires one
uninstall/reinstall to move to this identity. That uninstall removes private app
data; users who want to retain local saves must export them first. The maintainer
requested a fresh installation for this test device. Future releases must keep
the new key so normal in-place updates work.

The maintainer's private primary directory is
`C:/Users/plim/Documents/Codex/Secrets/PokeRogue`; a byte-verified backup is at
`E:/Codex-Secrets/PokeRogue`. Both are outside Git and restricted to the current
user and SYSTEM. They contain the JKS, password file and recovery notes. Never
upload them to a release, issue, chat log or source repository. Keep an additional
offline backup separately if these drives belong to the same PC.

`runtime/scripts/package_android.py` uses that primary location, or an explicit
`PKR_SIGNING_DIR`. It requires an existing key/password and validates the public
certificate against `runtime/android/release-signing.json` before and after
packaging. It never creates replacement keys automatically. Use JDK 21 and the
documented Android SDK dependencies.

`--test-staging` temporarily builds a debuggable manifest for staging game files
on a controlled test device. It is not a release artifact. Final APKs must use
the normal manifest with `android:debuggable="false"`. `--test-signing` is a
separate quarantined test identity and must not be used for distribution.
