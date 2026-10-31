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
    --panel: rgba(16, 22, 36, 0.92);
    --accent: #ff6b00;
    --accent2: #00a8ff;
    --text: #f1f5f9;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
  body {
    background: radial-gradient(circle at top, #141e33, #060911 70%);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    overflow-x: hidden;
  }
  header {
    width: 100%;
    padding: 12px 20px;
    background: rgba(8, 12, 22, 0.90);
    border-bottom: 1px solid rgba(59, 130, 246, 0.40);
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
    backdrop-filter: blur(10px);
  }
  .brand {
    display: flex;
    align-items: center;
    gap: 10px;
    font-weight: 800;
    font-size: 1.1rem;
    letter-spacing: 0.4px;
    color: #60a5fa;
  }
  .badge {
    background: linear-gradient(135deg, #2563eb, #38bdf8);
    color: white;
    font-size: 0.72rem;
    padding: 3px 9px;
    border-radius: 999px;
    font-weight: 700;
  }
  .stats {
    font-size: 0.85rem;
    color: #94a3b8;
    font-variant-numeric: tabular-nums;
  }
  .toolbar {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    justify-content: center;
    padding: 12px 16px;
    background: var(--panel);
    border: 1px solid rgba(255,255,255,0.1);
    border-radius: 14px;
    margin: 12px;
    max-width: 1120px;
    width: calc(100% - 24px);
  }
  button, select {
    background: #1b263b;
    color: var(--text);
    border: 1px solid rgba(59, 130, 246, 0.45);
    padding: 9px 14px;
    border-radius: 9px;
    font-weight: 600;
    font-size: 0.88rem;
    cursor: pointer;
    transition: all 0.15s ease;
  }
  button:hover { background: #263859; border-color: var(--accent); }
  button.primary {
    background: linear-gradient(135deg, #2563eb, #1d4ed8);
    color: #ffffff;
    border: none;
    font-weight: 700;
  }
  button.primary:hover { filter: brightness(1.12); }
  button.vr-btn {
    background: linear-gradient(135deg, #0088ff, #6d28d9);
    color: #fff;
    border: none;
    font-weight: 700;
  }
  .sliders {
    display: flex;
    flex-wrap: wrap;
    gap: 16px;
    align-items: center;
    justify-content: center;
    width: 100%;
    padding-top: 6px;
    font-size: 0.82rem;
    color: #cbd5e1;
  }
  .sliders label {
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .stage {
    position: relative;
    width: calc(100% - 24px);
    max-width: 1280px;
    aspect-ratio: 16 / 9;
    background: #000;
    border-radius: 16px;
    overflow: hidden;
    box-shadow: 0 20px 60px rgba(0,0,0,0.75), 0 0 28px rgba(59, 130, 246, 0.22);
    border: 1px solid rgba(255,255,255,0.12);
    display: flex;
    align-items: center;
    justify-content: center;
  }
  #glcanvas {
    width: 100%;
    height: 100%;
    display: block;
  }
  .footer-hint {
    margin: 12px 16px 20px;
    font-size: 0.8rem;
    color: #94a3b8;
    text-align: center;
  }
</style>
</head>
<body>
<header>
  <div class="brand">
    <span>FLYCAST DREAMCAST VR &amp; MIXED REALITY STUDIO</span>
    <span class="badge">QUEST 2 / 3 / 3S / PRO</span>
  </div>
  <div class="stats" id="stats">Connecting to Flycast stream...</div>
</header>

<div class="toolbar">
  <button class="primary" id="btnEnterAR">Enter Mixed Reality (Passthrough)</button>
  <button class="vr-btn" id="btnEnterVR">Enter VR Cinema (Virtual Void)</button>
  <button id="btnMode2D">2D Cinema</button>
  <button id="btnModeSBS">SBS 3D Depth</button>
  <button id="btnModeTB">Top-Bottom 3D</button>
  <button id="btnModeAnaglyph">Anaglyph 3D (Red/Cyan)</button>
  <button id="btnRecenter">Recenter Screen</button>
  <button id="btnFullscreen">2D Fullscreen</button>

  <div class="sliders">
    <label>Distance: <input type="range" id="rngDist" min="1.0" max="6.0" step="0.1" value="2.4"><span id="valDist">2.4m</span></label>
    <label>Scale: <input type="range" id="rngScale" min="0.5" max="3.0" step="0.05" value="1.35"><span id="valScale">1.35x</span></label>
    <label>Curvature: <input type="range" id="rngCurve" min="0.0" max="0.6" step="0.02" value="0.18"><span id="valCurve">0.18</span></label>
    <label>3D Depth: <input type="range" id="rngDepth" min="0.0" max="0.06" step="0.002" value="0.016"><span id="valDepth">0.016</span></label>
  </div>
</div>

<div class="stage" id="stageContainer">
  <canvas id="glcanvas"></canvas>
</div>

<div class="footer-hint">
  Tip: On Meta Quest Browser, tap <b>Enter Mixed Reality</b> to float your Sega Dreamcast / NAOMI screen in your real room, or <b>Enter VR Cinema</b> for a curved theater! Use Quest thumbsticks in XR to adjust distance &amp; size.
</div>

<script>
(function() {
  const canvas = document.getElementById('glcanvas');
  const statsEl = document.getElementById('stats');
  const gl = canvas.getContext('webgl', { xrCompatible: true, alpha: true, antialias: true });
  if (!gl) {
    statsEl.textContent = 'WebGL not supported in this browser.';
    return;
  }

  let renderMode = 0; // 0=2D, 1=SBS, 2=TopBottom, 3=Anaglyph
  let screenDist = 2.4;
  let screenScale = 1.35;
  let screenCurve = 0.18;
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

  document.getElementById('btnMode2D').onclick = () => renderMode = 0;
  document.getElementById('btnModeSBS').onclick = () => renderMode = 1;
  document.getElementById('btnModeTB').onclick = () => renderMode = 2;
  document.getElementById('btnModeAnaglyph').onclick = () => renderMode = 3;
  document.getElementById('btnRecenter').onclick = () => { yawOffset = lastHeadYaw; };
  document.getElementById('btnFullscreen').onclick = () => {
    const st = document.getElementById('stageContainer');
    if (st.requestFullscreen) st.requestFullscreen();
  };

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
)HTML";

static const char* kQuestWebXRAppHtml2 = R"HTML(
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
  const img = new Image();
  img.crossOrigin = 'anonymous';

  function pumpFrame() {
    if (fetching) return;
    fetching = true;
    img.src = '/frame.jpg?t=' + performance.now();
  }

  img.onload = function() {
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, img);
    streamWidth = img.naturalWidth;
    streamHeight = img.naturalHeight;
    frameCount++;
    const now = performance.now();
    if (now - lastStatTime >= 1000) {
      const fps = ((frameCount * 1000) / (now - lastStatTime)).toFixed(1);
      statsEl.textContent = `Live Stream: ${streamWidth}x${streamHeight} @ ${fps} FPS`;
      frameCount = 0;
      lastStatTime = now;
    }
    fetching = false;
    setTimeout(pumpFrame, 4);
  };
  img.onerror = function() {
    fetching = false;
    setTimeout(pumpFrame, 120);
  };
  pumpFrame();

  function makePerspective(fovRad, aspect, near, far) {
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
    if (canvas.width !== canvas.clientWidth || canvas.height !== canvas.clientHeight) {
      canvas.width = canvas.clientWidth;
      canvas.height = canvas.clientHeight;
    }
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.clearColor(0.03, 0.05, 0.09, 1.0);
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
      alert('WebXR is not available in this browser. Open this URL inside Meta Quest Browser over HTTPS or localhost/LAN.');
      return;
    }
    try {
      const supported = await navigator.xr.isSessionSupported(mode);
      if (!supported && mode === 'immersive-ar') {
        alert('immersive-ar not supported on this device, falling back to immersive-vr.');
        mode = 'immersive-vr';
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
      xrSession.requestAnimationFrame(onXRFrame);
    } catch (e) {
      alert('Could not start WebXR session: ' + e.message);
    }
  }

  function onXRFrame(time, frame) {
    const session = frame.session;
    session.requestAnimationFrame(onXRFrame);
    const pose = frame.getViewerPose(xrRefSpace);
    if (!pose) return;

    for (const source of session.inputSources) {
      if (source.gamepad && source.gamepad.axes.length >= 4) {
        const axY = source.gamepad.axes[3];
        const axX = source.gamepad.axes[2];
        if (Math.abs(axY) > 0.2) screenDist = Math.max(1.0, Math.min(6.0, screenDist + axY * 0.025));
        if (Math.abs(axX) > 0.2) {
          screenScale = Math.max(0.5, Math.min(3.0, screenScale - axX * 0.02));
          rebuildMesh();
        }
      }
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

#if defined(_WIN32) && !defined(TARGET_UWP)
	WSACleanup();
#endif
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

	SelectObject(hdc_mem, old_obj);

	CLSID jpeg_clsid{};
	bool ok = false;
	if (GetEncoderClsid(L"image/jpeg", &jpeg_clsid) >= 0)
	{
		IStream* stream = nullptr;
		if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) == S_OK)
		{
			Gdiplus::Bitmap bmp(hbm, nullptr);
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
			DWORD timeout_ms = 2500;
			setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
			setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));

			char req_buf[2048] = {};
			const int recvd = recv(client_sock, req_buf, sizeof(req_buf) - 1, 0);
			if (recvd > 0)
			{
				req_buf[recvd] = '\0';
				if (std::strstr(req_buf, "GET /frame.jpg") != nullptr)
				{
					std::vector<uint8_t> jpeg_copy;
					{
						std::lock_guard<std::mutex> lock(m_frame_mutex);
						jpeg_copy = m_latest_jpeg;
					}

					if (!jpeg_copy.empty())
					{
						std::ostringstream hdr;
						hdr << "HTTP/1.1 200 OK\r\n"
							<< "Content-Type: image/jpeg\r\n"
							<< "Content-Length: " << jpeg_copy.size() << "\r\n"
							<< "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
							<< "Access-Control-Allow-Origin: *\r\n"
							<< "Connection: close\r\n\r\n";
						const std::string hdr_str = hdr.str();
						send(client_sock, hdr_str.c_str(), static_cast<int>(hdr_str.size()), 0);
						send(client_sock, reinterpret_cast<const char*>(jpeg_copy.data()), static_cast<int>(jpeg_copy.size()), 0);
						m_frames_served.fetch_add(1);
					}
					else
					{
						const char* resp = "HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n";
						send(client_sock, resp, static_cast<int>(std::strlen(resp)), 0);
					}
				}
				else
				{
					static const std::string kFullHtml = std::string(kQuestWebXRAppHtml1) + kQuestWebXRAppHtml2;
					const size_t html_len = kFullHtml.size();
					std::ostringstream hdr;
					hdr << "HTTP/1.1 200 OK\r\n"
						<< "Content-Type: text/html; charset=utf-8\r\n"
						<< "Content-Length: " << html_len << "\r\n"
						<< "Cache-Control: no-cache\r\n"
						<< "Access-Control-Allow-Origin: *\r\n"
						<< "Connection: close\r\n\r\n";
					const std::string hdr_str = hdr.str();
					send(client_sock, hdr_str.c_str(), static_cast<int>(hdr_str.size()), 0);
					send(client_sock, kFullHtml.c_str(), static_cast<int>(html_len), 0);
				}
			}
			closesocket(client_sock);
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
		ImGui::TextColored(ImVec4(0.35f, 0.72f, 1.00f, 1.0f), ICON_FA_VR_CARDBOARD " %s", T("How to Connect on Meta Quest 2 / Quest 3 / Quest 3S / Quest Pro:"));
		ImGui::BulletText("%s", T("1. Connect your Meta Quest headset to the same Wi-Fi / LAN network as this PC."));
		ImGui::BulletText("%s", T("2. Click 'Start Wireless Quest VR Broadcast' above and open the Stream URL in Meta Quest Browser."));
		ImGui::BulletText("%s", T("3. Tap 'Enter Mixed Reality (Passthrough)' to anchor your Dreamcast screen in your real room, or 'Enter VR Cinema' for a curved theater."));
		ImGui::BulletText("%s", T("4. Playing 2-Player or 4-Player split-screen (Sonic Adventure 2, Power Stone 2, Quake III)? Select a Split-Screen Crop Mode to give the headset its own dedicated fullscreen view!"));
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
