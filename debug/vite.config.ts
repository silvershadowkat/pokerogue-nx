import {defineConfig} from "vite";
import official from "./vite.config";
export default defineConfig(async env=>{
 const config=await (official as any)(env);
 return {...config,define:{...config.define,"import.meta.env.MODE":JSON.stringify("app")},
  build:{...config.build,sourcemap:false},
  plugins:config.plugins.filter((p:any)=>p.name!=="minify-public-json-files")};
});
