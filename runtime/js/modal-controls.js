// SPDX-License-Identifier: AGPL-3.0-only
// Controller focus for upstream pointer-only modals. Reuses their existing
// controls/callbacks; never reads input values or implements authentication.
(function (g) {
  'use strict';
  const rt = g.__rt;
  // game/src/enums/buttons.ts (review alongside the private modal UI fields).
  const UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, SUBMIT = 4, ACTION = 5, CANCEL = 6;
  const states = new WeakMap();
  let scene, current;

  function visible(object) {
    if (!object) return false;
    for (let node = object; node; node = node.parentContainer)
      if (node.visible === false || node.alpha === 0 || node.active === false) return false;
    return true;
  }

  function items(handler) {
    const result = [];
    (handler.inputs || []).forEach((input, index) => {
      const container = handler.inputContainers?.[index];
      if (visible(container) && !input.node?.readOnly && !input.node?.disabled) {
        result.push({ input, node: input.node, visual: container.list?.[0],
          run: () => input.setFocus() });
      }
    });
    (handler.buttonBgs || []).forEach(button => {
      if (visible(button) && button.input?.enabled !== false)
        result.push({ visual: button, run: () => button.emit('pointerdown') });
    });
    return result;
  }

  function paint(state, controls) {
    state.index = Math.max(0, Math.min(state.index, controls.length - 1));
    controls.forEach((control, index) => {
      if (index === state.index) control.visual?.setTint?.(0xbbbbbb);
      else control.visual?.clearTint?.();
    });
  }

  function ready(handler) {
    return handler === scene.ui.getHandler() && handler.active &&
      !handler.blockInput && !scene.ui.overlayActive && visible(handler.modalContainer) &&
      scene.tweens.getTweensOf(handler.modalContainer).length === 0;
  }

  function attach(handler) {
    let state = states.get(handler);
    if (state) return state;
    state = { index: 0 };
    states.set(handler, state);
    const original = handler.processInput;
    handler.processInput = function (button) {
      // The official UI still owns overlays, tutorials, forms and validation.
      if (!ready(this)) return false;
      const controls = items(this);
      if (!controls.length) return original.call(this, button);
      if (button === UP || button === DOWN || button === LEFT || button === RIGHT) {
        state.index = (state.index + (button === UP || button === LEFT ? -1 : 1) + controls.length) % controls.length;
        paint(state, controls);
        return true;
      }
      if (button === CANCEL && typeof this.cancelAction === 'function') {
        this.cancelAction();
        return true;
      }
      // Start/submit retains the official form's submit behavior. A activates
      // the focused field/button, including Login after password entry.
      if (button === SUBMIT && this.inputs?.length) return original.call(this, button);
      if (button === ACTION || button === SUBMIT) {
        paint(state, controls);
        controls[state.index].run();
        return true;
      }
      return original.call(this, button);
    };
    return state;
  }

  // Only DOM element identities reach this hook, never username/password text.
  rt.onFormTextComplete = (element, next, accepted) => {
    if (!scene || !current || scene.ui.getHandler() !== current) return;
    const controls = items(current), state = states.get(current);
    if (!state) return;
    const target = controls.findIndex(control => control.node === (accepted && next ? next : element));
    if (accepted && !next) {
      const firstButton = controls.findIndex(control => !control.input);
      if (firstButton >= 0) state.index = firstButton;
    } else if (target >= 0) state.index = target;
    paint(state, controls);
  };

  rt.frameHooks.push(() => {
    const battle = rt.renderer?.game?.scene.getScene('battle');
    if (!battle?.ui) return;
    scene = battle;
    const handler = scene.ui.getHandler();
    if (!handler?.buttonBgs?.length || !visible(handler.modalContainer) || !handler.active) {
      current = null;
      return;
    }
    const state = attach(handler);
    if (current !== handler) {
      state.index = 0;
      current = handler;
      paint(state, items(handler));
    }
  });
})(globalThis);
