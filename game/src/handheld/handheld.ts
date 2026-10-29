// SPDX-License-Identifier: AGPL-3.0-only
// Handheld presentation adapter. Battle rules and progression remain upstream.
import "./handheld.css";
import { globalScene } from "#app/global-scene";
import { Button } from "#enums/buttons";
import { UiMode } from "#enums/ui-mode";

// Private upstream UI fields are deliberately confined to this adapter. The
// source pin and UI smoke tests must be reviewed when those fields change.
type View = any;
type Region = { label:string; x:number; y:number; w:number; h:number; run:()=>void };
const host = window as any;

function visible(obj:View):boolean {
  if (!obj) return false;
  for (let p=obj; p; p=p.parentContainer) if (p.visible === false || p.alpha === 0) return false;
  return true;
}
function rect(obj:View):{x:number;y:number;width:number;height:number}|null {
  if (!visible(obj) || !obj.getBounds) return null;
  const r=obj.getBounds();
  return r.width>0 && r.height>0 ? r : null;
}
function send(button:Button) {
  if (!globalScene?.ui) return;
  globalScene.game.events.emit("input_down", {button, controller_type:"keyboard", isTouch:true});
  globalScene.game.events.emit("input_up", {button, controller_type:"keyboard", isTouch:true});
}

function regions():Region[] {
  const ui=globalScene?.ui;
  if (!ui) return [];
  const h:View=ui.getHandler();
  if (!h || !h.active || h.blockInput || (ui as View).overlayActive) return [];
  const result:Region[]=[];
  const add=(obj:View,label:string,run:()=>void,pad=12)=>{
    const r=rect(obj); if (!r) return;
    result.push({label,x:r.x-pad,y:r.y-pad,w:r.width+pad*2,h:r.height+pad*2,run});
  };
  const select=(index:number)=>{h.setCursor(index); ui.processInput(Button.ACTION);};
  if (h.tutorialActive) return result;
  h.tabMenu?.headerTitles?.forEach((o:View,i:number)=>add(o,o.text,()=>{
    h.tabMenu.setIndex(i);h.tabMenu.onChangeCallback(i);
  },8));
  if (visible(h.optionSelectContainer) && h.options?.length) {
    const bg=rect(h.optionSelectBg);
    if(bg) {
      const step=114*h.scale-3;
      for(let i=0;i<Math.min(h.maxOptions,h.options.length-h.scrollCursor);i++) {
        const item=h.options[h.scrollCursor+i];
        if(!item?.selectable) continue;
        const matrix=h.optionSelectBg.getWorldTransformMatrix();
        const p=matrix.transformPoint(-h.optionSelectBg.displayWidth+8,-h.optionSelectBg.displayHeight+5+step*i);
        const q=matrix.transformPoint(-8,-h.optionSelectBg.displayHeight+5+step*(i+1));
        result.push({label:String(item.label).replace(/\[.*?\]/g,""),x:p.x,y:p.y,w:q.x-p.x,h:q.y-p.y,run:()=>select(i)});
      }
    }
    return result;
  }
  if (ui.mode===UiMode.COMMAND || ui.mode===UiMode.FIGHT) {
    const container=h.commandsContainer ?? h.movesContainer;
    const texts=container?.list.filter((o:View)=>typeof o.text==="string") ?? [];
    texts.forEach((o:View,i:number)=>add(o,o.text,()=>select(i),20));
    if(ui.mode===UiMode.COMMAND) add(h.teraButton,"Terastallize",()=>select(4));
  } else if(ui.mode===UiMode.STARTER_SELECT) {
    if(!h.filterMode) {
      h.starterContainers?.forEach((o:View,i:number)=>add(o.icon ?? o, o.species?.name ?? `Starter ${i+1}`,()=>{
        h.startCursorObj.setVisible(false);h.randomCursorObj.setVisible(false);h.partyCursorObj.setVisible(false);
        h.setFilterMode(false);select(i);
      },7));
    }
    h.filterBar?.labels?.forEach((o:View,i:number)=>add(o,o.text,()=>{
      h.startCursorObj.setVisible(false);h.randomCursorObj.setVisible(false);h.partyCursorObj.setVisible(false);
      h.setFilterMode(true);h.setCursor(i);ui.processInput(Button.ACTION);
    },6));
    if(h.filterMode && h.filterBar.openDropDown) {
      h.filterBar.dropDowns?.forEach((d:View)=>{
        if(!visible(d))return;
        d.options?.forEach((o:View,i:number)=>add(o,o.text?.text ?? `Filter ${i+1}`,()=>{
          d.setCursor(i);ui.processInput(Button.ACTION);
        },6));
      });
    }
  } else if(ui.mode===UiMode.BALL) {
    h.pokeballImages?.forEach((o:View,i:number)=>{
      const r=rect(o);if(!r)return;const bg=rect(h.pokeballSelectBg);
      result.push({label:`Ball ${i+1}`,x:r.x-12,y:r.y-18,w:bg?bg.width-24:200,h:r.height+36,run:()=>select(i)});
    });
    add(h.cancelText,"Cancel",()=>send(Button.CANCEL));
  } else if(ui.mode===UiMode.TARGET_SELECT) {
    h.targets?.forEach((i:number)=>add(globalScene.getField()[i],`Target ${i+1}`,()=>select(i),30));
  } else if(ui.mode===UiMode.PARTY) {
    if(visible(h.optionsContainer) && h.options?.length) {
      const bg=rect(h.optionsBg);
      if(bg) h.options.forEach((o:View,i:number)=>{
        const height=96;
        result.push({label:`Party option ${i+1}`,x:bg.x,y:bg.y+bg.height-height*(h.options.length-i)-24,w:bg.width,h:height,
          run:()=>{h.setCursor(i);ui.processInput(Button.ACTION);}});
      });
    } else h.partySlots?.forEach((o:View,i:number)=>add(o,`Pokémon ${i+1}`,()=>select(i),0));
  } else if(ui.mode===UiMode.SAVE_SLOT) {
    h.sessionSlots?.forEach((o:View,i:number)=>add(o,`Save ${i+1}`,()=>select(i),0));
  } else if(ui.mode===UiMode.MODIFIER_SELECT) {
    const choose=(row:number,i:number)=>{h.setRowCursor(row);select(i);};
    h.options?.forEach((o:View,i:number)=>add(o.itemContainer ?? o,o.modifierTypeOption.type.name,()=>choose(1,i),22));
    h.shopOptionsRows?.forEach((row:View[],r:number)=>row.forEach((o:View,i:number)=>
      add(o.itemContainer ?? o,o.modifierTypeOption.type.name,()=>choose(h.shopOptionsRows.length-r+1,i),18)));
    [h.rerollButtonContainer,h.lockRarityButtonContainer,h.transferButtonContainer,h.checkButtonContainer].forEach((o,i)=>add(o,["Reroll","Lock rarity","Manage items","Check team"][i],()=>choose(0,i)));
    add(h.continueButtonContainer,"Continue",()=>choose(1,0));
  } else if(h.optionValueLabels && h.settingLabels) {
    h.optionValueLabels.forEach((row:View[],r:number)=>row.forEach((o:View,i:number)=>
      add(o,o.text,()=>{h.setCursor(r-h.scrollCursor);h.setOptionCursor(r,i,true);},10)));
    h.settingLabels.forEach((o:View,i:number)=>add(o,o.text,()=>h.setCursor(i-h.scrollCursor),5));
  }
  return result;
}

export function installHandheld(game:Phaser.Game) {
  const bar=document.createElement("nav");bar.id="handheld-bar";bar.setAttribute("aria-label","Game actions");
  document.body.appendChild(bar);
  let signature="";
  let down:{x:number;y:number;mode:number}|null=null;
  const canvas=game.canvas;
  const point=(e:PointerEvent)=>{const b=canvas.getBoundingClientRect();return{x:(e.clientX-b.left)*canvas.width/b.width,y:(e.clientY-b.top)*canvas.height/b.height};};
  canvas.addEventListener("pointerdown",e=>{const p=point(e);down={...p,mode:globalScene?.ui?.mode};});
  canvas.addEventListener("pointerup",e=>{
    if(!down || !globalScene?.ui) return;
    const p=point(e),start=down;down=null;
    if(start.mode!==globalScene.ui.mode) return;
    const dx=p.x-start.x,dy=p.y-start.y;
    if(Math.hypot(dx,dy)>70){send(Math.abs(dx)>Math.abs(dy)?(dx>0?Button.LEFT:Button.RIGHT):(dy>0?Button.UP:Button.DOWN));return;}
    const h:View=globalScene.ui.getHandler();
    if(h.tutorialActive){globalScene.ui.processInput(Button.ACTION);return;}
    const hit=regions().find(r=>p.x>=r.x&&p.x<=r.x+r.w&&p.y>=r.y&&p.y<=r.y+r.h);
    if(hit){hit.run();return;}
    if([UiMode.MESSAGE,UiMode.EVOLUTION_SCENE,UiMode.EGG_HATCH_SCENE].includes(globalScene.ui.mode)) send(Button.ACTION);
  });
  canvas.addEventListener("pointercancel",()=>{down=null;});
  const render=()=>{
    if(!globalScene?.ui)return;
    const mode=globalScene.ui.mode,h:View=globalScene.ui.getHandler();
    const key=`${mode}:${h?.filterMode}:${h?.tutorialActive}`;
    if(key===signature)return; signature=key;
    bar.replaceChildren();
    const status=document.createElement("span");status.id="handheld-status";status.textContent="OFFLINE · PokéRogue";bar.append(status);
    const button=(label:string,action:()=>void,primary=false)=>{const b=document.createElement("button");b.textContent=label;b.className=primary?"primary":"";b.onclick=action;bar.append(b);};
    button("Menu",()=>send(Button.MENU));button("Back",()=>send(Button.CANCEL));
    if(mode===UiMode.STARTER_SELECT){
      button("Start run",()=>send(Button.SUBMIT),true);button("Filters",()=>send(Button.STATS));
      if(h.filterMode){button("Choose",()=>send(Button.ACTION));}
    } else if([UiMode.MESSAGE,UiMode.EVOLUTION_SCENE,UiMode.EGG_HATCH_SCENE].includes(mode)||h.tutorialActive){
      button("Continue",()=>send(Button.ACTION),true);
    } else if([UiMode.COMMAND,UiMode.FIGHT,UiMode.TARGET_SELECT].includes(mode)){
      button("Info",()=>{const w=globalScene as View;w.processInfoButton(!w.__handheldInfo);w.__handheldInfo=!w.__handheldInfo;});
    } else {
      button("Select",()=>send(Button.ACTION),true);
      // Named paging is useful in long menus and summary screens without a
      // simulated touchscreen controller.
      button("Previous",()=>send(Button.UP));button("Next",()=>send(Button.DOWN));
      if([UiMode.SUMMARY,UiMode.SETTINGS_GENERAL,UiMode.SETTINGS_AUDIO,UiMode.SETTINGS_DISPLAY,UiMode.SETTINGS_GAMEPAD,UiMode.SETTINGS_KEYBOARD].includes(mode)){
        button("Page ‹",()=>send(Button.LEFT));button("Page ›",()=>send(Button.RIGHT));
      }
    }
  };
  const timer=window.setInterval(render,150);
  game.events.once("destroy",()=>{clearInterval(timer);bar.remove();});
  // Read-only diagnostics plus normal user-input entry points, no game cheats.
  host.handheld={regions:()=>regions().map(({run,...r})=>r),input:send,
    inspect:()=>({loop:{frame:game.loop.frame,paused:game.isPaused,running:game.loop.running,pending:(game as View).pendingDestroy,delta:game.loop.delta},
      scenes:game.scene.scenes.map(s=>({key:s.sys.settings.key,status:s.sys.settings.status,visible:s.sys.settings.visible})),
      scene:globalScene?.sys?.settings,camera:{alpha:globalScene?.cameras?.main?.alpha,visible:globalScene?.cameras?.main?.visible,width:globalScene?.cameras?.main?.width,height:globalScene?.cameras?.main?.height,zoom:globalScene?.cameras?.main?.zoom},
      ui:{visible:globalScene?.ui?.visible,alpha:globalScene?.ui?.alpha,scale:globalScene?.uiContainer?.scale,
        message:(globalScene?.ui?.getCurrentMessageHandler() as View)?.message?.text,
        active:globalScene?.ui?.getHandler()?.active,tutorial:(globalScene?.ui?.getHandler() as View)?.tutorialActive},
      renderer:{lost:(game.renderer as View).gl?.isContextLost(),drawCount:(game.renderer as View).drawCount},children:globalScene?.children?.list?.map((o:View)=>({type:o.type,name:o.name,visible:o.visible,alpha:o.alpha,x:o.x,y:o.y,flags:o.renderFlags}))}),
    state:()=>({mode:globalScene?.ui?.mode,phase:globalScene?.phaseManager?.getCurrentPhase()?.constructor.name,
      wave:globalScene?.currentBattle?.waveIndex,cursor:globalScene?.ui?.getHandler()?.getCursor(),fps:game.loop.actualFps})};
  host.handheldBack=()=>send(Button.CANCEL);
  host.handheldPause=()=>{game.sound.pauseAll();(game.sound as any).context?.suspend();};
  host.handheldResume=()=>{(game.sound as any).context?.resume();game.sound.resumeAll();};
  const blobUrls=new Map<string,Blob>();
  const create=URL.createObjectURL.bind(URL),revoke=URL.revokeObjectURL.bind(URL);
  URL.createObjectURL=(obj:Blob|MediaSource)=>{const url=create(obj);if(obj instanceof Blob)blobUrls.set(url,obj);return url;};
  URL.revokeObjectURL=(url:string)=>{blobUrls.delete(url);revoke(url);};
  const click=HTMLAnchorElement.prototype.click;
  HTMLAnchorElement.prototype.click=function(){
    const blob=blobUrls.get(this.href);
    if(host.Android && this.download && blob){const reader=new FileReader();reader.onload=()=>host.Android.exportFile(this.download,String(reader.result).split(",")[1]);reader.readAsDataURL(blob);}
    else click.call(this);
  };
}
