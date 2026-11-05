// SPDX-License-Identifier: AGPL-3.0-only
// Keep exports made by our Debug build out of the stock Offline profile.
// This is provenance checking, not protection against deliberately edited saves.
(function (g) {
  "use strict";
  const rt = g.__rt, N = g.__native;
  const saveKey = "x0i2O7WRiANTqPmZ"; // Official public export format, not an account secret.
  rt.saveImportError = function (bytes) {
    if (N.clientMode === "online") return "Save import is unavailable in official online mode.";
    if (N.clientMode !== "offline") return null;
    try {
      let text = N.utf8Decode(bytes).replace(/^\uFEFF/, "").trim();
      if (text.startsWith("U2FsdGVkX1")) {
        const decoded = N.aesDecrypt(text, saveKey);
        if (!decoded || typeof decoded[0] !== "string") return null;
        text = decoded[0];
      }
      const value = JSON.parse(text);
      const stack = [value];
      while (stack.length) {
        const node = stack.pop();
        if (!node || typeof node !== "object") continue;
        if (node.silverShadowDebug === true)
          return "This save was created in Debug mode. Import it in Debug, not stock Offline.";
        for (const item of Object.values(node)) if (item && typeof item === "object") stack.push(item);
      }
    } catch (_) { /* The game's own importer reports unsupported/corrupt files. */ }
    return null;
  };
})(globalThis);
