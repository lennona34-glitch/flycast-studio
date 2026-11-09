/*
	Copyright 2026 Flycast Modding & VR Studio

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "gui_quest_vr.h"
#include "gui.h"
#include "gui_util.h"
#include "settings.h"
#include "IconsFontAwesome6.h"
#include "cfg/option.h"
#include "oslib/oslib.h"
#include "oslib/i18n.h"
#include "emulator.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "input/gamepad_device.h"
#include "input/gamepad.h"
#include "input/mapping.h"
#include "input/mouse.h"
#include "hw/maple/maple_cfg.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>

using namespace i18n;

#if defined(_WIN32) && !defined(TARGET_UWP)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

#if defined(USE_SDL)
extern HWND getNativeHwnd();
#endif
#endif

namespace
{
static const char* kQuestWebXRAppHtml1 = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Flycast Dreamcast — Meta Quest VR & Mixed Reality Studio</title>
<style>
  :root {
    --bg: #080b12;
    --panel: rgba(16, 22, 36, 0.95);
    --accent: #ff6b00;
    --accent2: #00a8ff;
    --text: #f1f5f9;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
  html, body {
    width: 100vw; height: 100vh;
    position: fixed; top: 0; left: 0;
    overflow: hidden !important;
    background: radial-gradient(circle at top, #141e33, #060911 75%);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    touch-action: none;
    overscroll-behavior: none;
  }
  body {
    display: flex;
    flex-direction: column;
    align-items: center;
  }
  header {
    width: 100%;
    padding: 10px 18px;
    background: rgba(8, 12, 22, 0.92);
    border-bottom: 1px solid rgba(59, 130, 246, 0.40);
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    align-items: center;
    gap: 8px;
    backdrop-filter: blur(10px);
    z-index: 10;
  }
  .brand {
    display: flex;
    align-items: center;
    gap: 10px;
    font-weight: 800;
    font-size: 1.05rem;
    letter-spacing: 0.4px;
    color: #60a5fa;
  }
  .badge {
    background: linear-gradient(135deg, #2563eb, #38bdf8);
    color: white;
    font-size: 0.70rem;
    padding: 3px 8px;
    border-radius: 999px;
    font-weight: 700;
  }
  .ctrl-badge {
    background: rgba(16, 185, 129, 0.18);
    border: 1px solid rgba(16, 185, 129, 0.5);
    color: #34d399;
    padding: 3px 8px;
    border-radius: 6px;
    font-size: 0.75rem;
    font-weight: 600;
  }
  .stats {
    font-size: 0.82rem;
    color: #94a3b8;
    font-variant-numeric: tabular-nums;
  }
  .toolbar {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    justify-content: center;
    align-items: center;
    padding: 8px 14px;
    background: var(--panel);
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 12px;
    margin: 8px 12px 6px;
    max-width: 1240px;
    width: calc(100% - 24px);
    z-index: 10;
  }
  button, select {
    background: #1b263b;
    color: var(--text);
    border: 1px solid rgba(59, 130, 246, 0.45);
    padding: 8px 14px;
    border-radius: 8px;
    font-weight: 600;
    font-size: 0.85rem;
    cursor: pointer;
    transition: all 0.15s ease;
  }
  button:hover { background: #263859; border-color: var(--accent); }
  button.theater-btn {
    background: linear-gradient(135deg, #059669, #10b981);
    color: #ffffff;
    border: none;
    font-weight: 800;
    box-shadow: 0 4px 12px rgba(16, 185, 129, 0.35);
  }
  button.theater-btn:hover { filter: brightness(1.15); }
  button.primary {
    background: linear-gradient(135deg, #2563eb, #1d4ed8);
    color: #ffffff;
    border: none;
    font-weight: 700;
  }
  button.primary:hover { filter: brightness(1.15); }
  button.vr-btn {
    background: linear-gradient(135deg, #0088ff, #7c3aed);
    color: #fff;
    border: none;
    font-weight: 700;
  }
  button.audio-btn {
    background: #4b5563;
    color: #fff;
    border: none;
    font-weight: 700;
  }
  button.audio-btn.live {
    background: #059669;
  }
  button.mode-btn.active {
    background: #2563eb;
    border-color: #60a5fa;
    color: #fff;
    box-shadow: 0 0 10px rgba(37, 99, 235, 0.5);
  }
  .sliders {
    display: flex;
    flex-wrap: wrap;
    gap: 14px;
    align-items: center;
    justify-content: center;
    width: 100%;
    padding-top: 4px;
    font-size: 0.80rem;
    color: #cbd5e1;
  }
  .sliders label {
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .stage {
    position: relative;
    flex: 1;
    width: calc(100% - 16px);
    max-width: 1400px;
    background: #000;
    border-radius: 12px;
    overflow: hidden;
    box-shadow: 0 15px 50px rgba(0,0,0,0.8), 0 0 24px rgba(59, 130, 246, 0.25);
    border: 1px solid rgba(255,255,255,0.12);
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 6px;
  }
  #glcanvas {
    width: 100%;
    height: 100%;
    display: block;
    cursor: pointer;
  }
  body.theater-mode {
    background: #000;
  }
  body.theater-mode header,
  body.theater-mode .toolbar {
    display: none !important;
  }
  body.theater-mode .stage {
    position: fixed !important;
    inset: 0 !important;
    width: 100vw !important;
    height: 100vh !important;
    max-width: 100vw !important;
    max-height: 100vh !important;
    border-radius: 0 !important;
    margin: 0 !important;
    border: none !important;
    box-shadow: none !important;
    z-index: 100 !important;
  }
  .float-menu-btn {
    position: fixed;
    top: 14px;
    right: 14px;
    z-index: 200;
    background: rgba(15, 23, 42, 0.75);
    color: #f1f5f9;
    border: 1px solid rgba(255, 255, 255, 0.3);
    border-radius: 8px;
    padding: 8px 16px;
    font-size: 0.85rem;
    font-weight: 700;
    cursor: pointer;
    display: none;
    backdrop-filter: blur(10px);
  }
  .float-menu-btn:hover {
    background: #1e293b;
    border-color: #38bdf8;
  }
  #reticle {
    position: fixed;
    width: 16px;
    height: 16px;
    border: 2px solid #38bdf8;
    background: rgba(56, 189, 248, 0.45);
    border-radius: 50%;
    transform: translate(-50%, -50%);
    pointer-events: none;
    display: none;
    z-index: 999;
    box-shadow: 0 0 10px #38bdf8;
  }
  .modal-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.85);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 500;
    padding: 16px;
  }
  .modal-card {
    background: #0f172a;
    border: 1px solid rgba(59, 130, 246, 0.5);
    border-radius: 16px;
    max-width: 600px;
    width: 100%;
    padding: 22px;
    box-shadow: 0 25px 60px rgba(0,0,0,0.85);
    color: #e2e8f0;
  }
  .modal-card h3 {
    color: #38bdf8;
    margin-bottom: 10px;
    font-size: 1.2rem;
  }
  .step-box {
    background: rgba(255,255,255,0.05);
    border-radius: 10px;
    padding: 12px 16px;
    margin: 12px 0;
    line-height: 1.6;
    font-size: 0.88rem;
  }
  .step-box code {
    background: #1e293b;
    color: #38bdf8;
    padding: 2px 6px;
    border-radius: 4px;
    font-family: monospace;
    font-weight: 700;
  }
  .modal-btns {
    display: flex;
    gap: 10px;
    justify-content: flex-end;
    margin-top: 14px;
    flex-wrap: wrap;
  }
</style>)HTML";

static const char* kQuestWebXRAppHtml2 = R"HTML(
</head>
<body>
<audio id="streamAudio" preload="none"></audio>
<button id="btnFloatMenu" class="float-menu-btn">⚙️ Settings / Exit Theater</button>
<div id="reticle"></div>

<div id="xrModal" class="modal-overlay" style="display: none;">
  <div class="modal-card">
    <h3>🥽 Meta Quest WebXR 6DoF Setup</h3>
    <p>WebXR requires a secure origin. Choose your preferred way to play:</p>
    <div class="step-box">
      <b>🚀 Instant 6DoF via ADB Wireless (Recommended):</b><br>
      Since Quest Wireless Debugging is active on your PC, simply open: <code>http://localhost:8765</code> in Quest Browser!<br>
      Localhost is automatically treated as a secure context and enables 6DoF VR &amp; Mixed Reality immediately.<br><br>
      <b>Or enable via chrome://flags:</b><br>
      1. Open a new tab and go to: <code>chrome://flags</code><br>
      2. Search: <b>Insecure origins treated as secure</b><br>
      3. Enter: <code id="modalUrl">http://...</code><br>
      4. Set dropdown to <b>Enabled</b> and tap <b>Relaunch</b> at bottom.
    </div>
    <div class="modal-btns">
      <button class="theater-btn" id="btnModalTheater">🎬 Play in Giant IMAX Cinema Now</button>
      <button id="btnModalClose">Close</button>
    </div>
  </div>
</div>

<header>
  <div class="brand">
    <span>FLYCAST DREAMCAST VR STUDIO</span>
    <span class="badge">QUEST 2 / 3 / 3S / PRO</span>
    <span class="ctrl-badge" id="ctrlStatus">🎮 P1 Ready (Quest Touch &amp; Bluetooth)</span>
  </div>
  <div class="stats" id="stats">Connecting to Flycast stream...</div>
</header>

<div class="toolbar">
  <button class="theater-btn" id="btnLaunchTheater">🎬 Launch IMAX Cinema (Theater View)</button>
  <button class="audio-btn" id="btnToggleAudio">🔊 Sound: Tap to Start</button>
  <button class="primary" id="btnEnterAR">🥽 Mixed Reality (Passthrough)</button>
  <button class="vr-btn" id="btnEnterVR">🌌 VR Cinema (Void)</button>
  <button class="mode-btn active" id="btnMode2D">2D Standard</button>
  <button class="mode-btn" id="btnModeSBS">SBS 3D Depth</button>
  <button class="mode-btn" id="btnModeTB">Top-Bottom 3D</button>
  <button class="mode-btn" id="btnModeAnaglyph">Anaglyph 3D</button>
  <button id="btnRecenter">Recenter View</button>

  <div class="sliders">
    <label>Distance: <input type="range" id="rngDist" min="1.0" max="6.0" step="0.1" value="2.0"><span id="valDist">2.0m</span></label>
    <label>Scale: <input type="range" id="rngScale" min="0.5" max="3.0" step="0.05" value="1.55"><span id="valScale">1.55x</span></label>
    <label>Curvature: <input type="range" id="rngCurve" min="0.0" max="0.6" step="0.02" value="0.16"><span id="valCurve">0.16</span></label>
    <label>3D Depth: <input type="range" id="rngDepth" min="0.0" max="0.06" step="0.002" value="0.016"><span id="valDepth">0.016</span></label>
  </div>
</div>

<div class="stage" id="stageContainer">
  <canvas id="glcanvas"></canvas>
</div>

<script>)HTML";

static const char* kQuestWebXRAppHtml3 = R"HTML(
(function() {
  const navKeys = new Set(['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'PageUp', 'PageDown', 'Home', 'End']);
  window.addEventListener('keydown', (e) => {
    if (navKeys.has(e.key) || e.key === 'Back' || e.keyCode === 27 || e.keyCode === 4) {
      e.preventDefault();
    }
  }, { passive: false });
  window.addEventListener('scroll', () => window.scrollTo(0, 0), { passive: false });

  // Trap back navigation so B button does not exit fullscreen or go back
  window.history.pushState(null, null, window.location.href);
  window.addEventListener('popstate', () => {
    window.history.pushState(null, null, window.location.href);
  });

  const canvas = document.getElementById('glcanvas');
  const statsEl = document.getElementById('stats');
  const reticle = document.getElementById('reticle');
  let pointerX = -1.0, pointerY = -1.0, pointerDown = false;

  canvas.addEventListener('pointermove', (e) => {
    const rect = canvas.getBoundingClientRect();
    if (rect.width > 0 && rect.height > 0) {
      pointerX = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
      pointerY = Math.max(0, Math.min(1, (e.clientY - rect.top) / rect.height));
      if (reticle) {
        reticle.style.left = e.clientX + 'px';
        reticle.style.top = e.clientY + 'px';
        reticle.style.display = 'block';
      }
    }
  });
  canvas.addEventListener('pointerleave', () => {
    if (reticle) reticle.style.display = 'none';
    if (!xrSession) { pointerX = -1.0; pointerY = -1.0; }
  });
  canvas.addEventListener('pointerdown', () => { pointerDown = true; });
  window.addEventListener('pointerup', () => { pointerDown = false; });

  const gl = canvas.getContext('webgl', { xrCompatible: true, alpha: true, antialias: true });
  if (!gl) {
    statsEl.textContent = 'WebGL not supported in this browser.';
    return;
  }

  let audioActive = false;
  function toggleAudio() {
    const a = document.getElementById('streamAudio');
    if (!a) return;
    const b = document.getElementById('btnToggleAudio');
    if (audioActive) {
      a.pause();
      a.src = '';
      audioActive = false;
      if (b) { b.textContent = '🔇 Sound: Muted'; b.classList.remove('live'); }
    } else {
      a.src = '/audio.wav?t=' + Date.now();
      a.play().then(() => {
        audioActive = true;
        if (b) { b.textContent = '🔊 Sound: Live'; b.classList.add('live'); }
      }).catch(e => console.log('Audio autoplay prevented:', e));
    }
  }
  document.getElementById('btnToggleAudio').onclick = toggleAudio;
  window.addEventListener('click', () => { if (!audioActive) toggleAudio(); }, { once: true });
  window.addEventListener('touchstart', () => { if (!audioActive) toggleAudio(); }, { once: true });

  let theaterActive = false;
  function setTheaterMode(active) {
    theaterActive = active;
    const floatBtn = document.getElementById('btnFloatMenu');
    if (theaterActive) {
      document.body.classList.add('theater-mode');
      if (floatBtn) floatBtn.style.display = 'block';
      if (!audioActive) toggleAudio();
    } else {
      document.body.classList.remove('theater-mode');
      if (floatBtn) floatBtn.style.display = 'none';
    }
  }
  document.getElementById('btnLaunchTheater').onclick = () => setTheaterMode(true);
  document.getElementById('btnFloatMenu').onclick = () => setTheaterMode(false);
  canvas.addEventListener('dblclick', () => setTheaterMode(!theaterActive));

  const modal = document.getElementById('xrModal');
  document.getElementById('btnModalClose').onclick = () => { modal.style.display = 'none'; };
  document.getElementById('btnModalTheater').onclick = () => {
    modal.style.display = 'none';
    setTheaterMode(true);
  };
  function showXRHelp() {
    const urlEl = document.getElementById('modalUrl');
    if (urlEl) urlEl.textContent = window.location.origin;
    if (modal) modal.style.display = 'flex';
  }
  if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
    const b = document.getElementById('ctrlStatus');
    if (b) { b.textContent = '🚀 6DoF WebXR Active (ADB Forwarded)'; }
  }

  let renderMode = 0; // 0=2D, 1=SBS, 2=TopBottom, 3=Anaglyph
  let screenDist = 2.0;
  let screenScale = 1.55;
  let screenCurve = 0.16;
  let depthStrength = 0.016;
  let yawOffset = 0.0;

  const bindSlider = (id, valId, suffix, cb) => {
    const el = document.getElementById(id);
    const vEl = document.getElementById(valId);
    el.addEventListener('input', () => {
      const v = parseFloat(el.value);
      vEl.textContent = v.toFixed(suffix === 'm' || suffix === 'x' ? 2 : 3) + suffix;
      cb(v);
    });
  };
  bindSlider('rngDist', 'valDist', 'm', v => screenDist = v);
  bindSlider('rngScale', 'valScale', 'x', v => { screenScale = v; rebuildMesh(); });
  bindSlider('rngCurve', 'valCurve', '', v => { screenCurve = v; rebuildMesh(); });
  bindSlider('rngDepth', 'valDepth', '', v => depthStrength = v);

  const modeBtns = [
    { id: 'btnMode2D', mode: 0 },
    { id: 'btnModeSBS', mode: 1 },
    { id: 'btnModeTB', mode: 2 },
    { id: 'btnModeAnaglyph', mode: 3 }
  ];
  modeBtns.forEach(({ id, mode }) => {
    const el = document.getElementById(id);
    if (el) {
      el.onclick = () => {
        renderMode = mode;
        modeBtns.forEach(b => {
          const bEl = document.getElementById(b.id);
          if (bEl) bEl.classList.toggle('active', b.mode === mode);
        });
      };
    }
  });
  document.getElementById('btnRecenter').onclick = () => { yawOffset = lastHeadYaw; };

  const vsSource = `
    attribute vec3 aPos;
    attribute vec2 aUV;
    uniform mat4 uProj;
    uniform mat4 uView;
    uniform mat4 uModel;
    varying vec2 vUV;
    void main() {
      vUV = aUV;
      gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    }
  `;

  const fsSource = `
    precision mediump float;
    varying vec2 vUV;
    uniform sampler2D uTex;
    uniform int uMode;
    uniform int uEye;
    uniform float uDepth;

    void main() {
      vec2 uv = vUV;
      float parallax = (float(uEye) - 0.5) * uDepth;
      if (uMode == 1) {
        uv.x = clamp(uv.x + parallax, 0.0, 1.0);
        vec4 c1 = texture2D(uTex, uv);
        vec4 c2 = texture2D(uTex, clamp(uv + vec2(parallax * 0.5, 0.0), 0.0, 1.0));
        gl_FragColor = mix(c1, c2, 0.35);
      } else if (uMode == 2) {
        uv.x = clamp(uv.x + parallax, 0.0, 1.0);
        gl_FragColor = texture2D(uTex, uv);
      } else if (uMode == 3) {
        vec4 leftCol = texture2D(uTex, clamp(uv - vec2(uDepth * 0.65, 0.0), 0.0, 1.0));
        vec4 rightCol = texture2D(uTex, clamp(uv + vec2(uDepth * 0.65, 0.0), 0.0, 1.0));
        gl_FragColor = vec4(leftCol.r, rightCol.g, rightCol.b, 1.0);
      } else {
        gl_FragColor = texture2D(uTex, uv);
      }
    }
  `;

  function compileShader(type, src) {
    const s = gl.createShader(type);
    gl.shaderSource(s, src);
    gl.compileShader(s);
    return s;
  }
  const prog = gl.createProgram();
  gl.attachShader(prog, compileShader(gl.VERTEX_SHADER, vsSource));
  gl.attachShader(prog, compileShader(gl.FRAGMENT_SHADER, fsSource));
  gl.linkProgram(prog);
  gl.useProgram(prog);

  const aPos = gl.getAttribLocation(prog, 'aPos');
  const aUV = gl.getAttribLocation(prog, 'aUV');
  const uProj = gl.getUniformLocation(prog, 'uProj');
  const uView = gl.getUniformLocation(prog, 'uView');
  const uModel = gl.getUniformLocation(prog, 'uModel');
  const uMode = gl.getUniformLocation(prog, 'uMode');
  const uEye = gl.getUniformLocation(prog, 'uEye');
  const uDepth = gl.getUniformLocation(prog, 'uDepth');

  const posBuf = gl.createBuffer();
  const uvBuf = gl.createBuffer();
  const idxBuf = gl.createBuffer();
  let indexCount = 0;

  function rebuildMesh() {
    const cols = 40, rows = 22;
    const w = 1.6 * screenScale;
    const h = 0.9 * screenScale;
    const positions = [];
    const uvs = [];
    const indices = [];

    for (let r = 0; r <= rows; r++) {
      const v = r / rows;
      const y = (0.5 - v) * h;
      for (let c = 0; c <= cols; c++) {
        const u = c / cols;
        const xNorm = (u - 0.5) * 2.0;
        const x = (u - 0.5) * w;
        const z = -screenCurve * (1.0 - xNorm * xNorm);
        positions.push(x, y, z);
        uvs.push(u, v);
      }
    }
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < cols; c++) {
        const i0 = r * (cols + 1) + c;
        const i1 = i0 + 1;
        const i2 = i0 + (cols + 1);
        const i3 = i2 + 1;
        indices.push(i0, i2, i1, i1, i2, i3);
      }
    }
    indexCount = indices.length;

    gl.bindBuffer(gl.ARRAY_BUFFER, posBuf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, uvBuf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(uvs), gl.STATIC_DRAW);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, idxBuf);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);
  }
  rebuildMesh();

  const tex = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([15, 25, 45, 255]));

  let frameCount = 0;
  let lastStatTime = performance.now();
  let streamWidth = 0, streamHeight = 0;
  let fetching = false;

  )HTML";

static const char* kQuestWebXRAppHtml4 = R"HTML(function getInputQuery() {
    let b = 0;
    let lx = 0, ly = 0;
    let lt = 0, rt = 0;

    const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
    for (let i = 0; i < gamepads.length; i++) {
      const gp = gamepads[i];
      if (!gp || !gp.connected) continue;

      const idLower = (gp.id || '').toLowerCase();
      const isTouch = idLower.includes('oculus') || idLower.includes('touch') || idLower.includes('quest') || gp.hand;

      if (isTouch) {
        const isLeft = (gp.hand === 'left' || idLower.includes('left'));
        const isRight = (gp.hand === 'right' || idLower.includes('right'));

        if (isLeft) {
          if (gp.axes && gp.axes.length >= 2) {
            if (Math.abs(gp.axes[0]) > 0.15) lx = gp.axes[0];
            if (Math.abs(gp.axes[1]) > 0.15) ly = gp.axes[1];
          }
          if (gp.buttons) {
            if (gp.buttons[1] && gp.buttons[1].value > 0.2) lt = Math.max(lt, gp.buttons[1].value);
            if (gp.buttons[2] && gp.buttons[2].pressed) b |= (1 << 11);
            if (gp.buttons[3] && gp.buttons[3].pressed) b |= (1 << 2);  // X
            if (gp.buttons[4] && gp.buttons[4].pressed) b |= (1 << 3);  // Y
            if (gp.buttons[5] && gp.buttons[5].pressed) b |= (1 << 9);  // Menu -> Coin
            if (gp.buttons[0] && gp.buttons[0].pressed) b |= (1 << 8);  // Stick click -> Start
          }
        } else if (isRight) {
          if (gp.buttons) {
            if (gp.buttons[1] && gp.buttons[1].value > 0.2) rt = Math.max(rt, gp.buttons[1].value);
            if (gp.buttons[2] && gp.buttons[2].pressed) b |= (1 << 12);
            if (gp.buttons[3] && gp.buttons[3].pressed) b |= (1 << 0);  // A
            if (gp.buttons[4] && gp.buttons[4].pressed) b |= (1 << 1);  // B
            if (gp.buttons[0] && gp.buttons[0].pressed) b |= (1 << 8);  // Stick click -> Start
          }
        }
      } else {
        if (gp.buttons) {
          if (gp.buttons[0] && gp.buttons[0].pressed) b |= (1 << 0);
          if (gp.buttons[1] && gp.buttons[1].pressed) b |= (1 << 1);
          if (gp.buttons[2] && gp.buttons[2].pressed) b |= (1 << 2);
          if (gp.buttons[3] && gp.buttons[3].pressed) b |= (1 << 3);
          if (gp.buttons[4] && gp.buttons[4].pressed) b |= (1 << 11);
          if (gp.buttons[5] && gp.buttons[5].pressed) b |= (1 << 12);
          if (gp.buttons[6] && gp.buttons[6].value > 0.2) lt = Math.max(lt, gp.buttons[6].value);
          if (gp.buttons[7] && gp.buttons[7].value > 0.2) rt = Math.max(rt, gp.buttons[7].value);
          if (gp.buttons[8] && gp.buttons[8].pressed) b |= (1 << 9);  // Select/Back -> Coin
          if (gp.buttons[9] && gp.buttons[9].pressed) b |= (1 << 8);  // Start
          if (gp.buttons[12] && gp.buttons[12].pressed) b |= (1 << 4);
          if (gp.buttons[13] && gp.buttons[13].pressed) b |= (1 << 5);
          if (gp.buttons[14] && gp.buttons[14].pressed) b |= (1 << 6);
          if (gp.buttons[15] && gp.buttons[15].pressed) b |= (1 << 7);
        }
        if (gp.axes && gp.axes.length >= 2) {
          if (Math.abs(gp.axes[0]) > 0.15) lx = gp.axes[0];
          if (Math.abs(gp.axes[1]) > 0.15) ly = gp.axes[1];
          if (gp.axes.length >= 6) {
            if (gp.axes[4] < -0.5) b |= (1 << 6);
            if (gp.axes[4] > 0.5) b |= (1 << 7);
            if (gp.axes[5] < -0.5) b |= (1 << 4);
            if (gp.axes[5] > 0.5) b |= (1 << 5);
          }
        }
      }
    }

    if (xrSession && xrSession.inputSources) {
      for (const source of xrSession.inputSources) {
        const gp = source.gamepad;
        if (!gp) continue;
        if (source.handedness === 'left') {
          if (gp.axes && gp.axes.length >= 4) {
            if (Math.abs(gp.axes[2]) > 0.15) lx = gp.axes[2];
            if (Math.abs(gp.axes[3]) > 0.15) ly = gp.axes[3];
          }
          if (gp.buttons) {
            if (gp.buttons[0] && gp.buttons[0].value > 0.2) lt = Math.max(lt, gp.buttons[0].value);
            if (gp.buttons[1] && gp.buttons[1].pressed) b |= (1 << 11);
            if (gp.buttons[3] && gp.buttons[3].pressed) b |= (1 << 9);
            if (gp.buttons[4] && gp.buttons[4].pressed) b |= (1 << 2);
            if (gp.buttons[5] && gp.buttons[5].pressed) b |= (1 << 3);
          }
        } else if (source.handedness === 'right') {
          if (gp.buttons) {
            if (gp.buttons[0] && gp.buttons[0].value > 0.2) rt = Math.max(rt, gp.buttons[0].value);
            if (gp.buttons[1] && gp.buttons[1].pressed) b |= (1 << 12);
            if (gp.buttons[3] && gp.buttons[3].pressed) b |= (1 << 8);
            if (gp.buttons[4] && gp.buttons[4].pressed) b |= (1 << 0);
            if (gp.buttons[5] && gp.buttons[5].pressed) b |= (1 << 1);
          }
        }
      }
    }

    let q = 'b=' + b + '&x=' + lx.toFixed(3) + '&y=' + ly.toFixed(3) + '&lt=' + lt.toFixed(3) + '&rt=' + rt.toFixed(3);
    if (pointerX >= 0.0 && pointerY >= 0.0) {
      q += '&px=' + pointerX.toFixed(3) + '&py=' + pointerY.toFixed(3) + '&pd=' + (pointerDown ? '1' : '0');
    }
    return q;
  }

  async function fetchFrame() {
    if (fetching) return;
    fetching = true;
    try {
      const q = getInputQuery();
      const resp = await fetch('/frame.jpg?' + q + '&t=' + performance.now(), { cache: 'no-store' });
      if (!resp.ok) throw new Error('HTTP ' + resp.status);
      const blob = await resp.blob();
      if (!blob || blob.size === 0) throw new Error('Empty blob');

      let bmp = null;
      if (window.createImageBitmap) {
        try {
          bmp = await createImageBitmap(blob);
        } catch (e) {}
      }

      if (bmp) {
        gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, bmp);
        streamWidth = bmp.width;
        streamHeight = bmp.height;
        bmp.close();
      } else {
        await new Promise((resolve, reject) => {
          const tempImg = new Image();
          tempImg.onload = () => {
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, tempImg);
            streamWidth = tempImg.naturalWidth;
            streamHeight = tempImg.naturalHeight;
            resolve();
          };
          tempImg.onerror = reject;
          tempImg.src = URL.createObjectURL(blob);
        });
      }

      frameCount++;
      const now = performance.now();
      if (now - lastStatTime >= 1000) {
        const fps = ((frameCount * 1000) / (now - lastStatTime)).toFixed(1);
        statsEl.textContent = 'Live Stream: ' + streamWidth + 'x' + streamHeight + ' @ ' + fps + ' FPS';
        frameCount = 0;
        lastStatTime = now;
      }
      fetching = false;
      requestAnimationFrame(fetchFrame);
    } catch (err) {
      fetching = false;
      setTimeout(fetchFrame, 60);
    }
  }
  fetchFrame();

  )HTML";

static const char* kQuestWebXRAppHtml5 = R"HTML(function makePerspective(fovRad, aspect, near, far) {
    const f = 1.0 / Math.tan(fovRad / 2);
    const nf = 1 / (near - far);
    return new Float32Array([
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (far + near) * nf, -1,
      0, 0, (2 * far * near) * nf, 0
    ]);
  }

  function makeModelMatrix(dist, yHeight, yaw) {
    const c = Math.cos(yaw), s = Math.sin(yaw);
    const tx = -Math.sin(yaw) * dist;
    const tz = -Math.cos(yaw) * dist;
    return new Float32Array([
      c, 0, -s, 0,
      0, 1, 0, 0,
      s, 0, c, 0,
      tx, yHeight, tz, 1
    ]);
  }

  const identityMat = new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]);
  let xrSession = null;
  let xrRefSpace = null;
  let isAR = false;
  let lastHeadYaw = 0.0;

  function drawMesh(projMat, viewMat, modelMat, eyeIdx) {
    gl.useProgram(prog);
    gl.uniformMatrix4fv(uProj, false, projMat);
    gl.uniformMatrix4fv(uView, false, viewMat);
    gl.uniformMatrix4fv(uModel, false, modelMat);
    gl.uniform1i(uMode, renderMode);
    gl.uniform1i(uEye, eyeIdx);
    gl.uniform1f(uDepth, depthStrength);

    gl.bindBuffer(gl.ARRAY_BUFFER, posBuf);
    gl.enableVertexAttribArray(aPos);
    gl.vertexAttribPointer(aPos, 3, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ARRAY_BUFFER, uvBuf);
    gl.enableVertexAttribArray(aUV);
    gl.vertexAttribPointer(aUV, 2, gl.FLOAT, false, 0, 0);

    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, idxBuf);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.drawElements(gl.TRIANGLES, indexCount, gl.UNSIGNED_SHORT, 0);
  }

  function render2D() {
    if (xrSession) return;
    const dpr = Math.min(window.devicePixelRatio || 1, 2.0);
    const targetW = Math.floor(canvas.clientWidth * dpr);
    const targetH = Math.floor(canvas.clientHeight * dpr);
    if (canvas.width !== targetW || canvas.height !== targetH) {
      canvas.width = targetW;
      canvas.height = targetH;
    }
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.enable(gl.DEPTH_TEST);

    const aspect = canvas.width / Math.max(1, canvas.height);
    const proj = makePerspective(46 * Math.PI / 180, aspect, 0.1, 50.0);
    const model = makeModelMatrix(screenDist, 0.0, 0.0);

    if (renderMode === 1) {
      const halfW = canvas.width >> 1;
      const sbsProj = makePerspective(46 * Math.PI / 180, (halfW) / Math.max(1, canvas.height), 0.1, 50.0);
      gl.viewport(0, 0, halfW, canvas.height);
      drawMesh(sbsProj, identityMat, model, 0);
      gl.viewport(halfW, 0, halfW, canvas.height);
      drawMesh(sbsProj, identityMat, model, 1);
    } else if (renderMode === 2) {
      const halfH = canvas.height >> 1;
      const tbProj = makePerspective(46 * Math.PI / 180, canvas.width / Math.max(1, halfH), 0.1, 50.0);
      gl.viewport(0, halfH, canvas.width, halfH);
      drawMesh(tbProj, identityMat, model, 0);
      gl.viewport(0, 0, canvas.width, halfH);
      drawMesh(tbProj, identityMat, model, 1);
    } else {
      gl.viewport(0, 0, canvas.width, canvas.height);
      drawMesh(proj, identityMat, model, 0);
    }
    requestAnimationFrame(render2D);
  }
  requestAnimationFrame(render2D);

  async function startXR(mode) {
    if (!navigator.xr) {
      showXRHelp();
      return;
    }
    try {
      const supported = await navigator.xr.isSessionSupported(mode);
      if (!supported && mode === 'immersive-ar') {
        mode = 'immersive-vr';
        const vrSupp = await navigator.xr.isSessionSupported('immersive-vr');
        if (!vrSupp) {
          showXRHelp();
          return;
        }
      }
      isAR = (mode === 'immersive-ar');
      xrSession = await navigator.xr.requestSession(mode, {
        optionalFeatures: ['local-floor', 'bounded-floor', 'hand-tracking']
      });
      xrSession.addEventListener('end', () => {
        xrSession = null;
        requestAnimationFrame(render2D);
      });
      await gl.makeXRCompatible();
      xrSession.updateRenderState({ baseLayer: new XRWebGLLayer(xrSession, gl) });
      xrRefSpace = await xrSession.requestReferenceSpace('local');
      if (!audioActive) toggleAudio();
      xrSession.requestAnimationFrame(onXRFrame);
    } catch (e) {
      console.error('WebXR session error:', e);
      showXRHelp();
    }
  }

  function onXRFrame(time, frame) {
    const session = frame.session;
    session.requestAnimationFrame(onXRFrame);
    const pose = frame.getViewerPose(xrRefSpace);
    if (!pose) return;

    // Use Right controller thumbstick for distance and screen size in VR
    for (const source of session.inputSources) {
      if (source.handedness === 'right' && source.gamepad && source.gamepad.axes.length >= 4) {
        const axY = source.gamepad.axes[3];
        const axX = source.gamepad.axes[2];
        if (Math.abs(axY) > 0.2) screenDist = Math.max(1.0, Math.min(6.0, screenDist + axY * 0.025));
        if (Math.abs(axX) > 0.2) {
          screenScale = Math.max(0.5, Math.min(3.0, screenScale - axX * 0.02));
          rebuildMesh();
        }
      }
    }

    // Raycast controller pointer onto virtual screen plane
    let xrHit = false;
    for (const source of session.inputSources) {
      if (source.targetRaySpace) {
        const rayPose = frame.getPose(source.targetRaySpace, xrRefSpace);
        if (rayPose) {
          const m = rayPose.transform.matrix;
          const ox = m[12], oy = m[13], oz = m[14];
          const dx = -m[8], dy = -m[9], dz = -m[10];
          if (dz < -0.05) {
            const t = (-screenDist - oz) / dz;
            if (t > 0.1 && t < 10.0) {
              const hitX = ox + dx * t;
              const hitY = oy + dy * t;
              const halfW = (1.6 * screenScale) * 0.5;
              const halfH = (0.9 * screenScale) * 0.5;
              if (Math.abs(hitX) <= halfW && Math.abs(hitY) <= halfH) {
                pointerX = (hitX + halfW) / (2.0 * halfW);
                pointerY = (halfH - hitY) / (2.0 * halfH);
                xrHit = true;
                if (source.gamepad && source.gamepad.buttons[0] && source.gamepad.buttons[0].value > 0.3) {
                  pointerDown = true;
                }
              }
            }
          }
        }
      }
    }
    if (!xrHit && xrSession) {
      pointerX = -1.0;
      pointerY = -1.0;
    }

    const q = pose.transform.orientation;
    lastHeadYaw = Math.atan2(2.0 * (q.w * q.y + q.x * q.z), 1.0 - 2.0 * (q.y * q.y + q.x * q.x));

    const glLayer = session.renderState.baseLayer;
    gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);
    if (isAR) {
      gl.clearColor(0.0, 0.0, 0.0, 0.0);
    } else {
      gl.clearColor(0.02, 0.03, 0.06, 1.0);
    }
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.enable(gl.DEPTH_TEST);

    const model = makeModelMatrix(screenDist, 0.0, yawOffset);
    let eyeIndex = 0;
    for (const view of pose.views) {
      const vp = glLayer.getViewport(view);
      gl.viewport(vp.x, vp.y, vp.width, vp.height);
      drawMesh(view.projectionMatrix, view.transform.inverse.matrix, model, eyeIndex);
      eyeIndex++;
    }
  }

  document.getElementById('btnEnterAR').onclick = () => startXR('immersive-ar');
  document.getElementById('btnEnterVR').onclick = () => startXR('immersive-vr');
})();
</script>
</body>
</html>
)HTML";

#if defined(_WIN32) && !defined(TARGET_UWP)
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;
	UINT size = 0;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	std::vector<uint8_t> buffer(size);
	Gdiplus::ImageCodecInfo* pImageCodecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			return static_cast<int>(j);
		}
	}
	return -1;
}

struct EnumVerifyWindowData
{
	DWORD target_pid;
	HWND best_hwnd;
	int best_area;
};

BOOL CALLBACK FindMainProcessWindowProc(HWND hwnd, LPARAM lParam)
{
	EnumVerifyWindowData* data = reinterpret_cast<EnumVerifyWindowData*>(lParam);
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid != data->target_pid || !IsWindowVisible(hwnd))
		return TRUE;

	RECT rc{};
	if (GetClientRect(hwnd, &rc))
	{
		const int w = rc.right - rc.left;
		const int h = rc.bottom - rc.top;
		if (w * h > data->best_area)
		{
			data->best_area = w * h;
			data->best_hwnd = hwnd;
		}
	}
	return TRUE;
}
#endif

class QuestGamepadDevice : public GamepadDevice
{
public:
	QuestGamepadDevice(int maple_port = 0)
		: GamepadDevice(maple_port, "QuestVR", false)
	{
		_name = "Quest VR Controller";
		_unique_id = "quest_vr_gamepad_p" + std::to_string(maple_port);
		input_mapper = std::make_shared<IdentityInputMapping>();
		input_mapper->addTrigger(DC_AXIS_LT, false);
		input_mapper->addTrigger(DC_AXIS_RT, false);
		hasAnalogStick = true;
	}

	void refreshName() override { _name = "Quest VR Controller"; }
	const char* get_button_name(u32 code) override { return "QuestBtn"; }
	const char* get_axis_name(u32 code) override { return "QuestAxis"; }

	void SetInput(uint32_t buttons, float joyx, float joyy, float lt, float rt)
	{
		gamepad_btn_input(DC_BTN_A, (buttons & (1 << 0)) != 0);
		gamepad_btn_input(DC_BTN_B, (buttons & (1 << 1)) != 0);
		gamepad_btn_input(DC_BTN_X, (buttons & (1 << 2)) != 0);
		gamepad_btn_input(DC_BTN_Y, (buttons & (1 << 3)) != 0);
		gamepad_btn_input(DC_DPAD_UP, (buttons & (1 << 4)) != 0);
		gamepad_btn_input(DC_DPAD_DOWN, (buttons & (1 << 5)) != 0);
		gamepad_btn_input(DC_DPAD_LEFT, (buttons & (1 << 6)) != 0);
		gamepad_btn_input(DC_DPAD_RIGHT, (buttons & (1 << 7)) != 0);
		gamepad_btn_input(DC_BTN_START, (buttons & (1 << 8)) != 0);

		const int lt_val = ((buttons & (1 << 9)) != 0 || (buttons & (1 << 11)) != 0) ? 32767 : static_cast<int>(std::clamp(lt, 0.0f, 1.0f) * 65535.0f - 32768.0f);
		const int rt_val = ((buttons & (1 << 10)) != 0 || (buttons & (1 << 12)) != 0) ? 32767 : static_cast<int>(std::clamp(rt, 0.0f, 1.0f) * 65535.0f - 32768.0f);
		gamepad_axis_input(DC_AXIS_LT, lt_val);
		gamepad_axis_input(DC_AXIS_RT, rt_val);

		const int jx = static_cast<int>(std::round(std::clamp(joyx, -1.0f, 1.0f) * 32767.0f));
		const int jy = static_cast<int>(std::round(std::clamp(joyy, -1.0f, 1.0f) * 32767.0f));
		if (jx > 0)
			gamepad_axis_input(DC_AXIS_RIGHT, jx);
		else if (jx < 0)
			gamepad_axis_input(DC_AXIS_LEFT, -jx);
		else
		{
			gamepad_axis_input(DC_AXIS_RIGHT, 0);
			gamepad_axis_input(DC_AXIS_LEFT, 0);
		}

		if (jy > 0)
			gamepad_axis_input(DC_AXIS_DOWN, jy);
		else if (jy < 0)
			gamepad_axis_input(DC_AXIS_UP, -jy);
		else
		{
			gamepad_axis_input(DC_AXIS_DOWN, 0);
			gamepad_axis_input(DC_AXIS_UP, 0);
		}
	}
	};
} // namespace

QuestBroadcastService& QuestBroadcastService::Instance()
{
	static QuestBroadcastService s_instance;
	return s_instance;
}

QuestBroadcastService::QuestBroadcastService() = default;

QuestBroadcastService::~QuestBroadcastService()
{
	Stop();
}

void QuestBroadcastService::SetError(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(m_error_mutex);
	m_last_error = msg;
}

std::string QuestBroadcastService::GetLastErrorMsg() const
{
	std::lock_guard<std::mutex> lock(m_error_mutex);
	return m_last_error;
}

std::string QuestBroadcastService::GetLocalIpAddress() const
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	WSADATA wsa{};
	const bool wsa_ok = (WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

	std::string ip_str = "127.0.0.1";
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock != INVALID_SOCKET)
	{
		sockaddr_in remote{};
		remote.sin_family = AF_INET;
		remote.sin_port = htons(53);
		inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

		if (connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0)
		{
			sockaddr_in local{};
			int len = sizeof(local);
			if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) == 0)
			{
				char buf[INET_ADDRSTRLEN] = {};
				if (inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf)))
					ip_str = buf;
			}
		}
		closesocket(sock);
	}

	if (wsa_ok)
		WSACleanup();
	return ip_str;
#else
	return "127.0.0.1";
#endif
}

std::string QuestBroadcastService::GetStreamUrl() const
{
	return "http://" + GetLocalIpAddress() + ":" + std::to_string(m_port.load()) + "/";
}

bool QuestBroadcastService::Start(uint16_t port, int target_fps, int jpeg_quality)
{
	if (m_running.load())
		return true;

	m_port.store(port);
	SetTargetFps(target_fps);
	SetJpegQuality(jpeg_quality);

#if defined(_WIN32) && !defined(TARGET_UWP)
#if defined(USE_SDL)
	if (m_target_hwnd.load() == nullptr)
	{
		HWND sdlHwnd = getNativeHwnd();
		if (sdlHwnd != NULL)
			m_target_hwnd.store(sdlHwnd);
	}
#endif

	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		SetError("Failed to initialize WinSock2.");
		return false;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET)
	{
		SetError("Failed to create TCP listen socket.");
		WSACleanup();
		return false;
	}

	BOOL reuse = TRUE;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

	sockaddr_in bind_addr{};
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(port);

	if (bind(listen_sock, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) == SOCKET_ERROR)
	{
		bool bound = false;
		for (uint16_t try_port = port + 1; try_port < port + 15; ++try_port)
		{
			bind_addr.sin_port = htons(try_port);
			if (bind(listen_sock, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) == 0)
			{
				m_port.store(try_port);
				bound = true;
				break;
			}
		}
		if (!bound)
		{
			SetError("Port " + std::to_string(port) + " is already in use.");
			closesocket(listen_sock);
			WSACleanup();
			return false;
		}
	}

	if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
	{
		SetError("Failed to listen on TCP socket.");
		closesocket(listen_sock);
		WSACleanup();
		return false;
	}

	m_listen_socket = static_cast<uintptr_t>(listen_sock);
	m_running.store(true);
	m_frames_captured.store(0);
	m_frames_served.store(0);
	SetError("");

	m_capture_thread = std::thread(&QuestBroadcastService::CaptureLoop, this);
	m_server_thread = std::thread(&QuestBroadcastService::ServerLoop, this);

	if (!m_vr_gamepad)
	{
		m_vr_gamepad = std::make_shared<QuestGamepadDevice>(0);
		GamepadDevice::Register(m_vr_gamepad);
	}

	std::thread([this]() {
		std::string dummy;
		RunAdbReverse(dummy);
	}).detach();

	return true;
#else
	SetError("Quest VR Broadcast Server requires Windows sockets.");
	return false;
#endif
}

void QuestBroadcastService::Stop()
{
	if (!m_running.exchange(false))
		return;

#if defined(_WIN32) && !defined(TARGET_UWP)
	if (m_listen_socket != ~static_cast<uintptr_t>(0))
	{
		closesocket(static_cast<SOCKET>(m_listen_socket));
		m_listen_socket = ~static_cast<uintptr_t>(0);
	}
#endif

	if (m_capture_thread.joinable())
		m_capture_thread.join();
	if (m_server_thread.joinable())
		m_server_thread.join();

	if (m_vr_gamepad)
	{
		GamepadDevice::Unregister(m_vr_gamepad);
		m_vr_gamepad.reset();
	}

#if defined(_WIN32) && !defined(TARGET_UWP)
	WSACleanup();
#endif
}

void QuestBroadcastPushAudio(const void* samples, size_t count)
{
	QuestBroadcastService::Instance().PushAudioSamples(samples, count);
}

void QuestBroadcastService::PushAudioSamples(const void* data, size_t sample_pairs)
{
	if (!m_running.load() || data == nullptr || sample_pairs == 0)
		return;
	const int16_t* pcm = static_cast<const int16_t*>(data);
	const size_t total_samples = sample_pairs * 2;
	std::lock_guard<std::mutex> lock(m_audio_mutex);
	if (m_audio_buffer.size() + total_samples > 88200)
	{
		const size_t drop = (m_audio_buffer.size() + total_samples) - 88200;
		m_audio_buffer.erase(m_audio_buffer.begin(), m_audio_buffer.begin() + std::min(drop, m_audio_buffer.size()));
	}
	m_audio_buffer.insert(m_audio_buffer.end(), pcm, pcm + total_samples);
}

void QuestBroadcastService::UpdateControllerInput(uint32_t buttons, float joyx_in, float joyy_in, float lt_in, float rt_in, float px, float py, bool pd)
{
	// 1. Direct hardware bitmask updates for Maple bus & Naomi/Dreamcast games
	uint32_t pressed_mask = 0;
	if (buttons & (1 << 0)) pressed_mask |= DC_BTN_A;
	if (buttons & (1 << 1)) pressed_mask |= DC_BTN_B;
	if (buttons & (1 << 2)) pressed_mask |= DC_BTN_X;
	if (buttons & (1 << 3)) pressed_mask |= DC_BTN_Y;
	if (buttons & (1 << 4)) pressed_mask |= DC_DPAD_UP;
	if (buttons & (1 << 5)) pressed_mask |= DC_DPAD_DOWN;
	if (buttons & (1 << 6)) pressed_mask |= DC_DPAD_LEFT;
	if (buttons & (1 << 7)) pressed_mask |= DC_DPAD_RIGHT;
	if (buttons & (1 << 8)) pressed_mask |= DC_BTN_START;
	if (buttons & (1 << 9)) pressed_mask |= DC_BTN_D; // Naomi Coin!
	if (buttons & (1 << 10)) pressed_mask |= DC_DPAD2_DOWN; // Naomi Test
	if (pd) pressed_mask |= DC_BTN_A; // Pointer click acts as A / Gun trigger

	const uint32_t kAllQuestButtons = DC_BTN_A | DC_BTN_B | DC_BTN_X | DC_BTN_Y |
	                                  DC_DPAD_UP | DC_DPAD_DOWN | DC_DPAD_LEFT | DC_DPAD_RIGHT |
	                                  DC_BTN_START | DC_BTN_D | DC_DPAD2_DOWN;

	// In Flycast, kcode is active-LOW (0 = pressed, 1 = released)
	kcode[0] = (kcode[0] | kAllQuestButtons) & ~pressed_mask;
	mapleInputState[0].kcode = kcode[0];

	// 2. Analog stick
	const s16 sx = static_cast<s16>(std::clamp(joyx_in, -1.0f, 1.0f) * 32767.0f);
	const s16 sy = static_cast<s16>(std::clamp(joyy_in, -1.0f, 1.0f) * 32767.0f);
	joyx[0] = sx;
	joyy[0] = sy;
	mapleInputState[0].fullAxes[PJAI_X1] = sx;
	mapleInputState[0].fullAxes[PJAI_Y1] = sy;

	// 3. Triggers
	u16 ltv = static_cast<u16>(std::clamp(lt_in, 0.0f, 1.0f) * 65535.0f);
	u16 rtv = static_cast<u16>(std::clamp(rt_in, 0.0f, 1.0f) * 65535.0f);
	if (buttons & (1 << 11)) ltv = 65535; // L digital trigger / bumper
	if (buttons & (1 << 12)) rtv = 65535; // R digital trigger / bumper
	lt[0] = ltv;
	rt[0] = rtv;
	mapleInputState[0].halfAxes[PJTI_L] = ltv;
	mapleInputState[0].halfAxes[PJTI_R] = rtv;

	// 4. Mouse / Lightgun pointer
	if (px >= 0.0f && py >= 0.0f)
	{
		SetMousePosition(static_cast<int>(px * 640.0f), static_cast<int>(py * 480.0f), 640, 480, 0);
		if (pd)
			mo_buttons[0] &= ~1;
		else
			mo_buttons[0] |= 1;
	}

	if (m_vr_gamepad)
		static_cast<QuestGamepadDevice*>(m_vr_gamepad.get())->SetInput(buttons, joyx_in, joyy_in, lt_in, rt_in);
}

bool QuestBroadcastService::RunAdbReverse(std::string& out_msg)
{
#if defined(_WIN32)
	FILE* pipe = _popen("adb reverse tcp:8765 tcp:8765 2>&1", "r");
	if (!pipe) {
		out_msg = "Could not execute ADB. Ensure Android platform-tools (adb) are installed.";
		return false;
	}
	char buf[256];
	std::string res;
	while (fgets(buf, sizeof(buf), pipe))
		res += buf;
	int code = _pclose(pipe);
	if (code == 0) {
		out_msg = "Port 8765 forwarded to Quest! In Quest Browser, open: http://localhost:8765";
		return true;
	} else {
		out_msg = res.empty() ? "ADB exited with non-zero status." : res;
		return false;
	}
#else
	out_msg = "ADB is not supported on this platform.";
	return false;
#endif
}

bool QuestBroadcastService::LaunchQuestBrowser(std::string& out_msg)
{
#if defined(_WIN32)
	FILE* pipe = _popen("adb shell am start -a android.intent.action.VIEW -d \"http://localhost:8765\" com.oculus.browser 2>&1", "r");
	if (!pipe) {
		out_msg = "Could not execute ADB.";
		return false;
	}
	char buf[256];
	std::string res;
	while (fgets(buf, sizeof(buf), pipe))
		res += buf;
	int code = _pclose(pipe);
	if (code == 0) {
		out_msg = "Quest Browser launched on headset to http://localhost:8765!";
		return true;
	} else {
		out_msg = "ADB launch failed: " + res;
		return false;
	}
#else
	out_msg = "ADB is not supported on this platform.";
	return false;
#endif
}

bool QuestBroadcastShouldMutePcAudio()
{
	return QuestBroadcastService::Instance().IsRunning() && QuestBroadcastService::Instance().GetMutePcAudio();
}

bool QuestBroadcastService::CaptureFrameToJpeg(std::vector<uint8_t>& out_jpeg)
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	HWND hwnd = reinterpret_cast<HWND>(m_target_hwnd.load());
#if defined(USE_SDL)
	if (!hwnd || !IsWindow(hwnd))
	{
		hwnd = getNativeHwnd();
		if (hwnd && IsWindow(hwnd))
			m_target_hwnd.store(hwnd);
	}
#endif
	if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd))
	{
		EnumVerifyWindowData find_data{GetCurrentProcessId(), nullptr, 0};
		EnumWindows(FindMainProcessWindowProc, reinterpret_cast<LPARAM>(&find_data));
		hwnd = find_data.best_hwnd;
	}

	int full_w = 0;
	int full_h = 0;
	int screen_x = 0;
	int screen_y = 0;
	bool use_desktop = false;

	if (hwnd && IsWindow(hwnd))
	{
		RECT rc{};
		GetClientRect(hwnd, &rc);
		full_w = rc.right - rc.left;
		full_h = rc.bottom - rc.top;
		POINT pt{0, 0};
		ClientToScreen(hwnd, &pt);
		screen_x = pt.x;
		screen_y = pt.y;
	}

	if (full_w < 64 || full_h < 64)
	{
		use_desktop = true;
		screen_x = 0;
		screen_y = 0;
		full_w = GetSystemMetrics(SM_CXSCREEN);
		full_h = GetSystemMetrics(SM_CYSCREEN);
	}

	if (full_w <= 0 || full_h <= 0)
		return false;

	int crop_x = 0;
	int crop_y = 0;
	int crop_w = full_w;
	int crop_h = full_h;

	const int mode = m_crop_mode.load();
	if (mode == 1) // Left Half (P1)
	{
		crop_w = full_w / 2;
	}
	else if (mode == 2) // Right Half (P2)
	{
		crop_x = full_w / 2;
		crop_w = full_w - crop_x;
	}
	else if (mode == 3) // Top Half (P1)
	{
		crop_h = full_h / 2;
	}
	else if (mode == 4) // Bottom Half (P2)
	{
		crop_y = full_h / 2;
		crop_h = full_h - crop_y;
	}
	else if (mode == 5) // Top-Left (P1 4P)
	{
		crop_w = full_w / 2;
		crop_h = full_h / 2;
	}
	else if (mode == 6) // Top-Right (P2 4P)
	{
		crop_x = full_w / 2;
		crop_w = full_w - crop_x;
		crop_h = full_h / 2;
	}
	else if (mode == 7) // Bottom-Left (P3 4P)
	{
		crop_y = full_h / 2;
		crop_w = full_w / 2;
		crop_h = full_h - crop_y;
	}
	else if (mode == 8) // Bottom-Right (P4 4P)
	{
		crop_x = full_w / 2;
		crop_y = full_h / 2;
		crop_w = full_w - crop_x;
		crop_h = full_h - crop_y;
	}

	HDC hdc_screen = GetDC(nullptr);
	HDC hdc_mem = CreateCompatibleDC(hdc_screen);
	HBITMAP hbm = CreateCompatibleBitmap(hdc_screen, crop_w, crop_h);
	HGDIOBJ old_obj = SelectObject(hdc_mem, hbm);

	bool captured = false;
	if (!use_desktop && hwnd && crop_x == 0 && crop_y == 0 && crop_w == full_w && crop_h == full_h)
	{
		// PW_RENDERFULLCONTENT (0x00000002) captures DirectX 11 / OpenGL / Vulkan swapchains on Windows
		if (PrintWindow(hwnd, hdc_mem, 0x00000002 | PW_CLIENTONLY))
			captured = true;
	}

	if (!captured)
	{
		BitBlt(hdc_mem, 0, 0, crop_w, crop_h, hdc_screen, screen_x + crop_x, screen_y + crop_y, SRCCOPY);
	}

	int target_w = crop_w;
	int target_h = crop_h;
	const int max_w = 1280;
	const int max_h = 720;
	if (target_w > max_w || target_h > max_h)
	{
		const double scale = std::min(static_cast<double>(max_w) / target_w, static_cast<double>(max_h) / target_h);
		target_w = static_cast<int>(target_w * scale);
		target_h = static_cast<int>(target_h * scale);
		if (target_w < 1) target_w = 1;
		if (target_h < 1) target_h = 1;
	}

	HDC hdc_scaled = CreateCompatibleDC(hdc_screen);
	HBITMAP hbm_scaled = CreateCompatibleBitmap(hdc_screen, target_w, target_h);
	HGDIOBJ old_scaled = SelectObject(hdc_scaled, hbm_scaled);
	SetStretchBltMode(hdc_scaled, COLORONCOLOR);
	StretchBlt(hdc_scaled, 0, 0, target_w, target_h, hdc_mem, 0, 0, crop_w, crop_h, SRCCOPY);
	SelectObject(hdc_scaled, old_scaled);

	SelectObject(hdc_mem, old_obj);

	CLSID jpeg_clsid{};
	bool ok = false;
	if (GetEncoderClsid(L"image/jpeg", &jpeg_clsid) >= 0)
	{
		IStream* stream = nullptr;
		if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) == S_OK)
		{
			Gdiplus::Bitmap bmp(hbm_scaled, nullptr);
			Gdiplus::EncoderParameters params{};
			params.Count = 1;
			params.Parameter[0].Guid = Gdiplus::EncoderQuality;
			params.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			params.Parameter[0].NumberOfValues = 1;
			ULONG quality = static_cast<ULONG>(m_jpeg_quality.load());
			params.Parameter[0].Value = &quality;

			if (bmp.Save(stream, &jpeg_clsid, &params) == Gdiplus::Ok)
			{
				HGLOBAL hg = nullptr;
				if (GetHGlobalFromStream(stream, &hg) == S_OK && hg)
				{
					const SIZE_T sz = GlobalSize(hg);
					const void* ptr = GlobalLock(hg);
					if (ptr && sz > 0)
					{
						out_jpeg.assign(static_cast<const uint8_t*>(ptr), static_cast<const uint8_t*>(ptr) + sz);
						ok = true;
					}
					if (ptr)
						GlobalUnlock(hg);
				}
			}
			stream->Release();
		}
	}

	DeleteObject(hbm_scaled);
	DeleteDC(hdc_scaled);
	DeleteObject(hbm);
	DeleteDC(hdc_mem);
	ReleaseDC(nullptr, hdc_screen);
	return ok;
#else
	(void)out_jpeg;
	return false;
#endif
}

void QuestBroadcastService::CaptureLoop()
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken = 0;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	auto fps_timer = std::chrono::steady_clock::now();
	uint64_t fps_counter = 0;

	while (m_running.load())
	{
		const auto frame_start = std::chrono::steady_clock::now();
		std::vector<uint8_t> jpeg;
		if (CaptureFrameToJpeg(jpeg) && !jpeg.empty())
		{
			{
				std::lock_guard<std::mutex> lock(m_frame_mutex);
				m_latest_jpeg.swap(jpeg);
			}
			m_frames_captured.fetch_add(1);
			fps_counter++;
		}

		const auto now = std::chrono::steady_clock::now();
		const double elapsed_sec = std::chrono::duration<double>(now - fps_timer).count();
		if (elapsed_sec >= 1.0)
		{
			m_actual_fps.store(static_cast<double>(fps_counter) / elapsed_sec);
			fps_counter = 0;
			fps_timer = now;
		}

		const int target_fps = m_target_fps.load();
		const auto frame_budget = std::chrono::milliseconds(1000 / (target_fps > 0 ? target_fps : 60));
		const auto work_time = std::chrono::steady_clock::now() - frame_start;
		if (work_time < frame_budget)
			std::this_thread::sleep_for(frame_budget - work_time);
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (gdiplusToken)
		Gdiplus::GdiplusShutdown(gdiplusToken);
	CoUninitialize();
#endif
}

#if defined(_WIN32) && !defined(TARGET_UWP)
static bool SendAll(SOCKET s, const void* data, size_t total_len)
{
	const char* ptr = static_cast<const char*>(data);
	size_t sent = 0;
	while (sent < total_len)
	{
		int chunk = static_cast<int>((total_len - sent) > 65536 ? 65536 : (total_len - sent));
		int res = send(s, ptr + sent, chunk, 0);
		if (res <= 0)
			return false;
		sent += res;
	}
	return true;
}

static void GracefulClose(SOCKET s)
{
	shutdown(s, SD_SEND);
	char drain[512];
	while (recv(s, drain, sizeof(drain), 0) > 0) {}
	closesocket(s);
}
#endif

void QuestBroadcastService::ServerLoop()
{
#if defined(_WIN32) && !defined(TARGET_UWP)
	SOCKET listen_sock = static_cast<SOCKET>(m_listen_socket);
	while (m_running.load())
	{
		sockaddr_in client_addr{};
		int addr_len = sizeof(client_addr);
		SOCKET client_sock = accept(listen_sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
		if (client_sock == INVALID_SOCKET)
		{
			if (!m_running.load())
				break;
			continue;
		}

		std::thread([this, client_sock]() {
			DWORD timeout_ms = 3000;
			setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
			setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));

			char req_buf[8192] = {};
			const int recvd = recv(client_sock, req_buf, sizeof(req_buf) - 1, 0);
			if (recvd <= 0)
			{
				closesocket(client_sock);
				return;
			}
			req_buf[recvd] = '\0';

			// 1. Handle preflight CORS OPTIONS requests
			if (std::strncmp(req_buf, "OPTIONS", 7) == 0)
			{
				static const char kRespOptions[] =
					"HTTP/1.1 204 No Content\r\n"
					"Access-Control-Allow-Origin: *\r\n"
					"Access-Control-Allow-Methods: GET, HEAD, OPTIONS\r\n"
					"Access-Control-Allow-Headers: *\r\n"
					"Access-Control-Max-Age: 86400\r\n"
					"Connection: close\r\n\r\n";
				SendAll(client_sock, kRespOptions, sizeof(kRespOptions) - 1);
				GracefulClose(client_sock);
				return;
			}

			// 2. Handle browser favicon / touch icons with 204 No Content
			if (std::strstr(req_buf, "/favicon.ico") != nullptr || std::strstr(req_buf, "apple-touch-icon") != nullptr)
			{
				static const char kResp204[] =
					"HTTP/1.1 204 No Content\r\n"
					"Access-Control-Allow-Origin: *\r\n"
					"Connection: close\r\n\r\n";
				SendAll(client_sock, kResp204, sizeof(kResp204) - 1);
				GracefulClose(client_sock);
				return;
			}

			// 2b. Handle live continuous audio streaming (44.1kHz 16-bit Stereo PCM)
			if (std::strstr(req_buf, "/audio.wav") != nullptr)
			{
				uint8_t wav_hdr[44] = {};
				std::memcpy(&wav_hdr[0], "RIFF", 4);
				uint32_t riff_sz = 0x7fff0000;
				std::memcpy(&wav_hdr[4], &riff_sz, 4);
				std::memcpy(&wav_hdr[8], "WAVEfmt ", 8);
				uint32_t sub1_sz = 16;
				uint16_t audio_fmt = 1; // PCM
				uint16_t num_ch = 2; // Stereo
				uint32_t sample_rate = 44100;
				uint32_t byte_rate = 44100 * 2 * 2;
				uint16_t block_align = 4;
				uint16_t bits_per_sample = 16;
				std::memcpy(&wav_hdr[16], &sub1_sz, 4);
				std::memcpy(&wav_hdr[20], &audio_fmt, 2);
				std::memcpy(&wav_hdr[22], &num_ch, 2);
				std::memcpy(&wav_hdr[24], &sample_rate, 4);
				std::memcpy(&wav_hdr[28], &byte_rate, 4);
				std::memcpy(&wav_hdr[32], &block_align, 2);
				std::memcpy(&wav_hdr[34], &bits_per_sample, 2);
				std::memcpy(&wav_hdr[36], "data", 4);
				uint32_t data_sz = 0x7fff0000 - 36;
				std::memcpy(&wav_hdr[40], &data_sz, 4);

				std::ostringstream hdr;
				hdr << "HTTP/1.1 200 OK\r\n"
					<< "Content-Type: audio/wav\r\n"
					<< "Cache-Control: no-cache, no-store, must-revalidate\r\n"
					<< "Access-Control-Allow-Origin: *\r\n"
					<< "Connection: close\r\n\r\n";
				const std::string hdr_str = hdr.str();
				if (!SendAll(client_sock, hdr_str.data(), hdr_str.size()) ||
					!SendAll(client_sock, wav_hdr, sizeof(wav_hdr)))
				{
					GracefulClose(client_sock);
					return;
				}

				std::vector<int16_t> pcm_chunk;
				pcm_chunk.reserve(4096);
				while (m_running.load())
				{
					pcm_chunk.clear();
					{
						std::lock_guard<std::mutex> lock(m_audio_mutex);
						if (!m_audio_buffer.empty())
						{
							size_t grab = std::min(m_audio_buffer.size(), size_t(4096));
							pcm_chunk.assign(m_audio_buffer.begin(), m_audio_buffer.begin() + grab);
							m_audio_buffer.erase(m_audio_buffer.begin(), m_audio_buffer.begin() + grab);
						}
					}

					if (!pcm_chunk.empty())
					{
						if (!SendAll(client_sock, pcm_chunk.data(), pcm_chunk.size() * sizeof(int16_t)))
							break;
					}
					else
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(8));
					}
				}
				GracefulClose(client_sock);
				return;
			}

			// 3. Handle live frame JPEG requests and parse input queries
			if (std::strstr(req_buf, "/frame.jpg") != nullptr)
			{
				const char* q = std::strstr(req_buf, "/frame.jpg?");
				if (q)
				{
					uint32_t btns = 0;
					float jx = 0.0f, jy = 0.0f, lt = 0.0f, rt = 0.0f;
					float px = -1.0f, py = -1.0f;
					bool pd = false;

					const char* pb = std::strstr(q, "b=");
					if (pb) btns = static_cast<uint32_t>(std::strtoul(pb + 2, nullptr, 10));
					const char* pjx = std::strstr(q, "x=");
					if (pjx) jx = std::strtof(pjx + 2, nullptr);
					const char* pjy = std::strstr(q, "y=");
					if (pjy) jy = std::strtof(pjy + 2, nullptr);
					const char* plt = std::strstr(q, "lt=");
					if (plt) lt = std::strtof(plt + 3, nullptr);
					const char* prt = std::strstr(q, "rt=");
					if (prt) rt = std::strtof(prt + 3, nullptr);
					const char* p_px = std::strstr(q, "px=");
					if (p_px) px = std::strtof(p_px + 3, nullptr);
					const char* p_py = std::strstr(q, "py=");
					if (p_py) py = std::strtof(p_py + 3, nullptr);
					const char* p_pd = std::strstr(q, "pd=");
					if (p_pd) pd = (std::strtoul(p_pd + 3, nullptr, 10) != 0);

					UpdateControllerInput(btns, jx, jy, lt, rt, px, py, pd);
				}

				std::vector<uint8_t> jpeg_copy;
				{
					std::lock_guard<std::mutex> lock(m_frame_mutex);
					jpeg_copy = m_latest_jpeg;
				}

				if (!jpeg_copy.empty())
				{
					// To prevent ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_LENGTH caused by proxies or antivirus
					// duplicating Content-Length on close-delimited streams, omit Content-Length and use Connection: close
					std::ostringstream hdr;
					hdr << "HTTP/1.1 200 OK\r\n"
						<< "Content-Type: image/jpeg\r\n"
						<< "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
						<< "Access-Control-Allow-Origin: *\r\n"
						<< "Connection: close\r\n\r\n";
					const std::string hdr_str = hdr.str();
					std::vector<uint8_t> payload;
					payload.reserve(hdr_str.size() + jpeg_copy.size());
					payload.insert(payload.end(), hdr_str.begin(), hdr_str.end());
					payload.insert(payload.end(), jpeg_copy.begin(), jpeg_copy.end());

					SendAll(client_sock, payload.data(), payload.size());
					m_frames_served.fetch_add(1);
				}
				else
				{
					static const char kResp503[] =
						"HTTP/1.1 503 Service Unavailable\r\n"
						"Access-Control-Allow-Origin: *\r\n"
						"Connection: close\r\n\r\n";
					SendAll(client_sock, kResp503, sizeof(kResp503) - 1);
				}
				GracefulClose(client_sock);
				return;
			}

			// 4. Handle HEAD requests
			if (std::strncmp(req_buf, "HEAD", 4) == 0)
			{
				std::ostringstream hdr;
				hdr << "HTTP/1.1 200 OK\r\n"
					<< "Content-Type: text/html; charset=utf-8\r\n"
					<< "Cache-Control: no-cache, no-store, must-revalidate\r\n"
					<< "Access-Control-Allow-Origin: *\r\n"
					<< "Connection: close\r\n\r\n";
				const std::string hdr_str = hdr.str();
				SendAll(client_sock, hdr_str.data(), hdr_str.size());
				GracefulClose(client_sock);
				return;
			}

			// 5. Handle WebXR HTML Studio app
			static const std::string kFullHtml = std::string(kQuestWebXRAppHtml1) + kQuestWebXRAppHtml2 + kQuestWebXRAppHtml3 + kQuestWebXRAppHtml4 + kQuestWebXRAppHtml5;
			std::ostringstream hdr;
			hdr << "HTTP/1.1 200 OK\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Cache-Control: no-cache, no-store, must-revalidate\r\n"
				<< "Access-Control-Allow-Origin: *\r\n"
				<< "Connection: close\r\n\r\n";
			const std::string hdr_str = hdr.str();
			std::string payload = hdr_str + kFullHtml;
			SendAll(client_sock, payload.data(), payload.size());
			GracefulClose(client_sock);
		}).detach();
	}
#endif
}

void gui_settings_quest_vr()
{
	auto& svc = QuestBroadcastService::Instance();
	const bool running = svc.IsRunning();

	static int s_port = 8765;
	static int s_fps_idx = 2; // 0=30, 1=60, 2=72, 3=90, 4=120
	static int s_jpeg_quality = 80;
	static int s_crop_mode = 0;
	static std::string s_status_msg;

	static const int kFpsValues[] = { 30, 60, 72, 90, 120 };
	static const char* kFpsLabels[] = {
		"30 FPS (Battery Saver)",
		"60 FPS (Standard Dreamcast NTSC)",
		"72 FPS (Quest 2 / 3 Default)",
		"90 FPS (Quest 3 Smooth VR)",
		"120 FPS (Ultra Low Latency)"
	};

	static const char* kCropLabels[] = {
		"Full Window (Standard 1P / Shared Screen)",
		"Player 1 — Left Half (Vertical Split-Screen)",
		"Player 2 — Right Half (Vertical Split-Screen)",
		"Player 1 — Top Half (Horizontal Split-Screen)",
		"Player 2 — Bottom Half (Horizontal Split-Screen)",
		"Player 1 — Top-Left Quadrant (4-Player Split)",
		"Player 2 — Top-Right Quadrant (4-Player Split)",
		"Player 3 — Bottom-Left Quadrant (4-Player Split)",
		"Player 4 — Bottom-Right Quadrant (4-Player Split)"
	};

	header(T("Meta Quest VR & Mixed Reality Broadcast Studio"));
	{
		ImGui::TextWrapped("%s", T("Stream Sega Dreamcast, NAOMI, and Atomiswave games with low-latency WebXR directly to Meta Quest 2, Quest 3, Quest 3S, and Quest Pro with Passthrough Mixed Reality, Curved VR Cinema, Stereoscopic 3D Depth, and 2P/4P Split-Screen Isolation."));
		ImGui::Spacing();

		// Status banner
		if (running)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.95f, 0.55f, 1.0f));
			ImGui::Text(ICON_FA_TOWER_BROADCAST " %s — %s", T("LIVE STREAMING ON"), svc.GetStreamUrl().c_str());
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.78f, 0.85f, 1.0f));
			ImGui::Text(ICON_FA_VR_CARDBOARD " %s", T("Status: Broadcast Server Stopped (Ready to Launch)"));
			ImGui::PopStyleColor();
		}

		if (!svc.GetLastErrorMsg().empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.38f, 0.38f, 1.0f));
			ImGui::TextWrapped(ICON_FA_TRIANGLE_EXCLAMATION " %s", svc.GetLastErrorMsg().c_str());
			ImGui::PopStyleColor();
		}
		if (!s_status_msg.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.80f, 1.0f, 1.0f));
			ImGui::TextWrapped(ICON_FA_CIRCLE_INFO " %s", s_status_msg.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::Spacing();
		ImGui::Dummy(ScaledVec2(0, 4));

		// Controls row with comfortable breathing room
		{
			DisabledScope _(running);
			ImGui::SetNextItemWidth(uiScaled(160));
			if (ImGui::InputInt(T("HTTP Stream Port"), &s_port))
			{
				if (s_port < 1024) s_port = 1024;
				if (s_port > 65535) s_port = 65535;
			}
		}

		ImGui::Dummy(ScaledVec2(0, 4));
		ImGui::SetNextItemWidth(std::min(uiScaled(500.0f), ImGui::GetContentRegionAvail().x * 0.60f));
		if (ImGui::BeginCombo(T("Capture Region / Split-Screen"), kCropLabels[s_crop_mode]))
		{
			for (int i = 0; i < (int)std::size(kCropLabels); i++)
			{
				bool sel = (s_crop_mode == i);
				if (ImGui::Selectable(kCropLabels[i], sel))
				{
					s_crop_mode = i;
					svc.SetCropMode(s_crop_mode);
				}
				if (sel)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Dummy(ScaledVec2(0, 4));
		ImGui::SetNextItemWidth(std::min(uiScaled(380.0f), ImGui::GetContentRegionAvail().x * 0.60f));
		if (ImGui::BeginCombo(T("Target VR Frame Rate"), kFpsLabels[s_fps_idx]))
		{
			for (int i = 0; i < (int)std::size(kFpsLabels); i++)
			{
				bool sel = (s_fps_idx == i);
				if (ImGui::Selectable(kFpsLabels[i], sel))
				{
					s_fps_idx = i;
					svc.SetTargetFps(kFpsValues[s_fps_idx]);
				}
				if (sel)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Dummy(ScaledVec2(0, 4));
		ImGui::SetNextItemWidth(std::min(uiScaled(380.0f), ImGui::GetContentRegionAvail().x * 0.60f));
		if (ImGui::SliderInt(T("JPEG Stream Quality"), &s_jpeg_quality, 30, 98, "%d%%"))
		{
			svc.SetJpegQuality(s_jpeg_quality);
		}

		ImGui::Spacing();
		ImGui::Dummy(ScaledVec2(0, 8));

		// Primary action buttons - responsive wrap so nothing is ever cut off
		const float availW = ImGui::GetContentRegionAvail().x;
		const float btnH = uiScaled(42.0f);
		const float broadcastBtnW = running ? uiScaled(210.0f) : uiScaled(340.0f);
		const float copyBtnW = uiScaled(190.0f);
		const float previewBtnW = uiScaled(255.0f);
		const float spacingX = ImGui::GetStyle().ItemSpacing.x;
		const bool fitsAllOneLine = (availW >= (broadcastBtnW + copyBtnW + previewBtnW + spacingX * 2.5f));

		if (!running)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.58f, 0.32f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.68f, 0.38f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.46f, 0.24f, 1.0f));
			if (ImGui::Button((std::string(ICON_FA_PLAY " ") + T("Start Wireless Quest VR Broadcast")).c_str(), ImVec2(broadcastBtnW, btnH)))
			{
				svc.SetCropMode(s_crop_mode);
				if (svc.Start(static_cast<uint16_t>(s_port), kFpsValues[s_fps_idx], s_jpeg_quality))
				{
					s_port = svc.GetPort();
					s_status_msg = "Wireless Quest VR server started on " + svc.GetStreamUrl();
				}
			}
			ImGui::PopStyleColor(3);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.18f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.24f, 0.24f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.12f, 0.12f, 1.0f));
			if (ImGui::Button((std::string(ICON_FA_STOP " ") + T("Stop Broadcast")).c_str(), ImVec2(broadcastBtnW, btnH)))
			{
				svc.Stop();
				s_status_msg = "Quest VR broadcast stopped.";
			}
			ImGui::PopStyleColor(3);
		}

		if (fitsAllOneLine)
		{
			ImGui::SameLine();
		}
		else
		{
			ImGui::Dummy(ScaledVec2(0, 6));
		}

		if (ImGui::Button((std::string(ICON_FA_COPY " ") + T("Copy Stream URL")).c_str(), ImVec2(copyBtnW, btnH)))
		{
			std::string url = svc.GetStreamUrl();
			ImGui::SetClipboardText(url.c_str());
			s_status_msg = "Copied to clipboard: " + url;
		}

		if (fitsAllOneLine || (availW >= (copyBtnW + previewBtnW + spacingX * 1.5f)))
		{
			ImGui::SameLine();
		}
		else
		{
			ImGui::Dummy(ScaledVec2(0, 6));
		}

		if (ImGui::Button((std::string(ICON_FA_GLOBE " ") + T("Open Local Browser Preview")).c_str(), ImVec2(previewBtnW, btnH)))
		{
			if (!svc.IsRunning())
			{
				svc.SetCropMode(s_crop_mode);
				svc.Start(static_cast<uint16_t>(s_port), kFpsValues[s_fps_idx], s_jpeg_quality);
				s_port = svc.GetPort();
			}
			std::string url = svc.GetStreamUrl();
#if defined(_WIN32) && !defined(TARGET_UWP)
			ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
			os_LaunchFromURL(url);
#endif
		}
	}

	ImGui::Spacing();
	ImGui::Dummy(ScaledVec2(0, 4));
	header(T("Dreamcast VR Hardware & Widescreen Enhancements"));
	{
		ImGui::Columns(2, "vr_dc_enhancements", false);
		OptionCheckbox(T("16:9 Widescreen Rendering"), config::Widescreen,
			T("Draw geometry outside the standard 4:3 aspect ratio for immersive VR theater projection."));
		OptionCheckbox(T("Super Widescreen (32:9 Curved VR)"), config::SuperWidescreen,
			T("Use the full ultra-wide viewport for wrap-around curved WebXR cinema screens (requires Widescreen)."));
		OptionCheckbox(T("Widescreen Game Hacks"), config::WidescreenGameHacks,
			T("Apply built-in Dreamcast & NAOMI widescreen hex patches automatically."));
		ImGui::NextColumn();
		OptionCheckbox(T("Rotate Screen 90° (Arcade Shmups)"), config::Rotate90,
			T("Rotate the display 90° for vertical games like Ikaruga, Under Defeat, and Mars Matrix."));
		OptionCheckbox(T("Load Custom Textures & Skins"), config::CustomTextures,
			T("Enable custom HD texture packs and GameBanana skins from data/textures/<game_id>."));
		OptionCheckbox(T("Preload Custom Textures"), config::PreloadCustomTextures,
			T("Preload custom textures at game boot for stutter-free VR frame pacing."));
		ImGui::Columns(1, nullptr, false);

		static const char* kResLabels[] = {
			"240p (0.5x Native)",
			"480p (1x Native Dreamcast VGA)",
			"720p (1.5x HD)",
			"960p (2x Crisp VR)",
			"1080p (2.25x Full HD VR)",
			"1440p (3x Quest 3 Supersampled)",
			"2160p (4.5x 4K Extreme VR)"
		};
		static const int kResHeights[] = { 240, 480, 720, 960, 1080, 1440, 2160 };
		int curResIdx = 1;
		for (int i = 0; i < (int)std::size(kResHeights); i++)
		{
			if (config::RenderResolution == kResHeights[i])
			{
				curResIdx = i;
				break;
			}
		}
		ImGui::SetNextItemWidth(uiScaled(300));
		if (ImGui::BeginCombo(T("Internal VR Supersampling Resolution"), kResLabels[curResIdx]))
		{
			for (int i = 0; i < (int)std::size(kResLabels); i++)
			{
				bool sel = (curResIdx == i);
				if (ImGui::Selectable(kResLabels[i], sel))
				{
					config::RenderResolution = kResHeights[i];
					SaveSettings();
				}
				if (sel)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	bool mute_pc = svc.GetMutePcAudio();
	if (ImGui::Checkbox(T("Mute Local PC Speakers while Broadcasting"), &mute_pc))
	{
		svc.SetMutePcAudio(mute_pc);
	}

	ImGui::Spacing();
	header(T("Quest Wireless 6DoF & ADB Fast Track"));
	ImGui::TextWrapped("%s", T("If Quest Wireless Debugging is connected, ADB forward creates an instant secure localhost tunnel to port 8765. Opening http://localhost:8765 on Quest instantly unlocks 6DoF WebXR with zero browser flag setup!"));

	static std::string s_adb_status;
	static bool s_adb_ok = false;

	if (ImGui::Button((std::string(ICON_FA_BOLT " ") + T("Auto-Forward Port 8765 to Quest (ADB Reverse)")).c_str(), ScaledVec2(360, 32)))
	{
		s_adb_ok = svc.RunAdbReverse(s_adb_status);
	}
	ImGui::SameLine();
	if (ImGui::Button((std::string(ICON_FA_EXPAND " ") + T("Launch Quest Browser on Headset (ADB)")).c_str(), ScaledVec2(320, 32)))
	{
		s_adb_ok = svc.LaunchQuestBrowser(s_adb_status);
	}
	if (!s_adb_status.empty())
	{
		ImGui::Spacing();
		ImGui::TextColored(s_adb_ok ? ImVec4(0.2f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "%s", s_adb_status.c_str());
	}

	ImGui::Spacing();
	header(T("Live Stream Telemetry & Quest Headset Guide"));
	{
		std::string activeGame = settings.content.title.empty()
			? (settings.content.gameId.empty() ? T("No Game Running (Menu Mode)") : settings.content.gameId)
			: (settings.content.title + " [" + settings.content.gameId + "]");

		ImGui::Columns(2, "vr_telemetry_cols", false);
		ImGui::Text(ICON_FA_NETWORK_WIRED " %s: %s:%u", T("Local LAN Endpoint"), svc.GetLocalIpAddress().c_str(), svc.GetPort());
		ImGui::Text(ICON_FA_COMPACT_DISC " %s: %s", T("Active Dreamcast Title"), activeGame.c_str());
		ImGui::NextColumn();
		ImGui::Text(ICON_FA_GAUGE_HIGH " %s: %.1f FPS", T("Measured Capture Rate"), svc.GetActualCaptureFps());
		ImGui::Text(ICON_FA_FILM " %s: %llu captured / %llu served", T("Frames"),
			static_cast<unsigned long long>(svc.GetFramesCaptured()),
			static_cast<unsigned long long>(svc.GetFramesServed()));
		ImGui::Columns(1, nullptr, false);

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.35f, 0.72f, 1.00f, 1.0f), ICON_FA_VR_CARDBOARD " %s", T("How to Play on Meta Quest 2 / Quest 3 / Quest 3S / Quest Pro:"));
		ImGui::BulletText("%s", T("1. Connect your Meta Quest headset to the same Wi-Fi / LAN network as this PC."));
		ImGui::BulletText("%s", T("2. Click 'Start Wireless Quest VR Broadcast' and open the Stream URL in Meta Quest Browser."));
		ImGui::BulletText("%s", T("3. Instant Play: Tap 'Launch IMAX Cinema' in Quest Browser for an instant giant curved theater screen right in your headset room with zero setup!"));
		ImGui::BulletText("%s", T("4. Immersive 6DoF Passthrough WebXR: To enable WebXR in Quest Browser, visit chrome://flags, search 'Insecure origins treated as secure', enter this Stream URL, set to Enabled, and tap Relaunch."));
		ImGui::BulletText("%s", T("5. Multiplayer: Select a Split-Screen Crop Mode above to give the headset its own dedicated private view (Sonic Adventure 2, Power Stone 2, etc.)!"));
	}
}

void gui_quest_vr()
{
	fullScreenWindow(false);
	ImguiStyleVar _(ImGuiStyleVar_WindowRounding, 0);

	ImGui::Begin(T("Quest VR & Mixed Reality Broadcast Studio"), nullptr,
		ImGuiWindowFlags_DragScrolling | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	ImVec2 normal_padding = ImGui::GetStyle().FramePadding;

	if (ImGui::Button((std::string(ICON_FA_ARROW_LEFT " ") + T("Back")).c_str(), ScaledVec2(110, 32)))
	{
		SaveSettings();
		if (game_started)
			gui_setState(GuiState::Commands);
		else
			gui_setState(GuiState::Main);
	}
	ImGui::SameLine();
	if (ImGui::Button((std::string(ICON_FA_PUZZLE_PIECE " ") + T("Open GameBanana Mods & Cheats")).c_str(), ScaledVec2(260, 32)))
	{
		SaveSettings();
		gui_setState(GuiState::GameBanana);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	{
		ImguiStyleVar _(ImGuiStyleVar_FramePadding, normal_padding);
		gui_settings_quest_vr();
	}

	scrollWhenDraggingOnVoid();
	windowDragScroll();
	ImGui::End();
}
