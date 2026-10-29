import { defineConfig, type ConfigEnv, type UserConfig } from "vite";
import officialConfig from "../vite.config";

export default defineConfig(async (env:ConfigEnv) => {
  const config=await (officialConfig as (env:ConfigEnv)=>Promise<UserConfig>)(env);
  // Static files are packed once by build.py. Avoid writing 30k copies to dist
  // on every web-only edit; all other official build plugins remain enabled.
  return {...config,
    // Official app mode enables offline save imports without enabling dev tools.
    // Keep native/handheld environment files for platform-specific build settings.
    define:{...config.define,"import.meta.env.MODE":JSON.stringify("app")},
    build:{...config.build,sourcemap:false},
    plugins:(config.plugins as any[]).filter(p=>p.name!=="minify-public-json-files")};
});
